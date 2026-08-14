// Copyright Epic Games, Inc. All Rights Reserved.

#include "PnPToolWidget.h"
#include "PnPSolverSubsystem.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SCanvas.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Colors/SColorPicker.h"

#include "Styling/AppStyle.h"
#include "EditorStyleSet.h"

#include "AssetRegistry/AssetData.h"
#include "PropertyCustomizationHelpers.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Engine/SceneCapture2D.h"
#include "Components/SceneCaptureComponent2D.h"

#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditor.h"
#include "Selection.h"
#include "EngineUtils.h"
#include "Components/MeshComponent.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "RenderingThread.h"

#define LOCTEXT_NAMESPACE "PnPToolWidget"

// =========================================================================
// 构造 / 析构
// =========================================================================

SPnPToolWidget::~SPnPToolWidget()
{
	if (GEditor)
	{
		if (UWorld* World = GEditor->GetEditorWorldContext().World())
		{
			FlushPersistentDebugLines(World);
		}
	}
	if (m_DebugSceneCapture.IsValid())
	{
		if (UWorld* World = m_DebugSceneCapture->GetWorld())
		{
			World->DestroyActor(m_DebugSceneCapture.Get());
		}
	}
}

void SPnPToolWidget::Construct(const FArguments& InArgs)
{
	// 左侧场景预览 RT（尺寸跟随内参分辨率）
	m_DebugRT = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), TEXT("PnPScenePreviewRT"), RF_Transient);
	m_DebugRT->InitAutoFormat(m_TargetTexRes.X, m_TargetTexRes.Y);
	m_DebugRT->UpdateResource();
	m_SceneBrush.SetResourceObject(m_DebugRT.Get());
	m_SceneBrush.ImageSize = FVector2D(m_TargetTexRes.X, m_TargetTexRes.Y);
	m_SceneBrush.DrawAs = ESlateBrushDrawType::Image;

	// 右侧目标纹理画刷（初始无纹理）
	m_TargetTextureBrush.DrawAs = ESlateBrushDrawType::Image;
	m_TargetTextureBrush.TintColor = FSlateColor(FLinearColor::White);

	// 左侧半透明叠加画刷（初始不绘制）
	m_SceneOverlayBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	m_SceneOverlayBrush.TintColor = FSlateColor(FLinearColor(1, 1, 1, 0.5f));

	ChildSlot
	[
		SNew(SHorizontalBox)

		// 左半：场景预览（无鼠标交互）
		+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(4)
		[
			BuildScenePreviewPanel()
		]

		// 右半：三段式
		+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(4)
		[
			SNew(SSplitter).Orientation(Orient_Vertical)
			+ SSplitter::Slot().Value(0.42f)[ BuildInputPanel() ]
			+ SSplitter::Slot().Value(0.38f)[ BuildRTPreviewPanel() ]
			+ SSplitter::Slot().Value(0.20f)[ BuildResultsPanel() ]
		]
	];

	RebuildPairsList();
	RebuildRTMarkers();
}

// =========================================================================
// 面板构建
// =========================================================================

TSharedRef<SWidget> SPnPToolWidget::BuildScenePreviewPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(6)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SceneHeader", "场景预览（求解后自动应用外参 + 半透明叠加目标纹理，肉眼对比对齐）"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SNew(SBorder).BorderBackgroundColor(FLinearColor::Black)
				.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
				[
					SAssignNew(m_SceneContainerBox, SBox)
					.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
					.MinAspectRatio(TAttribute<FOptionalSize>::Create(TAttribute<FOptionalSize>::FGetter::CreateSP(this, &SPnPToolWidget::GetSceneAspectRatio)))
					.MaxAspectRatio(TAttribute<FOptionalSize>::Create(TAttribute<FOptionalSize>::FGetter::CreateSP(this, &SPnPToolWidget::GetSceneAspectRatio)))
					[
						SNew(SOverlay)
						// 底层：场景渲染
						+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Fill)
						[
							SAssignNew(m_SceneImageWidget, SImage)
							.Image(this, &SPnPToolWidget::GetSceneBrush)
						]
						// 上层：目标纹理半透明叠加（居中，尺寸由 UpdateLeftOverlaySize 设定）
						+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
						[
							SAssignNew(m_SceneOverlayBox, SBox)
							.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
							.Visibility(EVisibility::Hidden)
							[
								SAssignNew(m_SceneOverlayImage, SImage)
								.Image(this, &SPnPToolWidget::GetSceneOverlayBrush)
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SPnPToolWidget::BuildInputPanel()
{
	auto MakeRow = [](const FText& Label, TSharedRef<SWidget> Widget) -> TSharedRef<SWidget>
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0, 8, 0)
			[
				SNew(STextBlock).Text(Label)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)[ Widget ];
	};

	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(6)
		[
			SNew(SScrollBox)

			+ SScrollBox::Slot().Padding(2)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("InputHeader", "输入：相机内参（手动） / 目标纹理"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			]

			+ SScrollBox::Slot().Padding(2)
			[
				MakeRow(LOCTEXT("fxLabel", "fx"),
					SNew(SSpinBox<double>).Value(this, &SPnPToolWidget::GetFx).OnValueChanged(this, &SPnPToolWidget::SetFx)
					.MinValue(1.0).MaxValue(100000.0).Delta(1.0))
			]
			+ SScrollBox::Slot().Padding(2)
			[
				MakeRow(LOCTEXT("fyLabel", "fy"),
					SNew(SSpinBox<double>).Value(this, &SPnPToolWidget::GetFy).OnValueChanged(this, &SPnPToolWidget::SetFy)
					.MinValue(1.0).MaxValue(100000.0).Delta(1.0))
			]
			+ SScrollBox::Slot().Padding(2)
			[
				MakeRow(LOCTEXT("cxLabel", "cx"),
					SNew(SSpinBox<double>).Value(this, &SPnPToolWidget::GetCx).OnValueChanged(this, &SPnPToolWidget::SetCx)
					.MinValue(0.0).MaxValue(8192.0).Delta(1.0))
			]
			+ SScrollBox::Slot().Padding(2)
			[
				MakeRow(LOCTEXT("cyLabel", "cy"),
					SNew(SSpinBox<double>).Value(this, &SPnPToolWidget::GetCy).OnValueChanged(this, &SPnPToolWidget::SetCy)
					.MinValue(0.0).MaxValue(8192.0).Delta(1.0))
			]
			+ SScrollBox::Slot().Padding(2)
			[
				MakeRow(LOCTEXT("resWLabel", "Res W"),
					SNew(SSpinBox<int32>).Value(this, &SPnPToolWidget::GetResW).OnValueChanged(this, &SPnPToolWidget::SetResW)
					.MinValue(1).MaxValue(8192).Delta(1))
			]
			+ SScrollBox::Slot().Padding(2)
			[
				MakeRow(LOCTEXT("resHLabel", "Res H"),
					SNew(SSpinBox<int32>).Value(this, &SPnPToolWidget::GetResH).OnValueChanged(this, &SPnPToolWidget::SetResH)
					.MinValue(1).MaxValue(8192).Delta(1))
			]
			// 内参辅助计算
			+ SScrollBox::Slot().Padding(2, 6, 2, 2)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(4)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("IntrinHelperHeader", "内参辅助")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 2)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
						[
							SNew(SButton)
							.Text(LOCTEXT("ApplyTexSizeBtn", "应用纹理尺寸→Resolution/cx/cy"))
							.OnClicked(this, &SPnPToolWidget::OnApplyTexSizeClicked)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 2)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0, 6, 0)
						[
							SNew(STextBlock).Text(LOCTEXT("HelperFovLabel", "水平FOV"))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
						[
							SNew(SSpinBox<double>).Value(this, &SPnPToolWidget::GetHelperFOV).OnValueChanged(this, &SPnPToolWidget::SetHelperFOV)
							.MinValue(0.1).MaxValue(179.9).Delta(1.0)
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
						[
							SNew(SButton)
							.Text(LOCTEXT("ComputeFxFyBtn", "FOV→fx=fy"))
							.OnClicked(this, &SPnPToolWidget::OnComputeFxFyFromFOVClicked)
						]
					]
				]
			]

			// 目标纹理选择器
			+ SScrollBox::Slot().Padding(2)
			[
				MakeRow(LOCTEXT("texLabel", "目标纹理"),
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(UTexture2D::StaticClass())
					.ObjectPath(this, &SPnPToolWidget::GetTargetTexturePath)
					.OnObjectChanged(this, &SPnPToolWidget::OnTargetTextureChanged)
				)
			]

			// 点对输入按钮
			+ SScrollBox::Slot().Padding(2, 8, 2, 2)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(4)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("PairInputHeader", "点对输入")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 2)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0, 8, 0)
						[
							SNew(STextBlock).Text(LOCTEXT("MatParamLabel", "材质颜色参数名"))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
						[
							SNew(SEditableTextBox)
							.Text(TAttribute<FText>::CreateLambda([this]()
							{
								return FText::FromName(m_MaterialColorParamName);
							}))
							.OnTextChanged(this, &SPnPToolWidget::SetMaterialColorParamName)
							.HintText(LOCTEXT("MatParamHint", "如 Color / BaseColor"))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 2)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
						[
							SNew(SButton)
							.Text(LOCTEXT("Add3DBtn", "添加3D点（来自选中Actor）"))
							.OnClicked(this, &SPnPToolWidget::OnAdd3DPointClicked)
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
						[
							SNew(SButton)
							.Text(LOCTEXT("CancelEditBtn", "取消编辑"))
							.OnClicked(this, &SPnPToolWidget::OnCancelEditClicked)
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
						[
							SNew(SButton)
							.Text(LOCTEXT("ClearAllBtn", "清除所有"))
							.OnClicked(this, &SPnPToolWidget::OnClearAllClicked)
						]
					]
				]
			]

			// 配对列表
			+ SScrollBox::Slot().Padding(2, 4, 2, 2)
			[
				SAssignNew(m_PairsListContainer, SVerticalBox)
			]

			// 求解按钮 + 自动求解开关
			+ SScrollBox::Slot().Padding(2, 8, 2, 2).HAlign(HAlign_Left)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("SolveBtn", "执行 PnP 求解"))
					.OnClicked(this, &SPnPToolWidget::OnSolveClicked)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return m_bAutoSolve ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { m_bAutoSolve = (State == ECheckBoxState::Checked); })
					[
						SNew(STextBlock).Text(LOCTEXT("AutoSolveLabel", "自动求解"))
					]
				]
			]
		];
}

