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
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Images/SImage.h"
#include "EditorStyleSet.h"
#include "Styling/AppStyle.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "PropertyCustomizationHelpers.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/SceneCapture2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditor.h"
#include "Selection.h"
#include "EngineUtils.h"
#include "Engine/Texture2D.h"
#include "RHIResources.h"
#include "Rendering/Texture2DResource.h"
#include "Engine/StaticMeshActor.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"

#define LOCTEXT_NAMESPACE "PnPToolWidget"

SPnPToolWidget::~SPnPToolWidget()
{
	// 清除场景中的所有持久 debug 绘制（3D 控制点、解算可视化等）
	if (GEditor)
	{
		if (UWorld* World = GEditor->GetEditorWorldContext().World())
		{
			FlushPersistentDebugLines(World);
		}
	}

	// 销毁 Pending Marker（未提交的 3D 点）
	if (PendingMarker.IsValid())
	{
		if (UWorld* World = PendingMarker->GetWorld())
		{
			World->DestroyActor(PendingMarker.Get(), false);
		}
		PendingMarker = nullptr;
	}

	// 销毁所有 3D 标记 Actor
	DestroyAllMarkerActors();

	if (m_SceneCapture.IsValid())
	{
		if (UWorld* World = m_SceneCapture->GetWorld())
		{
			World->DestroyActor(m_SceneCapture.Get());
		}
	}
	if (m_RightSceneCapture.IsValid())
	{
		if (UWorld* World = m_RightSceneCapture->GetWorld())
		{
			World->DestroyActor(m_RightSceneCapture.Get());
		}
	}
}

void SPnPToolWidget::Construct(const FArguments& InArgs)
{
	// 1. 创建左侧场景预览用的 RenderTarget（瞬态对象，不保存）
	m_ScenePreviewRT = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), TEXT("PnPScenePreviewRT"), RF_Transient);
	m_ScenePreviewRT->InitAutoFormat(960, 540);
	m_ScenePreviewRT->UpdateResource();

	m_SceneBrush.SetResourceObject(m_ScenePreviewRT.Get());
	m_SceneBrush.ImageSize = FVector2D(m_ScenePreviewRT->SizeX, m_ScenePreviewRT->SizeY);
	m_SceneBrush.DrawAs = ESlateBrushDrawType::Image;

	// 2. 创建右侧显示用的 RenderTarget（与左侧相同的初始化流程，保证 Slate 显示兼容）
	m_DisplayRT = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), TEXT("PnPDisplayRT"), RF_Transient);
	m_DisplayRT->InitAutoFormat(960, 540);
	m_DisplayRT->UpdateResource();

	RTBrush.SetResourceObject(m_DisplayRT.Get());
	RTBrush.ImageSize = FVector2D(m_DisplayRT->SizeX, m_DisplayRT->SizeY);
	RTBrush.DrawAs = ESlateBrushDrawType::Image;

	// 3. 2D 标记点 overlay 画刷（初始不绘制）
	RTMarkerBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	RTMarkerBrush.ImageSize = FVector2D(1.0f, 1.0f);

	// 4. 构建整个窗口的 UI 布局：左半场景预览 + 右半三段式面板
	ChildSlot
	[
		SNew(SHorizontalBox)

		// 左半部分：场景相机视口预览
		+ SHorizontalBox::Slot()
		.FillWidth(0.5f)
		.Padding(4)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SceneHeader", "场景视口（点'添加3D点'按钮后在此点击放置 3D 点）"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				]

				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					SNew(SBorder).BorderBackgroundColor(FLinearColor::Black)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SAssignNew(m_SceneContainerBox, SBox)
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Fill)
						.MinAspectRatio(TAttribute<FOptionalSize>::Create(TAttribute<FOptionalSize>::FGetter::CreateSP(this, &SPnPToolWidget::GetSceneAspectRatio)))
						.MaxAspectRatio(TAttribute<FOptionalSize>::Create(TAttribute<FOptionalSize>::FGetter::CreateSP(this, &SPnPToolWidget::GetSceneAspectRatio)))
						[
							SNew(SBorder)
							.OnMouseButtonDown(this, &SPnPToolWidget::OnSceneMouseDown)
							[
								SAssignNew(m_SceneImageWidget, SImage)
								.Image(this, &SPnPToolWidget::GetSceneBrush)
							]
						]
					]
				]
			]
		]

		// 右半部分：三段式布局
		+ SHorizontalBox::Slot()
		.FillWidth(0.5f)
		.Padding(4)
		[
			SNew(SSplitter).Orientation(Orient_Vertical)

			+ SSplitter::Slot().Value(0.34f)[ BuildInputPanel() ]
			+ SSplitter::Slot().Value(0.33f)[ BuildRTPreviewPanel() ]
			+ SSplitter::Slot().Value(0.33f)[ BuildResultsPanel() ]
		]
	];

	// 初始化配对列表空状态显示
	RebuildPairsList();
	UpdateInputModeUI();
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
					.Text(LOCTEXT("InputHeader", "输入：相机内参 / RT / Capture2D"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			]

			+ SScrollBox::Slot().Padding(2)
			[
				MakeRow(LOCTEXT("fxLabel", "fx"),
					SNew(SSpinBox<double>)
						.Value(this, &SPnPToolWidget::GetFx)
						.OnValueChanged(this, &SPnPToolWidget::SetFx)
						.MinValue(1.0).MaxValue(100000.0).Delta(1.0))
			]

			+ SScrollBox::Slot().Padding(2)
			[
				MakeRow(LOCTEXT("fyLabel", "fy"),
					SNew(SSpinBox<double>)
						.Value(this, &SPnPToolWidget::GetFy)
						.OnValueChanged(this, &SPnPToolWidget::SetFy)
						.MinValue(1.0).MaxValue(100000.0).Delta(1.0))
			]

			+ SScrollBox::Slot().Padding(2)
			[
				MakeRow(LOCTEXT("cxLabel", "cx"),
					SNew(SSpinBox<double>)
						.Value(this, &SPnPToolWidget::GetCx)
						.OnValueChanged(this, &SPnPToolWidget::SetCx)
						.MinValue(0.0).MaxValue(8192.0).Delta(1.0))
			]

			+ SScrollBox::Slot().Padding(2)
			[
				MakeRow(LOCTEXT("cyLabel", "cy"),
					SNew(SSpinBox<double>)
						.Value(this, &SPnPToolWidget::GetCy)
						.OnValueChanged(this, &SPnPToolWidget::SetCy)
						.MinValue(0.0).MaxValue(8192.0).Delta(1.0))
			]

			+ SScrollBox::Slot().Padding(2)
			[
				MakeRow(LOCTEXT("fovLabel", "FOV(deg)"),
					SNew(SSpinBox<double>)
						.Value(this, &SPnPToolWidget::GetFov)
						.OnValueChanged(this, &SPnPToolWidget::SetFov)
						.MinValue(1.0).MaxValue(170.0).Delta(0.1))
			]

			+ SScrollBox::Slot().Padding(2)
			[
				MakeRow(LOCTEXT("resWLabel", "Res W"),
					SNew(SSpinBox<int32>)
						.Value(this, &SPnPToolWidget::GetResW)
						.OnValueChanged(this, &SPnPToolWidget::SetResW)
						.MinValue(1).MaxValue(8192).Delta(1))
			]

			+ SScrollBox::Slot().Padding(2)
			[
				MakeRow(LOCTEXT("resHLabel", "Res H"),
					SNew(SSpinBox<int32>)
						.Value(this, &SPnPToolWidget::GetResH)
						.OnValueChanged(this, &SPnPToolWidget::SetResH)
						.MinValue(1).MaxValue(8192).Delta(1))
			]

			// 源 SceneCapture2D 选择器（RT 尺寸自动从其 TextureTarget 获取）
			+ SScrollBox::Slot().Padding(2)
			[
				MakeRow(LOCTEXT("capLabel", "Capture2D"),
					SNew(SObjectPropertyEntryBox)
						.AllowedClass(ASceneCapture2D::StaticClass())
						.ObjectPath(this, &SPnPToolWidget::GetSourceCapturePath)
						.OnObjectChanged(this, &SPnPToolWidget::OnSourceCaptureChanged)
				)
			]

			// 从 Capture2D 获取内参按钮
			+ SScrollBox::Slot().Padding(2).HAlign(HAlign_Left)
			[
				SNew(SButton)
					.Text(LOCTEXT("GetIntrinsicsBtn", "从 Capture2D 获取内参"))
					.OnClicked(this, &SPnPToolWidget::OnGetIntrinsicsClicked)
			]

			// 清除标记点按钮
		+ SScrollBox::Slot().Padding(2).HAlign(HAlign_Left)
		[
			SNew(SButton)
				.Text(LOCTEXT("ClearMarkersBtn", "清除所有标记点"))
				.OnClicked(this, &SPnPToolWidget::OnClearMarkersClicked)
		]

		// === 点对输入控制 ===
		+ SScrollBox::Slot().Padding(2, 6, 2, 2)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(4)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PairInputHeader", "点对输入"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				]
				// 当前编辑状态显示
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 2)
				[
					SAssignNew(CurrentStateText, STextBlock)
					.Text(FText::FromString(TEXT("状态：空闲")))
					.ColorAndOpacity(FLinearColor::Gray)
				]
				// 三个按钮：添加3D点 / 添加2D点 / 取消
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
					[
						SNew(SButton)
						.Content()
						[
							SAssignNew(Add3DBtnText, STextBlock)
							.Text(FText::FromString(TEXT("添加3D点")))
							.Justification(ETextJustify::Type::Center)
						]
						.OnClicked(this, &SPnPToolWidget::OnAdd3DPointClicked)
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
					[
						SNew(SButton)
						.Content()
						[
							SAssignNew(Add2DBtnText, STextBlock)
							.Text(FText::FromString(TEXT("添加2D点")))
							.Justification(ETextJustify::Type::Center)
						]
						.OnClicked(this, &SPnPToolWidget::OnAdd2DPointClicked)
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
					[
						SNew(SButton)
						.Text(LOCTEXT("CancelEditBtn", "取消"))
						.OnClicked(this, &SPnPToolWidget::OnCancelEditClicked)
					]
				]
			]
		]

		// 配对列表（可编辑）
		+ SScrollBox::Slot().Padding(2, 4, 2, 2)
		[
			SAssignNew(PairsListContainer, SVerticalBox)
		]

		// 求解按钮
		+ SScrollBox::Slot().Padding(2, 8, 2, 2).HAlign(HAlign_Left)
		[
			SNew(SButton)
				.Text(LOCTEXT("SolveBtn", "执行 PnP 求解"))
				.OnClicked(this, &SPnPToolWidget::OnSolveClicked)
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
					.Text(LOCTEXT("RTPreviewHeader", "RT 内容预览（点'添加2D点'按钮后在此点击放置 2D 点）"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			]

			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SNew(SBorder).BorderBackgroundColor(FLinearColor::Black)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SAssignNew(m_RTContainerBox, SBox)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					.MinAspectRatio(TAttribute<FOptionalSize>::Create(TAttribute<FOptionalSize>::FGetter::CreateSP(this, &SPnPToolWidget::GetRTAspectRatio)))
					.MaxAspectRatio(TAttribute<FOptionalSize>::Create(TAttribute<FOptionalSize>::FGetter::CreateSP(this, &SPnPToolWidget::GetRTAspectRatio)))
					[
						SNew(SBorder)
						.OnMouseButtonDown(this, &SPnPToolWidget::OnRTMouseDown)
						[
							SNew(SOverlay)
							+ SOverlay::Slot()
							.HAlign(HAlign_Fill)
							.VAlign(VAlign_Fill)
							[
								SAssignNew(m_RTImageWidget, SImage)
								.Image(this, &SPnPToolWidget::GetRTBrush)
							]
							+ SOverlay::Slot()
							.HAlign(HAlign_Fill)
							.VAlign(VAlign_Fill)
							[
								SAssignNew(RTOverlayImage, SImage)
								.Image(&RTMarkerBrush)
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
				SNew(STextBlock)
					.Text(LOCTEXT("ResultsHeader", "PnP 计算结果 / 日志"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			]

			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0, 4, 0, 0)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						if (!bLastSolveSuccess)
						{
							return FText::FromString(TEXT("未求解 / 求解失败"));
						}
						const FVector L = SolvedPose.GetLocation();
						const FRotator R = SolvedPose.Rotator();
						return FText::FromString(FString::Printf(
							TEXT("求解成功\n\n位置: (%.2f, %.2f, %.2f) cm\n旋转: P=%.2f Y=%.2f R=%.2f deg\n\n重投影误差: %.6f\n平移误差: %.4f cm\n旋转误差: %.4f deg"),
							L.X, L.Y, L.Z, R.Pitch, R.Yaw, R.Roll,
							ReprojectionError, TranslationError, RotationError));
					})
					.AutoWrapText(true)
				]
			]

			// 日志面板
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0, 4, 0, 0)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(4)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
							.Text(LOCTEXT("LogHeader", "操作日志"))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
							.ColorAndOpacity(FLinearColor::Gray)
					]
					+ SVerticalBox::Slot().FillHeight(1.0f)
					[
						SAssignNew(MessagesScrollBox, SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(MessagesTextWidget, STextBlock)
							.Text_Lambda([this]()
							{
								TArray<FText> TextMessages;
								TextMessages.Reserve(Messages.Num());
								for (const FString& Msg : Messages)
								{
									TextMessages.Add(FText::FromString(Msg));
								}
								return FText::Join(FText::FromString(TEXT("\n")), TextMessages);
							})
							.AutoWrapText(true)
							.ColorAndOpacity(FLinearColor::White)
						]
					]
				]
			]
		];
}

