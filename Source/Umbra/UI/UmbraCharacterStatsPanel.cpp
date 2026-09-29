#include "UI/UmbraCharacterStatsPanel.h"

#include "AbilitySystem/Abilities/UmbraBasicAttackAbility.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/PlayerController.h"
#include "Player/UmbraPlayerState.h"
#include "Umbra.h"

namespace
{
	FGameplayAttribute AttributeFor(EUmbraCharacterStat Stat)
	{
		switch (Stat)
		{
		case EUmbraCharacterStat::Strength: return UUmbraAttributeSet::GetStrengthAttribute();
		case EUmbraCharacterStat::Dexterity: return UUmbraAttributeSet::GetDexterityAttribute();
		case EUmbraCharacterStat::Intelligence: return UUmbraAttributeSet::GetIntelligenceAttribute();
		case EUmbraCharacterStat::Faith: return UUmbraAttributeSet::GetFaithAttribute();
		case EUmbraCharacterStat::Armor: return UUmbraAttributeSet::GetArmorAttribute();
		case EUmbraCharacterStat::MagicResistance: return UUmbraAttributeSet::GetMagicResistanceAttribute();
		case EUmbraCharacterStat::AttackSpeed: return UUmbraAttributeSet::GetAttackSpeedAttribute();
		case EUmbraCharacterStat::CriticalChance: return UUmbraAttributeSet::GetCriticalChanceAttribute();
		case EUmbraCharacterStat::MoveSpeed: return UUmbraAttributeSet::GetMoveSpeedAttribute();
		default: return FGameplayAttribute();
		}
	}
}

UUmbraCharacterStatsPanel::UUmbraCharacterStatsPanel(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	StatDisplayData.FindOrAdd(EUmbraCharacterStat::Strength).DisplayName = NSLOCTEXT("UmbraStats", "Strength", "力量");
	StatDisplayData.FindOrAdd(EUmbraCharacterStat::Dexterity).DisplayName = NSLOCTEXT("UmbraStats", "Dexterity", "敏捷");
	StatDisplayData.FindOrAdd(EUmbraCharacterStat::Intelligence).DisplayName = NSLOCTEXT("UmbraStats", "Intelligence", "智力");
	StatDisplayData.FindOrAdd(EUmbraCharacterStat::Faith).DisplayName = NSLOCTEXT("UmbraStats", "Faith", "信仰");
}

void UUmbraCharacterStatsPanel::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (IsDesignTime()) RebuildEntries();
}

void UUmbraCharacterStatsPanel::RebuildEntries()
{
	UnbindASC();
	Entries.Reset();
	if (WidgetTree)
	{
		TArray<UWidget*> Widgets;
		WidgetTree->GetAllWidgets(Widgets);
		for (UWidget* Widget : Widgets)
		{
			if (UUmbraStatEntry* Entry = Cast<UUmbraStatEntry>(Widget))
			{
				RegisterEntry(Entry);
			}
		}
	}
}

void UUmbraCharacterStatsPanel::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(false);
	RebuildEntries();
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &ThisClass::OnPawnChanged);
	}
	if (!LifecycleHandle.IsValid())
	{
		LifecycleHandle = UUmbraAbilitySystemComponent::OnLifecycleChanged.AddUObject(this, &ThisClass::OnLifecycle);
	}
	BindPlayer();
}

void UUmbraCharacterStatsPanel::NativeDestruct()
{
	Shutdown();
	Super::NativeDestruct();
}

void UUmbraCharacterStatsPanel::Shutdown()
{
	UnbindASC();
	UUmbraAbilitySystemComponent::OnLifecycleChanged.Remove(LifecycleHandle);
	LifecycleHandle.Reset();
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::OnPawnChanged);
	}
	BoundPlayerState.Reset();
	Entries.Reset();
}

