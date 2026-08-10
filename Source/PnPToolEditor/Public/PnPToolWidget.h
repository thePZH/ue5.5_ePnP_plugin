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
	virtual ~SPnPToolWidget() override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	FOptionalSize GetSceneAspectRatio() const;
	FOptionalSize GetRTAspectRatio() const;
	void DrawManualMarkers();
private:
	void EnsureSceneCapture();
	void EnsureRightSceneCapture();
	void UpdateSceneCaptureFromActiveViewport() const;
	void ResizeDisplayRT(int32 NewW, int32 NewH);

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

	// 3D 世界点 → 2D 像素投影（用 SourceCapture 位姿 + 当前内参）
	// 返回 (U, V)；点在相机后方时返回 (-1, -1)
	FVector2D ProjectWorldToImage(const FVector& WorldPoint) const;
	// 重建右侧输入面板里的配对数组 UI
	void RebuildPairsList();

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
	// 显示用的中间 RT（使用 InitAutoFormat 创建，保证 Slate 兼容）
	TWeakObjectPtr<UTextureRenderTarget2D> DisplayRT;

	// 右侧用于捕获并显示的 SceneCapture（由插件创建，位置跟随用户选定的 SourceCapture）
	TWeakObjectPtr<ASceneCapture2D> RightSceneCapture;

	// 用户选择的原始 RT（尚可保留用于其他用途）
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

	// 当前活动配对索引：右侧 RT 点击会覆盖该配对的 2D 点
	// - 3D 拾取后自动投影生成新配对时，活动索引 = 最新配对
	// - 用户可在配对列表中点"设为活动"切换；INDEX_NONE 表示无活动配对
	int32 ActivePairIndex = INDEX_NONE;

	// 右侧输入面板里的配对数组容器（每行：#i 3D(X,Y,Z) 2D(U,V) 删除 设活动）
	TSharedPtr<SVerticalBox> PairsListContainer;

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

