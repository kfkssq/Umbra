#include "UI/Inventory/UmbraInventoryMenu.h"
#include "UI/Inventory/UmbraInventorySlot.h"
#include "UI/Items/UmbraItemTooltip.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Equipment/UmbraEquipmentComponent.h"
#include "Async/Async.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Umbra.h"

namespace
{
	TArray<FGameplayAttribute> InventoryTooltipPrimaries()
	{
		return {UUmbraAttributeSet::GetStrengthAttribute(), UUmbraAttributeSet::GetDexterityAttribute(),
			UUmbraAttributeSet::GetIntelligenceAttribute(), UUmbraAttributeSet::GetFaithAttribute()};
	}
}

void UUmbraInventoryMenu::HandleTooltipVisibility(ESlateVisibility InVisibility)
{
	if (InVisibility == ESlateVisibility::Hidden || InVisibility == ESlateVisibility::Collapsed) ClearHoveredTooltip();
}

void UUmbraInventoryMenu::SetPageActive(bool bActive)
{
	if (IsDesignTime()) return;
	bPageActive = bActive;
	if (!bActive) ClearHoveredTooltip();
	else NotifyPlayerContextChanged();
}

void UUmbraInventoryMenu::HandleTooltipHover(UUmbraInventorySlot* HoveredCell, bool bHovered)
{
	if (!bHovered)
	{
		if (HoveredTooltipSlot.Get() == HoveredCell) ClearHoveredTooltip();
		return;
	}
	ClearHoveredTooltip();
	if (IsDesignTime() || !HoveredCell || !Slots.Contains(HoveredCell) || !bPageActive || !IsVisible()) return;
	HoveredTooltipSlot = HoveredCell;
	BindTooltipContext();
	RefreshHoveredTooltip();
}

void UUmbraInventoryMenu::ClearHoveredTooltip(bool bForgetHover)
{
	// Clear the data before hiding: an already-open tooltip must not keep old text.
	if (CachedTooltip) { CachedTooltip->ClearTooltipData(); CachedTooltip->HideTooltip(); }
	if (HoveredTooltipSlot.IsValid()) HoveredTooltipSlot->SetToolTip(nullptr);
	if (bForgetHover) HoveredTooltipSlot.Reset();
}

void UUmbraInventoryMenu::RefreshHoveredTooltip()
{
	auto* HoveredCell = HoveredTooltipSlot.Get();
	if (!HoveredCell) return;
	auto* PS = GetOwningPlayerState<AUmbraPlayerState>();
	if (!bObserving || !bPageActive || !IsVisible() || !bBindInventoryData || !bInventoryDataReady
		|| !PS || !BoundInventory.IsValid() || BoundInventory->GetOwner() != PS
		|| !BoundInventory->IsRegistered() || !Slots.Contains(HoveredCell)
		|| !ItemTooltipClass || ItemTooltipClass->HasAnyClassFlags(CLASS_Abstract))
	{
		ClearHoveredTooltip(false);
		return;
	}
	const auto Display = HoveredCell->GetItemDisplay();
	if (!HoveredCell->IsFromInventorySnapshot()) { ClearHoveredTooltip(false); return; }

	// The slot supplies expected identity only. Snapshot and requirement context always come from this menu.
	auto* Equipment = TooltipEquipment.Get();
	if (!Equipment || Equipment->GetOwner() != PS || !Equipment->IsRegistered()
		|| !TooltipASC.IsValid() || !TooltipASC->IsActorInfoReady())
	{
		ClearHoveredTooltip(false);
		return;
	}
	const auto Data = UUmbraItemTooltipDataBuilder::FromInventory(
		BoundInventory->GetSnapshot(), HoveredCell->GetSlotIndex(), Display.InstanceId, Cast<UUmbraItemDefinition>(Display.Item), Equipment, false, EUmbraEquipmentSlot::Head);
	if (!Data.bValid || Data.Result != EUmbraTooltipResult::Success)
	{
		// Normal empty slots and a component currently publishing are not warning-worthy.
		UE_LOG(LogUmbra, VeryVerbose, TEXT("Inventory tooltip unavailable: %s"), *UEnum::GetValueAsString(Data.Result));
		ClearHoveredTooltip(false);
		return;
	}
	if (CachedTooltip && CachedTooltip->GetClass() != ItemTooltipClass)
	{
		ClearHoveredTooltip(false);
		CachedTooltip = nullptr;
	}
	if (!CachedTooltip) CachedTooltip = CreateWidget<UUmbraItemTooltip>(this, ItemTooltipClass);
	if (!CachedTooltip) return;
	CachedTooltip->SetTooltipData(Data);
	CachedTooltip->PresentBeside(HoveredCell);
}

