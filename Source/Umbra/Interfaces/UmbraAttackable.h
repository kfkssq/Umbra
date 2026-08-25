// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UmbraAttackable.generated.h"

UINTERFACE(BlueprintType)
class UMBRA_API UUmbraAttackable : public UInterface
{
	GENERATED_BODY()
};

/** Implemented by actors that may be selected as a basic-attack target. */
class UMBRA_API IUmbraAttackable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	bool CanBeAttacked() const;

	/** Updates presentation when the local player's cursor enters or leaves this target. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void SetAttackHighlighted(bool bHighlighted);
};
