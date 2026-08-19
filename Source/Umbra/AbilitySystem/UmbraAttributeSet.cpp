// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/UmbraAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UUmbraAttributeSet::UUmbraAttributeSet()
{
	InitMaxHealth(100.0f);
	InitHealth(100.0f);
	InitMaxMana(100.0f);
	InitMana(100.0f);
}

void UUmbraAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetMaxHealthAttribute() || Attribute == GetMaxManaAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMana());
	}
}

void UUmbraAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	ClampResourceAttributes();
}

void UUmbraAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxHealthAttribute() && GetHealth() > NewValue)
	{
		SetHealth(FMath::Max(NewValue, 0.0f));
	}
	else if (Attribute == GetMaxManaAttribute() && GetMana() > NewValue)
	{
		SetMana(FMath::Max(NewValue, 0.0f));
	}
}

void UUmbraAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UUmbraAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUmbraAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUmbraAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUmbraAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
}

void UUmbraAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUmbraAttributeSet, Health, OldHealth);
}

void UUmbraAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUmbraAttributeSet, MaxHealth, OldMaxHealth);
}

void UUmbraAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUmbraAttributeSet, Mana, OldMana);
}

void UUmbraAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUmbraAttributeSet, MaxMana, OldMaxMana);
}

void UUmbraAttributeSet::ClampResourceAttributes()
{
	SetMaxHealth(FMath::Max(GetMaxHealth(), 0.0f));
	SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	SetMaxMana(FMath::Max(GetMaxMana(), 0.0f));
	SetMana(FMath::Clamp(GetMana(), 0.0f, GetMaxMana()));
}
