// Copyright Epic Games, Inc. All Rights Reserved.

#include "PnPSolverSubsystem.h"

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
// Subsystem 生命周期
// ============================================================

void UPnPSolverSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogTemp, Log, TEXT("[PnPSolverSubsystem] Initialize"));
}

void UPnPSolverSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("[PnPSolverSubsystem] Deinitialize"));
#if WITH_EDITOR
	if (GetWorld())
	{
		FlushPersistentDebugLines(GetWorld());
	}
#endif
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

void UPnPSolverSubsystem::SolvePnP()
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
#endif
}

// ============================================================
// 计算重投影误差
// ============================================================

void UPnPSolverSubsystem::ComputeReprojectionError()
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
// 对比求解位姿与 Ground Truth
// ============================================================

void UPnPSolverSubsystem::CompareSolvedVsGroundTruth()
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
// 编辑器调试绘制
// ============================================================

#if WITH_EDITOR

#endif // WITH_EDITOR
