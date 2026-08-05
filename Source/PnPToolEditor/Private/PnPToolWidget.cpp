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

	// 2. 右侧 RT 预览画刷（初始为空，OnRTChanged 中设置）
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
	LogMessage(FString::Printf(TEXT("[RT调试] OnRTChanged 调用, AssetData: %s, Asset=%s"),
		*InAssetData.GetExportTextName(), *InAssetData.GetObjectPathString()));

	UObject* AssetObj = InAssetData.GetAsset();
	LogMessage(FString::Printf(TEXT("[RT调试] Asset 对象指针=%p, 类=%s"), AssetObj, AssetObj ? *AssetObj->GetClass()->GetName() : TEXT("null")));

	SelectedRT = Cast<UTextureRenderTarget2D>(InAssetData.GetAsset());

	if (SelectedRT.IsValid())
	{
		LogMessage(FString::Printf(TEXT("[RT调试] Cast 成功, PathName=%s"), *SelectedRT->GetPathName()));

		const int32 W = SelectedRT->SizeX;
		const int32 H = SelectedRT->SizeY;
		const EPixelFormat Format = SelectedRT->GetFormat();
		const FLinearColor ClearColor = SelectedRT->ClearColor;
		const bool bInSceneCapture = (SelectedRT->GetOuter() != nullptr);

		LogMessage(FString::Printf(TEXT("[RT调试] 尺寸=%dx%d, Format=%d, ClearColor=(%.2f,%.2f,%.2f,%.2f)"),
			W, H, (int32)Format, ClearColor.R, ClearColor.G, ClearColor.B, ClearColor.A));

		// 确保渲染资源已创建
		SelectedRT->UpdateResource();
		SelectedRT->UpdateResourceImmediate(true);

		FTextureResource* Res = SelectedRT->GetResource();
		FTextureRHIRef rhiRef = SelectedRT->GetResource() ? SelectedRT->GetResource()->TextureRHI : nullptr;

		LogMessage(FString::Printf(TEXT("[RT调试] UpdateResource 后: Resource=%p, RHI=%p, ResourceValid=%s"),
			Res, rhiRef.GetReference(), Res ? TEXT("true") : TEXT("false")));

		if (Res)
		{
			const FTexture2DRHIRef Tex2D = rhiRef ? rhiRef->GetTexture2D() : nullptr;
			LogMessage(FString::Printf(TEXT("[RT调试] Resource 大小=%dx%d, Tex2D=%p"),
				Res->GetSizeX(), Res->GetSizeY(), Tex2D.GetReference()));
		}

		// 检查是否被 SceneCapture2D 引用，并强制 Capture 一次以确保 RHI 资源有内容
		int32 RefCount = 0;
		USceneCaptureComponent2D* CaptureComp = nullptr;
		for (TObjectIterator<USceneCaptureComponent2D> It; It; ++It)
		{
			if (It->TextureTarget == SelectedRT.Get())
			{
				RefCount++;
				CaptureComp = *It;
				if (AActor* Owner = It->GetOwner())
				{
					LogMessage(FString::Printf(TEXT("[RT调试] 被 SceneCapture2D 引用: %s (CaptureEveryFrame=%s, CaptureOnceOnMovement=%s)"),
						*Owner->GetPathName(),
						It->bCaptureEveryFrame ? TEXT("true") : TEXT("false"),
						It->bCaptureOnMovement ? TEXT("true") : TEXT("false")));
				}
			}
		}
		if (RefCount == 0)
		{
			LogMessage(TEXT("[RT警告] 此 RT 未被任何 SceneCaptureComponent2D 引用！将尝试用我们的 SceneCapture 渲染一次"));
			// 用工具自己的 SceneCapture 渲染一次到这个 RT
			if (SceneCapture.IsValid())
			{
				USceneCaptureComponent2D* Comp = SceneCapture->GetCaptureComponent2D();
				if (Comp)
				{
					UTextureRenderTarget2D* OldTarget = Comp->TextureTarget;
					Comp->TextureTarget = SelectedRT.Get();
					Comp->CaptureScene();
					Comp->TextureTarget = OldTarget;
					LogMessage(TEXT("[RT调试] 已用工具 SceneCapture 强制渲染一次"));
				}
			}
		}
		else if (CaptureComp)
		{
			// 强制 Capture 一次确保 RHI 资源有内容
			CaptureComp->CaptureScene();
			LogMessage(TEXT("[RT调试] 已强制 CaptureScene 一次"));
		}

		RTBrush.SetResourceObject(nullptr);
		RTBrush.SetResourceObject(SelectedRT.Get());
		RTBrush.ImageSize = FVector2D(W, H);
		RTBrush.DrawAs = ESlateBrushDrawType::Image;
		RTBrush.TintColor = FSlateColor(FLinearColor::White);
		RTBrush.Margin = FMargin(0, 0, 0, 0);
#if WITH_EDITOR
		RTBrush.InvalidateResourceHandle();
#endif

		LogMessage(FString::Printf(TEXT("[RT调试] RTBrush 已设置: ResourceObject=%p, ImageSize=(%.0f,%.0f), DrawAs=%d"),
			RTBrush.GetResourceObject(), RTBrush.ImageSize.X, RTBrush.ImageSize.Y, (int32)RTBrush.DrawAs));

		if (RTImageWidget.IsValid())
		{
			RTImageWidget->Invalidate(EInvalidateWidget::Layout | EInvalidateWidget::Paint);
			LogMessage(TEXT("[RT调试] RTImageWidget Invalidate 完成"));
		}
		else
		{
			LogMessage(TEXT("[RT警告] RTImageWidget 无效！"));
		}

		if (!Res)
		{
			LogMessage(TEXT("[RT警告] RT 渲染资源为 NULL，可能从未被渲染过"));
		}
	}
	else
	{
		LogMessage(TEXT("[RT调试] Cast<UTextureRenderTarget2D> 失败，可能选错资产类型"));

		RTBrush.SetResourceObject(nullptr);
		RTBrush.ImageSize = FVector2D::ZeroVector;

		if (RTImageWidget.IsValid())
		{
			RTImageWidget->Invalidate(EInvalidateWidget::Paint);
		}
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

void SPnPToolWidget::UpdateSceneCaptureFromActiveViewport()
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
	UpdateSceneCaptureFromActiveViewport();

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
	if (SelectedRT.IsValid() && SelectedRT->SizeY > 0)
	{
		return FOptionalSize(static_cast<float>(SelectedRT->SizeX) / static_cast<float>(SelectedRT->SizeY));
	}
	return FOptionalSize(16.0f / 9.0f);
}

