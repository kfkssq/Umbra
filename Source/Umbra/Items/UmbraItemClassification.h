#pragma once

#include "CoreMinimal.h"
#include "UmbraItemClassification.generated.h"

/** Stable categories. Accessory denotes relics and enables their four interchangeable equip slots. */
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
	Spear = 9, Halberd = 10, Staff = 11, Wand = 12, Bow = 13, Crossbow = 14,
	// Append-only presentation types; do not infer hand usage, damage or equip slots.
	Katana = 15,
	Twinblade = 16,
	HeavyThrustingSword = 17,
	Flail = 18,
	Scythe = 19,
	Fist = 20,
	Claw = 21,
	Whip = 22,
	ColossalWeapon = 23,
	Ballista = 24,
	SacredSeal = 25,
	SmallShield = 26,
	MediumShield = 27,
	Greatshield = 28,
	Torch = 29,
	HandToHand = 30,
	PerfumeBottle = 31,
	ThrustingShield = 32,
	ThrowingBlade = 33,
	BackhandBlade = 34,
	LightGreatsword = 35,
	GreatKatana = 36,
	BeastClaw = 37
};

class UUmbraItemDefinition;
namespace UmbraItemClassification
{
	UMBRA_API FText CategoryName(EUmbraItemCategory Category);
	UMBRA_API FText WeaponTypeName(EUmbraWeaponType Type);
	/** Non-fatal authored metadata diagnostics. Does not alter assets or equipment rules. */
	UMBRA_API FText Describe(const UUmbraItemDefinition& Item, TArray<FText>& Warnings);
}
