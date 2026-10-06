#include "UI/Inventory/UmbraInventoryMenu.h"

#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "UI/Inventory/UmbraInventorySlot.h"
#include "Umbra.h"
#include "Player/UmbraPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Items/UmbraItemDefinition.h"
#include "UI/Items/UmbraTransferFeedback.h"
#include "UI/Items/UmbraItemTooltip.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Async/Async.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UUmbraInventoryMenu::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshCapacityText();
}

void UUmbraInventoryMenu::NativeConstruct()
{
	Super::NativeConstruct();
	OnVisibilityChanged.AddUniqueDynamic(this, &ThisClass::HandleTooltipVisibility);
	if (!TooltipASCLifecycleHandle.IsValid()) TooltipASCLifecycleHandle = UUmbraAbilitySystemComponent::OnLifecycleChanged.AddUObject(this, &ThisClass::OnTooltipASCLifecycle);
	bObserving = true;
	if (APlayerController* PC = GetOwningPlayer()) PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &ThisClass::HandlePawnChanged);
	if (!LifecycleHandle.IsValid()) LifecycleHandle = UUmbraInventoryComponent::OnLifecycleChanged.AddUObject(this, &ThisClass::OnInventoryLifecycle);
	if (bBindInventoryData) BindInventory(); else InitializeSlots();
}

void UUmbraInventoryMenu::NativeDestruct()
{
	bObserving = false;
	if (APlayerController* PC = GetOwningPlayer()) PC->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePawnChanged);
	UUmbraInventoryComponent::OnLifecycleChanged.Remove(LifecycleHandle);
	LifecycleHandle.Reset();
	UnbindInventory();
	if (bBindInventoryData) RefreshInventory();
	UnbindSlots();
	// Retain cells and selection for the same UObject's next Construct.
	ClearHoveredTooltip();
	if (CachedTooltip && CachedTooltip->IsInViewport()) CachedTooltip->RemoveFromParent();
	CachedTooltip = nullptr;
	OnVisibilityChanged.RemoveDynamic(this, &ThisClass::HandleTooltipVisibility);
	UnbindTooltipContext();
	UUmbraAbilitySystemComponent::OnLifecycleChanged.Remove(TooltipASCLifecycleHandle);
	TooltipASCLifecycleHandle.Reset();
	Super::NativeDestruct();
}

void UUmbraInventoryMenu::RefreshCapacityText()
{
	if (!CapacityText) return;
	if (!IsDesignTime() && bBindInventoryData && !bInventoryDataReady)
	{
		CapacityText->SetText(NSLOCTEXT("UmbraInventory", "Unavailable", "—/—"));
		return;
	}
	CapacityText->SetText(FText::Format(NSLOCTEXT("UmbraInventory", "Capacity", "{0}/{1}"),
		FText::AsNumber(bInventoryDataReady ? ViewSnapshot.Items.Num() : 0), FText::AsNumber(GetInventoryCapacity())));
}

int32 UUmbraInventoryMenu::GetInventoryCapacity() const
{
	return !IsDesignTime() && bBindInventoryData ? (bInventoryDataReady ? ViewSnapshot.Capacity : 0) : FMath::Max(0, InventoryCapacity);
}

void UUmbraInventoryMenu::NotifyPlayerContextChanged()
{
	if (bObserving && bBindInventoryData) { BindInventory(); BindTooltipContext(); RefreshHoveredTooltip(); }
}

void UUmbraInventoryMenu::HandlePawnChanged(APawn*, APawn*) { NotifyPlayerContextChanged(); }

void UUmbraInventoryMenu::OnInventoryLifecycle(UUmbraInventoryComponent* Component)
{
	if (Component && (Component == BoundInventory.Get() || Component->GetOwner() == GetOwningPlayerState<AUmbraPlayerState>())) NotifyPlayerContextChanged();
}

