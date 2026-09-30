#include "UI/Inventory/UmbraInventoryMenu.h"

#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "UI/Inventory/UmbraInventorySlot.h"
#include "Umbra.h"

void UUmbraInventoryMenu::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshCapacityText();
}

void UUmbraInventoryMenu::NativeConstruct()
{
	Super::NativeConstruct();
	InitializeSlots();
}

void UUmbraInventoryMenu::NativeDestruct()
{
	UnbindSlots();
	// Retain cells and selection for the same UObject's next Construct.
	Super::NativeDestruct();
}

void UUmbraInventoryMenu::RefreshCapacityText()
{
	if (CapacityText) CapacityText->SetText(FText::Format(
		NSLOCTEXT("UmbraInventory", "EmptyCapacity", "0/{0}"), FText::AsNumber(GetInventoryCapacity())));
}

void UUmbraInventoryMenu::UnbindSlots()
{
	for (UUmbraInventorySlot* Cell : Slots)
	{
		if (Cell) Cell->OnSelectionRequested.RemoveDynamic(this, &ThisClass::HandleSelection);
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
	}
}

void UUmbraInventoryMenu::HandleSelection(UUmbraInventorySlot* RequestedSlot)
{
	if (!RequestedSlot || !Slots.Contains(RequestedSlot) || SelectedSlot == RequestedSlot) return;
	if (SelectedSlot) SelectedSlot->SetSelected(false);
	SelectedSlot = RequestedSlot;
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
