#pragma once

#include "CoreMinimal.h"
#include "UI/Inventory/UmbraInventoryMenu.h"
#include "UI/Inventory/UmbraInventorySlot.h"
#include "UmbraInventoryTestTypes.generated.h"

UCLASS(Transient, NotBlueprintable)
class UUmbraInventorySlotTestWidget : public UUmbraInventorySlot
{
	GENERATED_BODY()
public:
	void Configure(UImage* Background, UImage* Icon, UImage* Rarity, UTextBlock* Count, UImage* Equipped, UImage* Highlight)
	{
		SlotBackground = Background; ItemIcon = Icon; RarityFrame = Rarity;
		StackCountText = Count; EquippedMarker = Equipped; HighlightFrame = Highlight;
	}
	void EnterForTest() { NativeOnMouseEnter(FGeometry(), FPointerEvent()); }
	void LeaveForTest() { NativeOnMouseLeave(FPointerEvent()); }
};

UCLASS(Transient, NotBlueprintable)
class UUmbraInventoryMenuTestWidget : public UUmbraInventoryMenu
{
	GENERATED_BODY()
public:
	void Configure(UUniformGridPanel* Grid, UTextBlock* Text, int32 Capacity = 40, int32 InColumns = 8)
	{
		InventoryGrid = Grid; CapacityText = Text;
		InventoryCapacity = Capacity; Columns = InColumns;
		InventorySlotClass = UUmbraInventorySlotTestWidget::StaticClass();
	}
	void ClearClassForTest() { InventorySlotClass = nullptr; }
	void ConstructForTest() { NativeConstruct(); }
	void DestructForTest() { NativeDestruct(); }
};

