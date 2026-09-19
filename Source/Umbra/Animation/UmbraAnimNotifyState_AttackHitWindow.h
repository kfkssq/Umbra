// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "CoreMinimal.h"
#include "UmbraAnimNotifyState_AttackHitWindow.generated.h"

struct FGameplayTag;

/** Defines the active melee hit window authored on an attack montage. */
UCLASS(const, DisplayName = "Umbra Attack Hit Window")
class UMBRA_API UUmbraAnimNotifyState_AttackHitWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

private:
	static void SendHitWindowEvent(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FGameplayTag& EventTag);
};
