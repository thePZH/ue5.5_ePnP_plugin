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

	if (SceneCapture.IsValid())
	{
		if (UWorld* World = SceneCapture->GetWorld())
		{
			World->DestroyActor(SceneCapture.Get());
		}
	}
	if (RightSceneCapture.IsValid())
	{
		if (UWorld* World = RightSceneCapture->GetWorld())
		{
			World->DestroyActor(RightSceneCapture.Get());
		}
	}
}

void SPnPToolWidget::Construct(const FArguments& InArgs)
{
	// 1. 创建左侧场景预览用的 RenderTarget（瞬态对象，不保存）
	ScenePreviewRT = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), TEXT("PnPScenePreviewRT"), RF_Transient);
	ScenePreviewRT->InitAutoFormat(960, 540);
	ScenePreviewRT->UpdateResource();

	SceneBrush.SetResourceObject(ScenePreviewRT.Get());
	SceneBrush.ImageSize = FVector2D(ScenePreviewRT->SizeX, ScenePreviewRT->SizeY);
	SceneBrush.DrawAs = ESlateBrushDrawType::Image;

	// 2. 创建右侧显示用的 RenderTarget（与左侧相同的初始化流程，保证 Slate 显示兼容）
	DisplayRT = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), TEXT("PnPDisplayRT"), RF_Transient);
	DisplayRT->InitAutoFormat(960, 540);
	DisplayRT->UpdateResource();

	RTBrush.SetResourceObject(DisplayRT.Get());
	RTBrush.ImageSize = FVector2D(DisplayRT->SizeX, DisplayRT->SizeY);
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
					.Text(LOCTEXT("SceneHeader", "场景视口（左键点击选择3D点）"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				]

				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					SNew(SBorder).BorderBackgroundColor(FLinearColor::Black)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SAssignNew(SceneContainerBox, SBox)
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Fill)
						.MinAspectRatio(TAttribute<FOptionalSize>::Create(TAttribute<FOptionalSize>::FGetter::CreateSP(this, &SPnPToolWidget::GetSceneAspectRatio)))
						.MaxAspectRatio(TAttribute<FOptionalSize>::Create(TAttribute<FOptionalSize>::FGetter::CreateSP(this, &SPnPToolWidget::GetSceneAspectRatio)))
						[
							SNew(SBorder)
							.OnMouseButtonDown(this, &SPnPToolWidget::OnSceneMouseDown)
							[
								SAssignNew(SceneImageWidget, SImage)
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

			// RT 资产选择器
			+ SScrollBox::Slot().Padding(2)
			[
				MakeRow(LOCTEXT("rtLabel", "RT"),
					SNew(SObjectPropertyEntryBox)
						.AllowedClass(UTextureRenderTarget2D::StaticClass())
						.ObjectPath(this, &SPnPToolWidget::GetRTPickerPath)
						.OnObjectChanged(this, &SPnPToolWidget::OnRTChanged)
				)
			]

			// 源 SceneCapture2D 选择器
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
					.Text(LOCTEXT("RTPreviewHeader", "RT 内容预览（左键点击添加2D点）"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			]

			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SNew(SBorder).BorderBackgroundColor(FLinearColor::Black)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SAssignNew(RTContainerBox, SBox)
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
								SAssignNew(RTImageWidget, SImage)
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

FString SPnPToolWidget::GetRTPickerPath() const
{
	if (SelectedRT.IsValid())
	{
		return SelectedRT->GetPathName();
	}
	return FString();
}

void SPnPToolWidget::OnRTChanged(const FAssetData& InAssetData)
{
	SelectedRT = Cast<UTextureRenderTarget2D>(InAssetData.GetAsset());

	if (SelectedRT.IsValid())
	{
		LogMessage(FString::Printf(TEXT("[RT] 已选择目标 RT: %s, 尺寸=%dx%d"),
			*SelectedRT->GetPathName(), SelectedRT->SizeX, SelectedRT->SizeY));

		// 立即同步 DisplayRT 尺寸到目标 RT（Tick 中也会持续跟随）
		ResizeDisplayRT(SelectedRT->SizeX, SelectedRT->SizeY);
	}
	else
	{
		LogMessage(TEXT("[RT] Cast<UTextureRenderTarget2D> 失败，可能选错资产类型"));
	}

	UpdateRTMarkerOverlay();
}

FString SPnPToolWidget::GetSourceCapturePath() const
{
	if (SourceCapture.IsValid())
	{
		return SourceCapture->GetPathName();
	}
	return FString();
}

void SPnPToolWidget::OnSourceCaptureChanged(const FAssetData& InAssetData)
{
	SourceCapture = Cast<ASceneCapture2D>(InAssetData.GetAsset());
	if (SourceCapture.IsValid())
	{
		LogMessage(FString::Printf(TEXT("[Capture2D] 源 SceneCapture2D 已选择: %s"), *SourceCapture->GetPathName()));
	}
}

FReply SPnPToolWidget::OnGetIntrinsicsClicked()
{
	if (!SourceCapture.IsValid())
	{
		LogMessage(TEXT("[警告] 请先选择源 SceneCapture2D"));
		return FReply::Handled();
	}

	USceneCaptureComponent2D* Comp = SourceCapture->GetCaptureComponent2D();
	if (!Comp || !Comp->TextureTarget)
	{
		LogMessage(TEXT("[警告] SceneCapture2D 无 TextureTarget"));
		return FReply::Handled();
	}

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

	LogMessage(FString::Printf(TEXT("[内参] fx=%.2f fy=%.2f cx=%.2f cy=%.2f Res=%dx%d FOV=%.2f"),
		Fx, Fy, Cx, Cy, W, H, FOV));

	return FReply::Handled();
}

void SPnPToolWidget::EnsureSceneCapture()
{
	if (SceneCapture.IsValid()) return;

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SceneCapture = World->SpawnActor<ASceneCapture2D>(ASceneCapture2D::StaticClass(), FTransform::Identity, Params);

	if (ASceneCapture2D* Cap = SceneCapture.Get())
	{
		Cap->SetIsTemporarilyHiddenInEditor(true);

		if (USceneCaptureComponent2D* Comp = Cap->GetCaptureComponent2D())
		{
			Comp->TextureTarget = ScenePreviewRT.Get();
			Comp->bCaptureEveryFrame = true;
			Comp->bCaptureOnMovement = false;
			Comp->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		}
	}
}

void SPnPToolWidget::EnsureRightSceneCapture()
{
	if (RightSceneCapture.IsValid()) return;

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	RightSceneCapture = World->SpawnActor<ASceneCapture2D>(ASceneCapture2D::StaticClass(), FTransform::Identity, Params);

	if (ASceneCapture2D* Cap = RightSceneCapture.Get())
	{
		Cap->SetIsTemporarilyHiddenInEditor(true);

		if (USceneCaptureComponent2D* Comp = Cap->GetCaptureComponent2D())
		{
			Comp->TextureTarget = DisplayRT.Get();
			Comp->bCaptureEveryFrame = true;
			Comp->bCaptureOnMovement = false;
			Comp->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		}
	}
}

void SPnPToolWidget::ResizeDisplayRT(int32 NewW, int32 NewH)
{
	if (NewW <= 0 || NewH <= 0) return;
	if (!DisplayRT.IsValid()) return;

	// 尺寸相同则无需重建
	if (DisplayRT->SizeX == NewW && DisplayRT->SizeY == NewH)
	{
		return;
	}

	// 重新初始化 RT 尺寸（InitAutoFormat 会自动选择合适的像素格式）
	DisplayRT->InitAutoFormat(NewW, NewH);
	DisplayRT->UpdateResource();

	// 更新 brush 指向（始终指向 DisplayRT，由 RightSceneCapture 自行捕获内容）
	RTBrush.SetResourceObject(nullptr);
	RTBrush.SetResourceObject(DisplayRT.Get());
	RTBrush.ImageSize = FVector2D(NewW, NewH);
	RTBrush.DrawAs = ESlateBrushDrawType::Image;
	RTBrush.TintColor = FSlateColor(FLinearColor::White);
#if WITH_EDITOR
	RTBrush.InvalidateResourceHandle();
#endif

	if (RTImageWidget.IsValid())
	{
		RTImageWidget->Invalidate(EInvalidateWidget::Layout | EInvalidateWidget::Paint);
	}

	LogMessage(FString::Printf(TEXT("[RT] DisplayRT 已调整尺寸为 %dx%d"), NewW, NewH));
}

void SPnPToolWidget::UpdateSceneCaptureFromActiveViewport() const
{
	if (!SceneCapture.IsValid()) return;

	FViewport* ActiveVP = GEditor ? GEditor->GetActiveViewport() : nullptr;
	FEditorViewportClient* VPC = (ActiveVP && ActiveVP->GetClient())
		? static_cast<FEditorViewportClient*>(ActiveVP->GetClient())
		: nullptr;

	if (VPC && VPC->IsPerspective())
	{
		SceneCapture->SetActorLocation(VPC->GetViewLocation());
		SceneCapture->SetActorRotation(VPC->GetViewRotation());
	}
}

void SPnPToolWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	EnsureSceneCapture();
	EnsureRightSceneCapture();
	UpdateSceneCaptureFromActiveViewport();

	// 将右侧插件创建的 SceneCapture 位置/朝向与用户选择的 SourceCapture 同步（若无，则跟随编辑器视口）
	if (RightSceneCapture.IsValid())
	{
		if (SourceCapture.IsValid())
		{
			RightSceneCapture->SetActorLocation(SourceCapture->GetActorLocation());
			RightSceneCapture->SetActorRotation(SourceCapture->GetActorRotation());

			// 同步 FOV，使右侧视口捕获画面与 SourceCapture 一致
			if (USceneCaptureComponent2D* RightComp = RightSceneCapture->GetCaptureComponent2D())
			{
				if (USceneCaptureComponent2D* SrcComp = SourceCapture->GetCaptureComponent2D())
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
				RightSceneCapture->SetActorLocation(VPC->GetViewLocation());
				RightSceneCapture->SetActorRotation(VPC->GetViewRotation());
			}
		}
	}

	// 实时跟随目标 RT（SelectedRT）的尺寸，保持 DisplayRT 与之一致
	if (SelectedRT.IsValid() && DisplayRT.IsValid())
	{
		if (DisplayRT->SizeX != SelectedRT->SizeX || DisplayRT->SizeY != SelectedRT->SizeY)
		{
			ResizeDisplayRT(SelectedRT->SizeX, SelectedRT->SizeY);
		}
	}

	// 只在点变化时重绘 debug，避免每帧 FlushPersistentDebugLines 导致闪烁
	static int32 LastObjCount = -1;
	static bool bLastPending = false;
	const bool bNeedRedraw = (LastObjCount != ManualObjectPoints.Num()) || (bLastPending != PendingObjectPoint.IsSet());
	if (bNeedRedraw)
	{
		DrawManualMarkers();
		LastObjCount = ManualObjectPoints.Num();
		bLastPending = PendingObjectPoint.IsSet();
	}
}

FOptionalSize SPnPToolWidget::GetSceneAspectRatio() const
{
	if (ScenePreviewRT.IsValid() && ScenePreviewRT->SizeY > 0)
	{
		return FOptionalSize(static_cast<float>(ScenePreviewRT->SizeX) / static_cast<float>(ScenePreviewRT->SizeY));
	}
	return FOptionalSize(16.0f / 9.0f);
}

FOptionalSize SPnPToolWidget::GetRTAspectRatio() const
{
	if (DisplayRT.IsValid() && DisplayRT->SizeY > 0)
	{
		return FOptionalSize(static_cast<float>(DisplayRT->SizeX) / static_cast<float>(DisplayRT->SizeY));
	}
	return FOptionalSize(16.0f / 9.0f);
}

FVector2D SPnPToolWidget::ProjectWorldToImage(const FVector& WorldPoint) const
{
	if (!SourceCapture.IsValid())
	{
		return FVector2D(-1.0, -1.0);
	}
	const FVector CamLoc = SourceCapture->GetActorLocation();
	const FRotator CamRot = SourceCapture->GetActorRotation();
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
			SNew(STextBlock).Text(LOCTEXT("PairsHeader", "配对列表（3D / 2D 可编辑，黄色=活动，右侧RT点击覆盖活动2D）"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		]
	];

	if (ManualObjectPoints.Num() == 0)
	{
		PairsListContainer->AddSlot().AutoHeight().Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("PairsEmpty", "（无配对。在左侧场景视口点击 3D 点开始）"))
			.ColorAndOpacity(FLinearColor::Gray)
		];
		return;
	}

	auto MakeCoordBox = [](double InitialValue, TFunction<void(double)> OnCommit) -> TSharedRef<SWidget>
	{
		return SNew(SSpinBox<double>)
			.Value(InitialValue)
			.MinValue(-1000000.0).MaxValue(1000000.0).Delta(1.0)
			.MinDesiredWidth(70.0f)
			.OnValueCommitted_Lambda([OnCommit](double V, ETextCommit::Type) { OnCommit(V); });
	};

	for (int32 i = 0; i < ManualObjectPoints.Num(); ++i)
	{
		const bool bIsActive = (i == ActivePairIndex);
		const int32 CapturedIdx = i; // 供 lambda 捕获，重建后该行 widget 销毁，索引仍有效

		PairsListContainer->AddSlot().AutoHeight().Padding(2)
		[
			SNew(SBorder)
			.BorderBackgroundColor_Lambda([this, CapturedIdx]()
			{
				return (CapturedIdx == ActivePairIndex)
					? FLinearColor(1.0f, 0.85f, 0.0f, 0.45f)
					: FLinearColor(0.18f, 0.18f, 0.18f, 0.4f);
			})
			.Padding(2)
			[
				SNew(SHorizontalBox)

				// #i
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)
				[
					SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("#%d"), i)))
				]

				// 3D X
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					MakeCoordBox(ManualObjectPoints[i].X, [this, CapturedIdx](double V)
					{
						if (ManualObjectPoints.IsValidIndex(CapturedIdx))
						{
							ManualObjectPoints[CapturedIdx].X = V;
							DrawManualMarkers();
						}
					})
				]
				// 3D Y
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					MakeCoordBox(ManualObjectPoints[i].Y, [this, CapturedIdx](double V)
					{
						if (ManualObjectPoints.IsValidIndex(CapturedIdx))
						{
							ManualObjectPoints[CapturedIdx].Y = V;
							DrawManualMarkers();
						}
					})
				]
				// 3D Z
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					MakeCoordBox(ManualObjectPoints[i].Z, [this, CapturedIdx](double V)
					{
						if (ManualObjectPoints.IsValidIndex(CapturedIdx))
						{
							ManualObjectPoints[CapturedIdx].Z = V;
							DrawManualMarkers();
						}
					})
				]

				// 分隔
				+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("<->")))
				]

				// 2D U
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					MakeCoordBox(ManualImagePoints[i].X, [this, CapturedIdx](double V)
					{
						if (ManualImagePoints.IsValidIndex(CapturedIdx))
						{
							ManualImagePoints[CapturedIdx].X = V;
							UpdateRTMarkerOverlay();
						}
					})
				]
				// 2D V
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					MakeCoordBox(ManualImagePoints[i].Y, [this, CapturedIdx](double V)
					{
						if (ManualImagePoints.IsValidIndex(CapturedIdx))
						{
							ManualImagePoints[CapturedIdx].Y = V;
							UpdateRTMarkerOverlay();
						}
					})
				]

				// 设为活动
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0)
				[
					SNew(SButton)
					.Text(FText::FromString(bIsActive ? TEXT("●活动") : TEXT("设活动")))
					.OnClicked_Lambda([this, CapturedIdx]() -> FReply
					{
						ActivePairIndex = CapturedIdx;
						RebuildPairsList();
						UpdateRTMarkerOverlay();
						DrawManualMarkers();
						return FReply::Handled();
					})
				]

				// 删除
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("删除")))
					.OnClicked_Lambda([this, CapturedIdx]() -> FReply
					{
						if (ManualObjectPoints.IsValidIndex(CapturedIdx))
						{
							ManualObjectPoints.RemoveAt(CapturedIdx);
							ManualImagePoints.RemoveAt(CapturedIdx);
							if (ActivePairIndex == CapturedIdx) ActivePairIndex = INDEX_NONE;
							else if (ActivePairIndex > CapturedIdx) ActivePairIndex--;
							RebuildPairsList();
							UpdateRTMarkerOverlay();
							DrawManualMarkers();
						}
						return FReply::Handled();
					})
				]
			]
		];
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

	// 已配对的点用绿色；当前活动配对用黄色高亮（提示右侧 RT 点击会覆盖其 2D）
	for (int32 i = 0; i < ManualObjectPoints.Num(); ++i)
	{
		const FVector& Pt = ManualObjectPoints[i];
		const bool bActive = (i == ActivePairIndex);
		const FColor PtColor = bActive ? FColor::Yellow : FColor::Green;
		DrawDebugSphere(World, Pt, bActive ? 12.0f : 10.0f, 12, PtColor, true, -1.0f, 0, bActive ? 3.0f : 2.0f);
		const FString Label = FString::Printf(TEXT("#%d%s"), i, bActive ? TEXT(" (活动)") : TEXT(""));
		DrawDebugString(World, Pt + FVector(0, 0, 20.0f), Label, nullptr, PtColor, 0.0f, true);
	}

	// 待配对的点用红色
	if (PendingObjectPoint.IsSet())
	{
		const FVector& Pt = PendingObjectPoint.GetValue();
		DrawDebugSphere(World, Pt, 12.0f, 12, FColor::Red, true, -1.0f, 0, 3.0f);
		DrawDebugString(World, Pt + FVector(0, 0, 25.0f), TEXT("pending"), nullptr, FColor::Red, 0.0f, true);
	}
}

