// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UmbraPlayerState.h"

#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/UmbraGameplayAbility.h"

AUmbraPlayerState::AUmbraPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UUmbraAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UUmbraAttributeSet>(TEXT("AttributeSet"));

	SetNetUpdateFrequency(100.0f);
}

UAbilitySystemComponent* AUmbraPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AUmbraPlayerState::GrantInitialAbilities()
{
	if (!HasAuthority() || bInitialAbilitiesGranted || !AbilitySystemComponent)
	{
		return;
	}

	for (const TSubclassOf<UUmbraGameplayAbility>& AbilityClass : InitialAbilities)
	{
		if (AbilityClass)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
		}
	}

	bInitialAbilitiesGranted = true;
}