TSharedRef<SWidget> SPnPToolWidget::BuildRTPreviewPanel()
{
    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(6)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(STextBlock)
                .Text(LOCTEXT("RTHeader", "目标纹理（滚轮缩放 / 右键或Shift+Ctrl+滚轮平移 / 编辑2D时左键拖动定点）"))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
            ]
            + SVerticalBox::Slot().FillHeight(1.0f)
            [
                SNew(SBorder)
                .BorderBackgroundColor(FLinearColor::Black)
                .HAlign(HAlign_Fill)
                .VAlign(VAlign_Fill)
                .Clipping(EWidgetClipping::ClipToBounds) // 关键：裁剪到边界
                [
                    SAssignNew(m_TargetContainerBox, SBox)
                    .HAlign(HAlign_Fill)
                    .VAlign(VAlign_Fill)
                    .Clipping(EWidgetClipping::ClipToBounds) // 裁剪
                    .MinAspectRatio(TAttribute<FOptionalSize>::Create(
                        TAttribute<FOptionalSize>::FGetter::CreateSP(this, &SPnPToolWidget::GetRTAspectRatio)))
                    .MaxAspectRatio(TAttribute<FOptionalSize>::Create(
                        TAttribute<FOptionalSize>::FGetter::CreateSP(this, &SPnPToolWidget::GetRTAspectRatio)))
                    [
                        SAssignNew(m_TargetInteractionBorder, SBorder)
                        .HAlign(HAlign_Fill)
                        .VAlign(VAlign_Fill)
                        .Clipping(EWidgetClipping::ClipToBounds) // 裁剪
                        .OnMouseButtonDown(this, &SPnPToolWidget::OnRTMouseDown)
                        .OnMouseMove(this, &SPnPToolWidget::OnRTMouseMove)
                        .OnMouseButtonUp(this, &SPnPToolWidget::OnRTMouseUp)
                        [
                            SAssignNew(m_TargetCanvas, SCanvas)
                            .Clipping(EWidgetClipping::ClipToBounds) // SCanvas 裁剪
                            + SCanvas::Slot()
                            .Position(TAttribute<FVector2D>::CreateLambda([this]() 
                            { 
                                return GetTextureSlotPos(); 
                            }))
                            .Size(TAttribute<FVector2D>::CreateLambda([this]() 
                            { 
                                return GetTextureSlotSize(); 
                            }))
                            [
                                SAssignNew(m_TargetTextureImage, SImage)
                                .Image(this, &SPnPToolWidget::GetRTTextureBrush)
                                .Visibility(EVisibility::HitTestInvisible)
                            ]
                        ]
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> SPnPToolWidget::BuildResultsPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(6)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(LOCTEXT("ResultsHeader", "求解结果 / 日志")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			]
			+ SVerticalBox::Slot().FillHeight(0.35f).Padding(0, 4, 0, 0)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						if (!m_bLastSolveSuccess) return FText::FromString(TEXT("未求解 / 求解失败"));
						const FVector L = m_SolvedPose.GetLocation();
						const FRotator R = m_SolvedPose.Rotator();
						return FText::FromString(FString::Printf(
							TEXT("求解成功\n\n位置: (%.2f, %.2f, %.2f) cm\n旋转: P=%.2f Y=%.2f R=%.2f deg\n\n重投影误差: %.6f px"),
							L.X, L.Y, L.Z, R.Pitch, R.Yaw, R.Roll, m_ReprojectionError));
					})
					.AutoWrapText(true)
				]
			]
			+ SVerticalBox::Slot().FillHeight(0.65f).Padding(0, 4, 0, 0)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(4)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("LogHeader", "操作日志")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 9)).ColorAndOpacity(FLinearColor::Gray)
					]
					+ SVerticalBox::Slot().FillHeight(1.0f)
					[
						SAssignNew(m_MessagesScrollBox, SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(m_MessagesTextWidget, STextBlock)
							.Text_Lambda([this]()
							{
								TArray<FText> T; T.Reserve(m_Messages.Num());
								for (const FString& M : m_Messages) T.Add(FText::FromString(M));
								return FText::Join(FText::FromString(TEXT("\n")), T);
							})
							.AutoWrapText(true)
							.ColorAndOpacity(FLinearColor::White)
						]
					]
				]
			]
		];
}

// =========================================================================
// 左侧预览 SceneCapture
// =========================================================================

