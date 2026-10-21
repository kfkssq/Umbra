#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Equipment/UmbraEquipmentSlot.h"
#include "Items/UmbraWeaponProfile.h"
#include "Items/UmbraItemClassification.h"
#include "AbilitySystem/Damage/UmbraDamageBonusComponent.h"
#include "UmbraItemDefinition.generated.h"

class UTexture2D;
class UStaticMesh;
class USkeletalMesh;

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

UENUM(BlueprintType)
enum class EUmbraAffixKind : uint8 { Primary, Attribute, Damage };

/** Identity and gameplay target are copied into the instance when created. */
USTRUCT(BlueprintType)
struct FUmbraAffixIdentity
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName AffixId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EUmbraAffixKind Kind = EUmbraAffixKind::Attribute;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EUmbraPrimaryAttribute Primary = EUmbraPrimaryAttribute::Strength;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EUmbraEquipmentAffixStat Attribute = EUmbraEquipmentAffixStat::MaxHealth;
	/** Magnitude is supplied by the roll, never by this template's Magnitude field. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FUmbraDamageBonus DamageBonus;
};

USTRUCT(BlueprintType)
struct FUmbraAffixRollConfig : public FUmbraAffixIdentity
{
	GENERATED_BODY()
	/** Native units: attack speed/crit/A use fractions; X uses a factor (1.2 = +20%). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MinValue = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MaxValue = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "5", ClampMax = "6")) int32 TierCount = 6;
	/** Overrides uniform tiers; values must be finite, ascending, unique and within Min/Max. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<float> ExplicitValues;
	/** Round uniform candidates to integral native units. Explicit candidates must already be integral. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bIntegerValues = false;
	bool BuildCandidates(TArray<float>& Out) const;
};

USTRUCT(BlueprintType)
struct FUmbraRolledAffix : public FUmbraAffixIdentity
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) float Value = 0.f;
	/** Effective candidates at creation, independent of later definition tuning. */
	UPROPERTY(BlueprintReadOnly) TArray<float> CandidateValues;
	UPROPERTY(BlueprintReadOnly) int32 SelectedTier = INDEX_NONE;
};

/** Authored immutable definition. Generated values belong to FUmbraItemInstance. */
UCLASS(BlueprintType)
class UMBRA_API UUmbraItemDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	/** Editor-only authoring action. Fits the proxy to the transformed reference-pose bounds; does not save assets. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Presentation|Drop Physics") void FitWorldDropPhysics();
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
	/** Visual only. Null leaves the world label and independent pickup sphere available. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") TSoftObjectPtr<UStaticMesh> WorldDropMesh;
	/** Reference-pose model; takes precedence over WorldDropMesh and uses the asset's materials. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") TSoftObjectPtr<USkeletalMesh> WorldDropSkeletalMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") FVector WorldDropScale = FVector::OneVector;
	/** Model-specific orientation in degrees; no weapon-type assumptions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") FRotator WorldDropRotation = FRotator::ZeroRotator;
	/** Actor-local offset in cm; adjust for the imported model's pivot and ground clearance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") FVector WorldDropOffset = FVector::ZeroVector;
	/** Half-size in cm along the proxy's own axes, after visual scale. Independent of asset collision. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Drop Physics", meta=(ClampMin="1", ClampMax="200", Units="cm")) FVector WorldDropPhysicsExtent = FVector(20, 12, 8);
	/** Proxy center relative to the old drop origin, after the visual transform and armor pivot correction (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Drop Physics", meta=(Units="cm")) FVector WorldDropPhysicsCenter = FVector(0, 0, 8);
	/** Proxy axes relative to the original drop origin; avoids a large axis-aligned box around diagonal weapons. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Drop Physics") FRotator WorldDropPhysicsRotation = FRotator::ZeroRotator;
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
	/** Additional, distinct affixes. Do not also author the same grant in the fixed arrays. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment|Affixes") TArray<FUmbraAffixRollConfig> AffixRollConfigs;
	/** This stage supports weapons in MainHand only; null means armor/accessory. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment") TObjectPtr<UUmbraWeaponProfile> Weapon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (ClampMin = "0")) float Armor = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (ClampMin = "0")) float MagicResistance = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (ClampMin = "0")) float Weight = 0.f;
};
