#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Equipment/UmbraEquipmentSlot.h"
#include "UmbraItemDragDropOperation.generated.h"

class UUserWidget;
class UImage;
class AUmbraPlayerState;
class UUmbraInventorySlot;
class UUmbraEquipmentSlotWidget;

/** Captures identity, never ownership. Only an accepted drop mutates gameplay data. */
UCLASS()
class UMBRA_API UUmbraItemDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()
public:
	static UUmbraItemDragDropOperation* Begin(UUmbraInventorySlot* Source);
	static UUmbraItemDragDropOperation* Begin(UUmbraEquipmentSlotWidget* Source);
	bool DropOn(UUmbraInventorySlot* Target);
	bool DropOn(UUmbraEquipmentSlotWidget* Target);
private:
	static bool IsLive(UUmbraInventorySlot* Cell);
	static bool IsLive(UUmbraEquipmentSlotWidget* Cell);
	bool IsSourceCurrent() const;
	void SetVisual(UUserWidget* Source, UImage* Icon);
	bool Reject(UUserWidget* Target) const;
	TWeakObjectPtr<UUmbraInventorySlot> InventorySource;
	TWeakObjectPtr<UUmbraEquipmentSlotWidget> EquipmentSource;
	TWeakObjectPtr<AUmbraPlayerState> Owner;
	FGuid InstanceId;
	int32 InventoryIndex = INDEX_NONE;
	EUmbraEquipmentSlot EquipmentSlot = EUmbraEquipmentSlot::Head;
	bool bConsumed = false;
};
