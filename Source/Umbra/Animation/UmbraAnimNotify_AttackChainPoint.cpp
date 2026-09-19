// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/UmbraAnimNotify_AttackChainPoint.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayTags/UmbraGameplayTags.h"

void UUmbraAnimNotify_AttackChainPoint::Notify(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	AActor* OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!IsValid(OwnerActor) || OwnerActor->IsActorBeingDestroyed())
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.EventTag = UmbraGameplayTags::Event_Attack_ChainPoint;
	EventData.Instigator = OwnerActor;
	EventData.OptionalObject = Animation;
	if (const UUmbraAbilitySystemComponent* ASC = Cast<UUmbraAbilitySystemComponent>(
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor)))
	{
		EventData.EventMagnitude = static_cast<float>(ASC->GetActivePrimaryAttackInstanceId());
	}
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, EventData.EventTag, EventData);
}
