// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/UmbraEnemyBasicAttackAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/UmbraEnemyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTags/UmbraGameplayTags.h"

UUmbraEnemyBasicAttackAbility::UUmbraEnemyBasicAttackAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(UmbraGameplayTags::Ability_Attack_EnemyBasic);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(UmbraGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(UmbraGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(UmbraGameplayTags::State_Dead);
}

void UUmbraEnemyBasicAttackAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	AUmbraEnemyCharacter* Enemy = ActorInfo ? Cast<AUmbraEnemyCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Enemy || !Enemy->IsAIBehaviorEnabled() || Enemy->IsDead() || !IsValid(Enemy->GetCombatTarget()) || !AttackMontage
		|| !DamageEffectClass || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		FinishAttack(true);
		return;
	}

	bDamageApplied = false;
	if (AController* Controller = Enemy->GetController())
	{
		Controller->StopMovement();
	}
	PreviousMovementMode = Enemy->GetCharacterMovement()->MovementMode;
	PreviousCustomMovementMode = Enemy->GetCharacterMovement()->CustomMovementMode;
	Enemy->GetCharacterMovement()->DisableMovement();
	bMovementLocked = true;
	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, UmbraGameplayTags::Event_Attack_HitWindow, nullptr, false, false);
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, AttackMontage, 1.0f, NAME_None, true);
	if (!HitTask || !MontageTask)
	{
		FinishAttack(true);
		return;
	}
	HitTask->EventReceived.AddDynamic(this, &UUmbraEnemyBasicAttackAbility::HandleHitWindow);
	HitTask->ReadyForActivation();
	MontageTask->OnCompleted.AddDynamic(this, &UUmbraEnemyBasicAttackAbility::HandleCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UUmbraEnemyBasicAttackAbility::HandleInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UUmbraEnemyBasicAttackAbility::HandleCancelled);
	MontageTask->ReadyForActivation();
}

void UUmbraEnemyBasicAttackAbility::HandleHitWindow(FGameplayEventData Payload)
{
	if (!IsActive() || bEndingAttack || bDamageApplied || Payload.EventTag != UmbraGameplayTags::Event_Attack_HitWindowTick)
	{
		return;
	}
	AUmbraEnemyCharacter* Enemy = Cast<AUmbraEnemyCharacter>(GetAvatarActorFromActorInfo());
	AActor* Target = Enemy ? Enemy->GetCombatTarget() : nullptr;
	if (!Enemy || !Enemy->IsAIBehaviorEnabled() || !IsValid(Target)
		|| FVector::DistSquared2D(Enemy->GetActorLocation(), Target->GetActorLocation()) > FMath::Square(HitRadius))
	{
		return;
	}
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC)
	{
		return;
	}
	// Mark the confirmed hit before application, including blocked/rejected effects.
	bDamageApplied = true;
	UmbraPhysicalDamage::Apply(GetAbilitySystemComponentFromActorInfo(), TargetASC,
		DamageEffectClass, DamageConfig, GetAbilityLevel());
}

void UUmbraEnemyBasicAttackAbility::FinishAttack(bool bCancelled)
{
	if (IsActive() && !bEndingAttack)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bCancelled);
	}
}

void UUmbraEnemyBasicAttackAbility::EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bEndingAttack || !IsEndAbilityValid(Handle, ActorInfo)) return;
	if (ScopeLockCount > 0)
	{
		WaitingToExecute.Add(FPostLockDelegate::CreateUObject(this, &ThisClass::EndAbility,
			Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled));
		return;
	}
	TGuardValue<bool> EndingGuard(bEndingAttack, true);
	if (bMovementLocked)
	{
		if (AUmbraEnemyCharacter* Enemy = Cast<AUmbraEnemyCharacter>(GetAvatarActorFromActorInfo()))
		{
			if (!Enemy->IsDead() && Enemy->GetCharacterMovement()->MovementMode == MOVE_None)
			{
				Enemy->GetCharacterMovement()->SetMovementMode(EMovementMode(PreviousMovementMode), PreviousCustomMovementMode);
			}
		}
		bMovementLocked = false;
	}
	bDamageApplied = false;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UUmbraEnemyBasicAttackAbility::HandleCompleted() { FinishAttack(false); }
void UUmbraEnemyBasicAttackAbility::HandleInterrupted() { FinishAttack(true); }
void UUmbraEnemyBasicAttackAbility::HandleCancelled() { FinishAttack(true); }
