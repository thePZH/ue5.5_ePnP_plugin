// Copyright Epic Games, Inc. All Rights Reserved.

#include "PnPToolWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBorder.h"
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
#include "PnPSolverActor.h"

#define LOCTEXT_NAMESPACE "PnPToolWidget"

void SPnPToolWidget::Construct(const FArguments& InArgs)
{
	// 1. 创建左侧场景预览用的 RenderTarget（瞬态对象，不保存）
	ScenePreviewRT = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), TEXT("PnPScenePreviewRT"), RF_Transient);
	ScenePreviewRT->InitAutoFormat(960, 540);
	ScenePreviewRT->UpdateResource();

	SceneBrush.SetResourceObject(ScenePreviewRT.Get());
	SceneBrush.ImageSize = FVector2D(ScenePreviewRT->SizeX, ScenePreviewRT->SizeY);
	SceneBrush.DrawAs = ESlateBrushDrawType::Image;

	// 2. 创建右侧 RT 预览用的画刷
	RTBrush = MakeShareable(new FSlateBrush());
	RTBrush->DrawAs = ESlateBrushDrawType::Image;

	// 3. 构建整个窗口的 UI 布局：左半场景预览 + 右半三段式面板
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
					.Text(LOCTEXT("SceneHeader", "场景视口（跟随主相机）"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				]

				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					SNew(SBorder).BorderBackgroundColor(FLinearColor::Black)
					[
						SNew(SImage).Image(this, &SPnPToolWidget::GetSceneBrush)
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
	// 生成"标签 + 控件"一行的 lambda
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
				.Text(LOCTEXT("InputHeader", "输入：相机内参 / RT"))
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
					.Text(LOCTEXT("RTPreviewHeader", "RT 内容预览"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			]

			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SNew(SBorder).BorderBackgroundColor(FLinearColor::Black)
				[
					SNew(SImage).Image(this, &SPnPToolWidget::GetRTBrush)
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
					.Text(LOCTEXT("ResultsHeader", "PnP 计算结果"))
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
		RTBrush->SetResourceObject(SelectedRT.Get());
		RTBrush->ImageSize = FVector2D(SelectedRT->SizeX, SelectedRT->SizeY);
	}
	else
	{
		RTBrush->SetResourceObject(nullptr);
		RTBrush->ImageSize = FVector2D::ZeroVector;
	}
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

	// FEditorViewportClient 不是 UObject，必须用 static_cast
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
}

APnPSolverActor* SPnPToolWidget::FindSolverActor() const
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World) return nullptr;

	// 优先返回编辑器选中的 Actor
	for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
	{
		if (APnPSolverActor* Solver = Cast<APnPSolverActor>(*It))
		{
			return Solver;
		}
	}

	// 否则返回场景中第一个 PnPSolverActor
	for (TActorIterator<APnPSolverActor> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void SPnPToolWidget::ApplyIntrinsicsToSolver(APnPSolverActor* Solver) const
{
	if (!Solver) return;
	Solver->m_FocalLength = FVector2D(Fx, Fy);
	Solver->m_ImageCenter = FVector2D(Cx, Cy);
	Solver->m_ImageResolution = Resolution;
}

FReply SPnPToolWidget::OnSolveClicked()
{
	APnPSolverActor* Solver = FindSolverActor();
	if (!Solver)
	{
		bLastSolveSuccess = false;
		UE_LOG(LogTemp, Warning, TEXT("[PnPToolWidget] 场景中未找到 APnPSolverActor"));
		return FReply::Handled();
	}

	ApplyIntrinsicsToSolver(Solver);
	Solver->RunFullPipeline();

	// 读取结果
	bLastSolveSuccess = Solver->m_bLastSolveSuccess;
	SolvedPose = Solver->m_SolvedCameraPose;
	ReprojectionError = Solver->m_ReprojectionError;
	TranslationError = Solver->m_TranslationError;
	RotationError = Solver->m_RotationError;

	// 成功后再算一次与真值的对比误差
	if (bLastSolveSuccess)
	{
		Solver->CompareSolvedVsGroundTruth();
		TranslationError = Solver->m_TranslationError;
		RotationError = Solver->m_RotationError;
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
