// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/UmbraGameplayAbility.h"
#include "UmbraBasicAttackAbility.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAbilityTask_WaitDelay;

/** Plays the player's basic attack montage and owns the attacking state. */
UCLASS(Blueprintable)
class UMBRA_API UUmbraBasicAttackAbility : public UUmbraGameplayAbility
{
	GENERATED_BODY()

public:
	UUmbraBasicAttackAbility();

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

	/** Socket on the player's main Skeletal Mesh used as the center of hit-frame overlap detection. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Hit Detection")
	FName HitDetectionSocketName = NAME_None;

	/** Radius of the Pawn overlap sphere centered on HitDetectionSocketName. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Hit Detection", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float HitDetectionRadius = 100.0f;

private:
	void FacePrimaryAttackTarget();
	bool PlayCurrentAttackMontage();
	bool StartNextComboStep();
	bool StartComboGraceWindow();
	void FinishAbility(bool bWasCancelled);

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ComboInputTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> AttackHitWindowTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> ComboWindowTask;

	int32 CurrentComboIndex = INDEX_NONE;
	bool bComboInputQueued = false;
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
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();
};