FString SPnPToolWidget::GetSourceCapturePath() const
{
	if (m_SourceCapture.IsValid())
	{
		return m_SourceCapture->GetPathName();
	}
	return FString();
}

void SPnPToolWidget::OnSourceCaptureChanged(const FAssetData& InAssetData)
{
	m_SourceCapture = Cast<ASceneCapture2D>(InAssetData.GetAsset());
	if (m_SourceCapture.IsValid())
	{
		LogMessage(FString::Printf(TEXT("[Capture2D] 源 SceneCapture2D 已选择: %s"), *m_SourceCapture->GetPathName()));

		// 从 SceneCapture2D 的 TextureTarget 获取 RT 尺寸并同步 DisplayRT
		if (USceneCaptureComponent2D* Comp = m_SourceCapture->GetCaptureComponent2D())
		{
			if (UTextureRenderTarget2D* SrcRT = Comp->TextureTarget)
			{
				ResizeDisplayRT(SrcRT->SizeX, SrcRT->SizeY);
				// 同步内参，保证 ProjectWorldToImage 输出空间 == m_DisplayRT 像素空间
				RecomputeIntrinsicsFromSource();
				LogMessage(FString::Printf(TEXT("[内参] 已随源采集器同步: fx=%.2f fy=%.2f cx=%.2f cy=%.2f Res=%dx%d FOV=%.2f"),
					Fx, Fy, Cx, Cy, Resolution.X, Resolution.Y, Fov));
			}
			else
			{
				LogMessage(TEXT("[Capture2D] 警告：SceneCapture2D 没有 TextureTarget"));
			}
		}
	}
}

void SPnPToolWidget::RecomputeIntrinsicsFromSource()
{
	if (!m_SourceCapture.IsValid()) return;
	USceneCaptureComponent2D* Comp = m_SourceCapture->GetCaptureComponent2D();
	if (!Comp || !Comp->TextureTarget) return;

	UTextureRenderTarget2D* RT = Comp->TextureTarget;
	const int32 W = RT->SizeX;
	const int32 H = RT->SizeY;
	const float FOV = Comp->FOVAngle;

	const float HalfFOVRad = FMath::DegreesToRadians(FOV) * 0.5f;
	const float Focal = (W * 0.5f) / FMath::Tan(HalfFOVRad);

	Fx = Focal;
	Fy = Focal;
	Cx = W * 0.5f;
	Cy = H * 0.5f;
	Fov = FOV;
	Resolution = FIntPoint(W, H);
}

FReply SPnPToolWidget::OnGetIntrinsicsClicked()
{
	if (!m_SourceCapture.IsValid())
	{
		LogMessage(TEXT("[警告] 请先选择源 SceneCapture2D"));
		return FReply::Handled();
	}

	USceneCaptureComponent2D* Comp = m_SourceCapture->GetCaptureComponent2D();
	if (!Comp || !Comp->TextureTarget)
	{
		LogMessage(TEXT("[警告] SceneCapture2D 无 TextureTarget"));
		return FReply::Handled();
	}

	RecomputeIntrinsicsFromSource();

	LogMessage(FString::Printf(TEXT("[内参] fx=%.2f fy=%.2f cx=%.2f cy=%.2f Res=%dx%d FOV=%.2f"),
		Fx, Fy, Cx, Cy, Resolution.X, Resolution.Y, Fov));

	return FReply::Handled();
}

void SPnPToolWidget::EnsureSceneCapture()
{
	if (m_SceneCapture.IsValid()) return;

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	m_SceneCapture = World->SpawnActor<ASceneCapture2D>(ASceneCapture2D::StaticClass(), FTransform::Identity, Params);

	if (ASceneCapture2D* Cap = m_SceneCapture.Get())
	{
		Cap->SetIsTemporarilyHiddenInEditor(true);

		if (USceneCaptureComponent2D* Comp = Cap->GetCaptureComponent2D())
		{
			Comp->TextureTarget = m_ScenePreviewRT.Get();
			Comp->bCaptureEveryFrame = true;
			Comp->bCaptureOnMovement = false;
			Comp->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		}
	}
}