FReply SPnPToolWidget::OnSceneMouseDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Handled();
	if (!ScenePreviewRT.IsValid() || !SceneCapture.IsValid()) return FReply::Handled();

	// 仅在无 SourceCapture 的回退流程中，要求先完成上一个待配对点
	if (!SourceCapture.IsValid() && PendingObjectPoint.IsSet())
	{
		LogMessage(TEXT("[警告] 当前已有未配对的 3D 点（红色），请先在右侧 RT 上点击对应 2D 点完成配对"));
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
	if (USceneCaptureComponent2D* Comp = SceneCapture->GetCaptureComponent2D())
	{
		ViewportFOV = Comp->FOVAngle;
	}

	const float HalfFOVRad = FMath::DegreesToRadians(ViewportFOV) * 0.5f;
	const float RTAspect = static_cast<float>(ScenePreviewRT->SizeX) / static_cast<float>(ScenePreviewRT->SizeY);

	// UE 的 FOVAngle 是水平 FOV
	const float TanHalfFOVH = FMath::Tan(HalfFOVRad);
	const float TanHalfFOVV = TanHalfFOVH / RTAspect;

	// 相机空间射线方向：X=前 Y=右 Z=上
	const FVector RayDirLocal(1.0f, ScreenX * TanHalfFOVH, ScreenY * TanHalfFOVV);

	// 转世界方向
	const FVector CamPos = SceneCapture->GetActorLocation();
	const FRotator CamRot = SceneCapture->GetActorRotation();
	const FVector RayDirWorld = CamRot.RotateVector(RayDirLocal).GetSafeNormal();
	const FVector RayEnd = CamPos + RayDirWorld * 10000.0f;

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(PnPRaycast), false, SceneCapture.Get());
	FHitResult HitResult;
	const bool bHit = World->LineTraceSingleByChannel(HitResult, CamPos, RayEnd, ECC_Visibility, TraceParams);

	if (bHit)
	{
		const FVector HitPoint = HitResult.ImpactPoint;

		// 主流程：有 SourceCapture → 自动投影生成配对
		if (SourceCapture.IsValid())
		{
			const FVector2D ProjUV = ProjectWorldToImage(HitPoint);
			ManualObjectPoints.Add(HitPoint);
			ManualImagePoints.Add(ProjUV);
			ActivePairIndex = ManualObjectPoints.Num() - 1;

			if (ProjUV.X >= 0.0 && ProjUV.Y >= 0.0)
			{
				LogMessage(FString::Printf(TEXT("[配对 #%d] 3D(%.1f, %.1f, %.1f) <-> 2D(%.1f, %.1f) [自动投影] 可在右侧 RT 点击微调"),
					ActivePairIndex, HitPoint.X, HitPoint.Y, HitPoint.Z, ProjUV.X, ProjUV.Y));
			}
			else
			{
				LogMessage(FString::Printf(TEXT("[配对 #%d] 3D(%.1f, %.1f, %.1f) 自动投影失败(点在相机后方)，请手动点击 RT 或在列表修改 2D 坐标"),
					ActivePairIndex, HitPoint.X, HitPoint.Y, HitPoint.Z));
			}
			RebuildPairsList();
			UpdateRTMarkerOverlay();
			DrawManualMarkers();
		}
		else
		{
			// 回退流程：无 SourceCapture，存为待配对点，等右侧点击
			PendingObjectPoint = HitPoint;
			LogMessage(FString::Printf(TEXT("[3D点] 已选择 (红色, 待配对): (%.1f, %.1f, %.1f) - 请在右侧 RT 点击对应 2D 点"),
				HitPoint.X, HitPoint.Y, HitPoint.Z));
			DrawManualMarkers();
		}
	}

	return FReply::Handled();
}