void SPnPToolWidget::DrawManualMarkers()
{
	// 在编辑器视口中绘制已选 3D 点
	if (!GEditor) return;
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) return;

	// 清除旧的持久调试绘制
	FlushPersistentDebugLines(World);

	// 已配对的点用绿色
	for (int32 i = 0; i < ManualObjectPoints.Num(); ++i)
	{
		const FVector& Pt = ManualObjectPoints[i];
		DrawDebugSphere(World, Pt, 10.0f, 12, FColor::Green, true, -1.0f, 0, 2.0f);
		const FString Label = FString::Printf(TEXT("#%d"), i);
		DrawDebugString(World, Pt + FVector(0, 0, 20.0f), Label, nullptr, FColor::Green, 0.0f, true);
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

	// 配对未完成时禁止继续点击
	if (PendingObjectPoint.IsSet())
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
		PendingObjectPoint = HitResult.ImpactPoint;
		LogMessage(FString::Printf(TEXT("[3D点] 已选择 (红色, 待配对): (%.1f, %.1f, %.1f) - 请在右侧 RT 点击对应 2D 点"),
			HitResult.ImpactPoint.X, HitResult.ImpactPoint.Y, HitResult.ImpactPoint.Z));
	}

	return FReply::Handled();
}

FReply SPnPToolWidget::OnRTMouseDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Handled();
	if (!SelectedRT.IsValid()) return FReply::Handled();

	// 必须先在左侧场景视口选择 3D 点
	if (!PendingObjectPoint.IsSet())
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

	// UV 转像素坐标
	const float Px = UVPos.X * SelectedRT->SizeX;
	const float Py = UVPos.Y * SelectedRT->SizeY;

	// 完成配对
	ManualObjectPoints.Add(PendingObjectPoint.GetValue());
	ManualImagePoints.Add(FVector2D(Px, Py));
	const int32 Idx = ManualObjectPoints.Num() - 1;
	PendingObjectPoint.Reset();

	LogMessage(FString::Printf(TEXT("[配对 #%d] 3D(%.1f, %.1f, %.1f) <-> 2D(%.1f, %.1f)"),
		Idx,
		ManualObjectPoints[Idx].X, ManualObjectPoints[Idx].Y, ManualObjectPoints[Idx].Z,
		Px, Py));

	UpdateRTMarkerOverlay();

	return FReply::Handled();
}

void SPnPToolWidget::UpdateRTMarkerOverlay()
{
	if (!SelectedRT.IsValid())
	{
		RTMarkerBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
		if (RTOverlayImage.IsValid())
		{
			RTOverlayImage->Invalidate(EInvalidateWidget::Paint);
		}
		return;
	}

	const int32 RTW = SelectedRT->SizeX;
	const int32 RTH = SelectedRT->SizeY;
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

	// 画红色十字 + 中心圆
	auto DrawPoint = [&](int32 X, int32 Y)
	{
		const int32 Size = 8;
		for (int32 dx = -Size; dx <= Size; ++dx) SetPixel(X + dx, Y, 255, 0, 0, 255);
		for (int32 dy = -Size; dy <= Size; ++dy) SetPixel(X, Y + dy, 255, 0, 0, 255);
		const int32 Radius = 4;
		for (int32 dx = -Radius; dx <= Radius; ++dx)
		{
			for (int32 dy = -Radius; dy <= Radius; ++dy)
			{
				if (dx * dx + dy * dy <= Radius * Radius)
				{
					SetPixel(X + dx, Y + dy, 255, 0, 0, 255);
				}
			}
		}
	};

	for (const FVector2D& Pt : ManualImagePoints)
	{
		DrawPoint(FMath::RoundToInt(Pt.X), FMath::RoundToInt(Pt.Y));
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
