// Copyright Epic Games, Inc. All Rights Reserved.
// 说明：epnp是拟合一个误差最小的相机外参出来，重投影误差是用拟合出来的外参构造投影矩阵，对所有参与的点应用拟合矩阵，得到的点跟输入的2d图像点做差，而每个2d投影点和输入点差的平方和就是重投影误差
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/EngineTypes.h"
#include "PnPSolverSubsystem.generated.h"

class UCameraComponent;
class USceneCaptureComponent2D;
class AActor;

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
 * EPnP 求解器 Subsystem：输入 3D-2D 点对和相机内参，输出相机位姿
 * 生命周期跟随 World，编辑器/运行时/蓝图通用。
 * 蓝图通过 GetWorldSubsystem(UPnPSolverSubsystem) 获取。
 */
UCLASS(BlueprintType)
class PNPTOOL_API UPnPSolverSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "PnP|Actions", meta = (DisplayName = "Solve PnP"))
	void SolvePnP();

	UFUNCTION(BlueprintCallable, Category = "PnP|Actions", meta = (DisplayName = "Compare Solved vs Ground Truth"))
	void CompareSolvedVsGroundTruth();

	/** 计算重投影误差（已在 SolvePnP 后自动调用，无需手动点击） */
	UFUNCTION(BlueprintCallable, Category = "PnP|Actions", meta = (DisplayName = "Compute Reprojection Error"))
	void ComputeReprojectionError();

	/** 要获取内参的相机 Actor（CameraActor 或 SceneCapture2D） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PnP|Inputs|Camera", meta = (DisplayName = "Source Camera Actor"))
	TSoftObjectPtr<AActor> m_SourceCameraActor;

	/** 图像分辨率（像素），如 1920x1080 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PnP|Inputs|Camera", meta = (DisplayName = "Image Resolution (px)"))
	FIntPoint m_ImageResolution = FIntPoint(1920, 1080);

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

private:
	/** Cached rvec/tvec from cv::solvePnP for direct reprojection */
	TArray<double> m_CachedRvec;
	TArray<double> m_CachedTvec;
};