void SPnPToolWidget::EnsureSceneCapture()
{
	if (m_DebugSceneCapture.IsValid()) return;
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	m_DebugSceneCapture = World->SpawnActor<ASceneCapture2D>(ASceneCapture2D::StaticClass(), FTransform::Identity, Params);

	if (ASceneCapture2D* Cap = m_DebugSceneCapture.Get())
	{
		Cap->SetIsTemporarilyHiddenInEditor(true);
		if (USceneCaptureComponent2D* Comp = Cap->GetCaptureComponent2D())
		{
			Comp->TextureTarget = m_DebugRT.Get();
			Comp->bCaptureEveryFrame = true;
			Comp->bCaptureOnMovement = false;
			Comp->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		}
	}
}

void SPnPToolWidget::UpdateLeftPreviewCapture()
{
	if (!m_DebugSceneCapture.IsValid()) return;
	USceneCaptureComponent2D* Comp = m_DebugSceneCapture->GetCaptureComponent2D();
	if (!Comp) return;

	if (m_bPreviewPoseLocked && m_bLastSolveSuccess)
	{
		// 锁定到求解位姿；FOV 由内参推导（水平 FOV）
		m_DebugSceneCapture->SetActorTransform(m_SolvedPose);
		const double FOVh = 2.0 * FMath::Atan(static_cast<double>(m_TargetTexRes.X) / (2.0 * m_Fx)) * (180.0 / PI);
		Comp->FOVAngle = FOVh;
		// 强制渲染一帧，确保预览视口立即更新
		Comp->CaptureScene();
	}
	else
	{
		// 未求解：跟随编辑器活动视口，作为场景预览
		FViewport* VP = GEditor ? GEditor->GetActiveViewport() : nullptr;
		FEditorViewportClient* VPC = (VP && VP->GetClient()) ? static_cast<FEditorViewportClient*>(VP->GetClient()) : nullptr;
		if (VPC && VPC->IsPerspective())
		{
			m_DebugSceneCapture->SetActorLocation(VPC->GetViewLocation());
			m_DebugSceneCapture->SetActorRotation(VPC->GetViewRotation());
			Comp->FOVAngle = VPC->ViewFOV;
		}
	}
}

void SPnPToolWidget::UpdateLeftOverlaySize()
{
	if (!m_SceneOverlayBox.IsValid() || !m_SceneImageWidget.IsValid()) return;

	const bool bShow = m_bPreviewPoseLocked && m_bLastSolveSuccess && m_TargetTexture.IsValid();
	if (!bShow)
	{
		m_SceneOverlayBox->SetVisibility(EVisibility::Hidden);
		return;
	}
	m_SceneOverlayBox->SetVisibility(EVisibility::HitTestInvisible);

	const float TexW = m_TargetTexture->GetSurfaceWidth();
	const float TexH = m_TargetTexture->GetSurfaceHeight();
	if (TexW <= 0 || TexH <= 0) return;

	const FGeometry Geo = m_SceneImageWidget->GetCachedGeometry();
	const FVector2D SceneSize = Geo.GetLocalSize();
	if (SceneSize.X <= 0 || SceneSize.Y <= 0) return;

	// 保持纹理原始宽高比，fit-in 场景容器
	const float TexAspect = TexW / TexH;
	const float ContainerAspect = SceneSize.X / SceneSize.Y;
	FVector2D OverlaySize;
	if (ContainerAspect > TexAspect)
	{
		// 容器更宽 → 高度撑满，宽度按比例
		OverlaySize.Y = SceneSize.Y;
		OverlaySize.X = SceneSize.Y * TexAspect;
	}
	else
	{
		// 容器更高 → 宽度撑满，高度按比例
		OverlaySize.X = SceneSize.X;
		OverlaySize.Y = SceneSize.X / TexAspect;
	}

	m_SceneOverlayBox->SetWidthOverride(OverlaySize.X);
	m_SceneOverlayBox->SetHeightOverride(OverlaySize.Y);

	// 画刷保持纹理原始尺寸，SImage 按 SBox 尺寸缩放（不拉伸）
	m_SceneOverlayBrush.ImageSize = FVector2D(TexW, TexH);
}

FOptionalSize SPnPToolWidget::GetSceneAspectRatio() const
{
	if (m_TargetTexRes.X > 0 && m_TargetTexRes.Y > 0)
		return FOptionalSize(static_cast<float>(m_TargetTexRes.X) / static_cast<float>(m_TargetTexRes.Y));
	return FOptionalSize(16.0f / 9.0f);
}

FOptionalSize SPnPToolWidget::GetRTAspectRatio() const
{
	if (m_TargetTexture.IsValid())
	{
		const float W = m_TargetTexture->GetSurfaceWidth();
		const float H = m_TargetTexture->GetSurfaceHeight();
		if (W > 0 && H > 0) return FOptionalSize(W / H);
	}
	if (m_TargetTexRes.X > 0 && m_TargetTexRes.Y > 0)
		return FOptionalSize(static_cast<float>(m_TargetTexRes.X) / static_cast<float>(m_TargetTexRes.Y));
	return FOptionalSize(16.0f / 9.0f);
}

// =========================================================================
// 目标纹理
// =========================================================================

FString SPnPToolWidget::GetTargetTexturePath() const
{
	return m_TargetTexture.IsValid() ? m_TargetTexture->GetPathName() : FString();
}

void SPnPToolWidget::OnTargetTextureChanged(const FAssetData& InAssetData)
{
	UTexture2D* Tex = Cast<UTexture2D>(InAssetData.GetAsset());
	m_TargetTexture = Tex;
    
	if (Tex)
	{
		m_TargetTextureBrush.SetResourceObject(Tex);
		m_TargetTextureBrush.ImageSize = FVector2D(Tex->GetSurfaceWidth(), Tex->GetSurfaceHeight());
		m_TargetTextureBrush.DrawAs = ESlateBrushDrawType::Image;
		m_TargetTextureBrush.TintColor = FSlateColor(FLinearColor::White);

		// 从 RT 创建的静态纹理可能使用纹理流送系统，初始时 mipmap 未上传到 GPU，
		// 导致 Slate 无法渲染。设置 NeverStream=true 确保纹理数据立即可用。
		// （仅运行时修改，不持久化到磁盘）
		Tex->NeverStream = true;
		Tex->UpdateResource();
		FlushRenderingCommands(); // 等待渲染线程完成 FTextureResource 创建
	}
	else
	{
		m_TargetTextureBrush.SetResourceObject(nullptr);
		m_TargetTextureBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	}
    
	// 重置缩放/平移
	m_TargetZoom = 1.0f;
	m_TargetPanOffset = FVector2D::ZeroVector;
    
	// 若已有求解结果，同步更新左侧叠加纹理
	if (m_bPreviewPoseLocked)
	{
		m_SceneOverlayBrush.SetResourceObject(Tex);
		m_SceneOverlayBrush.DrawAs = Tex ? ESlateBrushDrawType::Image : ESlateBrushDrawType::NoDrawType;
	}
    
	// 强制刷新右侧纹理视口
	if (m_TargetTextureImage.IsValid())
	{
		m_TargetTextureImage->Invalidate(EInvalidateWidgetReason::Paint);
	}
	if (m_TargetCanvas.IsValid())
	{
		m_TargetCanvas->Invalidate(EInvalidateWidgetReason::Layout | EInvalidateWidgetReason::Paint);
	}
	if (m_TargetInteractionBorder.IsValid())
	{
		m_TargetInteractionBorder->Invalidate(EInvalidateWidgetReason::Layout | EInvalidateWidgetReason::Paint);
	}
    
	RebuildRTMarkers();
	LogMessage(Tex
		? FString::Printf(TEXT("[纹理] 已加载目标纹理 %dx%d"), 
			Tex->GetSurfaceWidth(), Tex->GetSurfaceHeight())
		: TEXT("[纹理] 已清除目标纹理"));
}

