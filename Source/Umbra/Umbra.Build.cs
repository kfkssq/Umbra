// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Umbra : ModuleRules
{
	public Umbra(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Umbra",
			"Umbra/Variant_Platforming",
			"Umbra/Variant_Platforming/Animation",
			"Umbra/Variant_Combat",
			"Umbra/Variant_Combat/AI",
			"Umbra/Variant_Combat/Animation",
			"Umbra/Variant_Combat/Gameplay",
			"Umbra/Variant_Combat/Interfaces",
			"Umbra/Variant_Combat/UI",
			"Umbra/Variant_SideScrolling",
			"Umbra/Variant_SideScrolling/AI",
			"Umbra/Variant_SideScrolling/Gameplay",
			"Umbra/Variant_SideScrolling/Interfaces",
			"Umbra/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
