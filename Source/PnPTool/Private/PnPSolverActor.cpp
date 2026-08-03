// Copyright Epic Games, Inc. All Rights Reserved.

#include "PnPSolverActor.h"

#if WITH_OPENCV
#include "PreOpenCVHeaders.h"
#include "opencv2/calib3d.hpp"
#include "opencv2/core.hpp"
#include "PostOpenCVHeaders.h"
#endif

#include "OpenCVHelper.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Engine/SceneCapture2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/EngineTypes.h"

#if WITH_EDITOR
#include "DrawDebugHelpers.h"
#include "Engine/Canvas.h"
#endif

// ============================================================
// 构造函数
// ============================================================

APnPSolverActor::APnPSolverActor()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	UnbindCameraTransformDelegate();
}

// ============================================================
// EPnPMethod → cv::SolvePnPMethod 转换
// ============================================================
#if WITH_OPENCV
static int32 GetCvSolvePnPFlag(EPnPMethod Method)
{
	switch (Method)
	{
	case EPnPMethod::Iterative: return cv::SOLVEPNP_ITERATIVE;
	case EPnPMethod::EPnP:      return cv::SOLVEPNP_EPNP;
	case EPnPMethod::P3P:       return cv::SOLVEPNP_P3P;
	case EPnPMethod::AP3P:      return cv::SOLVEPNP_AP3P;
	case EPnPMethod::IPPE:      return cv::SOLVEPNP_IPPE;
	case EPnPMethod::SQPnP:     return cv::SOLVEPNP_SQPNP;
	default:                    return cv::SOLVEPNP_EPNP;
	}
}
#endif

// ============================================================
// 核心功能：求解 PnP
// ============================================================

void APnPSolverActor::SolvePnP()
{
#if WITH_OPENCV
	const int32 NumPoints = m_ObjectPoints.Num();

	// 基本校验
	if (NumPoints < 4)
	{
		UE_LOG(LogTemp, Error, TEXT("[PnPSolver] 至少需要 4 个 3D-2D 点对，当前只有 %d 个"), NumPoints);
		m_bLastSolveSuccess = false;
		return;
	}

	if (m_ImagePoints.Num() != NumPoints)
	{
		UE_LOG(LogTemp, Error, TEXT("[PnPSolver] 3D 点数(%d) 与 2D 点数(%d) 不匹配"), NumPoints, m_ImagePoints.Num());
		m_bLastSolveSuccess = false;
		return;
	}

	// P3P / AP3P 需要正好 4 个点
	if ((m_PnPMethod == EPnPMethod::P3P || m_PnPMethod == EPnPMethod::AP3P) && NumPoints != 4)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PnPSolver] P3P/AP3P 需要正好 4 个点，当前 %d 个，将自动回退到 EPnP"), NumPoints);
	}

	// IPPE 要求共面点且 >= 4
	if (m_PnPMethod == EPnPMethod::IPPE && NumPoints < 4)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PnPSolver] IPPE 需要 >= 4 个共面点，当前 %d 个"), NumPoints);
	}

	// 将 UE 坐标系的 3D 点转换为 OpenCV 坐标系（X右 Y下 Z前）
	TArray<FVector> CvObjectPoints;
	CvObjectPoints.Reserve(NumPoints);
	for (const FVector& Pt : m_ObjectPoints)
	{
		CvObjectPoints.Add(FOpenCVHelper::ConvertUnrealToOpenCV(Pt));
	}

	// 构造 OpenCV Mat
	cv::Mat ObjectPointsMat(NumPoints, 1, CV_64FC3, CvObjectPoints.GetData());
	cv::Mat ImagePointsMat(NumPoints, 1, CV_32FC2, m_ImagePoints.GetData());

	// 构造相机内参矩阵
	//   | fx  0  cx |
	//   |  0  fy cy |
	//   |  0   0   1 |
	cv::Mat CameraMatrix = cv::Mat::eye(3, 3, CV_64F);
	CameraMatrix.at<double>(0, 0) = m_FocalLength.X;
	CameraMatrix.at<double>(1, 1) = m_FocalLength.Y;
	CameraMatrix.at<double>(0, 2) = m_ImageCenter.X;
	CameraMatrix.at<double>(1, 2) = m_ImageCenter.Y;

	// 畸变系数
	cv::Mat DistCoeffsMat;
	if (m_DistortionCoefficients.Num() > 0)
	{
		DistCoeffsMat = cv::Mat(m_DistortionCoefficients.Num(), 1, CV_32FC1, const_cast<float*>(m_DistortionCoefficients.GetData()));
	}

	// 求解
	cv::Mat Rvec, Tvec;
	const int32 Flags = GetCvSolvePnPFlag(m_PnPMethod);

	bool bSuccess = false;
	try
	{
		bSuccess = cv::solvePnP(ObjectPointsMat, ImagePointsMat, CameraMatrix, DistCoeffsMat, Rvec, Tvec, false, Flags);
	}
	catch (const cv::Exception& E)
	{
		UE_LOG(LogTemp, Error, TEXT("[PnPSolver] cv::solvePnP 异常: %s"), UTF8_TO_TCHAR(E.what()));
		m_bLastSolveSuccess = false;
		return;
	}

	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("[PnPSolver] cv::solvePnP 求解失败"));
		m_bLastSolveSuccess = false;
		return;
	}

	// 缓存 rvec 和 tvec，供 ComputeReprojectionError 直接使用
	m_CachedRvec.SetNum(3);
	m_CachedTvec.SetNum(3);
	for (int32 i = 0; i < 3; ++i)
	{
		m_CachedRvec[i] = Rvec.at<double>(i);
		m_CachedTvec[i] = Tvec.at<double>(i);
	}

	// 将 OpenCV 的 rvec/tvec 转换为 UE 的 FTransform
	FOpenCVHelper::MakeCameraPoseFromObjectVectors(Rvec, Tvec, m_SolvedCameraPose);

	m_bLastSolveSuccess = true;

	// 自动计算重投影误差
	ComputeReprojectionError();

	UE_LOG(LogTemp, Log, TEXT("[PnPSolver] PnP 求解成功！方法=%s，点数=%d，重投影误差=%.4f"),
		*UEnum::GetValueAsString(m_PnPMethod), NumPoints, m_ReprojectionError);

