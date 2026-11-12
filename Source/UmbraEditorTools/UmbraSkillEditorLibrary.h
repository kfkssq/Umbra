#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UmbraSkillEditorLibrary.generated.h"
class UUserWidget;
class UWidget;

/** Editor-only creation of editable widget templates; existing assets are never overwritten. */
UCLASS()
class UMBRAEDITORTOOLS_API UUmbraSkillEditorLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Authoring") static bool ConfigureTargetLeapCurve(class UCurveVector* Curve, float Height);
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Test") static bool InjectTargetPointer(AActor* Player, FKey Key, bool bPressed);
	UFUNCTION(BlueprintCallable, Category="Umbra|UI Authoring") static bool CreateTargetAndMenuPages();
    UFUNCTION(BlueprintCallable, Category="Umbra|UI Authoring") static bool AdoptEnemyNameplateTemplate();
    UFUNCTION(BlueprintCallable, Category="Umbra|UI Authoring") static bool CreateSkillTooltipPresentation();
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Lab") static bool ConfigureLabNavigation(UObject* WorldContext);
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Test") static TArray<FVector> GetLabNavigationPath(AActor* Player, FVector Destination);
    /** PIE-only cursor projection and real PrimaryAction injection for click-to-move acceptance. */
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Test") static bool InjectLabGroundClick(AActor* Player, FVector Destination, bool bPressed);
    /** Creates the isolated lab's editable idle/run blend and full-body montage slot. Never overwrites. */
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Lab") static bool CreateNativeLabAnimBlueprint();
    /** Import the installed engine's Characters template via package load/save; never overwrite existing project assets. */
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Lab") static int32 ImportEngineCharacterBaseline();
    /** Copies only resource-bar presentation into a static experience placeholder. */
    UFUNCTION(BlueprintCallable, Category="Umbra|UI Authoring") static bool CreateExperienceBarPlaceholder();
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Authoring") static bool CreateSkillWidgets();
    /** Explicit migration of the prototype bar into the existing HUD's middle column. */
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Authoring") static bool EmbedSkillBarInHUD();
    /** Explicit one-time adoption of Fantasy GUI 4 template widget trees. */
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Authoring") static bool AdoptActionbarTemplates();
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Authoring") static FString DescribeWidget(UObject* Asset);
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Authoring") static FString DescribeTemplateBehavior(UObject* Asset);
    /** Transient PIE fixture only; never saves map actors or assets. */
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Test") static AActor* PrepareSkillTest(UObject* WorldContext);
    UFUNCTION(BlueprintPure, Category="Umbra|Skill Test") static float ReadTestAttribute(AActor* Actor, FName Attribute);
    /** PIE-only fixtures; input enters the real Enhanced Input action binding. */
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Test") static bool InjectSkillTestInput(AActor* Actor, int32 SlotIndex, bool bPressed = true);
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Test") static bool InjectMovementTestInput(AActor* Actor, FVector2D Axis);
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Test") static bool SetSkillTestAttribute(AActor* Actor, FName Attribute, float Value);
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Test") static void InterruptSkillTest(AActor* Actor, bool bMovement);
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Test") static AActor* SpawnSkillTestWall(AActor* Player);
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Authoring") static FString DescribeParticleSystem(UObject* Asset);
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Authoring") static FString ConfigureStormParticle(class UParticleSystem* System, float CycleSeconds = 0.f);
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Authoring") static bool ConfigureStormHitParticle(class UParticleSystem* System);
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Test") static int32 CountStormParticles(AActor* Player, class UParticleSystem* System);
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Test") static int32 CountStormHitParticlesNear(AActor* Actor, class UParticleSystem* System, float Radius = 60.f);
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Authoring") static bool InstallRadialCooldown(class UMaterialInterface* Material);
    /** Retains the source timing/pose but scales its root height into a readable visual leap. */
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Authoring") static bool ScaleSkillRootHeight(class UAnimSequence* Source, class UAnimSequence* Destination, float MaximumHeight);
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Authoring") static bool SetSkillMontageSequence(class UAnimMontage* Montage, class UAnimSequence* Animation, int32 Loops);
    /** Normal-speed entry/exit, extended middle pose; source sequence is never modified. */
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Authoring") static bool ConfigureGuardMontage(class UAnimMontage* Montage, class UAnimSequence* Animation, float Duration);
    /** Bake an in-place 360-degree loop from the skill's sword pose and native stepping legs. Vendor sources stay untouched. */
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Authoring") static bool ConfigureSwordStormAnimation(class UAnimSequence* Upper, class UAnimSequence* Legs, class UAnimSequence* Destination);
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Authoring") static bool ConfigureSwordStormAnimBlueprint(class UAnimBlueprint* Blueprint, class UAnimMontage* Montage);
    UFUNCTION(BlueprintPure, Category="Umbra|Skill Test") static FString ReadCooldownText(UUserWidget* Widget);
    /** Keeps all three original impact variants in an editable Sound Cue. */
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Authoring") static bool ConfigurePebbleImpactAudio(
        class USoundCue* Cue, const TArray<class USoundWave*>& Variants);
    /** Creates editable, project-authored emitters. Stage: 0 cast, 1 flight, 2 impact, 3 release. Not an FXR converter. */
    UFUNCTION(BlueprintCallable, Category="Umbra|Skill Authoring") static bool ConfigurePebbleParticles(
        class UParticleSystem* System, class UMaterialInterface* GlowMaterial, class UMaterialInterface* NoiseMaterial, int32 Stage);
    UFUNCTION(BlueprintPure, Category="Umbra|Skill Test") static UWidget* FindPresentationWidget(UUserWidget* Widget, const FString& Path);
};
