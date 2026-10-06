#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Equipment/UmbraEquipmentComponent.h"
#include "Inventory/UmbraInventoryComponent.h"
#include "UmbraItemTooltipData.generated.h"

UENUM(BlueprintType)
enum class EUmbraTooltipSource : uint8 { DefinitionPreview, Inventory, Equipment };
UENUM(BlueprintType)
enum class EUmbraTooltipResult : uint8 { Success, InvalidDefinition, InvalidSnapshot, IdentityMismatch, InvalidSlot, ContextUnavailable, InvalidConfiguration };
UENUM(BlueprintType)
enum class EUmbraTooltipStatSource : uint8 { Intrinsic, FixedAffix, RolledAffix };
UENUM(BlueprintType)
enum class EUmbraTooltipUnit : uint8 { Points, Percent, CentimetersPerSecond, Rating, Factor };
UENUM(BlueprintType)
enum class EUmbraTooltipRequirementState : uint8 { Unknown, LevelTooLow, PrimaryPenalty, Met };

UENUM(BlueprintType)
enum class EUmbraTooltipRowStyle : uint8 { Neutral, Met, Unmet, Unknown };

UENUM(BlueprintType)
enum class EUmbraTooltipEntryDisplayMode : uint8 { KeyValue, FullText };

UENUM(BlueprintType)
enum class EUmbraTooltipEntryStyle : uint8 { Neutral, Positive, Met, Unmet, Warning, Note, Damage, Scaling };

USTRUCT(BlueprintType)
struct FUmbraTooltipTextRow
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FText Text;
	UPROPERTY(BlueprintReadOnly) EUmbraTooltipRowStyle Style = EUmbraTooltipRowStyle::Neutral;
};

/** Structured presentation entry; C++ supplies semantics, WBP supplies typography and color. */
USTRUCT(BlueprintType)
struct FUmbraTooltipEntry
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FText LabelText;
	UPROPERTY(BlueprintReadOnly) FText ValueText;
	UPROPERTY(BlueprintReadOnly) FText FullText;
	UPROPERTY(BlueprintReadOnly) EUmbraTooltipEntryDisplayMode DisplayMode = EUmbraTooltipEntryDisplayMode::FullText;
	UPROPERTY(BlueprintReadOnly) EUmbraTooltipEntryStyle Style = EUmbraTooltipEntryStyle::Neutral;
};

/** Raw identity and condition data remain separate from presentation for future comparison. */
USTRUCT(BlueprintType)
struct FUmbraTooltipStatLine
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FName StatId;
	UPROPERTY(BlueprintReadOnly) double RawValue = 0.;
	UPROPERTY(BlueprintReadOnly) EUmbraTooltipUnit Unit = EUmbraTooltipUnit::Points;
	UPROPERTY(BlueprintReadOnly) EUmbraTooltipStatSource Source = EUmbraTooltipStatSource::FixedAffix;
	UPROPERTY(BlueprintReadOnly) bool bDamageBonus = false;
	/** Includes bucket, type restrictions, attack source, crit/vulnerable and both tag queries. */
	UPROPERTY(BlueprintReadOnly) FUmbraDamageBonus DamageBonus;
	UPROPERTY(BlueprintReadOnly) FText Text;
};

USTRUCT(BlueprintType)
struct FUmbraTooltipDamageChannel
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) EUmbraWeaponDamageType Type = EUmbraWeaponDamageType::Blunt;
	UPROPERTY(BlueprintReadOnly) FText Name;
	UPROPERTY(BlueprintReadOnly) double BaseDamage = 0.;
	UPROPERTY(BlueprintReadOnly) FText Text;
};

USTRUCT(BlueprintType)
struct FUmbraTooltipScaling
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) EUmbraPrimaryAttribute Attribute = EUmbraPrimaryAttribute::Strength;
	UPROPERTY(BlueprintReadOnly) double ReferenceCoefficient = 0.;
	UPROPERTY(BlueprintReadOnly) EUmbraScalingGrade Grade = EUmbraScalingGrade::None;
	UPROPERTY(BlueprintReadOnly) bool bManual = false;
	UPROPERTY(BlueprintReadOnly) bool bHasCurve = false;
	UPROPERTY(BlueprintReadOnly) FText Text;
};

/** Central project configuration in DefaultGame.ini; overrides are display-only. */
UCLASS(Config = Game, DefaultConfig)
class UMBRA_API UUmbraTooltipSettings : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(Config, EditAnywhere, Category = "Scaling") double S = 1.0;
	UPROPERTY(Config, EditAnywhere, Category = "Scaling") double A = 0.8;
	UPROPERTY(Config, EditAnywhere, Category = "Scaling") double B = 0.6;
	UPROPERTY(Config, EditAnywhere, Category = "Scaling") double C = 0.4;
	bool IsValidThresholds() const;
	EUmbraScalingGrade GradeFor(double Coefficient) const;
};

