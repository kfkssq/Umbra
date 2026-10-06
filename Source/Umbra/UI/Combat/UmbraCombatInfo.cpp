#include "UI/Combat/UmbraCombatInfo.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "UI/Combat/UmbraCombatStatEntry.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Player/UmbraPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "GameplayEffect.h"

void UUmbraCombatInfo::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshSections();
}

void UUmbraCombatInfo::NativeConstruct()
{
	Super::NativeConstruct();
	if (AttackToggle) AttackToggle->OnClicked.AddUniqueDynamic(this, &ThisClass::ToggleAttack);
	if (DefenseToggle) DefenseToggle->OnClicked.AddUniqueDynamic(this, &ThisClass::ToggleDefense);
	if (UtilityToggle) UtilityToggle->OnClicked.AddUniqueDynamic(this, &ThisClass::ToggleUtility);
	RefreshSections();
	Shutdown();
	bObserving = true;
	// Descend through layout wrappers, but a row remains a pure view with its own widget tree.
	TFunction<void(UUserWidget*)> Discover = [&](UUserWidget* Root)
	{
		if (!Root || !Root->WidgetTree) return;
		TArray<UWidget*> Widgets;
		Root->WidgetTree->GetAllWidgets(Widgets);
		for (auto* Widget : Widgets)
		{
			if (auto* Entry = Cast<UUmbraCombatStatEntry>(Widget)) RegisterCombatEntry(Entry);
			else if (auto* Child = Cast<UUserWidget>(Widget)) Discover(Child);
		}
	};
	Discover(this);
	if (auto* PC = GetOwningPlayer()) PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &ThisClass::OnPawnChanged);
	LifecycleHandle = UUmbraAbilitySystemComponent::OnLifecycleChanged.AddUObject(this, &ThisClass::OnLifecycle);
	BindPlayer();
}

void UUmbraCombatInfo::NativeDestruct()
{
	if (AttackToggle) AttackToggle->OnClicked.RemoveDynamic(this, &ThisClass::ToggleAttack);
	if (DefenseToggle) DefenseToggle->OnClicked.RemoveDynamic(this, &ThisClass::ToggleDefense);
	if (UtilityToggle) UtilityToggle->OnClicked.RemoveDynamic(this, &ThisClass::ToggleUtility);
	Shutdown();
	Super::NativeDestruct();
}

void UUmbraCombatInfo::ToggleAttack()
{
	bAttackExpanded = !bAttackExpanded;
	RefreshSections();
}

void UUmbraCombatInfo::ToggleDefense()
{
	bDefenseExpanded = !bDefenseExpanded;
	RefreshSections();
}

void UUmbraCombatInfo::RefreshSections()
{
	if (AttackRows) AttackRows->SetVisibility(bAttackExpanded ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (DefenseRows) DefenseRows->SetVisibility(bDefenseExpanded ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (AttackArrow) AttackArrow->SetText(FText::FromString(bAttackExpanded ? TEXT("\u25bc") : TEXT("\u25b6")));
	if (DefenseArrow) DefenseArrow->SetText(FText::FromString(bDefenseExpanded ? TEXT("\u25bc") : TEXT("\u25b6")));
	if (UtilityRows) UtilityRows->SetVisibility(bUtilityExpanded ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (UtilityArrow) UtilityArrow->SetText(FText::FromString(bUtilityExpanded ? TEXT("\u25bc") : TEXT("\u25b6")));
}

void UUmbraCombatInfo::ToggleUtility() { bUtilityExpanded = !bUtilityExpanded; RefreshSections(); }

void UUmbraCombatInfo::RegisterCombatEntry(UUmbraCombatStatEntry* Entry)
{
	if (!IsValid(Entry)) return;
	Entries.AddUnique(Entry);
	Entry->ApplyCombatValue(UmbraCombatStats::Read(BoundASC.IsValid() ? BoundPlayerState.Get() : nullptr, Entry->ResolveCombatStat()));
}

void UUmbraCombatInfo::NotifyPlayerContextChanged() { if (bObserving) BindPlayer(); }
void UUmbraCombatInfo::OnPawnChanged(APawn*, APawn*) { BindPlayer(); }
void UUmbraCombatInfo::OnDerived(const FUmbraDerivedStatsSnapshot&) { RefreshValues(); }
void UUmbraCombatInfo::OnEquipment(const FUmbraEquipmentSnapshot&) { RefreshValues(); }
void UUmbraCombatInfo::OnAttribute(const FOnAttributeChangeData&) { RefreshValues(); }

void UUmbraCombatInfo::BindPlayer()
{
	if (!bObserving) return;
	auto* PS = GetOwningPlayerState<AUmbraPlayerState>();
	auto* ASC = PS ? PS->GetUmbraAbilitySystemComponent() : nullptr;
	if (!ASC || !ASC->IsActorInfoReady() || !ASC->GetSet<UUmbraAttributeSet>())
	{
		UnbindData(); RefreshValues(); return;
	}
	if (BoundASC.Get() == ASC) { RefreshValues(); return; }
	UnbindData();
	BoundPlayerState = PS;
	BoundASC = ASC;
	for (int32 Index = 1; Index < StaticEnum<EUmbraCombatStat>()->NumEnums() - 1; ++Index)
	{
		const auto Attribute = UmbraCombatStats::AttributeFor(EUmbraCombatStat(Index));
		if (Attribute.IsValid() && !AttributeHandles.Contains(Attribute))
			AttributeHandles.Add(Attribute, ASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &ThisClass::OnAttribute));
	}
	BoundDerived = PS->FindComponentByClass<UUmbraDerivedStatsComponent>();
	BoundEquipment = PS->FindComponentByClass<UUmbraEquipmentComponent>();
	if (BoundDerived.IsValid()) BoundDerived->OnDerivedStatsChanged.AddUniqueDynamic(this, &ThisClass::OnDerived);
	if (BoundEquipment.IsValid()) BoundEquipment->OnEquipmentChanged.AddUniqueDynamic(this, &ThisClass::OnEquipment);
	AddedHandle = ASC->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &ThisClass::OnEffectAdded);
	RemovedHandle = ASC->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &ThisClass::OnEffectRemoved);
	for (const auto Handle : ASC->GetActiveEffects(FGameplayEffectQuery())) ObserveEffect(Handle);
	RefreshValues();
}

