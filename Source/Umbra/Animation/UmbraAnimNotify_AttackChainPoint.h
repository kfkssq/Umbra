// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "CoreMinimal.h"
#include "UmbraAnimNotify_AttackChainPoint.generated.h"

/** Marks the earliest safe point at which a basic attack may transition to the next strike. */
UCLASS(const, DisplayName = "Umbra Attack Chain Point")
class UMBRA_API UUmbraAnimNotify_AttackChainPoint : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
