// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PnPTool : ModuleRules
{
	public PnPTool(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"OpenCVHelper",
				"OpenCV",			// 直接依赖以获取 WITH_OPENCV=1 宏定义（OpenCVHelper 将其作为 Private 依赖，不会传递）
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
			}
		);
	}
}