void UUmbraInventoryMenu::BindInventory()
{
	auto* PS = GetOwningPlayerState<AUmbraPlayerState>();
	auto* Component = PS ? PS->FindComponentByClass<UUmbraInventoryComponent>() : nullptr;
	if (Component && !Component->IsRegistered()) Component = nullptr;
	if (BoundInventory.Get() != Component)
	{
		if (SelectionInventory.Get() != Component) SelectedInstanceId.Invalidate();
		SelectionInventory = Component;
		UnbindInventory();
		BoundInventory = Component;
		if (Component) Component->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::HandleInventoryChanged);
	}
	RefreshInventory();
}

void UUmbraInventoryMenu::UnbindInventory()
{
	ClearHoveredTooltip();
	UnbindTooltipContext();
	if (BoundInventory.IsValid()) BoundInventory->OnInventoryChanged.RemoveDynamic(this, &ThisClass::HandleInventoryChanged);
	BoundInventory.Reset();
}

void UUmbraInventoryMenu::HandleInventoryChanged(const FUmbraInventorySnapshot&) { NotifyPlayerContextChanged(); }

EUmbraInventoryViewState UUmbraInventoryMenu::ValidateSnapshot(const FUmbraInventorySnapshot& Snapshot)
{
	if (!Snapshot.bReady) return EUmbraInventoryViewState::NotReady;
	if (Snapshot.Capacity < 0 || Snapshot.Capacity > 512) return EUmbraInventoryViewState::InvalidSnapshot;
	TSet<int32> Indices;
	TSet<FGuid> Identities;
	for (const auto& Item : Snapshot.Items)
	{
		if (!IsValid(Item.Definition) || !Item.InstanceId.IsValid() || Item.SlotIndex < 0
			|| Item.SlotIndex >= Snapshot.Capacity || Indices.Contains(Item.SlotIndex) || Identities.Contains(Item.InstanceId))
			return EUmbraInventoryViewState::InvalidSnapshot;
		Indices.Add(Item.SlotIndex); Identities.Add(Item.InstanceId);
	}
	return EUmbraInventoryViewState::Ready;
}

void UUmbraInventoryMenu::RefreshInventory()
{
	if (bRefreshing) return;
	TGuardValue<bool> Guard(bRefreshing, true);
	ViewSnapshot = bObserving && BoundInventory.IsValid() ? BoundInventory->GetSnapshot() : FUmbraInventorySnapshot();
	InventoryViewState = ValidateSnapshot(ViewSnapshot);
	bInventoryDataReady = InventoryViewState == EUmbraInventoryViewState::Ready;
	// Keep only identity while detached, never stale presentation. A fresh snapshot validates it on reopen.
	if (bObserving && (!bInventoryDataReady || !ViewSnapshot.Items.ContainsByPredicate(
		[this](const auto& Item) { return Item.InstanceId == SelectedInstanceId; }))) SelectedInstanceId.Invalidate();
	InitializeSlots();
	SelectedSlot = nullptr;
	for (UUmbraInventorySlot* Cell : Slots)
	{
		const auto* Entry = bInventoryDataReady ? ViewSnapshot.Items.FindByPredicate([Cell](const auto& Item) { return Item.SlotIndex == Cell->GetSlotIndex(); }) : nullptr;
		if (Entry) Cell->SetItemDefinition(Entry->Definition, Entry->InstanceId); else Cell->ClearItem();
		Cell->bFromInventorySnapshot = Entry != nullptr;
		const bool bSelect = Entry && Entry->InstanceId == SelectedInstanceId;
		Cell->SetSelected(bSelect);
		if (bSelect) SelectedSlot = Cell;
	}
	BindTooltipContext();
	RefreshHoveredTooltip();
	BP_InventoryDataChanged(bInventoryDataReady);
}

void UUmbraInventoryMenu::UnbindSlots()
{
	ClearHoveredTooltip();
	for (UUmbraInventorySlot* Cell : Slots)
	{
		if (!Cell) continue;
		Cell->OnSelectionRequested.RemoveDynamic(this, &ThisClass::HandleSelection);
		Cell->OnEquipRequested.RemoveDynamic(this, &ThisClass::HandleEquip);
		Cell->OnTooltipHoverChanged.RemoveAll(this);
		Cell->SetToolTip(nullptr);
	}
}