// =========================================================================
// 内参辅助计算
// =========================================================================

FReply SPnPToolWidget::OnApplyTexSizeClicked()
{
	if (!m_TargetTexture.IsValid())
	{
		LogMessage(TEXT("[提示] 请先选择目标纹理"));
		return FReply::Handled();
	}
	const int32 W = m_TargetTexture->GetSurfaceWidth();
	const int32 H = m_TargetTexture->GetSurfaceHeight();
	if (W <= 0 || H <= 0)
	{
		LogMessage(TEXT("[错误] 纹理尺寸无效"));
		return FReply::Handled();
	}
	m_TargetTexRes = FIntPoint(W, H);
	// 光心取图像中心 W/2、H/2（OpenCV 常用约定，数值整洁）
	m_Cx = static_cast<double>(W) * 0.5;
	m_Cy = static_cast<double>(H) * 0.5;
	LogMessage(FString::Printf(TEXT("[内参] Resolution=%dx%d, cx=%.1f, cy=%.1f 已应用"), W, H, m_Cx, m_Cy));
	return FReply::Handled();
}

FReply SPnPToolWidget::OnComputeFxFyFromFOVClicked()
{
	if (m_TargetTexRes.X <= 0)
	{
		LogMessage(TEXT("[提示] 请先设置 Resolution.Width"));
		return FReply::Handled();
	}
	const double HalfFOV_rad = m_HelperFOV * (PI / 180.0) * 0.5;
	const double TanHalf = FMath::Tan(HalfFOV_rad);
	if (TanHalf <= KINDA_SMALL_NUMBER)
	{
		LogMessage(TEXT("[错误] FOV 太小"));
		return FReply::Handled();
	}
	// 水平 FOV → fx = (W/2) / tan(FOVh/2) = W / (2 * tan(FOVh/2))
	// 对于渲染相机，像素方形 → fx ≈ fy
	m_Fx = static_cast<double>(m_TargetTexRes.X) * 0.5 / TanHalf;
	m_Fy = m_Fx;
	LogMessage(FString::Printf(TEXT("[内参] 水平FOV=%.2f° → fx=fy=%.3f (基于ResW=%d)"), m_HelperFOV, m_Fx, m_TargetTexRes.X));
	return FReply::Handled();
}

// =========================================================================
// 配对管理
// =========================================================================

FLinearColor SPnPToolWidget::GetPaletteColor(int32 Index) const
{
	static const FLinearColor Palette[] = {
		FLinearColor(1.0f, 0.3f, 0.3f),
		FLinearColor(0.3f, 1.0f, 0.3f),
		FLinearColor(0.3f, 0.6f, 1.0f),
		FLinearColor(1.0f, 1.0f, 0.2f),
		FLinearColor(1.0f, 0.5f, 0.0f),
		FLinearColor(0.8f, 0.2f, 1.0f),
		FLinearColor(0.0f, 1.0f, 1.0f),
		FLinearColor(1.0f, 0.2f, 0.8f),
	};
	constexpr int32 N = sizeof(Palette) / sizeof(Palette[0]);
	return Palette[Index % N];
}

FReply SPnPToolWidget::OnAdd3DPointClicked()
{
	if (!GEditor) return FReply::Handled();

	TArray<AActor*> Selected;
	GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(Selected);
	if (Selected.Num() == 0)
	{
		LogMessage(TEXT("[提示] 请先在场景中选中至少一个 Actor（拖入的球体）"));
		return FReply::Handled();
	}

	// 退出 2D 编辑
	m_ActivePairIndex = INDEX_NONE;
	m_InputMode = EInputMode::Idle;
	m_bTargetShowCurCrosshair = false;

	int32 Added = 0;
	int32 Skipped = 0;
	for (AActor* Actor : Selected)
	{
		if (!Actor) continue;

		// 重复性检测：已有相同 SourceActor 的 pair 不重复添加
		bool bAlreadyExists = false;
		for (const FPair& Existing : m_Pairs)
		{
			if (Existing.SourceActor.Get() == Actor)
			{
				bAlreadyExists = true;
				break;
			}
		}
		if (bAlreadyExists)
		{
			++Skipped;
			continue;
		}

		FPair P;
		P.Id = m_NextPairId++;
		P.Color = GetPaletteColor(P.Id);
		P.SourceActor = Actor;
		P.Point3D = Actor->GetActorLocation();
		P.DisplayName = Actor->GetActorLabel();
		P.bHas2D = false;
		m_Pairs.Add(MoveTemp(P));
		ApplyColorToActor(Actor, m_Pairs.Last().Color);
		++Added;
	}

	RebuildPairsList();
	RebuildRTMarkers();
	LogMessage(FString::Printf(TEXT("[3D] 已添加 %d 个点（跳过 %d 个重复）"), Added, Skipped));
	return FReply::Handled();
}

FReply SPnPToolWidget::OnEdit2DClicked(int32 Index)
{
	if (!m_Pairs.IsValidIndex(Index)) return FReply::Handled();
	m_ActivePairIndex = Index;
	m_InputMode = EInputMode::Edit2D;
	// 行视觉/按钮文字均通过 TAttribute 自动刷新，无需重建
	LogMessage(FString::Printf(TEXT("[编辑] 配对 #%d 进入 2D 点编辑，请在右侧纹理视口左键拖动定点"), m_Pairs[Index].Id));
	return FReply::Handled();
}

FReply SPnPToolWidget::OnDeletePairClicked(int32 Index)
{
	if (!m_Pairs.IsValidIndex(Index)) return FReply::Handled();
	const int32 RemovedId = m_Pairs[Index].Id;
	m_Pairs.RemoveAt(Index);
	if (m_ActivePairIndex == Index)
	{
		m_ActivePairIndex = INDEX_NONE;
		m_InputMode = EInputMode::Idle;
		m_bTargetShowCurCrosshair = false;
	}
	else if (m_ActivePairIndex > Index)
	{
		m_ActivePairIndex--;
	}
	RebuildPairsList();
	RebuildRTMarkers();
	LogMessage(FString::Printf(TEXT("[删除] 已删除配对 #%d"), RemovedId));
	return FReply::Handled();
}

FReply SPnPToolWidget::OnCancelEditClicked()
{
	m_ActivePairIndex = INDEX_NONE;
	m_InputMode = EInputMode::Idle;
	m_bTargetShowCurCrosshair = false;
	RebuildPairsList();
	LogMessage(TEXT("[编辑] 已取消当前编辑"));
	return FReply::Handled();
}

FReply SPnPToolWidget::OnClearAllClicked()
{
	m_Pairs.Empty();
	m_NextPairId = 0;
	m_ActivePairIndex = INDEX_NONE;
	m_InputMode = EInputMode::Idle;
	m_bTargetShowCurCrosshair = false;
	m_bPreviewPoseLocked = false;
	m_SceneOverlayBrush.SetResourceObject(nullptr);
	m_SceneOverlayBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	RebuildPairsList();
	RebuildRTMarkers();
	LogMessage(TEXT("[清除] 已清空所有配对并重置预览"));
	return FReply::Handled();
}

