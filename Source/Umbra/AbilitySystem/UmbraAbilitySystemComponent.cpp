// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/UmbraAbilitySystemComponent.h"

#include "AbilitySystem/UmbraGameplayAbility.h"

UUmbraAbilitySystemComponent::UUmbraAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
	SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

void UUmbraAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		const UUmbraGameplayAbility* UmbraAbility = Cast<UUmbraGameplayAbility>(AbilitySpec.Ability);
		if (UmbraAbility && UmbraAbility->GetInputTag().MatchesTagExact(InputTag))
		{
			InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
			InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
		}
	}
}

void UUmbraAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		const UUmbraGameplayAbility* UmbraAbility = Cast<UUmbraGameplayAbility>(AbilitySpec.Ability);
		if (UmbraAbility && UmbraAbility->GetInputTag().MatchesTagExact(InputTag))
		{
			InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
			InputHeldSpecHandles.Remove(AbilitySpec.Handle);
		}
	}
}

void UUmbraAbilitySystemComponent::ProcessAbilityInput(float DeltaTime)
{
	TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;

	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);
		const UUmbraGameplayAbility* UmbraAbility = AbilitySpec ? Cast<UUmbraGameplayAbility>(AbilitySpec->Ability) : nullptr;
		if (UmbraAbility
			&& !AbilitySpec->IsActive()
			&& UmbraAbility->GetActivationPolicy() == EUmbraAbilityActivationPolicy::WhileInputActive)
		{
			AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
		}
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);
		if (!AbilitySpec)
		{
			continue;
		}

		AbilitySpec->InputPressed = true;
		if (AbilitySpec->IsActive())
		{
			AbilitySpecInputPressed(*AbilitySpec);
		}
		else if (const UUmbraGameplayAbility* UmbraAbility = Cast<UUmbraGameplayAbility>(AbilitySpec->Ability))
		{
			if (UmbraAbility->GetActivationPolicy() == EUmbraAbilityActivationPolicy::OnInputTriggered)
			{
				AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
			}
		}
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(SpecHandle);
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);
		if (!AbilitySpec)
		{
			continue;
		}

		AbilitySpec->InputPressed = false;
		if (AbilitySpec->IsActive())
		{
			AbilitySpecInputReleased(*AbilitySpec);
		}
	}

	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UUmbraAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}