void SPnPToolWidget::EnsureRightSceneCapture()
{
	if (m_RightSceneCapture.IsValid()) return;

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	m_RightSceneCapture = World->SpawnActor<ASceneCapture2D>(ASceneCapture2D::StaticClass(), FTransform::Identity, Params);

	if (ASceneCapture2D* Cap = m_RightSceneCapture.Get())
	{
		Cap->SetIsTemporarilyHiddenInEditor(true);

		if (USceneCaptureComponent2D* Comp = Cap->GetCaptureComponent2D())
		{
			Comp->TextureTarget = m_DisplayRT.Get();
			Comp->bCaptureEveryFrame = true;
			Comp->bCaptureOnMovement = false;
			Comp->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		}
	}
}

void SPnPToolWidget::ResizeDisplayRT(int32 NewW, int32 NewH)
{
	if (NewW <= 0 || NewH <= 0) return;
	if (!m_DisplayRT.IsValid()) return;

	// 尺寸相同则无需重建
	if (m_DisplayRT->SizeX == NewW && m_DisplayRT->SizeY == NewH)
	{
		return;
	}

	// 重新初始化 RT 尺寸（InitAutoFormat 会自动选择合适的像素格式）
	m_DisplayRT->InitAutoFormat(NewW, NewH);
	m_DisplayRT->UpdateResource();

	// 更新 brush 指向（始终指向 DisplayRT，由 RightSceneCapture 自行捕获内容）
	RTBrush.SetResourceObject(nullptr);
	RTBrush.SetResourceObject(m_DisplayRT.Get());
	RTBrush.ImageSize = FVector2D(NewW, NewH);
	RTBrush.DrawAs = ESlateBrushDrawType::Image;
	RTBrush.TintColor = FSlateColor(FLinearColor::White);
#if WITH_EDITOR
	RTBrush.InvalidateResourceHandle();
#endif

	if (m_RTImageWidget.IsValid())
	{
		m_RTImageWidget->Invalidate(EInvalidateWidget::Layout | EInvalidateWidget::Paint);
	}

	LogMessage(FString::Printf(TEXT("[RT] DisplayRT 已调整尺寸为 %dx%d"), NewW, NewH));
}

void SPnPToolWidget::UpdateSceneCaptureFromActiveViewport() const
{
	if (!m_SceneCapture.IsValid()) return;

	FViewport* ActiveVP = GEditor ? GEditor->GetActiveViewport() : nullptr;
	FEditorViewportClient* VPC = (ActiveVP && ActiveVP->GetClient())
		? static_cast<FEditorViewportClient*>(ActiveVP->GetClient())
		: nullptr;

	if (VPC && VPC->IsPerspective())
	{
		m_SceneCapture->SetActorLocation(VPC->GetViewLocation());
		m_SceneCapture->SetActorRotation(VPC->GetViewRotation());
	}
}

void SPnPToolWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	EnsureSceneCapture();
	EnsureRightSceneCapture();
	UpdateSceneCaptureFromActiveViewport();

	// 将右侧插件创建的 SceneCapture 位置/朝向与用户选择的 SourceCapture 同步（若无，则跟随编辑器视口）
	if (m_RightSceneCapture.IsValid())
	{
		if (m_SourceCapture.IsValid())
		{
			m_RightSceneCapture->SetActorLocation(m_SourceCapture->GetActorLocation());
			m_RightSceneCapture->SetActorRotation(m_SourceCapture->GetActorRotation());

			// 同步 FOV，使右侧视口捕获画面与 SourceCapture 一致
			if (USceneCaptureComponent2D* RightComp = m_RightSceneCapture->GetCaptureComponent2D())
			{
				if (USceneCaptureComponent2D* SrcComp = m_SourceCapture->GetCaptureComponent2D())
				{
					RightComp->FOVAngle = SrcComp->FOVAngle;
				}
			}
		}
		else
		{
			FViewport* ActiveVP = GEditor ? GEditor->GetActiveViewport() : nullptr;
			FEditorViewportClient* VPC = (ActiveVP && ActiveVP->GetClient()) ? static_cast<FEditorViewportClient*>(ActiveVP->GetClient()) : nullptr;
			if (VPC && VPC->IsPerspective())
			{
				m_RightSceneCapture->SetActorLocation(VPC->GetViewLocation());
				m_RightSceneCapture->SetActorRotation(VPC->GetViewRotation());
			}
		}
	}

	// 实时跟随 SourceCapture 的 TextureTarget 尺寸，保持 DisplayRT 与之一致
	if (m_SourceCapture.IsValid() && m_DisplayRT.IsValid())
	{
		if (USceneCaptureComponent2D* Comp = m_SourceCapture->GetCaptureComponent2D())
		{
			if (UTextureRenderTarget2D* SrcRT = Comp->TextureTarget)
			{
				if (m_DisplayRT->SizeX != SrcRT->SizeX || m_DisplayRT->SizeY != SrcRT->SizeY)
				{
					ResizeDisplayRT(SrcRT->SizeX, SrcRT->SizeY);
					// 尺寸变化时同步内参，避免投影空间与显示空间错位
					RecomputeIntrinsicsFromSource();
				}
			}
		}
	}

	// 同步 Marker Actor 位置 → 数据（用户用 Gizmo 拖动后自动更新配对）
	SyncMarkerActorsInTick();

	// 只在点数 / 输入模式 / 活动 pair / Pending 状态变化时重绘 debug
	static int32 LastObjCount = -1;
	static EInputMode LastMode = EInputMode::Idle;
	static int32 LastActiveIdx = INDEX_NONE;
	static bool bLastPending3D = false;
	const bool bHasPending3D = Pending3DPoint.IsSet();
	const bool bNeedRedraw = (LastObjCount != ManualObjectPoints.Num())
		|| (LastMode != InputMode)
		|| (LastActiveIdx != ActivePairIndex)
		|| (bLastPending3D != bHasPending3D);
	if (bNeedRedraw)
	{
		DrawManualMarkers();
		LastObjCount = ManualObjectPoints.Num();
		LastMode = InputMode;
		LastActiveIdx = ActivePairIndex;
		bLastPending3D = bHasPending3D;
	}
}

FOptionalSize SPnPToolWidget::GetSceneAspectRatio() const
{
	if (m_ScenePreviewRT.IsValid() && m_ScenePreviewRT->SizeY > 0)
	{
		return FOptionalSize(static_cast<float>(m_ScenePreviewRT->SizeX) / static_cast<float>(m_ScenePreviewRT->SizeY));
	}
	return FOptionalSize(16.0f / 9.0f);
}

FOptionalSize SPnPToolWidget::GetRTAspectRatio() const
{
	if (m_DisplayRT.IsValid() && m_DisplayRT->SizeY > 0)
	{
		return FOptionalSize(static_cast<float>(m_DisplayRT->SizeX) / static_cast<float>(m_DisplayRT->SizeY));
	}
	return FOptionalSize(16.0f / 9.0f);
}

FVector2D SPnPToolWidget::ProjectWorldToImage(const FVector& WorldPoint) const
{
	if (!m_SourceCapture.IsValid())
	{
		return FVector2D(-1.0, -1.0);
	}
	const FVector CamLoc = m_SourceCapture->GetActorLocation();
	const FRotator CamRot = m_SourceCapture->GetActorRotation();
	// 世界 → UE 相机空间（X 前 Y 右 Z 上）
	const FVector PCam = CamRot.UnrotateVector(WorldPoint - CamLoc);
	if (PCam.X <= KINDA_SMALL_NUMBER)
	{
		return FVector2D(-1.0, -1.0);
	}
	// OpenCV 针孔模型：u = fx * Y'/X' + cx, v = fy * (-Z'/X') + cy
	const double U = Fx * (PCam.Y / PCam.X) + Cx;
	const double V = Fy * (-PCam.Z / PCam.X) + Cy;
	return FVector2D(U, V);
}