void SPnPToolWidget::SetPairColor(int32 Id, FLinearColor NewColor)
{
	for (FPair& P : m_Pairs)
	{
		if (P.Id == Id)
		{
			P.Color = NewColor;
			// 同步修改场景中 Actor 的材质颜色
			if (AActor* Actor = P.SourceActor.Get())
			{
				ApplyColorToActor(Actor, NewColor);
			}
			break;
		}
	}
	// 标记点颜色 / 行色块均通过 TAttribute 自动刷新
}

void SPnPToolWidget::ApplyColorToActor(AActor* Actor, const FLinearColor& Color)
{
	if (!Actor || m_MaterialColorParamName.IsNone()) return;

	TArray<UMeshComponent*> MeshComps;
	Actor->GetComponents<UMeshComponent>(MeshComps);
	for (UMeshComponent* Mesh : MeshComps)
	{
		if (Mesh)
		{
			Mesh->SetVectorParameterValueOnMaterials(m_MaterialColorParamName, FVector(Color));
		}
	}
}

void SPnPToolWidget::SetMaterialColorParamName(const FText& InName)
{
	const FName NewName = FName(*InName.ToString());
	if (NewName == m_MaterialColorParamName) return;

	m_MaterialColorParamName = NewName;

	// 参数名变更后，用当前颜色重新应用到所有 Actor
	for (const FPair& P : m_Pairs)
	{
		if (AActor* Actor = P.SourceActor.Get())
		{
			ApplyColorToActor(Actor, P.Color);
		}
	}
	LogMessage(FString::Printf(TEXT("[材质] 颜色参数名已更新为 '%s'，已重新应用到所有 Actor"), *m_MaterialColorParamName.ToString()));
}

void SPnPToolWidget::OpenColorPickerForPair(int32 Index)
{
	if (!m_Pairs.IsValidIndex(Index)) return;
	const int32 Id = m_Pairs[Index].Id;
	const FLinearColor Initial = m_Pairs[Index].Color;

	FColorPickerArgs Args(Initial, FOnLinearColorValueChanged::CreateLambda([this, Id](FLinearColor NewColor)
	{
		SetPairColor(Id, NewColor);
	}));
	Args.bUseAlpha = false;
	OpenColorPicker(Args);
}

void SPnPToolWidget::RebuildPairsList()
{
	if (!m_PairsListContainer.IsValid()) return;
	m_PairsListContainer->ClearChildren();

	if (m_Pairs.Num() == 0)
	{
		m_PairsListContainer->AddSlot().AutoHeight().Padding(2)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("（无配对。在场景选中球体 Actor 后点'添加3D点'）")))
			.ColorAndOpacity(FLinearColor::Gray)
		];
		return;
	}

	for (int32 i = 0; i < m_Pairs.Num(); ++i)
	{
		const int32 CapturedIdx = i;
		m_PairsListContainer->AddSlot().AutoHeight().Padding(2)
		[
			SNew(SBorder)
			.Padding(2)
			.BorderBackgroundColor(TAttribute<FSlateColor>::CreateLambda([this, CapturedIdx]()
		{
			return (CapturedIdx == m_ActivePairIndex)
				? FSlateColor(FLinearColor(1.0f, 0.85f, 0.0f, 0.45f))
				: FSlateColor(FLinearColor(0.18f, 0.18f, 0.18f, 0.4f));
		}))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)
				[
					SNew(STextBlock)
					.Text_Lambda([this, CapturedIdx]()
					{
						return FText::FromString(FString::Printf(TEXT("#%d"),
							m_Pairs.IsValidIndex(CapturedIdx) ? m_Pairs[CapturedIdx].Id : 0));
					})
				]
				+ SHorizontalBox::Slot().FillWidth(1.4f).Padding(2).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this, CapturedIdx]()
					{
						return FText::FromString(m_Pairs.IsValidIndex(CapturedIdx) ? m_Pairs[CapturedIdx].DisplayName : TEXT(""));
					})
				]
				+ SHorizontalBox::Slot().FillWidth(1.6f).Padding(2).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this, CapturedIdx]()
					{
						if (!m_Pairs.IsValidIndex(CapturedIdx)) return FText::GetEmpty();
						const FVector& P = m_Pairs[CapturedIdx].Point3D;
						return FText::FromString(FString::Printf(TEXT("3D(%.1f,%.1f,%.1f)"), P.X, P.Y, P.Z));
					})
				]
				+ SHorizontalBox::Slot().FillWidth(1.2f).Padding(2).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this, CapturedIdx]()
					{
						if (!m_Pairs.IsValidIndex(CapturedIdx)) return FText::GetEmpty();
						if (!m_Pairs[CapturedIdx].bHas2D) return FText::FromString(TEXT("2D:未设置"));
						const FVector2D& P = m_Pairs[CapturedIdx].Point2D;
						return FText::FromString(FString::Printf(TEXT("2D(%.1f,%.1f)"), P.X, P.Y));
					})
					.ColorAndOpacity(TAttribute<FSlateColor>::CreateLambda([this, CapturedIdx]()
					{
						if (!m_Pairs.IsValidIndex(CapturedIdx) || !m_Pairs[CapturedIdx].bHas2D)
							return FSlateColor(FLinearColor::Gray);
						return FSlateColor(m_Pairs[CapturedIdx].Color);
					}))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2).VAlign(VAlign_Center)
				[
					SNew(SColorBlock)
					.Color(TAttribute<FLinearColor>::CreateLambda([this, CapturedIdx]()
					{
						return m_Pairs.IsValidIndex(CapturedIdx) ? m_Pairs[CapturedIdx].Color : FLinearColor::White;
					}))
					.Size(FVector2D(16, 16))
					.OnMouseButtonDown(FPointerEventHandler::CreateLambda([this, CapturedIdx](const FGeometry&, const FPointerEvent&) -> FReply
					{
						OpenColorPickerForPair(CapturedIdx);
						return FReply::Handled();
					}))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2).VAlign(VAlign_Center)
				[
					SNew(SButton)
					.Content()
					[
						SNew(STextBlock)
						.Text_Lambda([this, CapturedIdx]()
						{
							const bool bAct = (CapturedIdx == m_ActivePairIndex && m_InputMode == EInputMode::Edit2D);
							return FText::FromString(bAct ? TEXT("●2D编辑中") : TEXT("编辑2D"));
						})
						.Justification(ETextJustify::Type::Center)
					]
					.OnClicked_Lambda([this, CapturedIdx]() { return OnEdit2DClicked(CapturedIdx); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2).VAlign(VAlign_Center)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("删除")))
					.OnClicked_Lambda([this, CapturedIdx]() { return OnDeletePairClicked(CapturedIdx); })
				]
			]
		];
	}
}

