#pragma once

#include "CoreMinimal.h"
#include "UmbraEquipmentSlot.generated.h"

/** Shared gameplay/UI identity. Preserve enumerator order for existing Blueprint assets. */
UENUM(BlueprintType)
enum class EUmbraEquipmentSlot : uint8
{
	Head, Chest, Hands, Legs, Feet, Amulet, Ring1, Ring2, MainHand, OffHand
};