void SPnPToolWidget::RebuildPairsList()
{
	if (!PairsListContainer.IsValid()) return;
	PairsListContainer->ClearChildren();

	// 表头
	PairsListContainer->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 2, 6, 2)
		[
			SNew(STextBlock).Text(LOCTEXT("PairsHeader", "已提交配对列表（黄色=编辑中 | 编辑3D/编辑2D=重新放置该点 | SpinBox可直接改数值）"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		]
	];

	// 清空旧的行引用（widget 已被销毁）
	PairRowWidgets.Empty();

	if (ManualObjectPoints.Num() == 0)
	{
		PairsListContainer->AddSlot().AutoHeight().Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("PairsEmpty", "（无配对。点上方'添加3D点'开始新建 pair）"))
			.ColorAndOpacity(FLinearColor::Gray)
		];
		return;
	}

	for (int32 i = 0; i < ManualObjectPoints.Num(); ++i)
	{
		const int32 CapturedIdx = i;

		// 创建 5 个 SpinBox（先创建，后面需要引用填充）
		TSharedRef<SSpinBox<double>> Spin3DX = SNew(SSpinBox<double>)
			.Value(ManualObjectPoints[i].X)
			.MinValue(-1000000.0).MaxValue(1000000.0).Delta(1.0)
			.MinDesiredWidth(70.0f)
			.OnValueCommitted_Lambda([this, CapturedIdx](double V, ETextCommit::Type)
			{
				if (ManualObjectPoints.IsValidIndex(CapturedIdx))
				{
					ManualObjectPoints[CapturedIdx].X = V;
					if (AActor* M = MarkerActors.IsValidIndex(CapturedIdx) ? MarkerActors[CapturedIdx].Get() : nullptr)
						M->SetActorLocation(ManualObjectPoints[CapturedIdx]);
					DrawManualMarkers();
				}
			});

		TSharedRef<SSpinBox<double>> Spin3DY = SNew(SSpinBox<double>)
			.Value(ManualObjectPoints[i].Y)
			.MinValue(-1000000.0).MaxValue(1000000.0).Delta(1.0)
			.MinDesiredWidth(70.0f)
			.OnValueCommitted_Lambda([this, CapturedIdx](double V, ETextCommit::Type)
			{
				if (ManualObjectPoints.IsValidIndex(CapturedIdx))
				{
					ManualObjectPoints[CapturedIdx].Y = V;
					if (AActor* M = MarkerActors.IsValidIndex(CapturedIdx) ? MarkerActors[CapturedIdx].Get() : nullptr)
						M->SetActorLocation(ManualObjectPoints[CapturedIdx]);
					DrawManualMarkers();
				}
			});

		TSharedRef<SSpinBox<double>> Spin3DZ = SNew(SSpinBox<double>)
			.Value(ManualObjectPoints[i].Z)
			.MinValue(-1000000.0).MaxValue(1000000.0).Delta(1.0)
			.MinDesiredWidth(70.0f)
			.OnValueCommitted_Lambda([this, CapturedIdx](double V, ETextCommit::Type)
			{
				if (ManualObjectPoints.IsValidIndex(CapturedIdx))
				{
					ManualObjectPoints[CapturedIdx].Z = V;
					if (AActor* M = MarkerActors.IsValidIndex(CapturedIdx) ? MarkerActors[CapturedIdx].Get() : nullptr)
						M->SetActorLocation(ManualObjectPoints[CapturedIdx]);
					DrawManualMarkers();
				}
			});

		TSharedRef<SSpinBox<double>> Spin2DU = SNew(SSpinBox<double>)
			.Value(ManualImagePoints[i].X)
			.MinValue(-1000000.0).MaxValue(1000000.0).Delta(1.0)
			.MinDesiredWidth(70.0f)
			.OnValueCommitted_Lambda([this, CapturedIdx](double V, ETextCommit::Type)
			{
				if (ManualImagePoints.IsValidIndex(CapturedIdx))
				{
					ManualImagePoints[CapturedIdx].X = V;
					UpdateRTMarkerOverlay();
				}
			});

		TSharedRef<SSpinBox<double>> Spin2DV = SNew(SSpinBox<double>)
			.Value(ManualImagePoints[i].Y)
			.MinValue(-1000000.0).MaxValue(1000000.0).Delta(1.0)
			.MinDesiredWidth(70.0f)
			.OnValueCommitted_Lambda([this, CapturedIdx](double V, ETextCommit::Type)
			{
				if (ManualImagePoints.IsValidIndex(CapturedIdx))
				{
					ManualImagePoints[CapturedIdx].Y = V;
					UpdateRTMarkerOverlay();
				}
			});

		TSharedRef<SBorder> RowBorder = SNew(SBorder)
		.Padding(2);

	TSharedPtr<STextBlock> Edit3DBtnText;
	TSharedPtr<STextBlock> Edit2DBtnText;

	// 编辑 3D 点按钮：选中该 pair 并进入 3D 点编辑模式
	TSharedRef<SButton> Edit3DBtn = SNew(SButton)
		.Content()
		[
			SAssignNew(Edit3DBtnText, STextBlock)
			.Text(FText::FromString(TEXT("编辑3D")))
			.Justification(ETextJustify::Type::Center)
		]
		.OnClicked_Lambda([this, CapturedIdx]() -> FReply
		{
			// 如果有未提交的 Pending pair，先取消
			CancelPendingEdit();
			ActivePairIndex = CapturedIdx;
			InputMode = EInputMode::Edit3D;
			UpdateInputModeUI();
			DrawManualMarkers();
			UpdateRTMarkerOverlay();
			LogMessage(FString::Printf(TEXT("[编辑] 配对 #%d 进入 3D 点编辑模式，请在左侧场景视口点击新位置"), CapturedIdx));
			return FReply::Handled();
		});

	// 编辑 2D 点按钮：选中该 pair 并进入 2D 点编辑模式
	TSharedRef<SButton> Edit2DBtn = SNew(SButton)
		.Content()
		[
			SAssignNew(Edit2DBtnText, STextBlock)
			.Text(FText::FromString(TEXT("编辑2D")))
			.Justification(ETextJustify::Type::Center)
		]
		.OnClicked_Lambda([this, CapturedIdx]() -> FReply
		{
			// 如果有未提交的 Pending pair，先取消
			CancelPendingEdit();
			ActivePairIndex = CapturedIdx;
			InputMode = EInputMode::Edit2D;
			UpdateInputModeUI();
			DrawManualMarkers();
			UpdateRTMarkerOverlay();
			LogMessage(FString::Printf(TEXT("[编辑] 配对 #%d 进入 2D 点编辑模式，请在右侧 RT 点击新位置"), CapturedIdx));
			return FReply::Handled();
		});

	// 删除按钮
	TSharedRef<SButton> DeleteBtn = SNew(SButton)
		.Text(FText::FromString(TEXT("删除")))
		.OnClicked_Lambda([this, CapturedIdx]() -> FReply
		{
			if (ManualObjectPoints.IsValidIndex(CapturedIdx))
			{
				ManualObjectPoints.RemoveAt(CapturedIdx);
				ManualImagePoints.RemoveAt(CapturedIdx);
				DestroyMarkerActor(CapturedIdx);
				MarkerActors.RemoveAt(CapturedIdx);

				if (ActivePairIndex == CapturedIdx)
				{
					ActivePairIndex = INDEX_NONE;
					InputMode = EInputMode::Idle;
				}
				else if (ActivePairIndex > CapturedIdx)
				{
					ActivePairIndex--;
				}

				RebuildPairsList();
				UpdateInputModeUI();
				UpdateRTMarkerOverlay();
				DrawManualMarkers();
			}
			return FReply::Handled();
		});

		// 组装行：先创建内容，再 SetContent 到 RowBorder（避免 TSharedRef 不支持 [] 操作符的问题）
		RowBorder->SetBorderBackgroundColor(FLinearColor(0.18f, 0.18f, 0.18f, 0.4f));

		TSharedRef<SHorizontalBox> RowContent = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)
			[
				SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("#%d"), i)))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				Spin3DX
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				Spin3DY
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				Spin3DZ
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("<->")))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				Spin2DU
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				Spin2DV
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0)
		[
			Edit3DBtn
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0)
		[
			Edit2DBtn
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0)
		[
			DeleteBtn
		];

	RowBorder->SetContent(RowContent);

	PairsListContainer->AddSlot().AutoHeight().Padding(2)
	[
		RowBorder
	];

	// 存储行引用（TSharedRef → TSharedPtr 自动转换）
	FPairRowWidgets RowW;
	RowW.Spin3DX = Spin3DX;
	RowW.Spin3DY = Spin3DY;
	RowW.Spin3DZ = Spin3DZ;
	RowW.Spin2DU = Spin2DU;
	RowW.Spin2DV = Spin2DV;
	RowW.RowBorder = RowBorder;
	RowW.Edit3DBtnText = Edit3DBtnText;
	RowW.Edit2DBtnText = Edit2DBtnText;
	PairRowWidgets.Add(RowW);
	}

	// 初始活动状态视觉
	UpdateActiveRowVisuals();
}

void SPnPToolWidget::UpdatePairRowValues(int32 Index)
{
	if (!ManualObjectPoints.IsValidIndex(Index) || !ManualImagePoints.IsValidIndex(Index)) return;
	if (!PairRowWidgets.IsValidIndex(Index)) return;

	const FVector& Pt = ManualObjectPoints[Index];
	const FVector2D& UV = ManualImagePoints[Index];
	FPairRowWidgets& Row = PairRowWidgets[Index];

	// 只在值变化时更新（避免不必要的 widget 刷新）
	if (!FMath::IsNearlyEqual(Row.Spin3DX->GetValue(), Pt.X, 0.01))
		Row.Spin3DX->SetValue(Pt.X);
	if (!FMath::IsNearlyEqual(Row.Spin3DY->GetValue(), Pt.Y, 0.01))
		Row.Spin3DY->SetValue(Pt.Y);
	if (!FMath::IsNearlyEqual(Row.Spin3DZ->GetValue(), Pt.Z, 0.01))
		Row.Spin3DZ->SetValue(Pt.Z);
	if (!FMath::IsNearlyEqual(Row.Spin2DU->GetValue(), UV.X, 0.01))
		Row.Spin2DU->SetValue(UV.X);
	if (!FMath::IsNearlyEqual(Row.Spin2DV->GetValue(), UV.Y, 0.01))
		Row.Spin2DV->SetValue(UV.Y);
}

