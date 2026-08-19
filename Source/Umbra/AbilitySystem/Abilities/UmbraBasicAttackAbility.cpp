// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/UmbraBasicAttackAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "UmbraPlayerController.h"

UUmbraBasicAttackAbility::UUmbraBasicAttackAbility()
{
	InputTag = UmbraGameplayTags::Input_Attack_Primary;
	ActivationPolicy = EUmbraAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(UmbraGameplayTags::Ability_Attack_Basic);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(UmbraGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(UmbraGameplayTags::State_Attacking);
}

void UUmbraBasicAttackAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	UAnimInstance* AnimInstance = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!AttackMontage || !AnimInstance || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		FinishAbility(true);
		return;
	}

	FaceCursorGroundLocation();

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		AttackMontage,
		1.0f,
		NAME_None,
		true);
	if (!MontageTask)
	{
		FinishAbility(true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UUmbraBasicAttackAbility::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UUmbraBasicAttackAbility::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UUmbraBasicAttackAbility::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UUmbraBasicAttackAbility::FaceCursorGroundLocation()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const AUmbraPlayerController* PlayerController = ActorInfo
		? Cast<AUmbraPlayerController>(ActorInfo->PlayerController.Get())
		: nullptr;
	if (!Character || !PlayerController)
	{
		return;
	}

	FHitResult CursorHit;
	if (!PlayerController->GetCursorGroundHit(CursorHit))
	{
		return;
	}

	FVector FacingDirection = CursorHit.ImpactPoint - Character->GetActorLocation();
	FacingDirection.Z = 0.0f;
	if (!FacingDirection.IsNearlyZero())
	{
		Character->SetActorRotation(FRotator(0.0f, FacingDirection.Rotation().Yaw, 0.0f));
	}
}

void UUmbraBasicAttackAbility::FinishAbility(bool bWasCancelled)
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
	}
}

void UUmbraBasicAttackAbility::HandleMontageCompleted()
{
	FinishAbility(false);
}

void UUmbraBasicAttackAbility::HandleMontageInterrupted()
{
	FinishAbility(true);
}

void UUmbraBasicAttackAbility::HandleMontageCancelled()
{
	FinishAbility(true);
}
