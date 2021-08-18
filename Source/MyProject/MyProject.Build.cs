// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MyProject : ModuleRules
{
	public MyProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore","UMG", "Slate", "SlateCore",
			"NavigationSystem",
			"Niagara",
			"CableComponent",
			"GameplayTags",
			"GameplayAbilities",
			"GameplayTasks",
			"GameplayCameras",
		});
		
		PrivateDependencyModuleNames.AddRange(new string[] {
				"CableComponent",
				"GameplayAbilities",
				"GameplayTags",
				"GameplayTasks"
			}
		);
	}
}