void SPnPToolWidget::UpdateActiveRowVisuals()
{
	for (int32 i = 0; i < PairRowWidgets.Num(); ++i)
	{
		const bool bActive = (i == ActivePairIndex);
		PairRowWidgets[i].RowBorder->SetBorderBackgroundColor(
			bActive ? FLinearColor(1.0f, 0.85f, 0.0f, 0.45f)
					: FLinearColor(0.18f, 0.18f, 0.18f, 0.4f));

		// 编辑3D 按钮文字反映当前是否在编辑该 pair 的 3D 点
		if (PairRowWidgets[i].Edit3DBtnText.IsValid())
		{
			const bool bEditing3D = bActive && (InputMode == EInputMode::Edit3D);
			PairRowWidgets[i].Edit3DBtnText->SetText(FText::FromString(bEditing3D ? TEXT("●3D编辑中") : TEXT("编辑3D")));
		}

		// 编辑2D 按钮文字反映当前是否在编辑该 pair 的 2D 点
		if (PairRowWidgets[i].Edit2DBtnText.IsValid())
		{
			const bool bEditing2D = bActive && (InputMode == EInputMode::Edit2D);
			PairRowWidgets[i].Edit2DBtnText->SetText(FText::FromString(bEditing2D ? TEXT("●2D编辑中") : TEXT("编辑2D")));
		}
	}
}

void SPnPToolWidget::CreateMarkerActor(int32 Index)
{
	if (!ManualObjectPoints.IsValidIndex(Index)) return;
	if (!GEditor) return;
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) return;

	// 缓存 Sphere mesh（避免每次加载）
	static UStaticMesh* SphereMesh = nullptr;
	if (!SphereMesh)
	{
		SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere"));
		if (!SphereMesh) return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AStaticMeshActor* Marker = World->SpawnActor<AStaticMeshActor>(
		ManualObjectPoints[Index], FRotator::ZeroRotator, Params);
	if (!Marker) return;

	if (UStaticMeshComponent* MeshComp = Marker->GetStaticMeshComponent())
	{
		MeshComp->SetStaticMesh(SphereMesh);
		// Engine Sphere 默认 50cm 半径，缩放到 0.1→约 0.5cm，小巧不挡视野
		MeshComp->SetWorldScale3D(FVector(0.1f));
		MeshComp->SetMobility(EComponentMobility::Movable);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
		MeshComp->SetCastShadow(false);
		// 选中时高亮（编辑器默认行为），无需额外材质
	}

	const bool bActive = (Index == ActivePairIndex);
	Marker->SetActorLabel(FString::Printf(TEXT("PnP #%d%s"), Index, bActive ? TEXT(" *") : TEXT("")));
	// 不在场景里保存
	Marker->SetFlags(RF_Transient);

	// 保证数组对齐
	if (MarkerActors.Num() <= Index)
	{
		MarkerActors.SetNum(Index + 1);
	}
	MarkerActors[Index] = Marker;
}

void SPnPToolWidget::DestroyMarkerActor(int32 Index)
{
	if (!MarkerActors.IsValidIndex(Index)) return;
	if (AActor* Marker = MarkerActors[Index].Get())
	{
		if (UWorld* World = Marker->GetWorld())
		{
			World->DestroyActor(Marker, false);
		}
	}
	MarkerActors[Index] = nullptr;
}

void SPnPToolWidget::DestroyAllMarkerActors()
{
	for (int32 i = 0; i < MarkerActors.Num(); ++i)
	{
		if (AActor* Marker = MarkerActors[i].Get())
		{
			if (UWorld* World = Marker->GetWorld())
			{
				World->DestroyActor(Marker, false);
			}
		}
	}
	MarkerActors.Empty();
}

void SPnPToolWidget::SyncMarkerActorsInTick()
{
	// 检测 Marker Actor 被用户用 Gizmo 拖动 → 同步位置到数据
	bool bAnyChanged = false;

	// 1. 已提交 pair 的 Marker
	const int32 Count = FMath::Min(MarkerActors.Num(), ManualObjectPoints.Num());
	for (int32 i = 0; i < Count; ++i)
	{
		AActor* Marker = MarkerActors[i].Get();
		if (!Marker) continue;
		const FVector Current = Marker->GetActorLocation();
		if (!Current.Equals(ManualObjectPoints[i], 0.5))
		{
			ManualObjectPoints[i] = Current;

			// 编辑 3D 点模式且为活动 pair 时，2D 点可选择性重投影（这里不自动重投影，保持用户手动值）
			// 仅更新 3D 点数据，2D 点由用户在 RT 点击时设置

			// 原地更新该行程数值（不重建列表，避免闪烁）
			UpdatePairRowValues(i);
			bAnyChanged = true;
		}
	}

	// 2. Pending Marker（新建 pair 的 3D 点）
	if (PendingMarker.IsValid() && Pending3DPoint.IsSet())
	{
		AActor* Marker = PendingMarker.Get();
		const FVector Current = Marker->GetActorLocation();
		if (!Current.Equals(Pending3DPoint.GetValue(), 0.5))
		{
			Pending3DPoint = Current;
			bAnyChanged = true;
		}
	}

	if (bAnyChanged)
	{
		UpdateInputModeUI();
		UpdateRTMarkerOverlay();
	}
}

void SPnPToolWidget::DrawManualMarkers()
{
	// 在编辑器视口中绘制已选 3D 点
	if (!GEditor) return;
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) return;

	// 清除旧的持久调试绘制
	FlushPersistentDebugLines(World);

	// 已提交 pair 的 Marker：更新 Label 和 Scale 反映编辑状态
	for (int32 i = 0; i < ManualObjectPoints.Num(); ++i)
	{
		if (AActor* Marker = MarkerActors.IsValidIndex(i) ? MarkerActors[i].Get() : nullptr)
		{
			const bool bActive = (i == ActivePairIndex);
			FString Suffix;
			if (bActive)
			{
				if (InputMode == EInputMode::Edit3D) Suffix += TEXT(" [3D编辑中]");
				else if (InputMode == EInputMode::Edit2D) Suffix += TEXT(" [2D编辑中]");
				else Suffix += TEXT(" *");
			}
			Marker->SetActorLabel(FString::Printf(TEXT("PnP #%d%s"), i, *Suffix));
			if (UStaticMeshComponent* MeshComp = Cast<AStaticMeshActor>(Marker) ? Cast<AStaticMeshActor>(Marker)->GetStaticMeshComponent() : nullptr)
			{
				float Scale = 0.1f;
				if (bActive)
				{
					if (InputMode == EInputMode::Edit3D) Scale = 0.2f;
					else Scale = 0.15f;
				}
				MeshComp->SetWorldScale3D(FVector(Scale));
			}
		}
	}

	// Pending Marker（新建 pair 的 3D 点，未提交）
	if (PendingMarker.IsValid())
	{
		AActor* Marker = PendingMarker.Get();
		FString Suffix;
		if (InputMode == EInputMode::Edit3D) Suffix = TEXT(" [3D输入中]");
		else if (InputMode == EInputMode::Edit2D) Suffix = TEXT(" [2D输入中]");
		else Suffix = TEXT(" [待提交]");
		Marker->SetActorLabel(FString::Printf(TEXT("PnP Pending%s"), *Suffix));
		if (UStaticMeshComponent* MeshComp = Cast<AStaticMeshActor>(Marker) ? Cast<AStaticMeshActor>(Marker)->GetStaticMeshComponent() : nullptr)
		{
			const float Scale = (InputMode == EInputMode::Edit3D) ? 0.2f : 0.15f;
			MeshComp->SetWorldScale3D(FVector(Scale));
		}
	}
}

