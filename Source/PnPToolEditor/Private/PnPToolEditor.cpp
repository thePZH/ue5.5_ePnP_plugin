// Copyright Epic Games, Inc. All Rights Reserved.

#include "PnPToolEditor.h"
#include "ToolMenus.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Widgets/Docking/SDockTab.h"
#include "PnPToolWidget.h"

#define LOCTEXT_NAMESPACE "FPnPToolEditorModule"

void FPnPToolEditorModule::StartupModule()
{
	if (UToolMenus* ToolMenus = UToolMenus::TryGet())
	{
		ToolMenus->RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FPnPToolEditorModule::RegisterMenus));
	}
}

void FPnPToolEditorModule::ShutdownModule()
{
	if (UObjectInitialized())
	{
		UToolMenus::UnRegisterStartupCallback(this);
	}
	m_PnPEditorWindow.Reset();
}

void FPnPToolEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	const FUIAction OpenAction(FExecuteAction::CreateRaw(this, &FPnPToolEditorModule::OpenPnPEditorWindow));

	// 1. Window 菜单项
	if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window"))
	{
		FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
		Section.AddEntry(FToolMenuEntry::InitMenuEntry(
			"PnPEditorTool",
			LOCTEXT("MenuLabel", "PnP Editor Tool"),
			LOCTEXT("MenuTooltip", "打开 PnP 编辑器工具窗口"),
			FSlateIcon(),
			OpenAction
		));
	}

	// 2. 主工具栏按钮
	if (UToolMenu* Toolbar = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar"))
	{
		FToolMenuSection& Section = Toolbar->FindOrAddSection("Content");
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(
			"PnPEditorTool",
			OpenAction,
			LOCTEXT("ToolbarLabel", "PnP Tool"),
			LOCTEXT("ToolbarTooltip", "打开 PnP 编辑器工具窗口"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.TripleState.Play")
		));
	}
}

void FPnPToolEditorModule::OpenPnPEditorWindow()
{
	if (m_PnPEditorWindow.IsValid())
	{
		m_PnPEditorWindow.Pin()->BringToFront(true);
		return;
	}

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("PnPEditorWindowTitle", "PnP 编辑器工具"))
		.ClientSize(FVector2D(1600, 900))
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		.SizingRule(ESizingRule::UserSized);

	Window->SetContent(SNew(SPnPToolWidget));

	FSlateApplication::Get().AddWindow(Window);
	m_PnPEditorWindow = Window;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPnPToolEditorModule, PnPToolEditor)