#if WITH_EDITOR
	DrawEditorDebug();
#endif

#else
	UE_LOG(LogTemp, Error, TEXT("[PnPSolver] WITH_OPENCV 未启用，无法使用 PnP 功能"));
	m_bLastSolveSuccess = false;
#endif
}

// ============================================================
// 计算重投影误差
// ============================================================

void APnPSolverActor::ComputeReprojectionError()
{
#if WITH_OPENCV
	if (!m_bLastSolveSuccess || m_ObjectPoints.Num() == 0)
	{
		m_ReprojectionError = -1.0;
		return;
	}

	const int32 NumPoints = m_ObjectPoints.Num();

	cv::Mat Rvec(3, 1, CV_64FC1);
	cv::Mat Tvec(3, 1, CV_64FC1);

	bool bUseCached = (m_CachedRvec.Num() == 3 && m_CachedTvec.Num() == 3);
	if (bUseCached)
	{
		for (int32 i = 0; i < 3; ++i)
		{
			Rvec.at<double>(i) = m_CachedRvec[i];
			Tvec.at<double>(i) = m_CachedTvec[i];
		}
	}
	else
	{
		FOpenCVHelper::MakeObjectVectorsFromCameraPose(m_SolvedCameraPose, Rvec, Tvec);
	}

	cv::Mat CameraMatrix = cv::Mat::eye(3, 3, CV_64F);
	CameraMatrix.at<double>(0, 0) = m_FocalLength.X;
	CameraMatrix.at<double>(1, 1) = m_FocalLength.Y;
	CameraMatrix.at<double>(0, 2) = m_ImageCenter.X;
	CameraMatrix.at<double>(1, 2) = m_ImageCenter.Y;

	TArray<FVector3f> CvObjectPoints;
	CvObjectPoints.Reserve(NumPoints);
	for (const FVector& Pt : m_ObjectPoints)
	{
		const FVector CvPt = FOpenCVHelper::ConvertUnrealToOpenCV(Pt);
		CvObjectPoints.Add(FVector3f((float)CvPt.X, (float)CvPt.Y, (float)CvPt.Z));
	}

	cv::Mat ObjectPointsMat(NumPoints, 1, CV_32FC3, (void*)CvObjectPoints.GetData());

	TArray<FVector2f> ProjectedPoints;
	ProjectedPoints.Init(FVector2f(), NumPoints);
	cv::Mat ProjectedPointsMat(NumPoints, 1, CV_32FC2, (void*)ProjectedPoints.GetData());

	cv::projectPoints(ObjectPointsMat, Rvec, Tvec, CameraMatrix, cv::noArray(), ProjectedPointsMat);

	double ReprojectionError = 0.0;
	for (int32 i = 0; i < NumPoints; ++i)
	{
		const FVector2f Diff = ProjectedPoints[i] - m_ImagePoints[i];
		ReprojectionError += (Diff.X * Diff.X) + (Diff.Y * Diff.Y);
	}

	m_ReprojectionError = ReprojectionError;

	UE_LOG(LogTemp, Log, TEXT("[PnPSolver] 重投影误差 = %.6f (使用%s)"),
		m_ReprojectionError, bUseCached ? TEXT("缓存rvec/tvec") : TEXT("位姿转换"));
#else
	m_ReprojectionError = -1.0;
#endif
}