FReply SPnPToolWidget::OnSceneMouseDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Handled();
	if (!m_ScenePreviewRT.IsValid() || !m_SceneCapture.IsValid()) return FReply::Handled();

	// 只有 Edit3D 模式才处理 3D 视口点击
	if (InputMode != EInputMode::Edit3D)
	{
		LogMessage(TEXT("[提示] 请先点'添加3D点'按钮进入 3D 点输入模式"));
		return FReply::Handled();
	}

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World) return FReply::Handled();

	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D WidgetSize = MyGeometry.GetLocalSize();
	if (WidgetSize.X <= 0 || WidgetSize.Y <= 0) return FReply::Handled();

	// MyGeometry 来自内层 SBorder，它跟 SBox/SImage 一样大，brush 拉伸填充
	// 直接用 LocalPos / WidgetSize = UV [0,1]
	const FVector2D UVPos = LocalPos / WidgetSize;
	if (UVPos.X < 0.0f || UVPos.X > 1.0f || UVPos.Y < 0.0f || UVPos.Y > 1.0f)
	{
		return FReply::Handled();
	}

	// UV 转归一化设备坐标 [-1, 1]
	const float ScreenX = UVPos.X * 2.0f - 1.0f;
	const float ScreenY = 1.0f - UVPos.Y * 2.0f;

	// FOV：优先用 SceneCapture 的 FOV
	float ViewportFOV = Fov;
	if (USceneCaptureComponent2D* Comp = m_SceneCapture->GetCaptureComponent2D())
	{
		ViewportFOV = Comp->FOVAngle;
	}

	const float HalfFOVRad = FMath::DegreesToRadians(ViewportFOV) * 0.5f;
	const float RTAspect = static_cast<float>(m_ScenePreviewRT->SizeX) / static_cast<float>(m_ScenePreviewRT->SizeY);

	// UE 的 FOVAngle 是水平 FOV
	const float TanHalfFOVH = FMath::Tan(HalfFOVRad);
	const float TanHalfFOVV = TanHalfFOVH / RTAspect;

	// 相机空间射线方向：X=前 Y=右 Z=上
	const FVector RayDirLocal(1.0f, ScreenX * TanHalfFOVH, ScreenY * TanHalfFOVV);

	// 转世界方向
	const FVector CamPos = m_SceneCapture->GetActorLocation();
	const FRotator CamRot = m_SceneCapture->GetActorRotation();
	const FVector RayDirWorld = CamRot.RotateVector(RayDirLocal).GetSafeNormal();
	const FVector RayEnd = CamPos + RayDirWorld * 10000.0f;

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(PnPRaycast), false, m_SceneCapture.Get());
	FHitResult HitResult;
	const bool bHit = World->LineTraceSingleByChannel(HitResult, CamPos, RayEnd, ECC_Visibility, TraceParams);

	if (bHit)
	{
		const FVector HitPoint = HitResult.ImpactPoint;

		if (ActivePairIndex != INDEX_NONE && ManualObjectPoints.IsValidIndex(ActivePairIndex))
		{
			// 编辑已提交 pair 的 3D 点
			ManualObjectPoints[ActivePairIndex] = HitPoint;
			DestroyMarkerActor(ActivePairIndex);
			CreateMarkerActor(ActivePairIndex);
			UpdatePairRowValues(ActivePairIndex);
			LogMessage(FString::Printf(TEXT("[编辑] 配对 #%d 的 3D 点已更新为 (%.1f, %.1f, %.1f)"),
				ActivePairIndex, HitPoint.X, HitPoint.Y, HitPoint.Z));
		}
		else
		{
			// 新建 pair 的 3D 点
			Pending3DPoint = HitPoint;

			// 创建/更新 Pending Marker
			if (PendingMarker.IsValid())
			{
				PendingMarker->SetActorLocation(HitPoint);
			}
			else
			{
				// 创建新的 Pending Marker（内联创建，不存入 MarkerActors）
				static UStaticMesh* SphereMesh = nullptr;
				if (!SphereMesh)
				{
					SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere"));
				}
				if (SphereMesh)
				{
					FActorSpawnParameters Params;
					Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					AStaticMeshActor* Marker = World->SpawnActor<AStaticMeshActor>(HitPoint, FRotator::ZeroRotator, Params);
					if (Marker)
					{
						if (UStaticMeshComponent* MeshComp = Marker->GetStaticMeshComponent())
						{
							MeshComp->SetStaticMesh(SphereMesh);
							MeshComp->SetWorldScale3D(FVector(0.2f));
							MeshComp->SetMobility(EComponentMobility::Movable);
							MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
							MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
							MeshComp->SetCastShadow(false);
						}
						Marker->SetActorLabel(TEXT("PnP Pending"));
						Marker->SetFlags(RF_Transient);
						PendingMarker = Marker;
					}
				}
			}

			LogMessage(FString::Printf(TEXT("[输入] 3D 点已设置: (%.1f, %.1f, %.1f) | 可继续点击或拖 Gizmo 调整；完成后点'添加2D点'"),
				HitPoint.X, HitPoint.Y, HitPoint.Z));
		}

		UpdateInputModeUI();
		DrawManualMarkers();
	}

	return FReply::Handled();
}

FReply SPnPToolWidget::OnRTMouseDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Handled();
	if (!m_DisplayRT.IsValid()) return FReply::Handled();

	// 只有 Edit2D 模式才处理 RT 点击
	if (InputMode != EInputMode::Edit2D)
	{
		LogMessage(TEXT("[提示] 请先点'添加2D点'按钮进入 2D 点输入模式"));
		return FReply::Handled();
	}

	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D WidgetSize = MyGeometry.GetLocalSize();
	if (WidgetSize.X <= 0 || WidgetSize.Y <= 0) return FReply::Handled();

	// MyGeometry 来自内层 SBorder，跟 SOverlay/SImage 一样大，brush 拉伸填充
	// 直接用 LocalPos / WidgetSize = UV [0,1]
	const FVector2D UVPos = LocalPos / WidgetSize;
	if (UVPos.X < 0.0f || UVPos.X > 1.0f || UVPos.Y < 0.0f || UVPos.Y > 1.0f)
	{
		return FReply::Handled();
	}

	// UV 转像素坐标（使用 DisplayRT，即实际显示的 RT 尺寸）
	const float Px = UVPos.X * m_DisplayRT->SizeX;
	const float Py = UVPos.Y * m_DisplayRT->SizeY;

	if (ActivePairIndex != INDEX_NONE && ManualImagePoints.IsValidIndex(ActivePairIndex))
	{
		// 编辑已提交 pair 的 2D 点
		ManualImagePoints[ActivePairIndex] = FVector2D(Px, Py);
		UpdatePairRowValues(ActivePairIndex);
		LogMessage(FString::Printf(TEXT("[编辑] 配对 #%d 的 2D 点已更新为 (%.1f, %.1f)"),
			ActivePairIndex, Px, Py));
	}
	else
	{
		// 新建 pair 的 2D 点
		Pending2DPoint = FVector2D(Px, Py);
		LogMessage(FString::Printf(TEXT("[输入] 2D 点已设置: (%.1f, %.1f) | 可继续在 RT 点击调整；完成后点'添加3D点'提交并开始下一对"),
			Px, Py));
	}

	UpdateInputModeUI();
	UpdateRTMarkerOverlay();
	DrawManualMarkers();

	return FReply::Handled();
}

void SPnPToolWidget::UpdateRTMarkerOverlay()
{
	if (!m_DisplayRT.IsValid())
	{
		RTMarkerBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
		if (RTOverlayImage.IsValid())
		{
			RTOverlayImage->Invalidate(EInvalidateWidget::Paint);
		}
		return;
	}

	const int32 RTW = m_DisplayRT->SizeX;
	const int32 RTH = m_DisplayRT->SizeY;
	if (RTW <= 0 || RTH <= 0) return;

	// 创建动态 Texture2D（用 CreateTransient 避免手动管理 PlatformData）
	UTexture2D* OverlayTex = UTexture2D::CreateTransient(RTW, RTH, PF_B8G8R8A8);
	if (!OverlayTex) return;
	OverlayTex->AddToRoot();
	OverlayTex->Filter = TF_Nearest;

	if (!OverlayTex->GetPlatformData() || OverlayTex->GetPlatformData()->Mips.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PnPToolWidget] OverlayTex 平台数据无效"));
		return;
	}

	FTexture2DMipMap& Mip = OverlayTex->GetPlatformData()->Mips[0];

	const int32 NumPixels = RTW * RTH;
	TArray<uint8> Pixels;
	Pixels.SetNumZeroed(NumPixels * 4);

	auto SetPixel = [&](int32 PX, int32 PY, uint8 R, uint8 G, uint8 B, uint8 A)
	{
		if (PX < 0 || PX >= RTW || PY < 0 || PY >= RTH) return;
		const int32 Idx = (PY * RTW + PX) * 4;
		Pixels[Idx + 0] = B;
		Pixels[Idx + 1] = G;
		Pixels[Idx + 2] = R;
		Pixels[Idx + 3] = A;
	};

	// 画十字 + 中心实心圆 + 外圈空心环；颜色由调用方传入（活动=黄色, 其余=绿色）
	auto DrawPoint = [&](int32 X, int32 Y, uint8 R, uint8 G, uint8 B)
	{
		// 长十字（半径 20px）
		const int32 Size = 20;
		for (int32 dx = -Size; dx <= Size; ++dx) SetPixel(X + dx, Y, R, G, B, 255);
		for (int32 dy = -Size; dy <= Size; ++dy) SetPixel(X, Y + dy, R, G, B, 255);

		// 中心实心圆（半径 6px）
		const int32 CoreRadius = 6;
		for (int32 dx = -CoreRadius; dx <= CoreRadius; ++dx)
		{
			for (int32 dy = -CoreRadius; dy <= CoreRadius; ++dy)
			{
				if (dx * dx + dy * dy <= CoreRadius * CoreRadius)
				{
					SetPixel(X + dx, Y + dy, R, G, B, 255);
				}
			}
		}

		// 外圈空心环（半径 14px，1px 厚），便于在复杂背景下定位
		const int32 RingRadius = 14;
		const int32 RingThickness = 2;
		for (int32 dx = -RingRadius - RingThickness; dx <= RingRadius + RingThickness; ++dx)
		{
			for (int32 dy = -RingRadius - RingThickness; dy <= RingRadius + RingThickness; ++dy)
			{
				const int32 DistSq = dx * dx + dy * dy;
				if (DistSq >= (RingRadius - RingThickness) * (RingRadius - RingThickness) &&
					DistSq <= (RingRadius + RingThickness) * (RingRadius + RingThickness))
				{
					SetPixel(X + dx, Y + dy, R, G, B, 255);
				}
			}
		}
	};

	for (int32 i = 0; i < ManualImagePoints.Num(); ++i)
	{
		const FVector2D& Pt = ManualImagePoints[i];
		if (i == ActivePairIndex && (InputMode == EInputMode::Edit2D))
		{
			DrawPoint(FMath::RoundToInt(Pt.X), FMath::RoundToInt(Pt.Y), 255, 255, 0); // 黄=编辑中
		}
		else
		{
			DrawPoint(FMath::RoundToInt(Pt.X), FMath::RoundToInt(Pt.Y), 0, 255, 0);   // 绿=普通
		}
	}

	// Pending 2D 点（新建 pair 的 2D 点，未提交）：红色显示
	if (Pending2DPoint.IsSet())
	{
		const FVector2D& Pt = Pending2DPoint.GetValue();
		DrawPoint(FMath::RoundToInt(Pt.X), FMath::RoundToInt(Pt.Y), 255, 0, 0); // 红=Pending
	}

	void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, Pixels.GetData(), Pixels.Num());
	Mip.BulkData.Unlock();

	OverlayTex->UpdateResource();

	RTMarkerBrush.SetResourceObject(OverlayTex);
	RTMarkerBrush.ImageSize = FVector2D(RTW, RTH);
	RTMarkerBrush.DrawAs = ESlateBrushDrawType::Image;

	if (RTOverlayImage.IsValid())
	{
		RTOverlayImage->SetImage(&RTMarkerBrush);
		RTOverlayImage->Invalidate(EInvalidateWidget::Layout);
	}

	UE_LOG(LogTemp, Log, TEXT("[PnPToolWidget] RT overlay 已更新，标记点数=%d，纹理=%dx%d"),
		ManualImagePoints.Num(), RTW, RTH);
}

