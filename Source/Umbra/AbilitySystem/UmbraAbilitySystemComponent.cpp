// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/UmbraAbilitySystemComponent.h"

#include "AbilitySystem/UmbraGameplayAbility.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "GameplayEffect.h"
#include "AbilitySystem/UmbraDebugInitialAttributes.h"
#include "Characters/UmbraPlayerCharacter.h"

void UUmbraAbilitySystemComponent::ServerReceivePrimaryAttackIntent_Implementation(AActor* Target, bool bCombo)
{
	if (AUmbraPlayerCharacter* Character = Cast<AUmbraPlayerCharacter>(GetAvatarActor()))
	{
		Character->ReceivePrimaryAttackIntent(Target, bCombo);
	}
}

FUmbraAbilitySystemLifecycle UUmbraAbilitySystemComponent::OnLifecycleChanged;

void UUmbraAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);
	bActorInfoReady = IsValid(InOwnerActor) && IsValid(InAvatarActor);
	OnLifecycleChanged.Broadcast(this, bActorInfoReady);
}

void UUmbraAbilitySystemComponent::ClearActorInfo()
{
	bActorInfoReady = false;
	Super::ClearActorInfo();
	OnLifecycleChanged.Broadcast(this, false);
}

void UUmbraAbilitySystemComponent::OnUnregister()
{
	bActorInfoReady = false;
	OnLifecycleChanged.Broadcast(this, false);
	Super::OnUnregister();
}

void UUmbraAbilitySystemComponent::InitializeAttributes(TSubclassOf<UGameplayEffect> InitialEffect, const FUmbraDebugInitialAttributes* DebugAttributes)
{
	if (bAttributesInitialized || !IsOwnerActorAuthoritative() || !GetSet<UUmbraAttributeSet>())
	{
		return;
	}
	if (InitialEffect && InitialEffect.GetDefaultObject()->DurationPolicy != EGameplayEffectDurationType::Instant)
	{
		UE_LOG(LogTemp, Error, TEXT("Umbra initial attributes must use an Instant Gameplay Effect: %s"), *GetNameSafe(InitialEffect));
		return;
	}
	bAttributesInitialized = true;
	if (InitialEffect)
	{
		const FGameplayEffectSpecHandle Spec = MakeOutgoingSpec(InitialEffect, 1.f, MakeEffectContext());
		if (Spec.IsValid())
		{
			ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
#if !UE_BUILD_SHIPPING
	if (DebugAttributes)
	{
		UGameplayEffect* DebugEffect = NewObject<UGameplayEffect>(GetTransientPackage());
		DebugEffect->DurationPolicy = EGameplayEffectDurationType::Instant;
		const auto AddOverride = [DebugEffect](const FGameplayAttribute& Attribute, float Value)
		{
			FGameplayModifierInfo& Modifier = DebugEffect->Modifiers.AddDefaulted_GetRef();
			Modifier.Attribute = Attribute;
			Modifier.ModifierOp = EGameplayModOp::Override;
			Modifier.ModifierMagnitude = FScalableFloat(Value);
		};
		AddOverride(UUmbraAttributeSet::GetMaxHealthAttribute(), DebugAttributes->MaxHealth);
		AddOverride(UUmbraAttributeSet::GetMaxResourceAttribute(), DebugAttributes->MaxResource);
		AddOverride(UUmbraAttributeSet::GetHealthRegenAttribute(), DebugAttributes->HealthRegen);
		AddOverride(UUmbraAttributeSet::GetResourceRegenAttribute(), DebugAttributes->ResourceRegen);
		AddOverride(UUmbraAttributeSet::GetAttackPowerAttribute(), DebugAttributes->AttackPower);
		AddOverride(UUmbraAttributeSet::GetAbilityPowerAttribute(), DebugAttributes->AbilityPower);
		AddOverride(UUmbraAttributeSet::GetAttackSpeedBonusAttribute(), DebugAttributes->AttackSpeedBonus);
		AddOverride(UUmbraAttributeSet::GetCriticalChanceAttribute(), DebugAttributes->CriticalChance);
		AddOverride(UUmbraAttributeSet::GetCriticalDamageMultiplierAttribute(), DebugAttributes->CriticalDamageMultiplier);
		AddOverride(UUmbraAttributeSet::GetArmorAttribute(), DebugAttributes->Armor);
		AddOverride(UUmbraAttributeSet::GetMagicResistanceAttribute(), DebugAttributes->MagicResistance);
		AddOverride(UUmbraAttributeSet::GetAbilityHasteAttribute(), DebugAttributes->AbilityHaste);
		AddOverride(UUmbraAttributeSet::GetMoveSpeedAttribute(), DebugAttributes->MoveSpeed);
		ApplyGameplayEffectToSelf(DebugEffect, 1.f, MakeEffectContext());
	}
#endif
	// Fill separately, after all initial maxima have been evaluated.
	const UUmbraAttributeSet* Attributes = GetSet<UUmbraAttributeSet>();
	SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(), Attributes->GetMaxHealth());
	SetNumericAttributeBase(UUmbraAttributeSet::GetResourceAttribute(), Attributes->GetMaxResource());
}

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
