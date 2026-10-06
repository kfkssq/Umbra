#include "UI/Equipment/UmbraEquipmentSlotWidget.h"
#include "InputCoreTypes.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UUmbraEquipmentSlotWidget::SetSlotType(EUmbraEquipmentSlot InSlotType)
{
	if (SlotType != InSlotType)
	{
		SlotType = InSlotType;
		OnSlotTypeChanged.Broadcast(this);
	}
	RefreshVisual();
}

void UUmbraEquipmentSlotWidget::SetItem(const FUmbraEquipmentItemDisplay& InItem)
{
	if (CurrentItem.Item != InItem.Item || CurrentItem.InstanceId != InItem.InstanceId) bSelected = false;
	CurrentItem = InItem.Item ? InItem : FUmbraEquipmentItemDisplay();
	RefreshVisual();
	if (bHovered) OnTooltipHoverChanged.Broadcast(this, true);
}

void UUmbraEquipmentSlotWidget::ClearItem()
{
	bSelected = false;
	CurrentItem = FUmbraEquipmentItemDisplay();
	RefreshVisual();
	if (bHovered) OnTooltipHoverChanged.Broadcast(this, true);
}

void UUmbraEquipmentSlotWidget::SetLocked(bool bInLocked)
{
	bLocked = bInLocked;
	if (bLocked) bSelected = false;
	RefreshVisual();
}

void UUmbraEquipmentSlotWidget::SetSelected(bool bInSelected)
{
	bSelected = bInSelected && !bLocked;
	RefreshVisual();
}

void UUmbraEquipmentSlotWidget::SetHovered(bool bInHovered)
{
	bHovered = bInHovered;
	RefreshVisual();
	OnTooltipHoverChanged.Broadcast(this, bHovered);
	if (!bHovered) SetToolTip(nullptr);
}

void UUmbraEquipmentSlotWidget::RequestSelection()
{
	if (!bLocked) OnSelectionRequested.Broadcast(this);
}

void UUmbraEquipmentSlotWidget::RequestUnequip()
{
	OnUnequipRequested.Broadcast(this);
}

EUmbraEquipmentSlotState UUmbraEquipmentSlotWidget::GetVisualState() const
{
	if (bLocked) return EUmbraEquipmentSlotState::Locked;
	if (bSelected) return EUmbraEquipmentSlotState::Selected;
	if (bHovered) return EUmbraEquipmentSlotState::Hovered;
	return HasItem() ? EUmbraEquipmentSlotState::Equipped : EUmbraEquipmentSlotState::Empty;
}

void UUmbraEquipmentSlotWidget::RefreshVisual()
{
	ItemVisual.CaptureDefaults(SlotBackground, RarityFrame);
	// Blueprint styles interaction overlays first; item-owned layers must reflect live data last.
	BP_RefreshVisual(GetVisualState(), CurrentItem, HasItem());
	if (EmptyIcon)
	{
		const TObjectPtr<UTexture2D>* Texture = EmptySlotIcons.Find(SlotType);
		// Clear missing entries as well, so changing SlotType cannot leave the previous icon.
		EmptyIcon->SetBrushFromTexture(Texture ? Texture->Get() : nullptr);
		EmptyIcon->SetVisibility(!HasItem() && Texture && Texture->Get()
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	ItemVisual.Apply(CurrentItem, ItemIcon, SlotBackground, RarityFrame);
	if (LockedOverlay) LockedOverlay->SetVisibility(bLocked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UUmbraEquipmentSlotWidget::NativeDestruct()
{
	SetHovered(false);
	Super::NativeDestruct();
}

void UUmbraEquipmentSlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshVisual();
}

void UUmbraEquipmentSlotWidget::NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event)
{
	Super::NativeOnMouseEnter(Geometry, Event);
	SetHovered(true);
}

void UUmbraEquipmentSlotWidget::NativeOnMouseLeave(const FPointerEvent& Event)
{
	Super::NativeOnMouseLeave(Event);
	SetHovered(false);
}

FReply UUmbraEquipmentSlotWidget::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() == EKeys::RightMouseButton)
	{
		RequestUnequip();
		return FReply::Handled();
	}
	if (Event.GetEffectingButton() == EKeys::LeftMouseButton && !bLocked)
	{
		RequestSelection();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(Geometry, Event);
}