FReply SPnPToolWidget::OnRTMouseDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Handled();
	if (!DisplayRT.IsValid()) return FReply::Handled();

	// 无活动配对且无待配对点 → 提示先选 3D
	if (!PendingObjectPoint.IsSet() && !ManualImagePoints.IsValidIndex(ActivePairIndex))
	{
		LogMessage(TEXT("[警告] 请先在左侧场景视口点击选择 3D 点"));
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
	const float Px = UVPos.X * DisplayRT->SizeX;
	const float Py = UVPos.Y * DisplayRT->SizeY;

	// 优先处理回退流程：有待配对的 3D 点 → 完成新配对
	if (PendingObjectPoint.IsSet())
	{
		ManualObjectPoints.Add(PendingObjectPoint.GetValue());
		ManualImagePoints.Add(FVector2D(Px, Py));
		ActivePairIndex = ManualObjectPoints.Num() - 1;
		PendingObjectPoint.Reset();

		LogMessage(FString::Printf(TEXT("[配对 #%d] 3D(%.1f, %.1f, %.1f) <-> 2D(%.1f, %.1f) [手动点击]"),
			ActivePairIndex,
			ManualObjectPoints[ActivePairIndex].X, ManualObjectPoints[ActivePairIndex].Y, ManualObjectPoints[ActivePairIndex].Z,
			Px, Py));
	}
	else if (ManualImagePoints.IsValidIndex(ActivePairIndex))
	{
		// 主流程：覆盖当前活动配对的 2D 点（多次点击，最后一次生效）
		const FVector& Obj = ManualObjectPoints[ActivePairIndex];
		ManualImagePoints[ActivePairIndex] = FVector2D(Px, Py);
		LogMessage(FString::Printf(TEXT("[配对 #%d] 2D 已更新为 (%.1f, %.1f) [覆盖] 3D(%.1f, %.1f, %.1f)"),
			ActivePairIndex, Px, Py, Obj.X, Obj.Y, Obj.Z));
	}

	RebuildPairsList();
	UpdateRTMarkerOverlay();
	DrawManualMarkers();

	return FReply::Handled();
}

