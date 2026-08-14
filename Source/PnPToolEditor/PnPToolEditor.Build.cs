// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PnPToolEditor : ModuleRules
{
	public PnPToolEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"RenderCore",
				"RHI",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"UnrealEd",
				"EditorStyle",
				"EditorSubsystem",
				"LevelEditor",
				"PropertyEditor",
				"AssetRegistry",
				"ContentBrowser",
				"ToolMenus",
			"PnPTool",
			"OpenCVHelper",
			"OpenCV",
			"AppFramework",
			}
		);
	}
}
