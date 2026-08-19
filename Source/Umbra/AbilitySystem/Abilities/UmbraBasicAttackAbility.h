// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/UmbraGameplayAbility.h"
#include "UmbraBasicAttackAbility.generated.h"

class UAnimMontage;

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
	/** Attack montage configured by GA_BasicAttack. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> AttackMontage;

private:
	void FaceCursorGroundLocation();
	void FinishAbility(bool bWasCancelled);

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();
};