// ============================================================
// 清空所有点
// ============================================================

void APnPSolverActor::ClearAllPoints()
{
	m_ObjectPoints.Empty();
	m_ImagePoints.Empty();
	m_bLastSolveSuccess = false;
	m_ReprojectionError = -1.0;
	m_TranslationError = -1.0;
	m_RotationError = -1.0;
#if WITH_EDITOR
	if (GetWorld())
	{
		FlushPersistentDebugLines(GetWorld());
	}
#endif
	UE_LOG(LogTemp, Log, TEXT("[PnPSolver] 已清空所有点"));
}

// ============================================================
// 一键执行完整流程
// ============================================================

void APnPSolverActor::RunFullPipeline()
{
	UE_LOG(LogTemp, Log, TEXT("[PnPSolver] === 开始完整流程 ==="));

	GetIntrinsicsFromCamera();

	GeneratePointsByRaycast();

	SolvePnP();

	UE_LOG(LogTemp, Log, TEXT("[PnPSolver] === 完整流程结束 ==="));
}

// ============================================================
// 生成像素网格 → 射线投射 → 命中点作为 3D 点（UE 坐标系）
// ============================================================

void APnPSolverActor::GeneratePointsByRaycast()
{
	if (!m_SourceCameraActor.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[PnPSolver] 请先指定 Source Camera Actor"));
		return;
	}

	AActor* CamActor = m_SourceCameraActor.LoadSynchronous();
	if (!CamActor)
	{
		UE_LOG(LogTemp, Error, TEXT("[PnPSolver] 无法加载 Source Camera Actor"));
		return;
	}

	GetIntrinsicsFromCamera();

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[PnPSolver] 无法获取 World"));
		return;
	}

	if (m_ImageResolution.X <= 0 || m_ImageResolution.Y <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[PnPSolver] 图像分辨率无效"));
		return;
	}

	const FTransform CamPose = CamActor->GetActorTransform();
	m_GroundTruthPose = CamPose;
	const FVector CamPos = CamPose.GetLocation();
	const FRotator CamRot = CamPose.Rotator();

	m_ObjectPoints.Reset();
	m_ImagePoints.Reset();

	const int32 Rows = FMath::Max(1, m_GridRows);
	const int32 Cols = FMath::Max(1, m_GridCols);
	const int32 W = m_ImageResolution.X;
	const int32 H = m_ImageResolution.Y;

	const float MarginX = FMath::Min(20.0f, W * 0.05f);
	const float MarginY = FMath::Min(20.0f, H * 0.05f);
	const float StartX = MarginX;
	const float EndX = W - MarginX;
	const float StartY = MarginY;
	const float EndY = H - MarginY;

	const int32 TotalPixels = (Rows + 1) * (Cols + 1);
	int32 HitCount = 0;
	int32 MissCount = 0;

	UE_LOG(LogTemp, Log, TEXT("[PnPSolver] 开始射线投射: %d 个像素点 (%dx%d 网格), 相机=(%.0f,%.0f,%.0f)"),
		TotalPixels, Rows + 1, Cols + 1, CamPos.X, CamPos.Y, CamPos.Z);

	for (int32 r = 0; r <= Rows; ++r)
	{
		const float Y = StartY + (EndY - StartY) * (float)r / (float)Rows;
		for (int32 c = 0; c <= Cols; ++c)
		{
			const float X = StartX + (EndX - StartX) * (float)c / (float)Cols;
			const FVector2f Px(X, Y);

			const double RayRight = (Px.X - m_ImageCenter.X) / m_FocalLength.X;
			const double RayUp = (m_ImageCenter.Y - Px.Y) / m_FocalLength.Y;

			const FVector RayDirLocal(1.0, RayRight, RayUp);
			const FVector RayDirWorld = CamRot.RotateVector(RayDirLocal).GetSafeNormal();
			const FVector RayEnd = CamPos + RayDirWorld * m_RaycastMaxDistance;

			FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(PnPRaycast), false, CamActor);
			TraceParams.AddIgnoredActor(this);

			FHitResult HitResult;
			const bool bHit = World->LineTraceSingleByChannel(HitResult, CamPos, RayEnd, m_RaycastChannel, TraceParams);

			if (bHit)
			{
				m_ObjectPoints.Add(HitResult.ImpactPoint);
				m_ImagePoints.Add(Px);
				HitCount++;
			}
			else
			{
				MissCount++;
			}

#if WITH_EDITOR
			const FColor RayColor = bHit ? FColor(255, 200, 0) : FColor(255, 0, 0);
			DrawDebugLine(GetWorld(), CamPos, bHit ? HitResult.ImpactPoint : RayEnd, RayColor, true, -1.0f, 0, 1.0f);
			if (bHit)
			{
				DrawDebugPoint(GetWorld(), HitResult.ImpactPoint, 10.0f, FColor::Yellow, true, -1.0f, 0);
				DrawDebugLine(GetWorld(), HitResult.ImpactPoint,
					HitResult.ImpactPoint + HitResult.ImpactNormal * 30.0f,
					FColor(0, 255, 255), true, -1.0f, 0, 1.0f);
			}
#endif
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[PnPSolver] 射线投射完成: %d 命中, %d 未命中 (共 %d)"), HitCount, MissCount, TotalPixels);

	if (HitCount < 4)
	{
		UE_LOG(LogTemp, Error, TEXT("[PnPSolver] 有效点数不足（至少需要 4 个），无法进行 PnP 求解"));
		return;
	}

#if WITH_EDITOR
	DrawEditorDebug();
#endif
}



