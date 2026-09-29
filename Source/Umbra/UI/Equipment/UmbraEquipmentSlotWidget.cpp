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
	CurrentItem = InItem.Item ? InItem : FUmbraEquipmentItemDisplay();
	RefreshVisual();
}

void UUmbraEquipmentSlotWidget::ClearItem()
{
	CurrentItem = FUmbraEquipmentItemDisplay();
	RefreshVisual();
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
}

void UUmbraEquipmentSlotWidget::RequestSelection()
{
	if (!bLocked) OnSelectionRequested.Broadcast(this);
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
	if (EmptyIcon)
	{
		const TObjectPtr<UTexture2D>* Texture = EmptySlotIcons.Find(SlotType);
		// Clear missing entries as well, so changing SlotType cannot leave the previous icon.
		EmptyIcon->SetBrushFromTexture(Texture ? Texture->Get() : nullptr);
		EmptyIcon->SetVisibility(!HasItem() && Texture && Texture->Get()
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (ItemIcon)
	{
		ItemIcon->SetBrush(CurrentItem.Icon);
		ItemIcon->SetColorAndOpacity(FLinearColor::White);
		ItemIcon->SetVisibility(HasItem() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (RarityFrame) RarityFrame->SetColorAndOpacity(HasItem() ? CurrentItem.RarityColor : FLinearColor::White);
	if (LockedOverlay) LockedOverlay->SetVisibility(bLocked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	BP_RefreshVisual(GetVisualState(), CurrentItem, HasItem());
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
	if (Event.GetEffectingButton() == EKeys::LeftMouseButton && !bLocked)
	{
		RequestSelection();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(Geometry, Event);
}
