// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"
#include "Widgets/Input/SSpinBox.h"
#include "Engine/EngineTypes.h"

class UTextureRenderTarget2D;
class UTexture2D;
class ASceneCapture2D;
class UPnPSolverSubsystem;
class APnPMarkerActor;
class AActor;
class SCanvas;
class SImage;
class SBox;
class SBorder;
class SVerticalBox;
class STextBlock;
class SScrollBox;
class SMultiLineEditableText;
struct FAssetData;
struct FPointerEvent;

/**
 * PnP 标定工具主控件。
 *
 * 设计：
 * - 左侧：场景预览视口（插件内部 SceneCapture2D 渲染）。求解成功后应用外参并把目标纹理半透明叠加，
 *   供用户肉眼对比图像与场景是否对齐。不处理任何鼠标事件。
 * - 右侧：目标纹理视口（用户传入 UTexture2D）。支持滚轮缩放、右键/Shift+Ctrl+滚轮平移、
 *   编辑2D模式下左键按下拖动精确定点。内参完全由用户手动输入（工具不感知任何 SceneCapture2D）。
 * - 3D 点：用户从编辑器选中 APnPMarkerActor 后点击「添加3D点」读取位置，实时跟随移动。
 *   如果选中普通 Actor 也会添加（但不会自动修改颜色）。
 * - 每个 pair(3D,2D) 共享一个颜色，用户可调。
 */
class SPnPToolWidget : public SCompoundWidget
{
public:
	/** 输入模式 */
	enum class EInputMode : uint8
	{
		Idle,    // 空闲
		Edit2D,  // 编辑某个 pair 的 2D 点（在右侧纹理视口点击/拖动）
	};

	/** 一组 3D-2D 配对 */
	struct FPair
	{
		int32 Id = 0;                       // 自增唯一 id
		FLinearColor Color = FLinearColor::White;
		TWeakObjectPtr<AActor> SourceActor; // Maker Actor（3D 点实时跟随其位置）
		FVector Point3D = FVector::ZeroVector;
		FVector2D Point2D = FVector2D::ZeroVector; // 目标纹理像素坐标（视觉坐标系，Y=0 顶）
		bool bHas2D = false;
	};