// ============================================================
// 对比求解位姿与 Ground Truth
// ============================================================

void APnPSolverActor::CompareSolvedVsGroundTruth()
{
	if (!m_bLastSolveSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("[PnPSolver] 请先 Solve PnP 再进行对比"));
		m_TranslationError = -1.0;
		m_RotationError = -1.0;
		return;
	}

	const FVector SolvedLoc = m_SolvedCameraPose.GetLocation();
	const FVector TruthLoc = m_GroundTruthPose.GetLocation();
	const FRotator SolvedRot = m_SolvedCameraPose.Rotator();
	const FRotator TruthRot = m_GroundTruthPose.Rotator();

	// 平移误差（欧氏距离）
	m_TranslationError = FVector::Distance(SolvedLoc, TruthLoc);

	// 旋转误差（角度差）
	const FRotator DeltaRot = SolvedRot - TruthRot;
	// Normalize the delta yaw to [-180, 180]
	float DeltaYaw = DeltaRot.Yaw;
	while (DeltaYaw > 180.0f) DeltaYaw -= 360.0f;
	while (DeltaYaw < -180.0f) DeltaYaw += 360.0f;
	float DeltaPitch = DeltaRot.Pitch;
	float DeltaRoll = DeltaRot.Roll;
	// Total rotation error as the magnitude of the angular difference
	m_RotationError = FMath::Sqrt(DeltaYaw * DeltaYaw + DeltaPitch * DeltaPitch + DeltaRoll * DeltaRoll);

	UE_LOG(LogTemp, Log, TEXT("[PnPSolver] 对比结果:"));
	UE_LOG(LogTemp, Log, TEXT("  Solved 位置: (%.2f, %.2f, %.2f)  旋转: (%.2f, %.2f, %.2f)"),
		SolvedLoc.X, SolvedLoc.Y, SolvedLoc.Z, SolvedRot.Pitch, SolvedRot.Yaw, SolvedRot.Roll);
	UE_LOG(LogTemp, Log, TEXT("  Truth  位置: (%.2f, %.2f, %.2f)  旋转: (%.2f, %.2f, %.2f)"),
		TruthLoc.X, TruthLoc.Y, TruthLoc.Z, TruthRot.Pitch, TruthRot.Yaw, TruthRot.Roll);
	UE_LOG(LogTemp, Log, TEXT("  平移误差: %.4f cm  |  旋转误差: %.4f deg"),
		m_TranslationError, m_RotationError);
}

