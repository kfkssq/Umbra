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
	if (!Enemy || Enemy->IsDead() || !IsValid(Enemy->GetCombatTarget()) || !AttackMontage
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
	if (bDamageApplied || Payload.EventTag != UmbraGameplayTags::Event_Attack_HitWindowTick)
	{
		return;
	}
	AUmbraEnemyCharacter* Enemy = Cast<AUmbraEnemyCharacter>(GetAvatarActorFromActorInfo());
	AActor* Target = Enemy ? Enemy->GetCombatTarget() : nullptr;
	if (!IsValid(Target) || FVector::DistSquared2D(Enemy->GetActorLocation(), Target->GetActorLocation()) > FMath::Square(HitRadius))
	{
		return;
	}
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC)
	{
		return;
	}
	const FGameplayEffectSpecHandle DamageSpec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
	if (DamageSpec.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC);
		bDamageApplied = true;
	}
}

void UUmbraEnemyBasicAttackAbility::FinishAttack(bool bCancelled)
{
	if (bMovementLocked)
	{
		if (AUmbraEnemyCharacter* Enemy = Cast<AUmbraEnemyCharacter>(GetAvatarActorFromActorInfo()))
		{
			if (!Enemy->IsDead())
			{
				Enemy->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			}
		}
		bMovementLocked = false;
	}
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bCancelled);
	}
}

void UUmbraEnemyBasicAttackAbility::HandleCompleted() { FinishAttack(false); }
void UUmbraEnemyBasicAttackAbility::HandleInterrupted() { FinishAttack(true); }
void UUmbraEnemyBasicAttackAbility::HandleCancelled() { FinishAttack(true); }