void UUmbraInventoryMenu::ClearSlots()
{
	UnbindSlots();
	for (UUmbraInventorySlot* Cell : Slots)
	{
		if (!Cell) continue;
		Cell->SetSelected(false);
		Cell->RemoveFromParent();
	}
	Slots.Reset();
	SelectedSlot = nullptr;
	BuiltSlotClass = nullptr;
	BuiltColumns = 0;
}

void UUmbraInventoryMenu::InitializeSlots()
{
	RefreshCapacityText();
	if (IsDesignTime()) return;
	if (!InventoryGrid || !InventorySlotClass || InventorySlotClass->HasAnyClassFlags(CLASS_Abstract))
	{
		ClearSlots();
		UE_LOG(LogUmbra, Warning, TEXT("Inventory %s needs InventoryGrid and a concrete InventorySlotClass in Class Defaults."), *GetName());
		return;
	}

	const int32 Capacity = GetInventoryCapacity();
	const int32 SafeColumns = FMath::Max(1, Columns);
	bool bMatches = BuiltSlotClass == InventorySlotClass && BuiltColumns == SafeColumns
		&& Slots.Num() == Capacity && InventoryGrid->GetChildrenCount() == Capacity;
	for (int32 Index = 0; bMatches && Index < Slots.Num(); ++Index)
	{
		bMatches = Slots[Index] && InventoryGrid->GetChildAt(Index) == Slots[Index];
	}
	if (!bMatches)
	{
		ClearSlots();
		// This panel is exclusively generated. Designer must not contain placeholder cells.
		InventoryGrid->ClearChildren();
		for (int32 Index = 0; Index < Capacity; ++Index)
		{
			UUmbraInventorySlot* Cell = CreateWidget<UUmbraInventorySlot>(this, InventorySlotClass);
			if (!Cell)
			{
				ClearSlots();
				UE_LOG(LogUmbra, Warning, TEXT("Inventory %s failed to create slot %d."), *GetName(), Index);
				return;
			}
			Cell->SlotIndex = Index;
			// An interactive cell must receive mouse events even if a WBP kept UUserWidget's default.
			Cell->SetVisibility(ESlateVisibility::Visible);
			UUniformGridSlot* GridSlot = InventoryGrid->AddChildToUniformGrid(Cell, Index / SafeColumns, Index % SafeColumns);
			GridSlot->SetHorizontalAlignment(HAlign_Center);
			GridSlot->SetVerticalAlignment(VAlign_Center);
			Slots.Add(Cell);
		}
		BuiltSlotClass = InventorySlotClass;
		BuiltColumns = SafeColumns;
	}
	for (UUmbraInventorySlot* Cell : Slots)
	{
		Cell->OnSelectionRequested.AddUniqueDynamic(this, &ThisClass::HandleSelection);
		Cell->OnEquipRequested.AddUniqueDynamic(this, &ThisClass::HandleEquip);
		Cell->OnTooltipHoverChanged.RemoveAll(this);
		Cell->OnTooltipHoverChanged.AddUObject(this, &ThisClass::HandleTooltipHover);
	}
}

void UUmbraInventoryMenu::HandleSelection(UUmbraInventorySlot* RequestedSlot)
{
	if (!RequestedSlot || !Slots.Contains(RequestedSlot) || SelectedSlot == RequestedSlot) return;
	if (SelectedSlot) SelectedSlot->SetSelected(false);
	SelectedSlot = RequestedSlot;
	SelectedInstanceId = RequestedSlot->GetItemDisplay().InstanceId;
	SelectedSlot->SetSelected(true);
}

int32 UUmbraInventoryMenu::GetSelectedSlotIndex() const
{
	return SelectedSlot ? SelectedSlot->GetSlotIndex() : INDEX_NONE;
}

