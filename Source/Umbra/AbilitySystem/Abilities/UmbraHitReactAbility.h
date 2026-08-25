// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystem/UmbraGameplayAbility.h"
#include "CoreMinimal.h"
#include "UmbraHitReactAbility.generated.h"

/** Plays an enemy-owned hit reaction in response to Event.Combat.HitReceived. */
UCLASS()
class UMBRA_API UUmbraHitReactAbility : public UUmbraGameplayAbility
{
	GENERATED_BODY()

public:
	UUmbraHitReactAbility();

	virtual void ActivateAbility(
		FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	void FinishHitReact(bool bWasCancelled);

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();
};
