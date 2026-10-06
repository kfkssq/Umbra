#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Equipment/UmbraEquipmentSlot.h"
#include "Items/UmbraWeaponProfile.h"
#include "Items/UmbraItemClassification.h"
#include "AbilitySystem/Damage/UmbraDamageBonusComponent.h"
#include "UmbraItemDefinition.generated.h"

class UTexture2D;

/** New display-only identity. Explicit values are serialization-stable; append future values. */
UENUM(BlueprintType)
enum class EUmbraItemRarity : uint8 { Common = 0, Magic = 1, Rare = 2, Epic = 3, Legendary = 4, Unique = 5 };

/** Only attributes with existing gameplay. No direct AD/AP, legacy critical multiplier or placeholders. */
UENUM(BlueprintType)
enum class EUmbraEquipmentAffixStat : uint8
{
	MaxHealth, MaxResource, AttackSpeed, CriticalChance, Armor, MagicResistance, MoveSpeed,
	SlashingResistance, BluntResistance, PiercingResistance, FireResistance, LightningResistance,
	ColdResistance, RadiantResistance, PoisonResistance, ShadowResistance
};

USTRUCT(BlueprintType)
struct FUmbraEquipmentAffix
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EUmbraEquipmentAffixStat Stat = EUmbraEquipmentAffixStat::MaxHealth;
	/** Flat additive units; AttackSpeed +0.2 means +0.2 multiplier, CriticalChance +0.1 means +10 percentage points. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0")) float Magnitude = 0.f;
};

USTRUCT(BlueprintType)
struct FUmbraPrimaryValues
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0")) float Strength = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0")) float Dexterity = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0")) float Intelligence = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0")) float Faith = 0.f;
	TArray<float> ToArray() const { return {Strength, Dexterity, Intelligence, Faith}; }
};

/** Authored immutable prototype definition; no random rolls, inventory or save identity. */
UCLASS(BlueprintType)
class UMBRA_API UUmbraItemDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	/** Presentation classification only; Unknown preserves legacy profile/slot fallback. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Classification") EUmbraItemCategory ItemCategory = EUmbraItemCategory::Unknown;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Classification") EUmbraWeaponType WeaponType = EUmbraWeaponType::Unknown;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation", meta = (ClampMin = "1")) int32 ItemLevel = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") EUmbraItemRarity Rarity = EUmbraItemRarity::Common;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation", meta = (MultiLine = "true")) FText FlavorText;
	/** Authored presentation only; no rarity generation or combat behavior. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") TObjectPtr<UTexture2D> Icon;
	/** Authored quality art shared by equipment and inventory cells. Null keeps the cell's default background. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") TObjectPtr<UTexture2D> SlotBackgroundTexture;
	/** Null preserves the cell's legacy frame and RarityColor styling. Configured art is shown untinted. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") TObjectPtr<UTexture2D> RarityFrameTexture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") FLinearColor RarityColor = FLinearColor::White;
	/** Tooltip title background; null preserves the WBP designer brush. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") TObjectPtr<UTexture2D> TooltipTitleBackgroundTexture;
	/** Tooltip icon cell background; null preserves the WBP designer brush. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") TObjectPtr<UTexture2D> TooltipIconBackgroundTexture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment") TArray<EUmbraEquipmentSlot> AllowedSlots;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (ClampMin = "1")) int32 RequiredLevel = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment") FUmbraPrimaryValues Requirements;
	/** Always active, including when requirements fail. No conditional primary bonuses. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment") FUmbraPrimaryValues PrimaryBonuses;
	/** Authored fixed affixes. Unique Stat entries; always active even with unmet requirements. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment|Affixes") TArray<FUmbraEquipmentAffix> AttributeBonuses;
	/** Explicit A entries or independent X effects; same hit filters as Buff GEs, evaluated at level 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment|Affixes") TArray<FUmbraDamageBonus> DamageBonuses;
	/** This stage supports weapons in MainHand only; null means armor/accessory. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment") TObjectPtr<UUmbraWeaponProfile> Weapon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (ClampMin = "0")) float Armor = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (ClampMin = "0")) float MagicResistance = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (ClampMin = "0")) float Weight = 0.f;
};
