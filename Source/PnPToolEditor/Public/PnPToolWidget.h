// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"
#include "Widgets/Input/SSpinBox.h"
#include "Engine/EngineTypes.h"

class UTextureRenderTarget2D;
class ASceneCapture2D;
class UPnPSolverSubsystem;

class SPnPToolWidget : public SCompoundWidget
{
public:
	/** 输入模式：决定当前视口点击作用于 3D 点还是 2D 点 */
	enum class EInputMode : uint8
	{
		Idle,    // 空闲：视口点击无效果
		Edit3D,  // 编辑 3D 点：3D 视口点击 / Gizmo 拖动修改当前 pair 的 3D 点
		Edit2D,  // 编辑 2D 点：RT 点击修改当前 pair 的 2D 点
	};

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

	FString GetSourceCapturePath() const;
	void    OnSourceCaptureChanged(const FAssetData& InAssetData);
	FReply  OnGetIntrinsicsClicked();
	// 依据当前 SourceCapture 的 FOV 与 TextureTarget 尺寸重算内参，
	// 保证 Resolution/Fx/Fy/Cx/Cy 与 m_DisplayRT 像素空间一致
	void    RecomputeIntrinsicsFromSource();

	const FSlateBrush* GetSceneBrush() const { return &m_SceneBrush; }
	const FSlateBrush* GetRTBrush() const { return &RTBrush; }

	// 鼠标交互
	FReply OnSceneMouseDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	FReply OnRTMouseDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	void   UpdateRTMarkerOverlay();
	FReply OnClearMarkersClicked();

	// 输入模式控制按钮
	FReply OnAdd3DPointClicked();   // 开始/提交新 pair，进入 3D 点输入模式
	FReply OnAdd2DPointClicked();   // 进入 2D 点输入模式
	FReply OnCancelEditClicked();   // 取消当前编辑
	void   CommitPendingPair();     // 提交当前 Pending pair 到已提交列表
	void   CancelPendingEdit();     // 清空 Pending 状态（不提交）
	void   UpdateInputModeUI();     // 更新按钮文字 / 状态显示 / 行视觉反映当前 InputMode

	// 3D 世界点 → 2D 像素投影（用 SourceCapture 位姿 + 当前内参）
	// 返回 (U, V)；点在相机后方时返回 (-1, -1)
	FVector2D ProjectWorldToImage(const FVector& WorldPoint) const;
	// 重建右侧输入面板里的配对数组 UI（仅在增删配对时调用）
	void RebuildPairsList();
	// 原地更新指定行的 SpinBox 数值（拖动 Gizmo 时调用，不重建列表）
	void UpdatePairRowValues(int32 Index);
	// 更新所有行的活动状态视觉（边框高亮 + 按钮文字）
	void UpdateActiveRowVisuals();

	// 3D 标记 Actor 管理（用真实 Sphere Actor 替代 DebugSphere，支持 Gizmo 拖动编辑）
	void CreateMarkerActor(int32 Index);          // 为 ManualObjectPoints[Index] 创建可视化 Actor
	void DestroyMarkerActor(int32 Index);         // 销毁指定索引的 Actor
	void DestroyAllMarkerActors();                // 销毁全部
	void SyncMarkerActorsInTick();                // Tick 中检测 Actor 被拖动 → 同步到数据 + 重新投影 2D

	FReply OnSolveClicked();
	UPnPSolverSubsystem* GetSolverSubsystem() const;
	void ApplyIntrinsicsToSolver(UPnPSolverSubsystem* Solver) const;

	// 日志面板
	void LogMessage(const FString& Msg);
	void UpdateMessages();

private:
	// 左侧场景预览
	TWeakObjectPtr<UTextureRenderTarget2D> m_ScenePreviewRT;
	TWeakObjectPtr<ASceneCapture2D> m_SceneCapture;
	FSlateBrush m_SceneBrush;
	TSharedPtr<SImage> m_SceneImageWidget;
	TSharedPtr<SBox> m_SceneContainerBox;

	// 右侧 RT 预览
	// 显示用的中间 RT（使用 InitAutoFormat 创建，保证 Slate 兼容）
	TWeakObjectPtr<UTextureRenderTarget2D> m_DisplayRT;

	// 右侧用于捕获并显示的 SceneCapture（由插件创建，位置跟随用户选定的 SourceCapture）
	TWeakObjectPtr<ASceneCapture2D> m_RightSceneCapture;

	FSlateBrush RTBrush;
	TSharedPtr<SImage> m_RTImageWidget;
	TSharedPtr<SBox> m_RTContainerBox;

	// 用户指定的源 SceneCapture2D（用于获取内参）
	TWeakObjectPtr<ASceneCapture2D> m_SourceCapture;

	// 2D 标记点 overlay
	FSlateBrush RTMarkerBrush;
	TSharedPtr<SImage> RTOverlayImage;
	TArray<FVector2D> ManualImagePoints;     // 已提交的 2D 点
	TArray<FVector> ManualObjectPoints;       // 已提交的 3D 点

	// 当前活动配对索引：用于编辑已提交 pair
	// - INDEX_NONE 表示正在新建 pair（数据存在 Pending3DPoint/Pending2DPoint）
	// - 其他值表示正在编辑该索引的已提交 pair（数据直接在 ManualObjectPoints/ManualImagePoints 中）
	int32 ActivePairIndex = INDEX_NONE;

	// 输入模式：决定视口点击作用于 3D 点还是 2D 点
	EInputMode InputMode = EInputMode::Idle;

	// 新建 pair 时的临时数据（ActivePairIndex == INDEX_NONE 时使用）
	TOptional<FVector> Pending3DPoint;
	TOptional<FVector2D> Pending2DPoint;
	TWeakObjectPtr<AActor> PendingMarker; // 新建 pair 的 3D 点 Marker（未提交）

	// 输入模式按钮引用（用于动态显示当前模式状态）
	TSharedPtr<STextBlock> Add3DBtnText;
	TSharedPtr<STextBlock> Add2DBtnText;
	TSharedPtr<STextBlock> CurrentStateText; // 显示当前编辑状态

	// 3D 标记 Actor 数组（与 ManualObjectPoints 索引对齐）
	// 用户可在编辑器视口选中 Actor 用 Gizmo 拖动，Tick 会自动同步位置到数据并重新投影 2D
	TArray<TWeakObjectPtr<AActor>> MarkerActors;

	// 右侧输入面板里的配对数组容器（每行：#i 3D(X,Y,Z) 2D(U,V) 删除 设活动）
	TSharedPtr<SVerticalBox> PairsListContainer;

	// 每行的 SpinBox 引用，用于拖动 Gizmo 时原地更新数值（避免重建列表导致闪烁）
	struct FPairRowWidgets
	{
		TSharedPtr<SSpinBox<double>> Spin3DX;
		TSharedPtr<SSpinBox<double>> Spin3DY;
		TSharedPtr<SSpinBox<double>> Spin3DZ;
		TSharedPtr<SSpinBox<double>> Spin2DU;
		TSharedPtr<SSpinBox<double>> Spin2DV;
		TSharedPtr<SBorder>          RowBorder;
		TSharedPtr<SButton>          Edit3DBtn;   // 编辑该 pair 的 3D 点
		TSharedPtr<STextBlock>       Edit3DBtnText;
		TSharedPtr<SButton>          Edit2DBtn;   // 编辑该 pair 的 2D 点
		TSharedPtr<STextBlock>       Edit2DBtnText;
	};
	TArray<FPairRowWidgets> PairRowWidgets;

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

