// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
#include "Components/SceneComponent.h"
#include "PnPSolverActor.generated.h"

class UCameraComponent;
class USceneCaptureComponent2D;

/** PnP 求解算法选择 */
UENUM(BlueprintType)
enum class EPnPMethod : uint8
{
	Iterative UMETA(DisplayName = "ITERATIVE (Levenberg-Marquardt)"),
	EPnP      UMETA(DisplayName = "EPnP (Efficient Perspective-n-Point)"),
	P3P       UMETA(DisplayName = "P3P (Perspective-3-Point)"),
	AP3P      UMETA(DisplayName = "AP3P (Algebraic P3P)"),
	IPPE      UMETA(DisplayName = "IPPE (Infinitesimal Plane-Based)"),
	SQPnP     UMETA(DisplayName = "SQPnP (Sequential Quadratic PnP)"),
};

/**
 * 在编辑器中放置此 Actor，指定相机后点击 "Raycast Grid to 3D Points"，
 * 自动生成像素网格 → 射线投射 → 命中点作为 3D 点 → 完美配对。
 * 然后点击 "Solve PnP" 求解相机位姿，无需播放。
 */
UCLASS(BlueprintType, hidecategories = (Tick, Replication, Rendering, Collision, HLOD, Physics, Networking, Input, Actor, Cooking))
class PNPTOOL_API APnPSolverActor : public AActor
{
	GENERATED_BODY()

public:
	APnPSolverActor();

	/** 一键执行完整流程：获取内参 → 射线投射 → PnP 求解 → 自动计算重投影误差 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "PnP|Actions", meta = (DisplayName = "Run Full Pipeline"))
	void RunFullPipeline();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "PnP|Actions", meta = (DisplayName = "Get Intrinsics from Camera"))
	void GetIntrinsicsFromCamera();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "PnP|Actions", meta = (DisplayName = "Raycast Grid to 3D Points"))
	void GeneratePointsByRaycast();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "PnP|Actions", meta = (DisplayName = "Solve PnP"))
	void SolvePnP();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "PnP|Actions", meta = (DisplayName = "Compare Solved vs Ground Truth"))
	void CompareSolvedVsGroundTruth();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "PnP|Actions", meta = (DisplayName = "Clear All Points"))
	void ClearAllPoints();

	/** 计算重投影误差（已在 SolvePnP 后自动调用，无需手动点击） */
	UFUNCTION(BlueprintCallable, Category = "PnP|Actions", meta = (DisplayName = "Compute Reprojection Error"))
	void ComputeReprojectionError();

	/** 要获取内参的相机 Actor（CameraActor 或 SceneCapture2D） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PnP|Inputs|Camera", meta = (DisplayName = "Source Camera Actor"))
	TSoftObjectPtr<AActor> m_SourceCameraActor;

	/** 图像分辨率（像素），如 1920x1080 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PnP|Inputs|Camera", meta = (DisplayName = "Image Resolution (px)"))
	FIntPoint m_ImageResolution = FIntPoint(1920, 1080);

	/** 射线最大长度（cm） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PnP|Inputs|Raycast", meta = (DisplayName = "Raycast Max Distance (cm)"))
	float m_RaycastMaxDistance = 10000.0f;

	/** 射线投射通道（默认可见通道） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PnP|Inputs|Raycast", meta = (DisplayName = "Raycast Channel"))
	TEnumAsByte<ECollisionChannel> m_RaycastChannel = ECC_Visibility;

	/** 网格行数（总点数 = (Rows+1)*(Cols+1)） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PnP|Inputs|Raycast", meta = (DisplayName = "Grid Rows"))
	int32 m_GridRows = 1;

	/** 网格列数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PnP|Inputs|Raycast", meta = (DisplayName = "Grid Cols"))
	int32 m_GridCols = 1;

	/** 3D 物体点（UE 世界坐标系） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PnP|Inputs", meta = (DisplayName = "3D Object Points (UE)"))
	TArray<FVector> m_ObjectPoints;

	/** 2D 图像点（像素坐标） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PnP|Inputs", meta = (DisplayName = "2D Image Points (px)"))
	TArray<FVector2f> m_ImagePoints;

	/** 焦距（像素）：X=fx, Y=fy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PnP|Inputs", meta = (DisplayName = "Focal Length (px)"))
	FVector2D m_FocalLength = FVector2D(1000.0, 1000.0);

	/** 主点 / 图像中心（像素）：X=cx, Y=cy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PnP|Inputs", meta = (DisplayName = "Image Center (px)"))
	FVector2D m_ImageCenter = FVector2D(640.0, 360.0);

	/** 畸变系数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PnP|Inputs", meta = (DisplayName = "Distortion Coefficients"))
	TArray<float> m_DistortionCoefficients;

	/** PnP 求解算法 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PnP|Inputs", meta = (DisplayName = "PnP Method"))
	EPnPMethod m_PnPMethod = EPnPMethod::EPnP;

	/** 求解得到的相机位姿（UE 坐标系） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PnP|Output", meta = (DisplayName = "Solved Camera Pose (UE)"))
	FTransform m_SolvedCameraPose;

	/** 真实相机位姿（Ground Truth） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PnP|Output", meta = (DisplayName = "Ground Truth Camera Pose (UE)"))
	FTransform m_GroundTruthPose;

	/** 上次求解是否成功 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PnP|Output", meta = (DisplayName = "Last Solve Success"))
	bool m_bLastSolveSuccess = false;

	/** 重投影误差（像素，越小越好） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PnP|Output", meta = (DisplayName = "Reprojection Error (px)"))
	double m_ReprojectionError = -1.0;

	/** 平移误差（cm） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PnP|Output", meta = (DisplayName = "Translation Error (cm)"))
	double m_TranslationError = -1.0;

	/** 旋转误差（度） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PnP|Output", meta = (DisplayName = "Rotation Error (deg)"))
	double m_RotationError = -1.0;

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void OnCameraTransformChanged(USceneComponent* InRootComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);
	void BindCameraTransformDelegate();
	void UnbindCameraTransformDelegate();

	FDelegateHandle m_CameraTransformHandle;

	/** Cached rvec/tvec from cv::solvePnP for direct reprojection */
	TArray<double> m_CachedRvec;
	TArray<double> m_CachedTvec;

#if WITH_EDITOR
	void DrawEditorDebug();
#endif
};
