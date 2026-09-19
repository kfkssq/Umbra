// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystem/UmbraGameplayAbility.h"
#include "AbilitySystem/Damage/UmbraPhysicalDamage.h"
#include "CoreMinimal.h"
#include "UmbraEnemyBasicAttackAbility.generated.h"

class UAnimMontage;
class UGameplayEffect;

/** Server-authoritative enemy melee attack driven by montage hit-window events. */
UCLASS(Blueprintable)
class UMBRA_API UUmbraEnemyBasicAttackAbility : public UUmbraGameplayAbility
{
	GENERATED_BODY()

public:
	UUmbraEnemyBasicAttackAbility();
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Damage")
	FUmbraPhysicalDamageConfig DamageConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Hit Detection", meta = (ClampMin = "1.0"))
	float HitRadius = 300.0f;

private:
	friend class FUmbraCombatMaintenanceTest;
	bool bEndingAttack = false;
	uint8 PreviousMovementMode = 0;
	uint8 PreviousCustomMovementMode = 0;
	void FinishAttack(bool bCancelled);

	UFUNCTION()
	void HandleHitWindow(FGameplayEventData Payload);
	UFUNCTION()
	void HandleCompleted();
	UFUNCTION()
	void HandleInterrupted();
	UFUNCTION()
	void HandleCancelled();

	bool bDamageApplied = false;
	bool bMovementLocked = false;
};
