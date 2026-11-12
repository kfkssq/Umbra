using UnrealBuildTool;

public class UmbraEditorTools : ModuleRules
{
    public UmbraEditorTools(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new[] { "Umbra", "UnrealEd", "UMG", "UMGEditor", "Slate", "SlateCore", "GameplayAbilities", "AIModule", "EnhancedInput", "InputCore", "AnimGraph", "AnimGraphRuntime", "BlueprintGraph", "NavigationSystem" });
    }
}
