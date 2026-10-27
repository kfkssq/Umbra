#pragma once

#include "CoreMinimal.h"
#include "Items/UmbraItemDefinition.h"
#include "Engine/EngineTypes.h"
#include "UmbraAudioTypes.generated.h"

UENUM(BlueprintType)
enum class EUmbraAudioEvent : uint8
{
	UIHover, UIClick, PageChanged, MenuOpen, MenuClose, EquipmentSelected, InventorySelected,
	TooltipShown, OperationFailed, EquipSuccess, UnequipSuccess, EquipFailed, InventoryTransfer,
	LootSpawn, LootLand,
	/** Persistent spatial loop owned by the world item; not a one-shot rarity announcement. */
	LootRarity, PickupSuccess, PickupFailed,
	AttackSwing, Hit, Critical, Hurt, Kill, Death, SkillCast, SkillHit, Block, Dodge,
	AttackVoice, Footstep, Clothing, Ambience,
	/** Attacker voice after a confirmed critical damage result; keep existing enum values stable. */
	CriticalVoice
};

UENUM(BlueprintType)
enum class EUmbraAudioCategory : uint8 { UI, Combat, Character, Equipment, Loot, Ambience };

USTRUCT(BlueprintType)
struct UMBRA_API FUmbraAudioContext
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EUmbraWeaponType Weapon = EUmbraWeaponType::Unknown;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bHasRarity = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EUmbraItemRarity Rarity = EUmbraItemRarity::Common;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TEnumAsByte<EPhysicalSurface> Surface = SurfaceType_Default;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName CharacterType;
	static FUmbraAudioContext FromItem(const UUmbraItemDefinition* Item);
};
