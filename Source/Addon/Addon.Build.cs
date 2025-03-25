// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Addon : ModuleRules
{
	public Addon(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
