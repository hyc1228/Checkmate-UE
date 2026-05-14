// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Checkmate : ModuleRules
{
	public Checkmate(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"UMG"  // For UUserWidget base class
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Slate", "SlateCore"  // For FWidgetTransform / advanced UMG features
		});
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
