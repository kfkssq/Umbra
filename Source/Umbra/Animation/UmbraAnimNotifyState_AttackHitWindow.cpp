// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/UmbraAnimNotifyState_AttackHitWindow.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayTags/UmbraGameplayTags.h"

void UUmbraAnimNotifyState_AttackHitWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	SendHitWindowEvent(MeshComp, UmbraGameplayTags::Event_Attack_HitWindowBegin);
}

void UUmbraAnimNotifyState_AttackHitWindow::NotifyTick(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	SendHitWindowEvent(MeshComp, UmbraGameplayTags::Event_Attack_HitWindowTick);
}

void UUmbraAnimNotifyState_AttackHitWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	SendHitWindowEvent(MeshComp, UmbraGameplayTags::Event_Attack_HitWindowEnd);
}

void UUmbraAnimNotifyState_AttackHitWindow::SendHitWindowEvent(
	USkeletalMeshComponent* MeshComp,
	const FGameplayTag& EventTag)
{
	AActor* OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!IsValid(OwnerActor) || OwnerActor->IsActorBeingDestroyed())
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.EventTag = EventTag;
	EventData.Instigator = OwnerActor;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, EventTag, EventData);
}
