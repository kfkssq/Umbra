#include "UI/Inventory/UmbraInventorySlot.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"

void UUmbraInventorySlot::SetItemDefinition(UUmbraItemDefinition* Definition, FGuid InstanceId)
{
	bFromInventorySnapshot = false;
	CurrentItem = FUmbraEquipmentItemDisplay::FromDefinition(Definition, InstanceId);
	RefreshVisual();
	if (bHovered) OnTooltipHoverChanged.Broadcast(this, true);
}

void UUmbraInventorySlot::ClearItem()
{
	bFromInventorySnapshot = false;
	CurrentItem = FUmbraEquipmentItemDisplay();
	RefreshVisual();
	if (bHovered) OnTooltipHoverChanged.Broadcast(this, true);
}

void UUmbraInventorySlot::RequestSelection()
{
	OnSelectionRequested.Broadcast(this);
}

void UUmbraInventorySlot::RequestEquip()
{
	if (HasItem()) OnEquipRequested.Broadcast(this);
}

FReply UUmbraInventorySlot::NativeOnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		RequestSelection();
		RequestEquip();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDoubleClick(Geometry, Event);
}

void UUmbraInventorySlot::SetSelected(bool bInSelected)
{
	bSelected = bInSelected;
	RefreshVisual();
}

void UUmbraInventorySlot::RefreshVisual()
{
	ItemVisual.CaptureDefaults(SlotBackground, RarityFrame);
	if (HighlightFrame) HighlightFrame->SetVisibility(ShouldHighlight()
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	BP_RefreshVisual(ShouldHighlight());
	ItemVisual.Apply(CurrentItem, ItemIcon, SlotBackground, RarityFrame);
	// Quantity and equipment ownership are not inferred from presentation data.
	if (StackCountText) StackCountText->SetVisibility(ESlateVisibility::Collapsed);
	if (EquippedMarker) EquippedMarker->SetVisibility(ESlateVisibility::Collapsed);
}

void UUmbraInventorySlot::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshVisual();
}

void UUmbraInventorySlot::NativeConstruct()
{
	Super::NativeConstruct();
	bHovered = false;
	RefreshVisual();
	OnTooltipHoverChanged.Broadcast(this, false);
	SetToolTip(nullptr);
}

void UUmbraInventorySlot::NativeDestruct()
{
	bHovered = false;
	RefreshVisual();
	OnTooltipHoverChanged.Broadcast(this, false);
	SetToolTip(nullptr);
	Super::NativeDestruct();
}

void UUmbraInventorySlot::NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event)
{
	Super::NativeOnMouseEnter(Geometry, Event);
	bHovered = true;
	RefreshVisual();
	OnTooltipHoverChanged.Broadcast(this, true);
}

void UUmbraInventorySlot::NativeOnMouseLeave(const FPointerEvent& Event)
{
	Super::NativeOnMouseLeave(Event);
	bHovered = false;
	RefreshVisual();
	OnTooltipHoverChanged.Broadcast(this, false);
	SetToolTip(nullptr);
}

FReply UUmbraInventorySlot::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		RequestSelection();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(Geometry, Event);
}
