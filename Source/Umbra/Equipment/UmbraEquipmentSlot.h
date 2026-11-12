#pragma once

#include "CoreMinimal.h"
#include "UmbraEquipmentSlot.generated.h"

/** Shared gameplay/UI identity. Preserve enumerator order for existing Blueprint assets. */
UENUM(BlueprintType)
enum class EUmbraEquipmentSlot : uint8
{
	Head = 0, Chest = 1, Hands = 2, Legs = 3,
	// Retain the serialized identity so old shoes cannot silently become accessories.
	Feet = 4 UMETA(Hidden),
	Amulet = 5, Ring1 = 6, Ring2 = 7, MainHand = 8, OffHand = 9,
	Amulet2 = 10
};

inline bool IsUmbraEquipmentSlotActive(EUmbraEquipmentSlot Slot)
{
	return uint8(Slot) <= uint8(EUmbraEquipmentSlot::Amulet2) && Slot != EUmbraEquipmentSlot::Feet;
}

inline bool IsUmbraRelicSlot(EUmbraEquipmentSlot Slot)
{
	return Slot == EUmbraEquipmentSlot::Amulet || Slot == EUmbraEquipmentSlot::Amulet2
		|| Slot == EUmbraEquipmentSlot::Ring1 || Slot == EUmbraEquipmentSlot::Ring2;
}