// ============================================================
// 从 UE 相机获取内参
// ============================================================

void APnPSolverActor::GetIntrinsicsFromCamera()
{
	if (!m_SourceCameraActor.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[PnPSolver] 请先在 Source Camera Actor 指定一个相机 Actor"));
		return;
	}

	AActor* CamActor = m_SourceCameraActor.LoadSynchronous();
	if (!CamActor)
	{
		UE_LOG(LogTemp, Error, TEXT("[PnPSolver] 无法加载 SourceCameraActor"));
		return;
	}

	// 获取相机参数（FOV 和分辨率），支持 CameraActor 和 SceneCapture2D
	float FOV = 0.0f;
	int32 W = m_ImageResolution.X;
	int32 H = m_ImageResolution.Y;

	// 优先尝试 CameraActor（通过 CameraComponent）
	if (UCameraComponent* CamComp = Cast<ACameraActor>(CamActor) ? Cast<ACameraActor>(CamActor)->GetCameraComponent() : nullptr)
	{
		FMinimalViewInfo ViewInfo;
		CamComp->GetCameraView(0.0f, ViewInfo);
		if (ViewInfo.ProjectionMode != ECameraProjectionMode::Perspective)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PnPSolver] 相机不是透视投影，无法计算焦距"));
			return;
		}
		FOV = ViewInfo.FOV;
	}
	// 其次尝试 SceneCapture2D（通过 CaptureComponent2D）
	else if (ASceneCapture2D* ScnCap = Cast<ASceneCapture2D>(CamActor))
	{
		USceneCaptureComponent2D* CapComp = ScnCap->GetCaptureComponent2D();
		if (!CapComp || CapComp->ProjectionType != ECameraProjectionMode::Perspective)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PnPSolver] SceneCapture2D 不是透视投影，无法计算焦距"));
			return;
		}
		FOV = CapComp->FOVAngle;
		// 如果用户没填分辨率，尝试从 RenderTarget 自动获取
		if ((W <= 0 || H <= 0) && CapComp->TextureTarget)
		{
			W = CapComp->TextureTarget->SizeX;
			H = CapComp->TextureTarget->SizeY;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[PnPSolver] 不支持的 Actor 类型: %s（请使用 CameraActor 或 SceneCapture2D）"), *CamActor->GetClass()->GetName());
		return;
	}

	if (FOV <= 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("[PnPSolver] 相机 FOV 无效: %.2f"), FOV);
		return;
	}

	if (W <= 0 || H <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[PnPSolver] 图像分辨率无效，宽高必须 > 0"));
		return;
	}

	// 针孔相机模型：tan(FOV/2) = (W/2) / fx → fx = (W/2) / tan(FOV/2)
	const float FOVRad = FMath::DegreesToRadians(FOV);
	const double FocalLengthX = (W / 2.0) / FMath::Tan(FOVRad / 2.0);
	const double FocalLengthY = FocalLengthX; // 正方形像素假设

	m_FocalLength = FVector2D(FocalLengthX, FocalLengthY);
	m_ImageCenter = FVector2D(W / 2.0, H / 2.0);
	m_ImageResolution = FIntPoint(W, H);

	UE_LOG(LogTemp, Log, TEXT("[PnPSolver] 从相机获取内参成功: fx=%.2f fy=%.2f cx=%.2f cy=%.2f (FOV=%.2fdeg, Res=%dx%d)"),
		FocalLengthX, FocalLengthY, m_ImageCenter.X, m_ImageCenter.Y, FOV, W, H);
}

// ============================================================
// 编辑器调试绘制
// ============================================================

#if WITH_EDITOR