void SPnPToolWidget::UpdateRTMarkerOverlay()
{
	if (!DisplayRT.IsValid())
	{
		RTMarkerBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
		if (RTOverlayImage.IsValid())
		{
			RTOverlayImage->Invalidate(EInvalidateWidget::Paint);
		}
		return;
	}

	const int32 RTW = DisplayRT->SizeX;
	const int32 RTH = DisplayRT->SizeY;
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

	// 画十字 + 中心圆；颜色由调用方传入（活动=黄色, 其余=绿色）
	auto DrawPoint = [&](int32 X, int32 Y, uint8 R, uint8 G, uint8 B)
	{
		const int32 Size = 8;
		for (int32 dx = -Size; dx <= Size; ++dx) SetPixel(X + dx, Y, R, G, B, 255);
		for (int32 dy = -Size; dy <= Size; ++dy) SetPixel(X, Y + dy, R, G, B, 255);
		const int32 Radius = 4;
		for (int32 dx = -Radius; dx <= Radius; ++dx)
		{
			for (int32 dy = -Radius; dy <= Radius; ++dy)
			{
				if (dx * dx + dy * dy <= Radius * Radius)
				{
					SetPixel(X + dx, Y + dy, R, G, B, 255);
				}
			}
		}
	};

	for (int32 i = 0; i < ManualImagePoints.Num(); ++i)
	{
		const FVector2D& Pt = ManualImagePoints[i];
		if (i == ActivePairIndex)
		{
			DrawPoint(FMath::RoundToInt(Pt.X), FMath::RoundToInt(Pt.Y), 255, 255, 0); // 黄=活动
		}
		else
		{
			DrawPoint(FMath::RoundToInt(Pt.X), FMath::RoundToInt(Pt.Y), 0, 255, 0);   // 绿=普通
		}
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
	PendingObjectPoint.Reset();
	ActivePairIndex = INDEX_NONE;

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
	LogMessage(TEXT("[清除] 已清空所有标记点和待配对点"));
	return FReply::Handled();
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
	if (SourceCapture.IsValid())
	{
		Solver->m_GroundTruthPose = SourceCapture->GetActorTransform();
	}

	// === GroundTruth 投影验证 ===
	// 用 SourceCapture 的真实位姿 + 内参，把每个 3D 点投影到 2D，与用户点击的 2D 点对比。
	// 若两者差距大 → 配对/内参/右侧画面不一致有问题（求解器无辜）。
	// 若两者接近  → 配对正确，问题在求解器本身或点分布。
	if (SourceCapture.IsValid())
	{
		const FVector CamLoc = SourceCapture->GetActorLocation();
		const FRotator CamRot = SourceCapture->GetActorRotation();
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