	SLATE_BEGIN_ARGS(SPnPToolWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPnPToolWidget() override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	TSharedRef<SWidget> BuildInputPanel();
	TSharedRef<SWidget> BuildScenePreviewPanel();
	TSharedRef<SWidget> BuildRTPreviewPanel();

	// 左侧预览 SceneCapture（插件内部）
	void EnsureSceneCapture();
	void UpdateLeftPreviewCapture();  // 锁定到求解位姿 / 否则跟随活动视口
	void UpdateLeftOverlaySize();     // 半透明纹理叠加的尺寸
	FOptionalSize GetSceneAspectRatio() const;
	FOptionalSize GetRTAspectRatio() const;

	// 内参 getter/setter（手动输入）
	double GetFx() const { return m_Fx; }
	void   SetFx(double V) { m_Fx = V; }
	double GetFy() const { return m_Fy; }
	void   SetFy(double V) { m_Fy = V; }
	double GetCx() const { return m_Cx; }
	void   SetCx(double V) { m_Cx = V; }
	double GetCy() const { return m_Cy; }
	void   SetCy(double V) { m_Cy = V; }
	int32  GetResW() const { return m_TargetTexRes.X; }
	void   SetResW(int32 V) { m_TargetTexRes.X = V; }
	int32  GetResH() const { return m_TargetTexRes.Y; }
	void   SetResH(int32 V) { m_TargetTexRes.Y = V; }
	// 内参辅助计算
	double  GetHelperFOV() const { return m_HelperFOV; }
	void    SetHelperFOV(double V) { m_HelperFOV = FMath::Clamp(V, 0.1, 179.9); }
	FReply  OnApplyTexSizeClicked();    // 用目标纹理尺寸设置 Resolution / cx / cy
	FReply  OnComputeFxFyFromFOVClicked(); // 用水平 FOV 计算 fx=fy（基于当前 Resolution）

	// 目标纹理
	FString GetTargetTexturePath() const;
	void    OnTargetTextureChanged(const FAssetData& InAssetData);

	const FSlateBrush* GetDebugSceneBrush() const { return &m_DebugBrush; }
	const FSlateBrush* GetRTTextureBrush() const { return &m_TargetTextureBrush; }
	const FSlateBrush* GetSceneOverlayBrush() const { return &m_DebugOverlayBrush; }

	// 配对管理
	FReply OnAdd3DPointClicked();            // 读取编辑器选中 Actor
	FReply OnEdit2DClicked(int32 Index);     // 进入某 pair 的 2D 编辑
	FReply OnDeletePairClicked(int32 Index);
	FReply OnCancelEditClicked();
	FReply OnClearAllClicked();
	FReply OnRecreateMakersClicked();        // 重新生成缺失的 Maker Actor
	void   SetPairColor(int32 Id, FLinearColor NewColor);
	void   OpenColorPickerForPair(int32 Index);
	FLinearColor GetPaletteColor(int32 Index) const;
	void   ApplyColorToActor(AActor* Actor, const FLinearColor& Color); // 通过材质参数修改 Actor 颜色

	// 材质参数名
	FName GetMaterialColorParamName() const { return m_MaterialColorParamName; }
	void  SetMaterialColorParamName(const FText& InName);

	// 叠加层不透明度
	float  GetOverlayOpacity() const { return m_OverlayOpacity; }
	void   SetOverlayOpacity(float V);

	// 叠加层放大倍率（>1.0 时 SceneCapture 的 FOV 更宽，RT 可视范围更大，叠加纹理保持原始内参 FOV 只覆盖中心区域）
	float  GetOverlayScale() const { return m_OverlayScale; }
	void   SetOverlayScale(float V) { m_OverlayScale = FMath::Clamp(V, 0.5f, 3.0f); UpdateLeftPreviewCapture(); UpdateLeftOverlaySize(); }

	void RebuildPairsList();
	void RebuildRTMarkers();   // 重建右侧纹理视口的标记点 widget（结构变化时调用）

	// 右侧纹理视口交互
	FReply OnRTMouseDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	FReply OnRTMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	FReply OnRTMouseUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	void   ComputeRTTransform(const FVector2D& WidgetSize, float& OutScale, FVector2D& OutTotalOffset) const;
	FVector2D ScreenToTexture(const FVector2D& ScreenPos, const FVector2D& WidgetSize) const;
	FVector2D TextureToScreen(const FVector2D& TexPos, const FVector2D& WidgetSize) const;
	FVector2D GetTextureSlotSize() const;
	FVector2D GetTextureSlotPos() const;
	FVector2D GetMarkerSize(int32 Index) const;
	// 视觉像素坐标系统（Y=0=图像顶行，OpenCV 标准）
	// 通过纹理槽位矩形将 Canvas 屏幕坐标 ↔ 视觉像素坐标互转
	FVector2D ScreenToVisualPixel(const FVector2D& CanvasPos) const;
	FVector2D VisualPixelToScreen(const FVector2D& VisPixel) const;

	// 求解
	FReply OnSolveClicked();
	void DoSolve();   // 实际的求解逻辑，OnSolveClicked 和 Tick 都可调用
	UPnPSolverSubsystem* GetSolverSubsystem() const;
	void ApplyIntrinsicsToSolver(UPnPSolverSubsystem* Solver) const;
	void ApplySolvedPoseToPreview();   // PnP 求解成功回调：把外参应用到左侧预览相机 + 叠加纹理

	// 日志
	void LogMessage(const FString& Msg);
	void UpdateMessages();

private:
	// 左侧验证窗口
	TWeakObjectPtr<UTextureRenderTarget2D> m_DebugRT;
	TWeakObjectPtr<ASceneCapture2D> m_DebugSceneCapture;
	FSlateBrush m_DebugBrush;
	TSharedPtr<SImage> m_DebugImage;
	TSharedPtr<SBox> m_DebugContainerBox;
	TSharedPtr<SImage> m_DebugOverlayImage;
	FSlateBrush m_DebugOverlayBrush;
	TSharedPtr<SBox> m_DebugOverlayBox;
	bool m_bDebugPoseLocked = false;

	// 目标纹理 + 右侧视口
	TWeakObjectPtr<UTexture2D> m_TargetTexture;
	FSlateBrush m_TargetTextureBrush;
	TSharedPtr<SBox> m_TargetContainerBox;
	TSharedPtr<SBorder> m_TargetInteractionBorder;
	TSharedPtr<SCanvas> m_TargetCanvas;
	TSharedPtr<SImage> m_TargetTextureImage;
	TArray<TSharedPtr<SWidget>> m_TargetMarkerWidgets; // 标记点 + 光标十字（与 Pairs 结构同步重建）
	float m_TargetZoom = 1.0f;
	FVector2D m_TargetPanOffset = FVector2D::ZeroVector;
	bool m_bIsTargetDragging = false;
	bool m_bIsTargetPanning = false;
	FVector2D m_TargetPanStartMouse = FVector2D::ZeroVector;
	FVector2D m_TargetPanStartOffset = FVector2D::ZeroVector;
	bool m_bTargetShowCurCrosshair = false;
	FVector2D m_TargetCursorScreenPos = FVector2D::ZeroVector;

	// 配对数据
	TArray<FPair> m_Pairs;
	int32 m_NextPairId = 0;
	int32 m_ActivePairIndex = INDEX_NONE;
	EInputMode m_InputMode = EInputMode::Idle;

	// 配对列表 UI 容器
	TSharedPtr<SVerticalBox> m_PairsListContainer;

	// 求解结果文本
	TSharedPtr<SMultiLineEditableText> m_ResultTextWidget;

	// 日志
	TArray<FString> m_Messages;
	TSharedPtr<SMultiLineEditableText> m_MessagesTextWidget;
	TSharedPtr<SScrollBox> m_MessagesScrollBox;

	// 内参（手动输入）
	double m_Fx = 1000.0;
	double m_Fy = 1000.0;
	double m_Cx = 960.0;
	double m_Cy = 540.0;
	FIntPoint m_TargetTexRes = FIntPoint(1920, 1080);
	double m_HelperFOV = 90.0;      // 辅助：计算 fx/fy 时使用的水平 FOV（度）

	// 求解结果
	FTransform m_SolvedPose;
	double m_ReprojectionError = -1.0;
	double m_TranslationError = -1.0;
	double m_RotationError = -1.0;
	bool m_bLastSolveSuccess = false;
	bool m_bAutoSolve = false;   // 自动求解（每帧触发）

	// 材质颜色参数名（用于修改场景中 Actor 的材质颜色）
	FName m_MaterialColorParamName = FName("Color");

	// 叠加层参数
	float m_OverlayOpacity = 0.5f;  // 目标纹理叠加不透明度
	float m_OverlayScale = 1.2f;    // SceneCapture FOV 放大倍率（>1 RT 可视范围更大，叠加纹理保持原始内参 FOV 只覆盖中心区域）
};