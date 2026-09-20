// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/UmbraGameplayAbility.h"
#include "AbilitySystem/Damage/UmbraPhysicalDamage.h"
#include "UmbraBasicAttackAbility.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAbilityTask_WaitDelay;
class UAbilityTask_WaitGameplayTagAdded;
class UGameplayEffect;
class UUmbraAbilitySystemComponent;

enum class EUmbraBasicAttackPhase : uint8
{
	Invalid,
	Windup,
	Active,
	Recovery
};

/** Plays the player's basic attack montage and owns the attacking state. */
UCLASS(Blueprintable)
class UMBRA_API UUmbraBasicAttackAbility : public UUmbraGameplayAbility
{
	GENERATED_BODY()

public:
	UUmbraBasicAttackAbility();
	/** 1x logical strike-start interval in seconds; zero means the attack asset is not configured. */
	float GetConfiguredBaseAttackInterval() const;
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	/** Ordered basic-attack montages. Configure A, B, and C on GA_BasicAttack. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	TArray<TObjectPtr<UAnimMontage>> AttackMontages;

	/** Grace period after a montage completes during which the next combo step can still be requested. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Combo", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ComboWindowDuration = 0.35f;

	/** Uses the ordered high-speed montage loop when the final multiplier reaches the threshold. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Speed",
		meta = (ToolTip = "Chooses the high-speed montage independently for each strike. Changes never replace a montage already playing."))
	bool bEnableHighSpeedAttackMode = true;

	/** Direct attack speed threshold. 3.0 means 300% base attack speed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Speed",
		meta = (ClampMin = "0.2", ClampMax = "10.0", UIMin = "0.2", UIMax = "10.0", Units = "x"))
	float HighSpeedAttackThreshold = 3.f;

	/**
	 * Ordered high-speed loop. Configure A then B for A-B-A-B; null entries are skipped.
	 * If no valid entry exists, AttackMontages[0] is used safely.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Speed",
		meta = (ToolTip = "Ordered high-speed attack loop. Add A at index 0 and B at index 1 for A-B-A-B."))
	TArray<TObjectPtr<UAnimMontage>> HighSpeedAttackMontages;

	/**
	 * Desired seconds between strike starts at final multiplier 1.0.
	 * Zero automatically uses the full authored duration of AttackMontages[0], preserving 1x authored speed.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Speed",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s",
			ToolTip = "0 = derive from the first normal montage so base attack speed plays at 1x. A positive value is an explicit target interval."))
	float BaseAttackInterval = 0.f;

	/** Fraction of the logical period before the server resolves the strike. Valid range (0, 1). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Speed", meta = (ClampMin = "0.01", ClampMax = "0.99", UIMin = "0.01", UIMax = "0.99"))
	float AttackWindupRatio = 0.3f;

	/** Optional authored strike time in montage seconds. Overrides the first legacy Hit Window Begin. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Speed")
	TMap<TObjectPtr<UAnimMontage>, float> VisualStrikeTimeOverrides;

	/** Extra centimeters beyond the shared primary attack range for hit resolution. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Hit Detection", meta = (ClampMin = "0.0", Units = "cm"))
	float HitDistanceTolerance = 0.f;

	/** Trace Visibility from attacker to target at the strike time. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Hit Detection")
	bool bCheckAttackOcclusion = true;

	/**
	 * Safety ceiling for actual montage playback. A shorter high-speed montage is required when this ceiling would prevent the target attack period.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Speed",
		meta = (ClampMin = "0.1", ClampMax = "10.0", UIMin = "0.1", UIMax = "10.0", Units = "x",
			ToolTip = "Visual playback limit only. Logical attack timing is unaffected; use shorter high-speed montages if the strike pose arrives late."))
	float MaxAttackMontagePlayRate = 3.f;

	/** Visual blend-out used by movement cancellation; movement execution does not wait for this blend. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Cancellation",
		meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.25", Units = "s"))
	float MovementCancelBlendOutTime = 0.10f;

	/** Socket on the player's main Skeletal Mesh used as the center of hit-frame overlap detection. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Hit Detection")
	FName HitDetectionSocketName = NAME_None;

	/** Optional blade root. The root-to-tip segment covers close targets the tip arc passes beyond. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Hit Detection")
	FName HitDetectionStartSocketName = TEXT("FX_Sword_Bottom");

	/** Sweep radius in cm, shared by the blade segment and the tip's temporal sweep. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Hit Detection", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float HitDetectionRadius = 100.0f;

	/** Instant GE with one UmbraPhysicalDamageExecution and no Modifiers. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Damage")
	FUmbraPhysicalDamageConfig DamageConfig;

private:
	friend class FUmbraCombatMaintenanceTest;
	bool bEndingAttack = false;
	void FacePrimaryAttackTarget();
	bool PlayCurrentAttackMontage();
	bool StartNextComboStep(bool bAllowWrap);
	bool StartComboGraceWindow();
	float ReadAttackSpeed() const;
	float ScaleAttackDuration(float BaseDuration) const;
	float GetSafeHighSpeedThreshold() const;
	float ResolveBaseAttackInterval() const;
	float GetSafeMaxMontagePlayRate() const;
	static float CalculateMontagePlayRate(float EffectiveAuthoredDuration,
		float InBaseAttackInterval, float AttackSpeedMultiplier, float MaxPlayRate);
	static float CalculateEffectiveAttackPeriod(float EffectiveAuthoredDuration, float MontagePlayRate,
		float InBaseAttackInterval, float AttackSpeedMultiplier);
	bool FindValidChainPointTime(const UAnimMontage* Montage, float& OutChainPointTime) const;
	bool AnalyzeHitWindows(const UAnimMontage* Montage, float& OutFirstBegin,
		float& OutLastEnd, int32& OutWindowCount) const;
	UAnimMontage* SelectAttackMontage(bool bHighSpeedMode);
	UAnimMontage* GetFirstValidHighSpeedAttackMontage() const;
	int32 FindNextValidHighSpeedMontageIndex(int32 PreviousIndex) const;
	bool HasAnyConfiguredAttackMontage() const;
	bool TryAdvanceAtTransition();
	bool IsCurrentAttackEvent(const FGameplayEventData& Payload) const;
	UUmbraAbilitySystemComponent* GetUmbraAbilitySystemComponent() const;
	void QueueTransitionEvaluation(bool bFromMontageEnd);
	void EvaluatePendingTransition(uint32 ExpectedAttackInstanceId);
	void ScheduleIntervalTransition(uint32 ExpectedAttackInstanceId);
	void ResolveLogicalStrike(uint32 ExpectedAttackInstanceId);
	float ResolveVisualStrikeTime(const UAnimMontage* Montage) const;
	void CancelMovementAtCurrentPhase();
	void StopMontageForMovementCancellation();
	bool ShouldContinueAutoAttack() const;
	void ReleaseActiveMontageTask();
	void FinishAbility(bool bWasCancelled);

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ComboInputTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> AttackHitWindowTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> AttackChainPointTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> MovementCommandTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> ComboWindowTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayTagAdded> DeathInterruptTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayTagAdded> StunInterruptTask;

	int32 CurrentComboIndex = INDEX_NONE;
	int32 CurrentHighSpeedMontageIndex = INDEX_NONE;
	/** Snapshot once per montage so a mid-swing attribute change applies to the next strike. */
	float CapturedAttackSpeed = 1.f;
	float CurrentMontagePlayRate = 1.f;
	float CurrentEffectiveAttackDuration = 0.f;
	float CurrentEffectiveAttackPeriod = 0.f;
	float CurrentFirstHitWindowTime = 0.f;
	float CurrentLastHitWindowTime = 0.f;
	TObjectPtr<UAnimMontage> CurrentAttackMontage;
	uint32 AttackInstanceId = 0;
	int32 ExpectedHitWindowCount = 0;
	int32 EndedHitWindowCount = 0;
	int32 OpenHitWindowCount = 0;
	EUmbraBasicAttackPhase AttackPhase = EUmbraBasicAttackPhase::Invalid;
	bool bComboInputQueued = false;
	bool bCurrentStepHighSpeed = false;
	bool bPreviousStepHighSpeed = false;
	bool bCurrentMontageHasChainPoint = false;
	bool bHitWindowActive = false;
	bool bHitWindowConfigurationValid = false;
	bool bAttackIntervalCommitted = false;
	bool bMovementCancelRequested = false;
	bool bExecuteQueuedCommandOnEnd = false;
	bool bPendingTransitionFromMontageEnd = false;
	FTimerHandle AttackIntervalTimerHandle;
	FTimerHandle AttackWindupTimerHandle;
	TWeakObjectPtr<AActor> StrikeTarget;
	float CurrentWindupSeconds = 0.f;
	bool bStrikeResolved = false;
	bool bHasPreviousHitSocketLocation = false;
	FVector PreviousHitSocketLocation = FVector::ZeroVector;
	TSet<TWeakObjectPtr<AActor>> HitActorsThisComboStep;

	UFUNCTION()
	void HandleComboInput(FGameplayEventData Payload);

	UFUNCTION()
	void HandleComboWindowExpired();

	UFUNCTION()
	void HandleAttackHitWindow(FGameplayEventData Payload);

	UFUNCTION()
	void HandleAttackChainPoint(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMovementCommand(FGameplayEventData Payload);

	UFUNCTION()
	void HandleForcedInterrupt();

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();
};