void UUmbraCharacterStatsPanel::RegisterEntry(UUmbraStatEntry* Entry)
{
	if (!IsValid(Entry))
	{
		return;
	}
	if (const TWeakObjectPtr<UUmbraStatEntry>* Existing = Entries.Find(Entry->GetStat()); Existing && Existing->IsValid() && Existing->Get() != Entry)
	{
		UE_LOG(LogUmbra, Warning, TEXT("Character stats panel %s: %s and %s both use Stat=%s. Set unique Stat values on Designer instances."),
			*GetNameSafe(this), *GetNameSafe(Existing->Get()), *GetNameSafe(Entry),
			*UEnum::GetValueAsString(Entry->GetStat()));
	}
	Entries.Add(Entry->GetStat(), Entry);
	BindStat(Entry->GetStat());
	RefreshStat(Entry->GetStat());
}

void UUmbraCharacterStatsPanel::NotifyPlayerContextChanged()
{
	BindPlayer();
}

void UUmbraCharacterStatsPanel::BindPlayer()
{
	AUmbraPlayerState* PlayerState = GetOwningPlayerState<AUmbraPlayerState>();
	if (BoundPlayerState.Get() != PlayerState)
	{
		UnbindASC();
		BoundPlayerState = PlayerState;
	}
	UUmbraAbilitySystemComponent* ASC = PlayerState ? PlayerState->GetUmbraAbilitySystemComponent() : nullptr;
	if (!IsValid(ASC) || !ASC->IsActorInfoReady() || !ASC->GetSet<UUmbraAttributeSet>())
	{
		UnbindASC();
		RefreshAll();
		return;
	}
	if (BoundASC.Get() != ASC)
	{
		UnbindASC();
		BoundASC = ASC;
		for (const auto& Entry : Entries)
		{
			BindStat(Entry.Key);
		}
	}
	RefreshAll();
}

void UUmbraCharacterStatsPanel::BindStat(EUmbraCharacterStat Stat)
{
	const FGameplayAttribute Attribute = AttributeFor(Stat);
	if (UUmbraAbilitySystemComponent* ASC = BoundASC.Get(); ASC && Attribute.IsValid() && !AttributeHandles.Contains(Attribute))
	{
		AttributeHandles.Add(Attribute, ASC->GetGameplayAttributeValueChangeDelegate(Attribute)
			.AddUObject(this, &ThisClass::OnAttributeChanged));
	}
}

void UUmbraCharacterStatsPanel::UnbindASC()
{
	if (UUmbraAbilitySystemComponent* ASC = BoundASC.Get())
	{
		for (const auto& Pair : AttributeHandles)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Pair.Key).Remove(Pair.Value);
		}
	}
	AttributeHandles.Reset();
	BoundASC.Reset();
}

void UUmbraCharacterStatsPanel::OnAttributeChanged(const FOnAttributeChangeData& Data)
{
	for (const auto& Entry : Entries)
	{
		if (AttributeFor(Entry.Key) == Data.Attribute)
		{
			RefreshStat(Entry.Key);
			return;
		}
	}
}

void UUmbraCharacterStatsPanel::OnLifecycle(UUmbraAbilitySystemComponent* ASC, bool bReady)
{
	if (!bReady && BoundASC.Get() == ASC)
	{
		UnbindASC();
		RefreshAll();
		return;
	}
	const AUmbraPlayerState* PlayerState = GetOwningPlayerState<AUmbraPlayerState>();
	if (!PlayerState || ASC != PlayerState->GetUmbraAbilitySystemComponent())
	{
		return;
	}
	if (bReady)
	{
		BindPlayer();
	}
}

void UUmbraCharacterStatsPanel::OnPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	BindPlayer();
}

void UUmbraCharacterStatsPanel::RefreshAll()
{
	for (const auto& Entry : Entries)
	{
		RefreshStat(Entry.Key);
	}
}