USTRUCT(BlueprintType)
struct FUmbraItemTooltipData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) bool bValid = false;
	UPROPERTY(BlueprintReadOnly) EUmbraTooltipResult Result = EUmbraTooltipResult::InvalidDefinition;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UUmbraItemDefinition> ItemDefinition;
	UPROPERTY(BlueprintReadOnly) FGuid InstanceId;
	UPROPERTY(BlueprintReadOnly) EUmbraTooltipSource Source = EUmbraTooltipSource::DefinitionPreview;
	UPROPERTY(BlueprintReadOnly) bool bEquipped = false;
	UPROPERTY(BlueprintReadOnly) int32 InventorySlot = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) bool bHasTargetSlot = false;
	UPROPERTY(BlueprintReadOnly) EUmbraEquipmentSlot TargetSlot = EUmbraEquipmentSlot::Head;
	UPROPERTY(BlueprintReadOnly) FText DisplayName;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> Icon;
	UPROPERTY(BlueprintReadOnly) EUmbraItemCategory ItemCategory = EUmbraItemCategory::Unknown;
	UPROPERTY(BlueprintReadOnly) EUmbraWeaponType WeaponType = EUmbraWeaponType::Unknown;
	/** Localized specific type, explicit category, or reliable legacy fallback. */
	UPROPERTY(BlueprintReadOnly) FText ItemType;
	UPROPERTY(BlueprintReadOnly) TArray<FText> ClassificationWarnings;
	UPROPERTY(BlueprintReadOnly) FText RarityText;
	UPROPERTY(BlueprintReadOnly) FText RarityAndTypeText;
	UPROPERTY(BlueprintReadOnly) FText ItemLevelText;
	UPROPERTY(BlueprintReadOnly) FText TotalDamageText;
	UPROPERTY(BlueprintReadOnly) FText ScalingNote;
	UPROPERTY(BlueprintReadOnly) TArray<FUmbraTooltipTextRow> RequirementRows;
	UPROPERTY(BlueprintReadOnly) FText RequirementStatusText;
	UPROPERTY(BlueprintReadOnly) FText WeightText;
	UPROPERTY(BlueprintReadOnly) EUmbraItemRarity Rarity = EUmbraItemRarity::Common;
	UPROPERTY(BlueprintReadOnly) int32 ItemLevel = 1;
	UPROPERTY(BlueprintReadOnly) FLinearColor RarityColor = FLinearColor::White;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> TooltipTitleBackgroundTexture;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> TooltipIconBackgroundTexture;
	UPROPERTY(BlueprintReadOnly) double TotalBaseDamage = 0.;
	UPROPERTY(BlueprintReadOnly) TArray<FUmbraTooltipDamageChannel> DamageChannels;
	/** Structured presentation rows projected from the raw channels/scaling/stats above. */
	UPROPERTY(BlueprintReadOnly) TArray<FUmbraTooltipEntry> DamageEntries;
	UPROPERTY(BlueprintReadOnly) TArray<FUmbraTooltipEntry> ScalingEntries;
	UPROPERTY(BlueprintReadOnly) TArray<FUmbraTooltipEntry> StatEntries;
	/** Four entries in primary enum order. Hide None; coefficients are reference strength only. */
	UPROPERTY(BlueprintReadOnly) TArray<FUmbraTooltipScaling> Scaling;
	UPROPERTY(BlueprintReadOnly) TArray<FUmbraTooltipStatLine> Stats;
	UPROPERTY(BlueprintReadOnly) int32 RequiredLevel = 1;
	UPROPERTY(BlueprintReadOnly) FUmbraPrimaryValues RequiredPrimaries;
	UPROPERTY(BlueprintReadOnly) FUmbraEquipmentRequirementsPreview Requirements;
	UPROPERTY(BlueprintReadOnly) EUmbraTooltipRequirementState RequirementState = EUmbraTooltipRequirementState::Unknown;
	/** Equipment snapshots can establish aggregate primary status even without live GAS detail. */
	UPROPERTY(BlueprintReadOnly) bool bPrimaryStatusKnown = false;
	UPROPERTY(BlueprintReadOnly) bool bRequirementsMet = false;
	UPROPERTY(BlueprintReadOnly) bool bHasEquipmentPenalty = false;
	UPROPERTY(BlueprintReadOnly) FText PenaltyText;
	UPROPERTY(BlueprintReadOnly) float Weight = 0.f;
	UPROPERTY(BlueprintReadOnly) FText FlavorText;
	/** Reserved presentation extension; builder never invents special effects. */
	UPROPERTY(BlueprintReadOnly) TArray<FText> SpecialEffects;
};

/** Stateless, read-only; every failure returns a fresh empty data object with an explicit result. */
UCLASS()
class UMBRA_API UUmbraItemTooltipDataBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "Umbra|Item Tooltip")
	static FUmbraItemTooltipData FromDefinition(UUmbraItemDefinition* Definition);
	UFUNCTION(BlueprintPure, Category = "Umbra|Item Tooltip")
	static FUmbraItemTooltipData FromInventory(const FUmbraInventorySnapshot& Snapshot, int32 SlotIndex,
		FGuid ExpectedId, UUmbraItemDefinition* ExpectedDefinition, const UUmbraEquipmentComponent* Equipment,
		bool bHasTargetSlot, EUmbraEquipmentSlot TargetSlot);
	UFUNCTION(BlueprintPure, Category = "Umbra|Item Tooltip")
	static FUmbraItemTooltipData FromEquipment(const FUmbraEquipmentSnapshot& Snapshot, EUmbraEquipmentSlot Slot,
		FGuid ExpectedId, UUmbraItemDefinition* ExpectedDefinition, const UUmbraEquipmentComponent* Equipment);
};

namespace UmbraTooltipFormatting
{
	UMBRA_API FText RarityName(EUmbraItemRarity Rarity);
	/** Formats already computed values and states; no gameplay queries or requirement comparisons. */
	UMBRA_API void BuildDisplayText(FUmbraItemTooltipData& Data);
	UMBRA_API FText DamageName(EUmbraWeaponDamageType Type);
	UMBRA_API FText StatName(FName Id);
	UMBRA_API FText Format(const FUmbraTooltipStatLine& Line);
	/** Signed value with unit, without the stat name (used for KeyValue entries). */
	UMBRA_API FText StatValue(const FUmbraTooltipStatLine& Line);
}
