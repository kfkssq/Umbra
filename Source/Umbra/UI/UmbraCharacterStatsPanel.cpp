#include "UI/UmbraCharacterStatsPanel.h"

#include "UI/Combat/UmbraCombatStatData.h"
#include "Stats/UmbraDerivedStatsComponent.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/PlayerController.h"
#include "Player/UmbraPlayerState.h"
#include "Umbra.h"

namespace
{
	EUmbraCombatStat CombatStatFor(EUmbraCharacterStat Stat)
	{
		switch (Stat)
		{
		case EUmbraCharacterStat::AttackPower: return EUmbraCombatStat::AttackPower;
		case EUmbraCharacterStat::AbilityPower: return EUmbraCombatStat::AbilityPower;
		case EUmbraCharacterStat::Armor: return EUmbraCombatStat::Armor;
		case EUmbraCharacterStat::MagicResistance: return EUmbraCombatStat::MagicResist;
		case EUmbraCharacterStat::AttackSpeed: return EUmbraCombatStat::AttackSpeed;
		case EUmbraCharacterStat::CriticalChance: return EUmbraCombatStat::CriticalChance;
		case EUmbraCharacterStat::MoveSpeed: return EUmbraCombatStat::MoveSpeed;
		case EUmbraCharacterStat::AbilityHaste: return EUmbraCombatStat::AbilityHaste;
		default: return EUmbraCombatStat::None;
		}
	}

	FGameplayAttribute AttributeFor(EUmbraCharacterStat Stat)
	{
		const auto CombatStat = CombatStatFor(Stat);
		if (CombatStat != EUmbraCombatStat::None) return UmbraCombatStats::AttributeFor(CombatStat);
		switch (Stat)
		{
		case EUmbraCharacterStat::Strength: return UUmbraAttributeSet::GetStrengthAttribute();
		case EUmbraCharacterStat::Dexterity: return UUmbraAttributeSet::GetDexterityAttribute();
		case EUmbraCharacterStat::Intelligence: return UUmbraAttributeSet::GetIntelligenceAttribute();
		case EUmbraCharacterStat::Faith: return UUmbraAttributeSet::GetFaithAttribute();
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
		BoundDerived = PlayerState->FindComponentByClass<UUmbraDerivedStatsComponent>();
		if (BoundDerived.IsValid()) BoundDerived->OnDerivedStatsChanged.AddUniqueDynamic(this, &ThisClass::OnDerived);
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

void UUmbraCharacterStatsPanel::OnDerived(const FUmbraDerivedStatsSnapshot&)
{
	RefreshStat(EUmbraCharacterStat::AttackPower);
	RefreshStat(EUmbraCharacterStat::AbilityPower);
}

void UUmbraCharacterStatsPanel::UnbindASC()
{
	if (BoundDerived.IsValid()) BoundDerived->OnDerivedStatsChanged.RemoveDynamic(this, &ThisClass::OnDerived);
	BoundDerived.Reset();
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
	const auto CombatStat = CombatStatFor(Stat);
	if (CombatStat != EUmbraCombatStat::None)
	{
		ApplyDisplay(UmbraCombatStats::Read(BoundASC.IsValid() ? BoundPlayerState.Get() : nullptr, CombatStat).Text);
		return;
	}
	if (!TryReadStat(Stat, Value) || !FMath::IsFinite(Value))
	{
		ApplyDisplay(FText::FromString(TEXT("—")));
		return;
	}
	FNumberFormattingOptions Options;
	Options.SetMinimumFractionalDigits(0);
	Options.SetMaximumFractionalDigits(0);
	ApplyDisplay(FText::AsNumber(double(Value), &Options));
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
	default: return false;
	}
}