void SPnPToolWidget::RebuildRTMarkers()
{
	if (!m_TargetCanvas.IsValid()) return;

	// 移除旧的动态 widget（标记点 + 光标十字），保留纹理 slot
	for (TSharedPtr<SWidget>& W : m_TargetMarkerWidgets)
	{
		if (W.IsValid()) m_TargetCanvas->RemoveSlot(W.ToSharedRef());
	}
	m_TargetMarkerWidgets.Reset();

	auto AddCrosshairLine = [this](const TAttribute<FVector2D>& Pos, const FVector2D& Size)
	{
		TSharedRef<SBorder> Line = SNew(SBorder)
			.Padding(0)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(TAttribute<FSlateColor>::CreateLambda([this]()
		{
			return FSlateColor(m_Pairs.IsValidIndex(m_ActivePairIndex) ? m_Pairs[m_ActivePairIndex].Color : FLinearColor::Yellow);
		}))
			.Visibility(TAttribute<EVisibility>::CreateLambda([this]()
			{
				return m_bTargetShowCurCrosshair ? EVisibility::HitTestInvisible : EVisibility::Hidden;
			}));
		m_TargetCanvas->AddSlot()
			.Position(Pos)
			.Size(Size)
			.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
			[
				Line
			];
		m_TargetMarkerWidgets.Add(Line);
	};

	// 每个 pair 一个标记点
	for (int32 i = 0; i < m_Pairs.Num(); ++i)
	{
		const int32 CapturedIdx = i;
		TSharedRef<SBorder> Marker = SNew(SBorder)
			.Padding(0)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor::Black) // 黑色描边
			.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
			.Visibility(TAttribute<EVisibility>::CreateLambda([this, CapturedIdx]()
			{
				return (m_Pairs.IsValidIndex(CapturedIdx) && m_Pairs[CapturedIdx].bHas2D)
					? EVisibility::HitTestInvisible : EVisibility::Hidden;
			}))
			[
				SNew(SBorder)
				.Padding(0)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(TAttribute<FSlateColor>::CreateLambda([this, CapturedIdx]()
			{
				return FSlateColor(m_Pairs.IsValidIndex(CapturedIdx) ? m_Pairs[CapturedIdx].Color : FLinearColor::White);
			}))
			];

		m_TargetCanvas->AddSlot()
			.Position(TAttribute<FVector2D>::CreateLambda([this, CapturedIdx]()
			{
				if (!m_TargetCanvas.IsValid() || !m_Pairs.IsValidIndex(CapturedIdx) || !m_Pairs[CapturedIdx].bHas2D || !m_TargetTexture.IsValid())
					return FVector2D(-1000, -1000);
				const FVector2D WSize = m_TargetCanvas->GetCachedGeometry().GetLocalSize();
				// Point2D 存视觉像素坐标（Y=0=顶行，OpenCV 标准），直接映射到 Canvas 屏幕
				return VisualPixelToScreen(m_Pairs[CapturedIdx].Point2D) - GetMarkerSize(CapturedIdx) * 0.5f;
			}))
			.Size(TAttribute<FVector2D>::CreateLambda([this, CapturedIdx]() { return GetMarkerSize(CapturedIdx); }))
			.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
			[
				Marker
			];
		m_TargetMarkerWidgets.Add(Marker);
	}

	// 定义十字线尺寸
	const float LineThickness = 2.f;  // 线条粗细
	const float CrosshairLength = 20.0f;  // 十字线长度

	// 竖线（垂直方向）
	AddCrosshairLine(
		TAttribute<FVector2D>::CreateLambda([this, LineThickness, CrosshairLength]() 
		{ 
			return m_TargetCursorScreenPos - FVector2D(LineThickness * 0.5f, CrosshairLength * 0.5f); 
		}),
		FVector2D(LineThickness, CrosshairLength));

	// 横线（水平方向）
	AddCrosshairLine(
		TAttribute<FVector2D>::CreateLambda([this, LineThickness, CrosshairLength]() 
		{ 
			return m_TargetCursorScreenPos - FVector2D(CrosshairLength * 0.5f, LineThickness * 0.5f); 
		}),
		FVector2D(CrosshairLength, LineThickness));
}

// =========================================================================
// 右侧纹理视口：坐标变换
// =========================================================================

void SPnPToolWidget::ComputeRTTransform(const FVector2D& WidgetSize, float& OutScale, FVector2D& OutTotalOffset) const
{
	if (!m_TargetTexture.IsValid() || WidgetSize.X <= 0 || WidgetSize.Y <= 0)
	{
		OutScale = 1.0f;
		OutTotalOffset = FVector2D::ZeroVector;
		return;
	}
	const float TexW = m_TargetTexture->GetSurfaceWidth();
	const float TexH = m_TargetTexture->GetSurfaceHeight();
	if (TexW <= 0 || TexH <= 0)
	{
		OutScale = 1.0f;
		OutTotalOffset = FVector2D::ZeroVector;
		return;
	}
	const float BaseScale = FMath::Min(WidgetSize.X / TexW, WidgetSize.Y / TexH);
	const float Scale = BaseScale * m_TargetZoom;
	const FVector2D Centering = (WidgetSize - FVector2D(TexW, TexH) * Scale) * 0.5f;
	OutScale = Scale;
	OutTotalOffset = Centering + m_TargetPanOffset;
}

FVector2D SPnPToolWidget::TextureToScreen(const FVector2D& TexPos, const FVector2D& WidgetSize) const
{
	float Scale; FVector2D Total;
	ComputeRTTransform(WidgetSize, Scale, Total);
	return TexPos * Scale + Total;
}

FVector2D SPnPToolWidget::ScreenToTexture(const FVector2D& ScreenPos, const FVector2D& WidgetSize) const
{
	float Scale; FVector2D Total;
	ComputeRTTransform(WidgetSize, Scale, Total);
	if (Scale <= KINDA_SMALL_NUMBER) return FVector2D::ZeroVector;
	return (ScreenPos - Total) / Scale;
}

FVector2D SPnPToolWidget::GetTextureSlotSize() const
{
	if (!m_TargetCanvas.IsValid() || !m_TargetTexture.IsValid()) return FVector2D(0, 0);
	const FVector2D WSize = m_TargetCanvas->GetCachedGeometry().GetLocalSize();
	float Scale; FVector2D Total;
	ComputeRTTransform(WSize, Scale, Total);
	return FVector2D(m_TargetTexture->GetSurfaceWidth(), m_TargetTexture->GetSurfaceHeight()) * Scale;
}

FVector2D SPnPToolWidget::GetTextureSlotPos() const
{
	if (!m_TargetCanvas.IsValid()) return FVector2D::ZeroVector;
	const FVector2D WSize = m_TargetCanvas->GetCachedGeometry().GetLocalSize();
	float Scale; FVector2D Total;
	ComputeRTTransform(WSize, Scale, Total);
	return Total;
}

FVector2D SPnPToolWidget::GetMarkerSize(int32 Index) const
{
	const bool bActive = (Index == m_ActivePairIndex && m_InputMode == EInputMode::Edit2D);
	return bActive ? FVector2D(5, 5) : FVector2D(2, 2);
}

FVector2D SPnPToolWidget::ScreenToVisualPixel(const FVector2D& CanvasPos) const
{
	// 通过纹理槽位矩形将 Canvas 屏幕坐标映射为视觉像素坐标（Y=0=图像顶行，OpenCV 标准）
	// 与旧代码 UV*TextureSize 方法等价，但兼容 zoom/pan
	if (!m_TargetTexture.IsValid()) return FVector2D::ZeroVector;
	const FVector2D SlotPos = GetTextureSlotPos();
	const FVector2D SlotSize = GetTextureSlotSize();
	if (SlotSize.X <= 0 || SlotSize.Y <= 0) return FVector2D::ZeroVector;
	const FVector2D UV = (CanvasPos - SlotPos) / SlotSize;
	const float TexW = m_TargetTexture->GetSurfaceWidth();
	const float TexH = m_TargetTexture->GetSurfaceHeight();
	return FVector2D(UV.X * TexW, UV.Y * TexH);
}

