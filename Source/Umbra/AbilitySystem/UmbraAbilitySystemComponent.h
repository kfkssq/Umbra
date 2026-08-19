// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "UmbraAbilitySystemComponent.generated.h"

/** Ability system component shared by Umbra player ability owners. */
UCLASS()
class UMBRA_API UUmbraAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UUmbraAbilitySystemComponent();

	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);
	void ProcessAbilityInput(float DeltaTime);
	void ClearAbilityInput();

private:
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
};
