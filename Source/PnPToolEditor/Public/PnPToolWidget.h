// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"

class UTextureRenderTarget2D;
class ASceneCapture2D;
class APnPSolverActor;

class SPnPToolWidget : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SPnPToolWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	void EnsureSceneCapture();
	void UpdateSceneCaptureFromActiveViewport();

	TSharedRef<SWidget> BuildInputPanel();
	TSharedRef<SWidget> BuildRTPreviewPanel();
	TSharedRef<SWidget> BuildResultsPanel();

	// 内参 getter/setter
	double GetFx() const { return Fx; }
	void   SetFx(double V) { Fx = V; }
	double GetFy() const { return Fy; }
	void   SetFy(double V) { Fy = V; }
	double GetCx() const { return Cx; }
	void   SetCx(double V) { Cx = V; }
	double GetCy() const { return Cy; }
	void   SetCy(double V) { Cy = V; }
	double GetFov() const { return Fov; }
	void   SetFov(double V) { Fov = V; }
	int32  GetResW() const { return Resolution.X; }
	void   SetResW(int32 V) { Resolution.X = V; }
	int32  GetResH() const { return Resolution.Y; }
	void   SetResH(int32 V) { Resolution.Y = V; }

	FString GetRTPickerPath() const;
	void    OnRTChanged(const FAssetData& InAssetData);

	const FSlateBrush* GetSceneBrush() const { return &SceneBrush; }
	// RTBrush 无效时返回 SceneBrush 占位
	const FSlateBrush* GetRTBrush() const { return RTBrush.IsValid() ? RTBrush.Get() : &SceneBrush; }

	FReply OnSolveClicked();
	APnPSolverActor* FindSolverActor() const;
	void ApplyIntrinsicsToSolver(APnPSolverActor* Solver) const;

private:
	// 左侧场景预览
	TWeakObjectPtr<UTextureRenderTarget2D> ScenePreviewRT;
	TWeakObjectPtr<ASceneCapture2D> SceneCapture;
	FSlateBrush SceneBrush;

	// 右侧 RT 预览
	TWeakObjectPtr<UTextureRenderTarget2D> SelectedRT;
	TSharedPtr<FSlateBrush> RTBrush;

	// 内参
	double Fx = 1720.54;
	double Fy = 1720.54;
	double Cx = 960.00;
	double Cy = 540.00;
	double Fov = 58.32;
	FIntPoint Resolution = FIntPoint(1920, 1080);

	// 求解结果
	FTransform SolvedPose;
	double ReprojectionError = -1.0;
	double TranslationError = -1.0;
	double RotationError = -1.0;
	bool bLastSolveSuccess = false;
};