void APnPSolverActor::DrawEditorDebug()
{
	if (GetWorld() == nullptr)
	{
		return;
	}

	FlushPersistentDebugLines(GetWorld());

	// 获取相机位置
	FVector CamPos = FVector::ZeroVector;
	FRotator CamRot = FRotator::ZeroRotator;
	if (m_SourceCameraActor.IsValid())
	{
		AActor* CamActor = m_SourceCameraActor.LoadSynchronous();
		if (CamActor)
		{
			CamPos = CamActor->GetActorLocation();
			CamRot = CamActor->GetActorRotation();
		}
	}

	// 绘制 Ground Truth 相机位姿（蓝色轴）
	if (!CamPos.IsNearlyZero())
	{
		DrawDebugPoint(GetWorld(), CamPos, 14.0f, FColor::Blue, true, -1.0f, 0);
		DrawDebugCoordinateSystem(GetWorld(), CamPos, CamRot, 40.0f, true, -1.0f, 0, 2.0f);
	}

	// 绘制 3D 物体点 + 投影射线 + 像素标签
	for (int32 i = 0; i < m_ObjectPoints.Num(); ++i)
	{
		const FVector& Pt3D = m_ObjectPoints[i];
		const FColor PtColor = FColor::Green;

		// 3D 点（绿色大球）
		DrawDebugPoint(GetWorld(), Pt3D, 16.0f, PtColor, true, -1.0f, 0);

		// 编号标签
		const FString Label = FString::Printf(TEXT("#%d"), i);
		DrawDebugString(GetWorld(), Pt3D + FVector(0, 0, 20.0f), Label, nullptr, PtColor, 0.0f, true);

		if (!CamPos.IsNearlyZero())
		{
			DrawDebugLine(GetWorld(), CamPos, Pt3D, FColor(255, 255, 0), true, -1.0f, 0, 1.0f);
		}

		if (m_ImagePoints.IsValidIndex(i))
		{
			const FVector2f& Px = m_ImagePoints[i];
			const FString PixelLabel = FString::Printf(TEXT("(%d, %d)"), (int32)Px.X, (int32)Px.Y);
			DrawDebugString(GetWorld(), Pt3D + FVector(0, 0, 40.0f), PixelLabel, nullptr, FColor::Cyan, 0.0f, true);
		}
	}

	// 绘制相机视锥体（可视化 FOV 范围）
	if (!CamPos.IsNearlyZero() && m_ImageResolution.X > 0 && m_ImageResolution.Y > 0)
	{
		const float Dist = 2000.0f;
		const double FOVRad = 2.0 * FMath::Atan(m_ImageResolution.X / (2.0 * m_FocalLength.X));
		const float FOVDeg = FMath::RadiansToDegrees(FOVRad);

		// 视锥远端 4 个角点
		const float HalfW = Dist * FMath::Tan(FOVRad / 2.0f);
		const float Ratio = (float)m_ImageResolution.Y / (float)m_ImageResolution.X;
		const float HalfH = HalfW * Ratio;

		const FVector Forward = CamRot.Vector();
		const FVector Right = FRotationMatrix(CamRot).GetScaledAxis(EAxis::Y);
		const FVector Up = FRotationMatrix(CamRot).GetScaledAxis(EAxis::Z);

		const FVector FarCenter = CamPos + Forward * Dist;
		const FVector FarTL = FarCenter + Up * HalfH - Right * HalfW;
		const FVector FarTR = FarCenter + Up * HalfH + Right * HalfW;
		const FVector FarBL = FarCenter - Up * HalfH - Right * HalfW;
		const FVector FarBR = FarCenter - Up * HalfH + Right * HalfW;

		const FColor FrustumColor = FColor(100, 200, 255);

		// 视锥边线
		DrawDebugLine(GetWorld(), CamPos, FarTL, FrustumColor, true, -1.0f, 0, 1.0f);
		DrawDebugLine(GetWorld(), CamPos, FarTR, FrustumColor, true, -1.0f, 0, 1.0f);
		DrawDebugLine(GetWorld(), CamPos, FarBL, FrustumColor, true, -1.0f, 0, 1.0f);
		DrawDebugLine(GetWorld(), CamPos, FarBR, FrustumColor, true, -1.0f, 0, 1.0f);

		// 远端矩形
		DrawDebugLine(GetWorld(), FarTL, FarTR, FrustumColor, true, -1.0f, 0, 1.0f);
		DrawDebugLine(GetWorld(), FarTR, FarBR, FrustumColor, true, -1.0f, 0, 1.0f);
		DrawDebugLine(GetWorld(), FarBR, FarBL, FrustumColor, true, -1.0f, 0, 1.0f);
		DrawDebugLine(GetWorld(), FarBL, FarTL, FrustumColor, true, -1.0f, 0, 1.0f);

		// FOV 文本
		const FString FOVLabel = FString::Printf(TEXT("FOV=%.1f deg  Res=%dx%d"),
			FOVDeg, m_ImageResolution.X, m_ImageResolution.Y);
		DrawDebugString(GetWorld(), CamPos + Forward * 50.0f + Up * 30.0f, FOVLabel, nullptr, FrustumColor, 0.0f, true);
	}

	// 如果求解成功，绘制求解位姿（红色）与真实位姿（蓝色）对比
	if (m_bLastSolveSuccess)
	{
		const FVector SolvedPos = m_SolvedCameraPose.GetLocation();
		const FRotator SolvedRot = m_SolvedCameraPose.Rotator();

		// 求解位姿
		DrawDebugPoint(GetWorld(), SolvedPos, 16.0f, FColor::Red, true, -1.0f, 0);
		DrawDebugCoordinateSystem(GetWorld(), SolvedPos, SolvedRot, 35.0f, true, -1.0f, 0, 2.0f);

		// 从每个 3D 点到求解相机位置的连线（红色）
		for (const FVector& Pt : m_ObjectPoints)
		{
			DrawDebugLine(GetWorld(), Pt, SolvedPos, FColor::Red, true, -1.0f, 0, 1.0f);
		}

		// 位姿误差线
		if (!CamPos.IsNearlyZero())
		{
			DrawDebugLine(GetWorld(), CamPos, SolvedPos, FColor(255, 0, 255), true, -1.0f, 0, 2.0f);
			const double DistErr = FVector::Distance(CamPos, SolvedPos);
			const FString ErrLabel = FString::Printf(TEXT("Delta=%.1f cm"), DistErr);
			DrawDebugString(GetWorld(), (CamPos + SolvedPos) * 0.5 + FVector(0, 0, 30.0f), ErrLabel, nullptr, FColor::Magenta, 0.0f, true);
		}

		// 误差信息
		const FString Info = FString::Printf(TEXT("ReprojErr=%.3f  TransErr=%.2fcm  RotErr=%.2fdeg"),
			m_ReprojectionError, m_TranslationError, m_RotationError);
		DrawDebugString(GetWorld(), SolvedPos + FVector(0, 0, 60.0f), Info, nullptr, FColor::Yellow, 0.0f, true);
	}
}

