#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/Damage/UmbraPhysicalDamage.h"
#include "UmbraSweepDefinition.generated.h"

class UAnimMontage;
class UParticleSystem;
class UNiagaraSystem;
class USoundBase;
class UGameplayEffect;
class UTexture2D;

namespace UmbraSkills
{
    /** Prototype switch; authored cooldown values are preserved. */
    UMBRA_API bool AreCooldownsEnabled();
}

UENUM(BlueprintType)
enum class EUmbraSkillExecution : uint8
{
    Sector,
    SwordStorm,
    LeapSlam,
    Guard,
    StoneRecovery,
    Projectile,
    BattleFocus,
    LinearWave
};

/** Immutable authoring data. Two skill specs may execute the same ability class with different definitions. */
UCLASS(BlueprintType)
class UMBRA_API UUmbraSweepDefinition : public UDataAsset
{
    GENERATED_BODY()
public:
    /** Sector remains the default for previously authored sweep assets. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EUmbraSkillExecution Execution = EUmbraSkillExecution::Sector;
    /** Opt-in only: existing skills retain immediate casting. Range uses LeapDistance, impact uses Radius. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Targeting") bool bTargetedLeap = false;
    /** Isolated directional prototypes; existing definitions default to immediate casting. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Targeting") bool bTargetedDirection = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Targeting", meta=(Units="cm")) float TargetLineLength = 800.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Targeting", meta=(Units="cm")) float TargetLineHalfWidth = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Targeting", meta=(Units="cm/s")) float TargetWaveSpeed = 1000.f;
    bool UsesTargeting() const { return bTargetedLeap || bTargetedDirection; }
    // World-aligned decal bounds must include a rotated rectangle's far corners.
    float DirectionExtent() const { return Execution == EUmbraSkillExecution::LinearWave ? FMath::Sqrt(FMath::Square(TargetLineLength) + FMath::Square(TargetLineHalfWidth)) : Radius; }

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Targeting", meta=(Units="cm")) float TargetMaxHeightDifference = 150.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Targeting") TObjectPtr<class UMaterialInterface> TargetIndicatorMaterial;
    /** Shared by trajectory preview sweeps and the actual root-motion move. Z is height in cm, time is 0..1. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Targeting") TObjectPtr<class UCurveVector> TargetLeapCurve;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SkillId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
    /** Player-facing mechanics description; UI does not calculate combat damage. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(MultiLine=true)) FText Description;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UTexture2D> Icon;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTag CooldownTag;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimMontage> Montage;
    /** Authored montage seconds, divided by PlayRate for both presentation and the server strike. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Units="s")) float StrikeTime = 0.4f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1", ClampMax="3")) float PlayRate = 1.f;
    /** Source montage seconds used for the preparation motion; zero disables separate windup. Projectile skills only. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing|Windup", meta=(ClampMin="0", Units="s")) float WindupEndTime = 0.f;
    /** Real seconds spent playing 0..WindupEndTime. Remaining animation uses PlayRate. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing|Windup", meta=(ClampMin="0", ClampMax="3", Units="s")) float WindupDuration = 0.f;
    UFUNCTION(BlueprintPure, Category="Timing") float GetReleaseDelay() const;
    float GetInitialPlayRate() const;
    /** Target actor centers, in centimeters; geometry is a horizontal sector with a vertical tolerance. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Units="cm")) float Radius = 250.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Units="deg")) float ArcDegrees = 120.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Units="cm")) float HeightTolerance = 120.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float ResourceCost = 15.f;
    /** Fixed seconds for this prototype. AbilityHaste and AttackSpeed do not modify these timings. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Units="s")) float CooldownSeconds = 4.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FUmbraPhysicalDamageConfig Damage;
    /** Required only by the legacy damage model. Typed damage uses the existing native execution. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<UGameplayEffect> DamageEffect;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UParticleSystem> StrikeEffect;
    /** Choose one renderer per phase. Legacy Cascade assets remain supported. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Niagara") TObjectPtr<UNiagaraSystem> StrikeNiagara;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector EffectOffset = FVector(0, 0, 0);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FRotator EffectRotation = FRotator::ZeroRotator;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector EffectScale = FVector::OneVector;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<USoundBase> StrikeSound;

    /** Storm pulses start at StrikeTime / PlayRate. Each pulse may hit each target once. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Storm", meta=(ClampMin="1", ClampMax="20")) int32 PulseCount = 6;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Storm", meta=(Units="s")) float PulseInterval = 0.5f;
    /** When enabled, damage follows the first montage segment cycle instead of PulseInterval. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Storm") bool bSyncStormToAnimation = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Storm", meta=(ClampMin="1", ClampMax="8")) int32 StormPulsesPerCycle = 1;
    float GetStormPulseInterval() const;
    /** Storm repeats until input release/interruption; PulseCount is ignored while channeling. Cost is still activation-only. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Storm") bool bChannelWhileHeld = false;
    /** Guard duration starts when the skill commits, independent of montage play rate. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Guard", meta=(Units="s")) float GuardDuration = 2.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Guard") float GuardArmor = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Guard") float GuardPhysicalResistance = 50.f;
    /** Leap starts at StrikeTime / PlayRate; impact requires a real CharacterMovement landing. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Leap", meta=(Units="cm")) float LeapDistance = 450.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Leap", meta=(Units="cm")) float LeapHeight = 160.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Leap", meta=(Units="s")) float LeapDuration = 0.65f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recovery", meta=(ClampMin="0", ClampMax="1")) float HealMaxHealthFraction = 0.3f;
    /** Seconds after the cast resolves. Refreshes rather than stacks; removed on death. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Focus", meta=(Units="s")) float FocusDuration = 6.f;
    /** Additive probability: 0.2 means twenty percentage points; GAS clamps the resulting chance. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Focus", meta=(ClampMin="0", ClampMax="1")) float FocusCriticalChance = .2f;
    /** Attached at capsule origin, with its own transform. Lifetime is owned by the ability. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TObjectPtr<UParticleSystem> ActiveEffect;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Niagara") TObjectPtr<UNiagaraSystem> ActiveNiagara;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") FVector ActiveEffectOffset = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") FVector ActiveEffectScale = FVector::OneVector;
    /** Optional mesh socket; empty retains the existing capsule attachment for melee skills. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") FName CastSocket;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TObjectPtr<USoundBase> CastSound;
    /** Optional voice layers; never use weapon sounds as dialogue. Resolved voice follows gameplay resolution. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Audio") TObjectPtr<USoundBase> CastVoice;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Audio") TObjectPtr<USoundBase> ResolveVoice;
    /** Authored looping sound, stopped on every ability exit. Leave empty when no suitable loop exists. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Audio") TObjectPtr<USoundBase> ActiveSound;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Audio") TObjectPtr<class USoundAttenuation> SkillAttenuation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Audio") TObjectPtr<class USoundConcurrency> SkillSoundConcurrency;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Audio", meta=(ClampMin="0", ClampMax="2")) float SkillSoundVolume = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Audio", meta=(ClampMin="0", ClampMax="2")) float SkillVoiceVolume = 0.65f;
    /** Projectile tuning is in UE centimeters/seconds, independent of animation playback rate. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile") float ProjectileInitialSpeed = 1300.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile") float ProjectileMaxSpeed = 2500.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile") float ProjectileAcceleration = 4000.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile") float ProjectileAccelerationDelay = .32f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile") float ProjectileLifetime = 1.12f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile") float ProjectileRadius = 10.f;
    /** Zero keeps spherical projectiles. Positive values use a forward-facing wide wave collision box (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile") float ProjectileHalfWidth = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile") TObjectPtr<class UStaticMesh> ProjectileMesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile") TObjectPtr<class UMaterialInterface> ProjectileMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile") FVector ProjectileMeshScale = FVector::OneVector;
    /** Degrees/second toward a launch-selected attackable target; 0 disables steering. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile") float ProjectileTurnRate = 35.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile") TObjectPtr<UParticleSystem> ProjectileEffect;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile") TObjectPtr<UParticleSystem> ImpactEffect;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Niagara") TObjectPtr<UNiagaraSystem> ProjectileNiagara;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Niagara") TObjectPtr<UNiagaraSystem> ImpactNiagara;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile") TObjectPtr<USoundBase> ImpactSound;
    /** Attached loop owned by the projectile; stopped on impact or destruction. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile") TObjectPtr<USoundBase> ProjectileSound;

    bool Validate(FString& Reason) const;
    bool ContainsTarget(const FVector& Origin, const FVector& Forward, const FVector& Target) const;
};

USTRUCT(BlueprintType)
struct FUmbraSkillSlot
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UUmbraSweepDefinition> Definition;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Categories="Input")) FGameplayTag InputTag;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText KeyLabel;
};