void UUmbraInventoryMenu::BindTooltipContext()
{
	auto* PS = bObserving && bBindInventoryData ? GetOwningPlayerState<AUmbraPlayerState>() : nullptr;
	auto* ASC = PS ? PS->GetUmbraAbilitySystemComponent() : nullptr;
	auto* Equipment = PS ? PS->GetEquipmentComponent() : nullptr;
	if (ASC && (!ASC->IsRegistered() || !ASC->IsActorInfoReady())) ASC = nullptr;
	if (Equipment && !Equipment->IsRegistered()) Equipment = nullptr;
	if (TooltipASC.Get() == ASC && TooltipEquipment.Get() == Equipment) return;
	ClearHoveredTooltip(false);
	UnbindTooltipContext();
	TooltipASC = ASC;
	TooltipEquipment = Equipment;
	if (Equipment) Equipment->OnEquipmentChanged.AddUniqueDynamic(this, &ThisClass::OnTooltipEquipmentChanged);
	if (ASC) for (const auto& Attribute : InventoryTooltipPrimaries())
		TooltipAttributeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &ThisClass::OnTooltipPrimaryChanged));
}

void UUmbraInventoryMenu::UnbindTooltipContext()
{
	if (TooltipEquipment.IsValid()) TooltipEquipment->OnEquipmentChanged.RemoveDynamic(this, &ThisClass::OnTooltipEquipmentChanged);
	if (TooltipASC.IsValid())
	{
		const auto Attributes = InventoryTooltipPrimaries();
		for (int32 Index = 0; Index < TooltipAttributeHandles.Num(); ++Index)
			TooltipASC->GetGameplayAttributeValueChangeDelegate(Attributes[Index]).Remove(TooltipAttributeHandles[Index]);
	}
	TooltipAttributeHandles.Reset();
	TooltipASC.Reset();
	TooltipEquipment.Reset();
}

void UUmbraInventoryMenu::OnTooltipASCLifecycle(UUmbraAbilitySystemComponent* ASC, bool bReady)
{
	if (!bObserving || !ASC || (ASC != TooltipASC.Get() && ASC->GetOwner() != GetOwningPlayerState<AUmbraPlayerState>())) return;
	if (!bReady) ClearHoveredTooltip();
	BindTooltipContext();
	QueueTooltipRefresh();
}

void UUmbraInventoryMenu::OnTooltipPrimaryChanged(const FOnAttributeChangeData&) { QueueTooltipRefresh(); }
void UUmbraInventoryMenu::OnTooltipEquipmentChanged(const FUmbraEquipmentSnapshot&)
{
	// Invalidate immediately (including transfers); QueryRequirements rejects in-progress publication.
	RefreshHoveredTooltip();
	QueueTooltipRefresh();
}

void UUmbraInventoryMenu::QueueTooltipRefresh()
{
	if (bTooltipRefreshQueued || !HoveredTooltipSlot.IsValid() || !bObserving) return;
	bTooltipRefreshQueued = true;
	// One coalesced game-thread task per context event, after the equipment/GAS stack unwinds.
	// No polling, Tick or gameplay mutation; a closed/rebound menu cannot resurrect captured data.
	TWeakObjectPtr<UUmbraInventoryMenu> WeakThis(this);
	AsyncTask(ENamedThreads::GameThread, [WeakThis]()
	{
		if (auto* Menu = WeakThis.Get())
		{
			Menu->bTooltipRefreshQueued = false;
			Menu->BindTooltipContext();
			Menu->RefreshHoveredTooltip();
		}
	});
}