FVector2D SPnPToolWidget::VisualPixelToScreen(const FVector2D& VisPixel) const
{
	// 视觉像素坐标（Y=0=图像顶行）→ Canvas 屏幕坐标
	if (!m_TargetTexture.IsValid()) return FVector2D::ZeroVector;
	const FVector2D SlotPos = GetTextureSlotPos();
	const FVector2D SlotSize = GetTextureSlotSize();
	const float TexW = m_TargetTexture->GetSurfaceWidth();
	const float TexH = m_TargetTexture->GetSurfaceHeight();
	if (TexW <= 0 || TexH <= 0) return SlotPos;
	const FVector2D UV = FVector2D(VisPixel.X / TexW, VisPixel.Y / TexH);
	return SlotPos + UV * SlotSize;
}

// =========================================================================
// 右侧纹理视口：鼠标交互
// =========================================================================

FReply SPnPToolWidget::OnRTMouseDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (!m_TargetCanvas.IsValid() || !m_TargetTexture.IsValid() || !m_TargetInteractionBorder.IsValid())
        return FReply::Unhandled();

    const FGeometry CanvasGeo = m_TargetCanvas->GetCachedGeometry();
    const FVector2D CursorLocal = CanvasGeo.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
    const FVector2D WSize = CanvasGeo.GetLocalSize();

    const FKey EffectiveButton = MouseEvent.GetEffectingButton();

    if (EffectiveButton == EKeys::RightMouseButton)
    {
        m_bIsTargetPanning = true;
        m_TargetPanStartMouse = CursorLocal;
        m_TargetPanStartOffset = m_TargetPanOffset;
        return FReply::Handled().CaptureMouse(m_TargetInteractionBorder.ToSharedRef());
    }
    else if (EffectiveButton == EKeys::LeftMouseButton)
    {
        if (m_InputMode != EInputMode::Edit2D || 
            m_ActivePairIndex == INDEX_NONE || 
            !m_Pairs.IsValidIndex(m_ActivePairIndex))
        {
            LogMessage(TEXT("[提示] 请先在配对列表点'编辑2D'选择要编辑的点"));
            return FReply::Unhandled();
        }
        
        FVector2D TexPos = ScreenToVisualPixel(CursorLocal);

        // 边界：像素坐标范围 [0, W-1] x [0, H-1]
        const float MaxX = static_cast<float>(m_TargetTexture->GetSurfaceWidth() - 1);
        const float MaxY = static_cast<float>(m_TargetTexture->GetSurfaceHeight() - 1);
        TexPos.X = FMath::Clamp(TexPos.X, 0.0f, MaxX);
        TexPos.Y = FMath::Clamp(TexPos.Y, 0.0f, MaxY);

        // 视觉像素坐标（Y=0=图像顶行，OpenCV 标准），直接存储
        m_Pairs[m_ActivePairIndex].Point2D = TexPos;
        m_Pairs[m_ActivePairIndex].bHas2D = true;
        m_bIsTargetDragging = true;
        m_bTargetShowCurCrosshair = true;
        // 十字光标显示在 Canvas 上
        m_TargetCursorScreenPos = VisualPixelToScreen(TexPos);
        // 点击落点实时触发自动求解
        if (m_bAutoSolve)
        {
            int32 Completed = 0;
            for (const FPair& P : m_Pairs) { if (P.bHas2D) ++Completed; }
            if (Completed >= 4) DoSolve();
        }
        
        return FReply::Handled().CaptureMouse(m_TargetInteractionBorder.ToSharedRef());
    }
    else if (EffectiveButton == EKeys::MiddleMouseButton)
    {
        // 可选：中键平移
        m_bIsTargetPanning = true;
        m_TargetPanStartMouse = CursorLocal;
        m_TargetPanStartOffset = m_TargetPanOffset;
        return FReply::Handled().CaptureMouse(m_TargetInteractionBorder.ToSharedRef());
    }
    
    return FReply::Unhandled();
}

FReply SPnPToolWidget::OnRTMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!m_TargetCanvas.IsValid() || !m_TargetTexture.IsValid()) return FReply::Unhandled();
	const FGeometry CanvasGeo = m_TargetCanvas->GetCachedGeometry();
	const FVector2D CursorLocal = CanvasGeo.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D WSize = CanvasGeo.GetLocalSize();

	if (m_bIsTargetPanning)
	{
		m_TargetPanOffset = m_TargetPanStartOffset + (CursorLocal - m_TargetPanStartMouse);
		return FReply::Handled();
	}
	if (m_bIsTargetDragging && m_Pairs.IsValidIndex(m_ActivePairIndex))
	{
		FVector2D TexPos = ScreenToVisualPixel(CursorLocal);
		const float MaxX = static_cast<float>(m_TargetTexture->GetSurfaceWidth() - 1);
		const float MaxY = static_cast<float>(m_TargetTexture->GetSurfaceHeight() - 1);
		TexPos.X = FMath::Clamp(TexPos.X, 0.0f, MaxX);
		TexPos.Y = FMath::Clamp(TexPos.Y, 0.0f, MaxY);
		m_Pairs[m_ActivePairIndex].Point2D = TexPos;
		m_Pairs[m_ActivePairIndex].bHas2D = true;
		m_TargetCursorScreenPos = VisualPixelToScreen(TexPos);
		// 拖动中实时触发自动求解
		if (m_bAutoSolve)
		{
			int32 Completed = 0;
			for (const FPair& P : m_Pairs) { if (P.bHas2D) ++Completed; }
			if (Completed >= 4) DoSolve();
		}
		return FReply::Handled();
	}

	// 悬停：编辑2D 模式下显示十字光标
	if (m_InputMode == EInputMode::Edit2D && m_ActivePairIndex != INDEX_NONE)
	{
		m_bTargetShowCurCrosshair = true;
		m_TargetCursorScreenPos = CursorLocal;
	}
	else
	{
		m_bTargetShowCurCrosshair = false;
	}
	return FReply::Handled();
}

FReply SPnPToolWidget::OnRTMouseUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const bool bWas = m_bIsTargetDragging || m_bIsTargetPanning;
	m_bIsTargetDragging = false;
	m_bIsTargetPanning = false;
	if (bWas) return FReply::Handled().ReleaseMouseCapture();
	return FReply::Unhandled();
}

FReply SPnPToolWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (!m_TargetCanvas.IsValid() || !m_TargetTexture.IsValid()) return FReply::Unhandled();

    const FGeometry CanvasGeo = m_TargetCanvas->GetCachedGeometry();
    const FVector2D CursorLocal = CanvasGeo.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
    const FVector2D WSize = CanvasGeo.GetLocalSize();
    
    if (CursorLocal.X < 0 || CursorLocal.Y < 0 || CursorLocal.X > WSize.X || CursorLocal.Y > WSize.Y)
        return FReply::Unhandled();

    const float TexW = m_TargetTexture->GetSurfaceWidth();
    const float TexH = m_TargetTexture->GetSurfaceHeight();
    const float Wheel = MouseEvent.GetWheelDelta();

    float S0; FVector2D Total0;
    ComputeRTTransform(WSize, S0, Total0);

    if (MouseEvent.IsShiftDown() || MouseEvent.IsControlDown())
    {
        // 平移模式
        const float PanSpeed = 40.0f / FMath::Max(m_TargetZoom, 0.1f);
        if (MouseEvent.IsShiftDown())
            m_TargetPanOffset.Y += Wheel * PanSpeed;
        if (MouseEvent.IsControlDown())
            m_TargetPanOffset.X += Wheel * PanSpeed;
    }
    else
    {
        // 缩放模式
        FVector2D TexUnderCursor;
        if (S0 > KINDA_SMALL_NUMBER)
        {
            TexUnderCursor = (CursorLocal - Total0) / S0;
        }
        else
        {
            TexUnderCursor = FVector2D(TexW * 0.5f, TexH * 0.5f); // 使用纹理中心
        }

        const float NewZoom = FMath::Clamp(m_TargetZoom * (1.0f + Wheel * 0.1f), 0.1f, 40.0f);
        const float BaseScale = (m_TargetZoom > KINDA_SMALL_NUMBER) ? (S0 / m_TargetZoom) : 1.0f;
        const float S1 = BaseScale * NewZoom;
        
        // 重新计算偏移以保持光标下的纹理点不变
        const FVector2D Centering1 = (WSize - FVector2D(TexW, TexH) * S1) * 0.5f;
        m_TargetPanOffset = CursorLocal - TexUnderCursor * S1 - Centering1;
        m_TargetZoom = NewZoom;
    }
    
    return FReply::Handled();
}