#endif // WITH_EDITOR

// ============================================================
// 相机移动检测
// ============================================================

void APnPSolverActor::OnCameraTransformChanged(USceneComponent* InRootComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	UE_LOG(LogTemp, Log, TEXT("[PnPSolver] 相机位置变化，自动清理点"));
	ClearAllPoints();
}

void APnPSolverActor::BindCameraTransformDelegate()
{
	AActor* CamActor = m_SourceCameraActor.LoadSynchronous();
	if (!CamActor)
	{
		return;
	}

	USceneComponent* RootComp = CamActor->GetRootComponent();
	if (!RootComp)
	{
		return;
	}

	UnbindCameraTransformDelegate();
	m_CameraTransformHandle = RootComp->TransformUpdated.AddUObject(this, &APnPSolverActor::OnCameraTransformChanged);
	UE_LOG(LogTemp, Log, TEXT("[PnPSolver] 已绑定相机 TransformUpdated 委托"));
}

void APnPSolverActor::UnbindCameraTransformDelegate()
{
	if (m_CameraTransformHandle.IsValid())
	{
		AActor* CamActor = m_SourceCameraActor.LoadSynchronous();
		if (CamActor)
		{
			USceneComponent* RootComp = CamActor->GetRootComponent();
			if (RootComp)
			{
				RootComp->TransformUpdated.Remove(m_CameraTransformHandle);
			}
		}
		m_CameraTransformHandle.Reset();
	}
}

#if WITH_EDITOR
void APnPSolverActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(APnPSolverActor, m_SourceCameraActor))
	{
		UnbindCameraTransformDelegate();
		BindCameraTransformDelegate();
		ClearAllPoints();
	}

	DrawEditorDebug();
}
#endif