void UUmbraCharacterStatsPanel::RefreshStat(EUmbraCharacterStat Stat)
{
	const TWeakObjectPtr<UUmbraStatEntry>* Entry = Entries.Find(Stat);
	if (!Entry || !Entry->IsValid())
	{
		return;
	}
	float Value = 0.f;
	const auto ApplyDisplay = [this, Entry, Stat](const FText& Text)
	{
		if (const FUmbraStatDisplayData* Display = StatDisplayData.Find(Stat))
			Entry->Get()->SetStatDisplay(Display->Icon, Display->DisplayName, Text);
		else Entry->Get()->SetDisplayValue(Text);
	};
	if (!TryReadStat(Stat, Value) || !FMath::IsFinite(Value))
	{
		ApplyDisplay(FText::FromString(TEXT("—")));
		return;
	}
	if (Stat == EUmbraCharacterStat::AttackSpeed)
	{
		FNumberFormattingOptions Options;
		Options.SetMinimumFractionalDigits(2);
		Options.SetMaximumFractionalDigits(2);
		ApplyDisplay(FText::AsNumber(Value, &Options));
	}
	else if (Stat == EUmbraCharacterStat::CriticalChance)
	{
		ApplyDisplay(FText::Format(NSLOCTEXT("UmbraStats", "Percent", "{0}%"),
			FText::AsNumber(FMath::RoundToInt(Value * 100.f))));
	}
	else
	{
		FNumberFormattingOptions Options;
		Options.SetMinimumFractionalDigits(0);
		Options.SetMaximumFractionalDigits(0);
		ApplyDisplay(FText::AsNumber(double(Value), &Options));
	}
}

bool UUmbraCharacterStatsPanel::TryReadStat(EUmbraCharacterStat Stat, float& OutValue) const
{
	const UUmbraAbilitySystemComponent* ASC = BoundASC.Get();
	const UUmbraAttributeSet* Attributes = ASC ? ASC->GetSet<UUmbraAttributeSet>() : nullptr;
	if (!Attributes || !ASC->IsActorInfoReady())
	{
		return false;
	}
	switch (Stat)
	{
	case EUmbraCharacterStat::Strength: OutValue = Attributes->GetStrength(); return true;
	case EUmbraCharacterStat::Dexterity: OutValue = Attributes->GetDexterity(); return true;
	case EUmbraCharacterStat::Intelligence: OutValue = Attributes->GetIntelligence(); return true;
	case EUmbraCharacterStat::Faith: OutValue = Attributes->GetFaith(); return true;
	case EUmbraCharacterStat::Armor: OutValue = Attributes->GetArmor(); return true;
	case EUmbraCharacterStat::MagicResistance: OutValue = Attributes->GetMagicResistance(); return true;
	case EUmbraCharacterStat::CriticalChance: OutValue = Attributes->GetCriticalChance(); return true;
	case EUmbraCharacterStat::MoveSpeed: OutValue = Attributes->GetMoveSpeed(); return true;
	case EUmbraCharacterStat::AttackSpeed:
	{
		const AUmbraPlayerState* PlayerState = BoundPlayerState.Get();
		const UUmbraBasicAttackAbility* Attack = PlayerState ? PlayerState->GetPrimaryAttackAbilityDefaults() : nullptr;
		const float BasePeriod = Attack ? Attack->GetConfiguredBaseAttackInterval() : 0.f;
		if (BasePeriod <= 0.f)
		{
			return false;
		}
		const float Speed = FMath::Clamp(Attributes->GetAttackSpeed(),
			UUmbraAttributeSet::MinAttackSpeedMultiplier, UUmbraAttributeSet::MaxAttackSpeedMultiplier);
		OutValue = Speed / BasePeriod;
		return true;
	}
	case EUmbraCharacterStat::AttackPower:
	case EUmbraCharacterStat::AbilityPower:
		// Combat does not expose separate final physical/magical weapon damage values yet.
		// Wire a combat-owned derived-value provider here; do not mirror execution math in UI.
		return false;
	case EUmbraCharacterStat::AbilityHaste:
		// AbilityHaste is stored but no effective cooldown/haste rule exists yet.
		return false;
	}
	return false;
}
