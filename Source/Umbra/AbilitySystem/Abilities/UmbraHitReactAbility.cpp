// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/UmbraHitReactAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Characters/UmbraEnemyCharacter.h"
#include "GameplayTags/UmbraGameplayTags.h"

UUmbraHitReactAbility::UUmbraHitReactAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(UmbraGameplayTags::Ability_HitReact);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(UmbraGameplayTags::State_HitReact);
	ActivationBlockedTags.AddTag(UmbraGameplayTags::State_HitReact);
	ActivationBlockedTags.AddTag(UmbraGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(UmbraGameplayTags::State_SuperArmor);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = UmbraGameplayTags::Event_Combat_HitReceived;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UUmbraHitReactAbility::ActivateAbility(
	FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const AUmbraEnemyCharacter* Enemy = ActorInfo ? Cast<AUmbraEnemyCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	UAnimMontage* HitReactMontage = Enemy ? Enemy->GetHitReactMontage() : nullptr;
	if (!HitReactMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		FinishHitReact(true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, HitReactMontage, 1.0f, NAME_None, true);
	if (!MontageTask)
	{
		FinishHitReact(true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UUmbraHitReactAbility::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UUmbraHitReactAbility::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UUmbraHitReactAbility::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UUmbraHitReactAbility::FinishHitReact(bool bWasCancelled)
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
	}
}

void UUmbraHitReactAbility::HandleMontageCompleted()
{
	FinishHitReact(false);
}

void UUmbraHitReactAbility::HandleMontageInterrupted()
{
	FinishHitReact(true);
}

void UUmbraHitReactAbility::HandleMontageCancelled()
{
	FinishHitReact(true);
}
