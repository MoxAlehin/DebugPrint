// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DebugPrint : ModuleRules
{
	public DebugPrint(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});
	
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"BlueprintGraph",
			"KismetCompiler",
			"GraphEditor",
			"UnrealEd",
			"ToolMenus",
			"DeveloperSettings"
		});
	}
}
