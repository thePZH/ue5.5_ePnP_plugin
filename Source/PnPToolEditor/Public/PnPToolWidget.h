// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"
#include "Engine/EngineTypes.h"

class UTextureRenderTarget2D;
class ASceneCapture2D;
class UPnPSolverSubsystem;

class SPnPToolWidget : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SPnPToolWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPnPToolWidget();
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	FOptionalSize GetSceneAspectRatio() const;
	FOptionalSize GetRTAspectRatio() const;
	void DrawManualMarkers();
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

	FString GetSourceCapturePath() const;
	void    OnSourceCaptureChanged(const FAssetData& InAssetData);
	FReply  OnGetIntrinsicsClicked();

	const FSlateBrush* GetSceneBrush() const { return &SceneBrush; }
	const FSlateBrush* GetRTBrush() const { return &RTBrush; }

	// 鼠标交互
	FReply OnSceneMouseDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	FReply OnRTMouseDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	void   UpdateRTMarkerOverlay();
	FReply OnClearMarkersClicked();

	FReply OnSolveClicked();
	UPnPSolverSubsystem* GetSolverSubsystem() const;
	void ApplyIntrinsicsToSolver(UPnPSolverSubsystem* Solver) const;

	// 日志面板
	void LogMessage(const FString& Msg);
	void UpdateMessages();

private:
	// 左侧场景预览
	TWeakObjectPtr<UTextureRenderTarget2D> ScenePreviewRT;
	TWeakObjectPtr<ASceneCapture2D> SceneCapture;
	FSlateBrush SceneBrush;
	TSharedPtr<SImage> SceneImageWidget;
	TSharedPtr<SBox> SceneContainerBox;

	// 右侧 RT 预览
	TWeakObjectPtr<UTextureRenderTarget2D> SelectedRT;
	FSlateBrush RTBrush;
	TSharedPtr<SImage> RTImageWidget;
	TSharedPtr<SBox> RTContainerBox;

	// 用户指定的源 SceneCapture2D（用于获取内参）
	TWeakObjectPtr<ASceneCapture2D> SourceCapture;

	// 2D 标记点 overlay
	FSlateBrush RTMarkerBrush;
	TSharedPtr<SImage> RTOverlayImage;
	TArray<FVector2D> ManualImagePoints;
	TArray<FVector> ManualObjectPoints;
	TOptional<FVector> PendingObjectPoint;  // 待配对的 3D 点（红色显示）

	// 日志面板
	TArray<FString> Messages;
	TSharedPtr<STextBlock> MessagesTextWidget;
	TSharedPtr<SScrollBox> MessagesScrollBox;

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