UUmbraInventorySlot* UUmbraInventoryMenu::GetInventorySlot(int32 Index) const
{
	return Slots.IsValidIndex(Index) ? Slots[Index].Get() : nullptr;
}

void UUmbraInventoryMenu::HandleEquip(UUmbraInventorySlot* RequestedSlot)
{
	if (RequestedSlot && Slots.Contains(RequestedSlot) && RequestedSlot->HasItem())
		RequestEquipItem(RequestedSlot->GetItemDisplay().InstanceId);
}

EUmbraTransferResult UUmbraInventoryMenu::FinishEquip(EUmbraTransferResult Result, FGuid InstanceId)
{
	LastEquipResult = Result;
	LastEquipMessage = UmbraTransferFeedback::Message(Result);
	BP_EquipFinished(Result, InstanceId, LastEquipMessage);
	return Result;
}

EUmbraTransferResult UUmbraInventoryMenu::RequestEquipItem(FGuid InstanceId)
{
	return FinishEquip(TryEquip(InstanceId, nullptr), InstanceId);
}

EUmbraTransferResult UUmbraInventoryMenu::RequestEquipInSlot(FGuid InstanceId, EUmbraEquipmentSlot TargetSlot)
{
	return FinishEquip(TryEquip(InstanceId, &TargetSlot), InstanceId);
}

EUmbraTransferResult UUmbraInventoryMenu::TryEquip(FGuid InstanceId, const EUmbraEquipmentSlot* ExplicitSlot)
{
	auto* PS = GetOwningPlayerState<AUmbraPlayerState>();
	if (!bObserving || !bBindInventoryData || !bInventoryDataReady || !PS || !BoundInventory.IsValid()
		|| BoundInventory->GetOwner() != PS) return EUmbraTransferResult::NotReady;
	if (!PS->HasAuthority()) return EUmbraTransferResult::AuthorityRequired;
	if (!InstanceId.IsValid()) return EUmbraTransferResult::InvalidIdentity;
	const auto Inventory = BoundInventory->GetSnapshot();
	if (ValidateSnapshot(Inventory) != EUmbraInventoryViewState::Ready) return EUmbraTransferResult::NotReady;
	const auto* Item = Inventory.Items.FindByPredicate([InstanceId](const auto& Entry) { return Entry.InstanceId == InstanceId; });
	if (!Item) return EUmbraTransferResult::NotFound;
	auto* Equipment = PS->GetEquipmentComponent();
	if (!Equipment || !Equipment->IsRegistered() || !Equipment->IsReadyForCombat()) return EUmbraTransferResult::NotReady;
	TArray<EUmbraEquipmentSlot> Allowed;
	for (const auto Candidate : Item->Definition->AllowedSlots)
	{
		if (uint8(Candidate) > uint8(EUmbraEquipmentSlot::OffHand)) return EUmbraTransferResult::InvalidSlot;
		Allowed.AddUnique(Candidate);
	}
	if (Allowed.IsEmpty()) return EUmbraTransferResult::InvalidSlot;
	if (ExplicitSlot)
	{
		if (!Allowed.Contains(*ExplicitSlot)) return EUmbraTransferResult::InvalidSlot;
		return PS->EquipFromInventory(*ExplicitSlot, InstanceId);
	}
	if (Allowed.Num() == 1) return PS->EquipFromInventory(Allowed[0], InstanceId);
	const auto Equipped = Equipment->GetSnapshot();
	Allowed.RemoveAll([&Equipped](EUmbraEquipmentSlot Candidate)
	{
		return Equipped.Items.ContainsByPredicate([Candidate](const auto& Entry) { return Entry.Slot == Candidate; });
	});
	if (Allowed.IsEmpty()) return EUmbraTransferResult::SlotOccupied;
	if (Allowed.Num() > 1) return EUmbraTransferResult::SlotSelectionRequired;
	return PS->EquipFromInventory(Allowed[0], InstanceId);
}
