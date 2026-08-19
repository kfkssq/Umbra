// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "UmbraGameplayAbility.generated.h"

UENUM(BlueprintType)
enum class EUmbraAbilityActivationPolicy : uint8
{
	OnInputTriggered,
	WhileInputActive
};

/** Base class for Umbra gameplay abilities and their input association. */
UCLASS(Abstract, Blueprintable)
class UMBRA_API UUmbraGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UUmbraGameplayAbility();

	FGameplayTag GetInputTag() const { return InputTag; }
	EUmbraAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }

protected:
	/** Input tag that will later be used to route Enhanced Input to this ability. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Input")
	FGameplayTag InputTag;

	/** Determines how the ASC attempts activation while this ability's input is active. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Input")
	EUmbraAbilityActivationPolicy ActivationPolicy = EUmbraAbilityActivationPolicy::OnInputTriggered;
};