FReply SPnPToolWidget::OnClearMarkersClicked()
{
	ManualImagePoints.Empty();
	ManualObjectPoints.Empty();
	ActivePairIndex = INDEX_NONE;
	InputMode = EInputMode::Idle;
	CancelPendingEdit();
	DestroyAllMarkerActors();

	RTMarkerBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	if (RTOverlayImage.IsValid())
	{
		RTOverlayImage->Invalidate(EInvalidateWidget::Paint);
	}

	if (GEditor)
	{
		if (UWorld* World = GEditor->GetEditorWorldContext().World())
		{
			FlushPersistentDebugLines(World);
		}
	}

	RebuildPairsList();
	UpdateInputModeUI();
	LogMessage(TEXT("[清除] 已清空所有标记点和待编辑点"));
	return FReply::Handled();
}

// === 输入模式控制 ===

void SPnPToolWidget::CommitPendingPair()
{
	// 提交当前 Pending pair 到已提交列表（要求 3D 和 2D 都已设置）
	if (!Pending3DPoint.IsSet() || !Pending2DPoint.IsSet())
	{
		return;
	}

	ManualObjectPoints.Add(Pending3DPoint.GetValue());
	ManualImagePoints.Add(Pending2DPoint.GetValue());

	// 把 Pending Marker 转移到 MarkerActors 数组
	if (PendingMarker.IsValid())
	{
		MarkerActors.Add(PendingMarker.Get());
		PendingMarker = nullptr;
	}

	LogMessage(FString::Printf(TEXT("[提交] 配对 #%d: 3D(%.1f, %.1f, %.1f) <-> 2D(%.1f, %.1f)"),
		ManualObjectPoints.Num() - 1,
		Pending3DPoint.GetValue().X, Pending3DPoint.GetValue().Y, Pending3DPoint.GetValue().Z,
		Pending2DPoint.GetValue().X, Pending2DPoint.GetValue().Y));

	Pending3DPoint.Reset();
	Pending2DPoint.Reset();
	PendingMarker = nullptr;
}

void SPnPToolWidget::CancelPendingEdit()
{
	// 销毁 Pending Marker（未提交的 3D 点可视化）
	if (PendingMarker.IsValid())
	{
		if (UWorld* World = PendingMarker->GetWorld())
		{
			World->DestroyActor(PendingMarker.Get(), false);
		}
		PendingMarker = nullptr;
	}
	Pending3DPoint.Reset();
	Pending2DPoint.Reset();
}

FReply SPnPToolWidget::OnAdd3DPointClicked()
{
	// 如果正在编辑已提交 pair（ActivePairIndex 有效），先退出编辑模式
	if (ActivePairIndex != INDEX_NONE)
	{
		ActivePairIndex = INDEX_NONE;
		LogMessage(TEXT("[编辑] 已退出已提交 pair 的编辑模式"));
	}

	// 如果当前 Pending pair 完整（3D + 2D 都有），先提交
	if (Pending3DPoint.IsSet() && Pending2DPoint.IsSet())
	{
		CommitPendingPair();
		RebuildPairsList();
	}

	// 进入 3D 点输入模式（开始新 pair）
	InputMode = EInputMode::Edit3D;
	UpdateInputModeUI();
	DrawManualMarkers();
	UpdateRTMarkerOverlay();

	if (Pending3DPoint.IsSet())
	{
		LogMessage(FString::Printf(TEXT("[输入] 3D 点输入模式 | 当前 3D(%.1f, %.1f, %.1f) 可在 3D 视口点击或拖 Gizmo 调整"),
			Pending3DPoint.GetValue().X, Pending3DPoint.GetValue().Y, Pending3DPoint.GetValue().Z));
	}
	else
	{
		LogMessage(TEXT("[输入] 3D 点输入模式 | 请在左侧场景视口点击放置 3D 点"));
	}
	return FReply::Handled();
}

FReply SPnPToolWidget::OnAdd2DPointClicked()
{
	// 编辑已提交 pair 的 2D 点
	if (ActivePairIndex != INDEX_NONE)
	{
		InputMode = EInputMode::Edit2D;
		UpdateInputModeUI();
		LogMessage(FString::Printf(TEXT("[输入] 2D 点输入模式 | 编辑配对 #%d 的 2D 点，请在右侧 RT 点击"),
			ActivePairIndex));
		return FReply::Handled();
	}

	// 新建 pair：要求 3D 点已设置
	if (!Pending3DPoint.IsSet())
	{
		LogMessage(TEXT("[警告] 请先点'添加3D点'并在 3D 视口点击放置 3D 点"));
		return FReply::Handled();
	}

	// 如果有 3D 点但还没有 2D 点，自动投影生成默认精准 2D 点
	if (!Pending2DPoint.IsSet())
	{
		FVector2D Default2D = ProjectWorldToImage(Pending3DPoint.GetValue());
		Pending2DPoint = Default2D;
		LogMessage(FString::Printf(TEXT("[自动] 已根据 3D 点投影生成默认 2D 点: (%.1f, %.1f)，可在右侧 RT 点击调整"),
			Default2D.X, Default2D.Y));
	}

	InputMode = EInputMode::Edit2D;
	UpdateInputModeUI();

	if (Pending2DPoint.IsSet())
	{
		UpdateRTMarkerOverlay();
		LogMessage(FString::Printf(TEXT("[输入] 2D 点输入模式 | 当前 2D(%.1f, %.1f) 可在右侧 RT 点击调整"),
			Pending2DPoint.GetValue().X, Pending2DPoint.GetValue().Y));
	}
	else
	{
		LogMessage(TEXT("[输入] 2D 点输入模式 | 请在右侧 RT 点击放置 2D 点"));
	}
	return FReply::Handled();
}

FReply SPnPToolWidget::OnCancelEditClicked()
{
	// 如果在编辑已提交 pair，只是退出编辑模式
	if (ActivePairIndex != INDEX_NONE)
	{
		ActivePairIndex = INDEX_NONE;
		InputMode = EInputMode::Idle;
		UpdateInputModeUI();
		DrawManualMarkers();
		UpdateRTMarkerOverlay();
		LogMessage(TEXT("[取消] 已退出已提交 pair 的编辑模式"));
		return FReply::Handled();
	}

	// 新建 pair 的取消：销毁 Pending Marker，清空数据
	CancelPendingEdit();
	InputMode = EInputMode::Idle;
	UpdateInputModeUI();
	DrawManualMarkers();
	UpdateRTMarkerOverlay();
	LogMessage(TEXT("[取消] 已取消当前 pair 的编辑"));
	return FReply::Handled();
}

