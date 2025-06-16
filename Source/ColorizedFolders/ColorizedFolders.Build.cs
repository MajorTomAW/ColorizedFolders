// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ColorizedFolders : ModuleRules
{
	public ColorizedFolders(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PrivateDependencyModuleNames.AddRange(new []
		{ 
			"Core",
			"CoreUObject", 
			"Engine", 
			"Slate", 
			"SlateCore",
			"Json",
			"JsonUtilities",
			"ContentBrowser",
			"ContentBrowserData",
			"UnrealEd",
			"Projects", 
			"SettingsEditor",
			"AssetTools",
			"ToolWidgets",
		});
	}
}
