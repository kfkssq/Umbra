#include "UI/Items/UmbraItemDragDropOperation.h"
#include "UmbraPlayerController.h"
#include "UI/Inventory/UmbraInventorySlot.h"
#include "UI/Inventory/UmbraInventoryMenu.h"
#include "UI/Equipment/UmbraEquipmentSlotWidget.h"
#include "UI/Equipment/UmbraEquipmentMenu.h"
#include "Audio/UmbraAudioFunctionLibrary.h"
#include "Components/Image.h"
#include "GameFramework/PlayerController.h"

bool UUmbraItemDragDropOperation::IsLive(UUmbraInventorySlot* Cell)
{
	auto* Menu = Cell ? Cell->InteractionMenu.Get() : nullptr;
	return Menu && Menu->bObserving && Menu->bPageActive && Menu->IsVisible()
		&& Menu->bBindInventoryData && Menu->bInventoryDataReady && Menu->Slots.Contains(Cell)
		&& Menu->BoundInventory.IsValid() && Menu->BoundInventory->GetOwner() == Menu->GetOwningPlayerState<AUmbraPlayerState>();
}

bool UUmbraItemDragDropOperation::IsLive(UUmbraEquipmentSlotWidget* Cell)
{
	auto* Menu = Cell ? Cell->InteractionMenu.Get() : nullptr;
	return Menu && Menu->bObservingEquipment && Menu->bPageActive && Menu->IsVisible()
		&& Menu->bBindEquipmentData && Menu->bEquipmentDataReady && !Cell->IsLocked()
		&& Menu->GetEquipmentSlot(Cell->GetSlotType()) == Cell && Menu->BoundEquipment.IsValid()
		&& Menu->BoundEquipment->GetOwner() == Menu->GetOwningPlayerState<AUmbraPlayerState>();
}

void UUmbraItemDragDropOperation::SetVisual(UUserWidget* Source, UImage* Icon)
{
	Owner = Source->GetOwningPlayerState<AUmbraPlayerState>();
	Pivot = EDragPivot::CenterCenter;
	if (Icon)
	{
		auto* Visual = NewObject<UImage>(this);
		auto Brush = Icon->GetBrush();
		// Use the authored cell size in Slate units, independent of texture resolution.
		Brush.ImageSize = Source->GetCachedGeometry().GetLocalSize();
		Visual->SetBrush(Brush);
		Visual->SetVisibility(ESlateVisibility::HitTestInvisible);
		DefaultDragVisual = Visual;
	}
}

UUmbraItemDragDropOperation* UUmbraItemDragDropOperation::Begin(UUmbraInventorySlot* Source)
{
	if (!IsLive(Source) || !Source->IsFromInventorySnapshot() || !Source->HasItem()) return nullptr;
	auto* Drag = NewObject<UUmbraItemDragDropOperation>();
	Drag->InventorySource = Source;
	Drag->InventoryIndex = Source->GetSlotIndex();
	Drag->InstanceId = Source->GetItemDisplay().InstanceId;
	Drag->SetVisual(Source, Source->ItemIcon);
	Source->InteractionMenu->ClearHoveredTooltip();
	return Drag;
}

UUmbraItemDragDropOperation* UUmbraItemDragDropOperation::Begin(UUmbraEquipmentSlotWidget* Source)
{
	if (!IsLive(Source) || !Source->GetItemDisplay().bFromEquipmentSnapshot || !Source->HasItem()) return nullptr;
	auto* Drag = NewObject<UUmbraItemDragDropOperation>();
	Drag->EquipmentSource = Source;
	Drag->EquipmentSlot = Source->GetSlotType();
	Drag->InstanceId = Source->GetItemDisplay().InstanceId;
	Drag->SetVisual(Source, Source->ItemIcon);
	Source->InteractionMenu->ClearHoveredTooltip();
	return Drag;
}

bool UUmbraItemDragDropOperation::IsSourceCurrent() const
{
	if (!Owner.IsValid() || !InstanceId.IsValid()) return false;
	if (auto* Cell = InventorySource.Get())
		return IsLive(Cell) && Cell->IsFromInventorySnapshot() && Cell->GetSlotIndex() == InventoryIndex
			&& Cell->GetItemDisplay().InstanceId == InstanceId && Cell->GetOwningPlayerState<AUmbraPlayerState>() == Owner.Get();
	if (auto* Cell = EquipmentSource.Get())
		return IsLive(Cell) && Cell->GetItemDisplay().bFromEquipmentSnapshot && Cell->GetSlotType() == EquipmentSlot
			&& Cell->GetItemDisplay().InstanceId == InstanceId && Cell->GetOwningPlayerState<AUmbraPlayerState>() == Owner.Get();
	return false;
}

bool UUmbraItemDragDropOperation::Reject(UUserWidget* Target) const
{
	if (Target) UUmbraAudioFunctionLibrary::PlayUI(Target->GetOwningPlayer(), EUmbraAudioEvent::EquipFailed, FUmbraAudioContext());
	return true;
}

bool UUmbraItemDragDropOperation::DropOn(UUmbraInventorySlot* Target)
{
	if (bConsumed) return true;
	bConsumed = true;
	if (!IsSourceCurrent() || !IsLive(Target) || Target->GetOwningPlayerState<AUmbraPlayerState>() != Owner.Get()) return Reject(Target);
	if (auto* Source = InventorySource.Get())
	{
		if (Source == Target) return true;
		if (!Owner->HasAuthority())
		{
			auto* Controller = Cast<AUmbraPlayerController>(Target->GetOwningPlayer());
			if (!Controller || Controller->RequestItemTransfer(EUmbraItemTransferOperation::Move, InstanceId,
				EUmbraEquipmentSlot::Head, Target->GetSlotIndex()) != EUmbraTransferResult::Pending) return Reject(Target);
			return true;
		}
		const auto Context = Source->GetItemDisplay().AudioContext;
		const auto Result = Owner->MoveInventoryItem(InstanceId, Target->GetSlotIndex());
		UUmbraAudioFunctionLibrary::PlayUI(Target->GetOwningPlayer(), Result == EUmbraInventoryResult::Success
			? EUmbraAudioEvent::InventoryTransfer : EUmbraAudioEvent::EquipFailed, Context);
	}
	else if (auto* EquippedSource = EquipmentSource.Get())
		EquippedSource->InteractionMenu->UnequipToCell(EquippedSource, Target->GetSlotIndex());
	return true;
}

bool UUmbraItemDragDropOperation::DropOn(UUmbraEquipmentSlotWidget* Target)
{
	if (bConsumed) return true;
	bConsumed = true;
	if (!IsSourceCurrent() || !IsLive(Target) || Target->GetOwningPlayerState<AUmbraPlayerState>() != Owner.Get()) return Reject(Target);
	if (auto* Source = InventorySource.Get()) Source->InteractionMenu->RequestEquipInSlot(InstanceId, Target->GetSlotType());
	else if (EquipmentSource.Get() != Target) return Reject(Target);
	return true;
}
