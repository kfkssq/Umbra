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

	/** Updates presentation for local cursor hover or a selected automatic-attack target. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void SetAttackHighlighted(bool bHighlighted);
};
