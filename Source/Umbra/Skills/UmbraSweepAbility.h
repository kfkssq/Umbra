#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/UmbraGameplayAbility.h"
#include "GameplayEffect.h"
#include "UmbraSweepAbility.generated.h"

class UUmbraSweepDefinition;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_ApplyRootMotionJumpForce;

UCLASS()
class UMBRA_API UUmbraSwordStormEffect : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UUmbraSwordStormEffect();
};

UCLASS()
class UMBRA_API UUmbraSkillCostEffect : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UUmbraSkillCostEffect();
};

UCLASS()
class UMBRA_API UUmbraSkillCooldownEffect : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UUmbraSkillCooldownEffect();
};

UCLASS()
class UMBRA_API UUmbraSkillGuardEffect : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UUmbraSkillGuardEffect();
};

UCLASS()
class UMBRA_API UUmbraSkillFocusEffect : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UUmbraSkillFocusEffect();
};

UCLASS()
class UMBRA_API UUmbraSkillHealEffect : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UUmbraSkillHealEffect();
};

/** Data-driven melee skill executor. The legacy class name preserves existing asset references. */
UCLASS()
class UMBRA_API UUmbraSweepAbility : public UUmbraGameplayAbility
{
    GENERATED_BODY()
public:
    UUmbraSweepAbility();
    virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo) override;
    virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags=nullptr, const FGameplayTagContainer* TargetTags=nullptr,
        FGameplayTagContainer* OptionalRelevantTags=nullptr) const override;
    virtual bool CheckCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* Tags=nullptr) const override;
    virtual bool CheckCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* Tags=nullptr) const override;
    virtual void ApplyCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const override;
    virtual void ApplyCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const override;
    virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
private:
    const UUmbraSweepDefinition* FindDefinition(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;
    void Strike();
    void FinishWindup();
    void DamagePulse();
    void LaunchProjectile();
    void BeginGuard();
    UFUNCTION() void Landed();
    UFUNCTION() void TargetLeapArrived();
    UFUNCTION() void TargetLeapLanded(const FHitResult& Hit);
    UPROPERTY() TObjectPtr<class UAbilityTask_ApplyRootMotionMoveToForce> TargetLeapTask;
    FVector ConfirmedGround;
    UFUNCTION() void MontageCompleted();
    UFUNCTION() void Complete();
    UFUNCTION() void Interrupt();
    UFUNCTION() void MovementRequested(FGameplayEventData Event);
    UPROPERTY() TObjectPtr<const UUmbraSweepDefinition> ActiveDefinition;
    UPROPERTY() TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;
    UPROPERTY() TObjectPtr<UAbilityTask_ApplyRootMotionJumpForce> LeapTask;
    FVector Origin;
    FVector Forward;
    FTimerHandle StrikeTimer;
    FTimerHandle WindupTimer;
    FTimerHandle FinishTimer;
    FActiveGameplayEffectHandle GuardHandle;
    FActiveGameplayEffectHandle StormHandle;
    int32 PulsesRemaining = 0;
    bool bLanded = false;
    double ActivationStartTime = 0.;
    bool bStruck = false;
    bool bEnding = false;
    bool bExecuteMovement = false;
    bool bChannelReleased = false;
};