void SPnPToolWidget::UpdateInputModeUI()
{
	// 更新按钮文字
	if (Add3DBtnText.IsValid())
	{
		if (InputMode == EInputMode::Edit3D)
		{
			Add3DBtnText->SetText(FText::FromString(TEXT("●3D输入中")));
		}
		else if (Pending3DPoint.IsSet() && Pending2DPoint.IsSet())
		{
			Add3DBtnText->SetText(FText::FromString(TEXT("添加3D点(提交)")));
		}
		else
		{
			Add3DBtnText->SetText(FText::FromString(TEXT("添加3D点")));
		}
	}

	if (Add2DBtnText.IsValid())
	{
		if (InputMode == EInputMode::Edit2D)
		{
			Add2DBtnText->SetText(FText::FromString(TEXT("●2D输入中")));
		}
		else
		{
			Add2DBtnText->SetText(FText::FromString(TEXT("添加2D点")));
		}
	}

	// 更新状态文字
	if (CurrentStateText.IsValid())
	{
		FString StateText;
		if (InputMode == EInputMode::Idle)
		{
			StateText = TEXT("状态：空闲");
		}
		else if (InputMode == EInputMode::Edit3D)
		{
			if (ActivePairIndex != INDEX_NONE)
			{
				StateText = FString::Printf(TEXT("状态：编辑配对 #%d 的 3D 点"), ActivePairIndex);
			}
			else if (Pending3DPoint.IsSet())
			{
				StateText = FString::Printf(TEXT("状态：3D输入中 (%.1f, %.1f, %.1f)"),
					Pending3DPoint.GetValue().X, Pending3DPoint.GetValue().Y, Pending3DPoint.GetValue().Z);
			}
			else
			{
				StateText = TEXT("状态：3D输入中（待点击）");
			}
		}
		else if (InputMode == EInputMode::Edit2D)
		{
			if (ActivePairIndex != INDEX_NONE)
			{
				StateText = FString::Printf(TEXT("状态：编辑配对 #%d 的 2D 点"), ActivePairIndex);
			}
			else if (Pending2DPoint.IsSet())
			{
				StateText = FString::Printf(TEXT("状态：2D输入中 (%.1f, %.1f)"),
					Pending2DPoint.GetValue().X, Pending2DPoint.GetValue().Y);
			}
			else
			{
				StateText = TEXT("状态：2D输入中（待点击）");
			}
		}
		CurrentStateText->SetText(FText::FromString(StateText));
	}

	// 更新配对列表行视觉
	UpdateActiveRowVisuals();
}

void SPnPToolWidget::LogMessage(const FString& Msg)
{
	// 同时输出到 UE 日志
	UE_LOG(LogTemp, Log, TEXT("[PnPToolWidget] %s"), *Msg);

	// 添加到消息数组（最多保留 100 条）
	Messages.Add(Msg);
	if (Messages.Num() > 100)
	{
		Messages.RemoveAt(0, Messages.Num() - 100);
	}

	UpdateMessages();
}

void SPnPToolWidget::UpdateMessages()
{
	if (MessagesTextWidget.IsValid())
	{
		MessagesTextWidget->Invalidate(EInvalidateWidget::Paint);
	}
	// 延迟一帧滚动到底部，确保布局已更新
	if (MessagesScrollBox.IsValid())
	{
		// 在下一帧滚动，因为当前帧的布局还没更新
		MessagesScrollBox->ScrollToEnd();
	}
}

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
	Solver->m_FocalLength = FVector2D(Fx, Fy);
	Solver->m_ImageCenter = FVector2D(Cx, Cy);
	Solver->m_ImageResolution = Resolution;
}

FReply SPnPToolWidget::OnSolveClicked()
{
	UPnPSolverSubsystem* Solver = GetSolverSubsystem();
	if (!Solver)
	{
		bLastSolveSuccess = false;
		LogMessage(TEXT("[错误] 无法获取 UPnPSolverSubsystem"));
		return FReply::Handled();
	}

	// 如果有未提交的 Pending pair，提示先提交
	if (Pending3DPoint.IsSet() || Pending2DPoint.IsSet())
	{
		LogMessage(TEXT("[警告] 当前有未提交的 pair（3D 或 2D 点未完成），请点'添加3D点'提交后再求解"));
		bLastSolveSuccess = false;
		return FReply::Handled();
	}

	// 设置内参
	ApplyIntrinsicsToSolver(Solver);

	// 用手动选择的点对
	Solver->m_ObjectPoints = ManualObjectPoints;
	Solver->m_ImagePoints.Reset();
	for (const FVector2D& Pt : ManualImagePoints)
	{
		Solver->m_ImagePoints.Add(FVector2f(Pt.X, Pt.Y));
	}

	if (ManualObjectPoints.Num() < 4)
	{
		LogMessage(FString::Printf(TEXT("[警告] 至少需要 4 对点才能求解，当前只有 %d 对"), ManualObjectPoints.Num()));
		bLastSolveSuccess = false;
		return FReply::Handled();
	}

	// 设置 Ground Truth（如果有 SourceCapture）
	if (m_SourceCapture.IsValid())
	{
		Solver->m_GroundTruthPose = m_SourceCapture->GetActorTransform();
	}

	// === GroundTruth 投影验证 ===
	// 用 SourceCapture 的真实位姿 + 内参，把每个 3D 点投影到 2D，与用户点击的 2D 点对比。
	// 若两者差距大 → 配对/内参/右侧画面不一致有问题（求解器无辜）。
	// 若两者接近  → 配对正确，问题在求解器本身或点分布。
	if (m_SourceCapture.IsValid())
	{
		const FVector CamLoc = m_SourceCapture->GetActorLocation();
		const FRotator CamRot = m_SourceCapture->GetActorRotation();
		double TotalGtErr = 0.0;
		double MaxGtErr = 0.0;
		LogMessage(TEXT("[GT验证] 用 SourceCapture 真实位姿投影 3D 点，对比用户 2D 点:"));
		for (int32 i = 0; i < ManualObjectPoints.Num(); ++i)
		{
			const FVector PWorld = ManualObjectPoints[i];
			// 世界 → UE 相机空间（X前 Y右 Z上）
			const FVector PCam = CamRot.UnrotateVector(PWorld - CamLoc);
			// 点在相机后方或正好在相机平面上，跳过
			if (PCam.X <= KINDA_SMALL_NUMBER)
			{
				LogMessage(FString::Printf(TEXT("[GT验证] #%d 3D(%.1f,%.1f,%.1f) 在相机后方或过近, PCam.X=%.4f"),
					i, PWorld.X, PWorld.Y, PWorld.Z, PCam.X));
				continue;
			}
			// OpenCV 投影：u = fx*Y/X + cx, v = fy*(-Z)/X + cy
			const double U = Fx * (PCam.Y / PCam.X) + Cx;
			const double V = Fy * (-PCam.Z / PCam.X) + Cy;
			const FVector2D UserPt = ManualImagePoints[i];
			const double Dx = U - UserPt.X;
			const double Dy = V - UserPt.Y;
			const double Err = FMath::Sqrt(Dx * Dx + Dy * Dy);
			TotalGtErr += Err;
			if (Err > MaxGtErr) MaxGtErr = Err;
			LogMessage(FString::Printf(TEXT("[GT验证] #%d 3D(%.1f,%.1f,%.1f) 投影=(%.1f,%.1f) 用户=(%.1f,%.1f) 误差=%.1fpx"),
				i, PWorld.X, PWorld.Y, PWorld.Z, U, V, UserPt.X, UserPt.Y, Err));
		}
		const double AvgErr = ManualObjectPoints.Num() > 0 ? TotalGtErr / ManualObjectPoints.Num() : 0.0;
		LogMessage(FString::Printf(TEXT("[GT验证] 平均误差=%.1fpx 最大误差=%.1fpx（<5px=配对正确, >20px=配对/内参/画面不一致）"),
			AvgErr, MaxGtErr));
	}

	Solver->SolvePnP();

	// 读取结果
	bLastSolveSuccess = Solver->m_bLastSolveSuccess;
	SolvedPose = Solver->m_SolvedCameraPose;
	ReprojectionError = Solver->m_ReprojectionError;
	TranslationError = Solver->m_TranslationError;
	RotationError = Solver->m_RotationError;

	if (bLastSolveSuccess)
	{
		Solver->CompareSolvedVsGroundTruth();
		TranslationError = Solver->m_TranslationError;
		RotationError = Solver->m_RotationError;

		const FVector L = SolvedPose.GetLocation();
		LogMessage(FString::Printf(TEXT("[求解成功] 点数=%d 位置=(%.2f,%.2f,%.2f) 重投影误差=%.4fpx 平移误差=%.2fcm 旋转误差=%.2fdeg"),
			ManualObjectPoints.Num(), L.X, L.Y, L.Z, ReprojectionError, TranslationError, RotationError));
	}
	else
	{
		LogMessage(TEXT("[求解失败] 请检查点对是否正确"));
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
