#pragma once

#include "CoreMinimal.h"
#include "UmbraItemClassification.generated.h"

/** Display-only identities. Preserve explicit values when adding types. */
UENUM(BlueprintType)
enum class EUmbraItemCategory : uint8
{
	Unknown = 0, Weapon = 1, Armor = 2, Accessory = 3, Consumable = 4, Material = 5, Misc = 6
};

UENUM(BlueprintType)
enum class EUmbraWeaponType : uint8
{
	Unknown = 0, OneHandedSword = 1, TwoHandedSword = 2, Dagger = 3, Rapier = 4,
	OneHandedAxe = 5, TwoHandedAxe = 6, OneHandedMace = 7, TwoHandedMace = 8,
	Spear = 9, Halberd = 10, Staff = 11, Wand = 12, Bow = 13, Crossbow = 14
};

class UUmbraItemDefinition;
namespace UmbraItemClassification
{
	UMBRA_API FText CategoryName(EUmbraItemCategory Category);
	UMBRA_API FText WeaponTypeName(EUmbraWeaponType Type);
	/** Non-fatal authored metadata diagnostics. Does not alter assets or equipment rules. */
	UMBRA_API FText Describe(const UUmbraItemDefinition& Item, TArray<FText>& Warnings);
}