void UUmbraCombatInfo::ObserveEffect(FActiveGameplayEffectHandle Handle)
{
	auto* ASC = BoundASC.Get();
	if (!ASC || ObservedEffects.Contains(Handle)) return;
	if (auto* Events = ASC->GetActiveEffectEventSet(Handle))
	{
		ObservedEffects.Add(Handle);
		Events->OnStackChanged.AddWeakLambda(this, [this](FActiveGameplayEffectHandle, int32, int32) { RefreshValues(); });
		Events->OnTimeChanged.AddWeakLambda(this, [this](FActiveGameplayEffectHandle, float, float) { RefreshValues(); });
		Events->OnInhibitionChanged.AddWeakLambda(this, [this](FActiveGameplayEffectHandle, bool) { RefreshValues(); });
	}
}

void UUmbraCombatInfo::OnEffectAdded(UAbilitySystemComponent*, const FGameplayEffectSpec&, FActiveGameplayEffectHandle Handle)
{
	ObserveEffect(Handle); RefreshValues();
}

void UUmbraCombatInfo::OnEffectRemoved(const FActiveGameplayEffect& Effect)
{
	ObservedEffects.Remove(Effect.Handle);
	RefreshValues();
}

void UUmbraCombatInfo::OnLifecycle(UUmbraAbilitySystemComponent* ASC, bool bReady)
{
	if (!bObserving) return;
	if (!bReady && BoundASC.Get() == ASC) { UnbindData(); RefreshValues(); return; }
	const auto* PS = GetOwningPlayerState<AUmbraPlayerState>();
	if (bReady && PS && PS->GetUmbraAbilitySystemComponent() == ASC) BindPlayer();
}

void UUmbraCombatInfo::RefreshValues()
{
	Entries.RemoveAll([](const auto& Entry) { return !Entry.IsValid(); });
	for (const auto& Entry : Entries)
		Entry->ApplyCombatValue(UmbraCombatStats::Read(BoundASC.IsValid() ? BoundPlayerState.Get() : nullptr, Entry->ResolveCombatStat()));
}

void UUmbraCombatInfo::UnbindData()
{
	if (auto* ASC = BoundASC.Get())
	{
		for (const auto& Pair : AttributeHandles) ASC->GetGameplayAttributeValueChangeDelegate(Pair.Key).Remove(Pair.Value);
		ASC->OnActiveGameplayEffectAddedDelegateToSelf.Remove(AddedHandle);
		ASC->OnAnyGameplayEffectRemovedDelegate().Remove(RemovedHandle);
		for (const auto Handle : ObservedEffects)
			if (auto* Events = ASC->GetActiveEffectEventSet(Handle))
			{
				Events->OnStackChanged.RemoveAll(this);
				Events->OnTimeChanged.RemoveAll(this);
				Events->OnInhibitionChanged.RemoveAll(this);
			}
	}
	if (BoundDerived.IsValid()) BoundDerived->OnDerivedStatsChanged.RemoveDynamic(this, &ThisClass::OnDerived);
	if (BoundEquipment.IsValid()) BoundEquipment->OnEquipmentChanged.RemoveDynamic(this, &ThisClass::OnEquipment);
	BoundDerived.Reset(); BoundEquipment.Reset(); BoundASC.Reset(); BoundPlayerState.Reset();
	AttributeHandles.Reset(); ObservedEffects.Reset(); AddedHandle.Reset(); RemovedHandle.Reset();
}

void UUmbraCombatInfo::Shutdown()
{
	bObserving = false;
	UnbindData();
	UUmbraAbilitySystemComponent::OnLifecycleChanged.Remove(LifecycleHandle);
	LifecycleHandle.Reset();
	if (auto* PC = GetOwningPlayer()) PC->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::OnPawnChanged);
	for (const auto& Entry : Entries) if (Entry.IsValid()) Entry->ApplyCombatValue(UmbraCombatStats::Read(nullptr, Entry->ResolveCombatStat()));
	Entries.Reset();
}
