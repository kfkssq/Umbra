#include "UI/Inventory/UmbraInventorySlot.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"

void UUmbraInventorySlot::RequestSelection()
{
	OnSelectionRequested.Broadcast(this);
}

void UUmbraInventorySlot::SetSelected(bool bInSelected)
{
	bSelected = bInSelected;
	RefreshVisual();
}

void UUmbraInventorySlot::RefreshVisual()
{
	if (HighlightFrame) HighlightFrame->SetVisibility(ShouldHighlight()
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	BP_RefreshVisual(ShouldHighlight());
	// Empty is a data invariant, independent of Blueprint styling and interaction state.
	if (SlotBackground) SlotBackground->SetVisibility(ESlateVisibility::Visible);
	if (ItemIcon) ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
	if (RarityFrame) RarityFrame->SetVisibility(ESlateVisibility::Collapsed);
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
}

void UUmbraInventorySlot::NativeDestruct()
{
	bHovered = false;
	RefreshVisual();
	Super::NativeDestruct();
}

void UUmbraInventorySlot::NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event)
{
	Super::NativeOnMouseEnter(Geometry, Event);
	bHovered = true;
	RefreshVisual();
}

void UUmbraInventorySlot::NativeOnMouseLeave(const FPointerEvent& Event)
{
	Super::NativeOnMouseLeave(Event);
	bHovered = false;
	RefreshVisual();
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
