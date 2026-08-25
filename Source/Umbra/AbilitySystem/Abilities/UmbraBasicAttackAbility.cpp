// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/UmbraBasicAttackAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Characters/UmbraPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/HitResult.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Umbra.h"

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
	if (AttackMontages.IsEmpty() || !AnimInstance || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		FinishAbility(true);
		return;
	}

	CurrentComboIndex = 0;
	bComboInputQueued = false;
	ComboInputTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		UmbraGameplayTags::Event_Attack_ComboInput,
		nullptr,
		false,
		true);
	if (!ComboInputTask)
	{
		FinishAbility(true);
		return;
	}
	ComboInputTask->EventReceived.AddDynamic(this, &UUmbraBasicAttackAbility::HandleComboInput);
	ComboInputTask->ReadyForActivation();

	AttackHitWindowTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		UmbraGameplayTags::Event_Attack_HitWindow,
		nullptr,
		false,
		false);
	if (!AttackHitWindowTask)
	{
		FinishAbility(true);
		return;
	}
	AttackHitWindowTask->EventReceived.AddDynamic(this, &UUmbraBasicAttackAbility::HandleAttackHitWindow);
	AttackHitWindowTask->ReadyForActivation();

	if (!PlayCurrentAttackMontage())
	{
		FinishAbility(true);
	}
}

bool UUmbraBasicAttackAbility::PlayCurrentAttackMontage()
{
	if (!AttackMontages.IsValidIndex(CurrentComboIndex) || !AttackMontages[CurrentComboIndex])
	{
		return false;
	}

	FacePrimaryAttackTarget();
	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		AttackMontages[CurrentComboIndex],
		1.0f,
		NAME_None,
		true);
	if (!ActiveMontageTask)
	{
		return false;
	}

	ActiveMontageTask->OnCompleted.AddDynamic(this, &UUmbraBasicAttackAbility::HandleMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UUmbraBasicAttackAbility::HandleMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UUmbraBasicAttackAbility::HandleMontageCancelled);
	ActiveMontageTask->ReadyForActivation();
	HitActorsThisComboStep.Reset();
	bHasPreviousHitSocketLocation = false;
	return true;
}

bool UUmbraBasicAttackAbility::StartNextComboStep()
{
	if (!AttackMontages.IsValidIndex(CurrentComboIndex + 1))
	{
		return false;
	}

	if (ComboWindowTask)
	{
		ComboWindowTask->EndTask();
		ComboWindowTask = nullptr;
	}

	bComboInputQueued = false;
	++CurrentComboIndex;
	return PlayCurrentAttackMontage();
}

bool UUmbraBasicAttackAbility::StartComboGraceWindow()
{
	if (ComboWindowDuration <= 0.0f || !AttackMontages.IsValidIndex(CurrentComboIndex + 1))
	{
		return false;
	}

	ComboWindowTask = UAbilityTask_WaitDelay::WaitDelay(this, ComboWindowDuration);
	if (!ComboWindowTask)
	{
		return false;
	}

	ComboWindowTask->OnFinish.AddDynamic(this, &UUmbraBasicAttackAbility::HandleComboWindowExpired);
	ComboWindowTask->ReadyForActivation();
	return true;
}

void UUmbraBasicAttackAbility::FacePrimaryAttackTarget()
{
	AUmbraPlayerCharacter* Character = Cast<AUmbraPlayerCharacter>(GetAvatarActorFromActorInfo());
	const AActor* TargetActor = Character ? Character->GetPrimaryAttackTarget() : nullptr;
	if (!Character || !TargetActor)
	{
		return;
	}

	FVector FacingDirection = TargetActor->GetActorLocation() - Character->GetActorLocation();
	FacingDirection.Z = 0.0f;
	if (!FacingDirection.IsNearlyZero())
	{
		Character->SetActorRotation(FRotator(0.0f, FacingDirection.Rotation().Yaw, 0.0f));
	}
}