// =========================================================================
// 求解
// =========================================================================

UPnPSolverSubsystem* SPnPToolWidget::GetSolverSubsystem() const
{
	if (GEditor)
	{
		if (UWorld* World = GEditor->GetEditorWorldContext().World())
		{
			return World->GetSubsystem<UPnPSolverSubsystem>();
		}
	}
	return nullptr;
}

void SPnPToolWidget::ApplyIntrinsicsToSolver(UPnPSolverSubsystem* Solver) const
{
	if (!Solver) return;
	Solver->m_FocalLength = FVector2D(m_Fx, m_Fy);
	Solver->m_ImageCenter = FVector2D(m_Cx, m_Cy);
	Solver->m_ImageResolution = m_TargetTexRes;
}

void SPnPToolWidget::ApplySolvedPoseToPreview()
{
	m_bPreviewPoseLocked = true;
	m_SceneOverlayBrush.SetResourceObject(m_TargetTexture.Get());
	m_SceneOverlayBrush.DrawAs = m_TargetTexture.IsValid() ? ESlateBrushDrawType::Image : ESlateBrushDrawType::NoDrawType;
	m_SceneOverlayBrush.TintColor = FSlateColor(FLinearColor(1, 1, 1, 0.5f));
	UpdateLeftPreviewCapture();
	UpdateLeftOverlaySize();
	LogMessage(TEXT("[预览] 已将求解外参应用到左侧预览相机，并叠加目标纹理（半透明）"));
}

FReply SPnPToolWidget::OnSolveClicked()
{
	DoSolve();
	return FReply::Handled();
}

void SPnPToolWidget::DoSolve()
{
	UPnPSolverSubsystem* Solver = GetSolverSubsystem();
	if (!Solver)
	{
		m_bLastSolveSuccess = false;
		LogMessage(TEXT("[错误] 无法获取 UPnPSolverSubsystem"));
		return;
	}

	// 只取同时有 3D 与 2D 的 pair
	TArray<FVector> ObjPts;
	TArray<FVector2f> ImgPts;
	int32 Skipped = 0;
	for (const FPair& P : m_Pairs)
	{
		if (!P.bHas2D) { ++Skipped; continue; }
		ObjPts.Add(P.Point3D);
		ImgPts.Add(FVector2f(P.Point2D.X, P.Point2D.Y));
	}
	if (Skipped > 0)
	{
		LogMessage(FString::Printf(TEXT("[求解] 跳过 %d 个未设置 2D 点的配对"), Skipped));
	}
	if (ObjPts.Num() < 4)
	{
		LogMessage(FString::Printf(TEXT("[警告] 至少需要 4 对完整的点才能求解，当前只有 %d 对"), ObjPts.Num()));
		m_bLastSolveSuccess = false;
		return;
	}

	ApplyIntrinsicsToSolver(Solver);
	Solver->m_ObjectPoints = ObjPts;
	Solver->m_ImagePoints = ImgPts;
	// 无 SceneCapture 来源，无 Ground Truth
	Solver->m_GroundTruthPose = FTransform::Identity;
	m_TranslationError = -1.0;
	m_RotationError = -1.0;

	Solver->SolvePnP();

	m_bLastSolveSuccess = Solver->m_bLastSolveSuccess;
	m_SolvedPose = Solver->m_SolvedCameraPose;
	m_ReprojectionError = Solver->m_ReprojectionError;

	if (m_bLastSolveSuccess)
	{
		const FVector L = m_SolvedPose.GetLocation();
		LogMessage(FString::Printf(TEXT("[求解成功] 点数=%d 位置=(%.2f,%.2f,%.2f) 重投影误差=%.4fpx"),
			ObjPts.Num(), L.X, L.Y, L.Z, m_ReprojectionError));
		ApplySolvedPoseToPreview(); // 求解成功回调
	}
	else
	{
		LogMessage(TEXT("[求解失败] 请检查点对/内参是否正确"));
	}
}

// =========================================================================
// 日志
// =========================================================================

void SPnPToolWidget::LogMessage(const FString& Msg)
{
	UE_LOG(LogTemp, Log, TEXT("[PnPToolWidget] %s"), *Msg);
	m_Messages.Add(Msg);
	if (m_Messages.Num() > 100) m_Messages.RemoveAt(0, m_Messages.Num() - 100);
	UpdateMessages();
}

void SPnPToolWidget::UpdateMessages()
{
	if (m_MessagesTextWidget.IsValid()) m_MessagesTextWidget->Invalidate(EInvalidateWidgetReason::Paint);
	if (m_MessagesScrollBox.IsValid()) m_MessagesScrollBox->ScrollToEnd();
}

// =========================================================================
// Tick
// =========================================================================

void SPnPToolWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	EnsureSceneCapture();

	// 预览 RT 尺寸跟随内参分辨率（保证场景渲染宽高比与目标纹理一致）
	if (m_DebugRT.IsValid() && (m_DebugRT->SizeX != m_TargetTexRes.X || m_DebugRT->SizeY != m_TargetTexRes.Y))
	{
		m_DebugRT->InitAutoFormat(m_TargetTexRes.X, m_TargetTexRes.Y);
		m_DebugRT->UpdateResource();
		m_SceneBrush.ImageSize = FVector2D(m_TargetTexRes.X, m_TargetTexRes.Y);
		if (m_DebugSceneCapture.IsValid() && m_DebugSceneCapture->GetCaptureComponent2D())
		{
			m_DebugSceneCapture->GetCaptureComponent2D()->TextureTarget = m_DebugRT.Get();
		}
	}

	// 3D 点实时跟随 Actor 位置（行文本通过 TAttribute 自动刷新）
	// 注意：左侧预览捕获 UpdateLeftPreviewCapture/UpdateLeftOverlaySize 只在
	// ApplySolvedPoseToPreview 中调用，不在 Tick 中持续执行
	for (FPair& P : m_Pairs)
	{
		if (AActor* A = P.SourceActor.Get())
		{
			const FVector Cur = A->GetActorLocation();
			if (!Cur.Equals(P.Point3D, 0.5f)) P.Point3D = Cur;
		}
	}

	// 自动求解：4 组以上完整点对且开启自动求解
	if (m_bAutoSolve)
	{
		int32 Completed = 0;
		for (const FPair& P : m_Pairs)
		{
			if (P.bHas2D) ++Completed;
		}
		if (Completed >= 4)
		{
			// 直接调用求解逻辑（不返回 FReply）
			DoSolve();
		}
	}

	UpdateMessages();
}

#undef LOCTEXT_NAMESPACE
