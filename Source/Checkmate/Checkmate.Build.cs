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
			"Slate", "SlateCore",  // For FWidgetTransform / advanced UMG features
			"LevelSequence", "MovieScene", "MovieSceneTracks",  // Cinematic sequence integration
			"FMODStudio"  // 音频中间件；插件在 Plugins/FMODStudio/
		});

		// FMOD 路径启用——AudioService.cpp 内 #if WITH_FMOD_CHECKMATE 块会被编译
		PublicDefinitions.Add("WITH_FMOD_CHECKMATE=1");

		// Editor-only：关卡编辑器 EUW 需要 Blutility（UEditorUtilityWidget）+
		// UnrealEd（UEditorAssetLibrary / asset save）。Shipping build 不打包。
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"Blutility", "UMGEditor", "UnrealEd", "EditorScriptingUtilities"
			});
		}
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