void UUmbraBasicAttackAbility::FinishAbility(bool bWasCancelled)
{
	CurrentComboIndex = INDEX_NONE;
	bComboInputQueued = false;
	HitActorsThisComboStep.Reset();
	bHasPreviousHitSocketLocation = false;
	ActiveMontageTask = nullptr;
	ComboInputTask = nullptr;
	AttackHitWindowTask = nullptr;
	ComboWindowTask = nullptr;

	if (AUmbraPlayerCharacter* Character = Cast<AUmbraPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		Character->ClearPrimaryAttackTarget();
	}

	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
	}
}

void UUmbraBasicAttackAbility::HandleMontageCompleted()
{
	ActiveMontageTask = nullptr;
	if (bComboInputQueued)
	{
		if (StartNextComboStep())
		{
			return;
		}
	}
	else if (StartComboGraceWindow())
	{
		return;
	}

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

void UUmbraBasicAttackAbility::HandleComboInput(FGameplayEventData Payload)
{
	(void)Payload;
	if (IsActive() && AttackMontages.IsValidIndex(CurrentComboIndex + 1))
	{
		if (!ActiveMontageTask && ComboWindowTask)
		{
			if (!StartNextComboStep())
			{
				FinishAbility(true);
			}
			return;
		}

		bComboInputQueued = true;
	}
}

void UUmbraBasicAttackAbility::HandleComboWindowExpired()
{
	ComboWindowTask = nullptr;
	FinishAbility(false);
}

void UUmbraBasicAttackAbility::HandleAttackHitWindow(FGameplayEventData Payload)
{
	AUmbraPlayerCharacter* Character = Cast<AUmbraPlayerCharacter>(GetAvatarActorFromActorInfo());
	AActor* TargetActor = Character ? Character->GetPrimaryAttackTarget() : nullptr;
	USkeletalMeshComponent* CharacterMesh = Character ? Character->GetMesh() : nullptr;
	if (!IsValid(TargetActor)
		|| TargetActor->IsActorBeingDestroyed()
		|| HitActorsThisComboStep.Contains(TargetActor)
		|| !CharacterMesh)
	{
		return;
	}

	if (Payload.EventTag == UmbraGameplayTags::Event_Attack_HitWindowEnd)
	{
		bHasPreviousHitSocketLocation = false;
		return;
	}
	if (HitDetectionSocketName.IsNone() || !CharacterMesh->DoesSocketExist(HitDetectionSocketName))
	{
		UE_LOG(LogUmbra, Warning,
			TEXT("Cannot detect a basic-attack hit for %s: socket '%s' does not exist on mesh %s."),
			*GetNameSafe(Character),
			*HitDetectionSocketName.ToString(),
			*GetNameSafe(CharacterMesh));
		return;
	}

	const FVector CurrentSocketLocation = CharacterMesh->GetSocketLocation(HitDetectionSocketName);
	if (Payload.EventTag == UmbraGameplayTags::Event_Attack_HitWindowBegin)
	{
		PreviousHitSocketLocation = CurrentSocketLocation;
		bHasPreviousHitSocketLocation = true;
		return;
	}
	if (Payload.EventTag != UmbraGameplayTags::Event_Attack_HitWindowTick)
	{
		return;
	}

	const FVector SweepStart = bHasPreviousHitSocketLocation ? PreviousHitSocketLocation : CurrentSocketLocation;
	PreviousHitSocketLocation = CurrentSocketLocation;
	bHasPreviousHitSocketLocation = true;

	TArray<FHitResult> Hits;
	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UmbraBasicAttackHit), false, Character);
	const bool bFoundPawn = Character->GetWorld()->SweepMultiByObjectType(
		Hits,
		SweepStart,
		CurrentSocketLocation,
		FQuat::Identity,
		ObjectQuery,
		FCollisionShape::MakeSphere(HitDetectionRadius),
		QueryParams);
	if (!bFoundPawn || !Hits.ContainsByPredicate([TargetActor](const FHitResult& Hit)
		{
			return Hit.GetActor() == TargetActor;
		}))
	{
		return;
	}

	HitActorsThisComboStep.Add(TargetActor);
	FGameplayEventData HitReactEvent;
	HitReactEvent.EventTag = UmbraGameplayTags::Event_Combat_HitReceived;
	HitReactEvent.Instigator = Character;
	HitReactEvent.Target = TargetActor;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		TargetActor,
		UmbraGameplayTags::Event_Combat_HitReceived,
		HitReactEvent);
}
