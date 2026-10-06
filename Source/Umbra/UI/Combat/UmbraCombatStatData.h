#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "UmbraCombatStatData.generated.h"

class AUmbraPlayerState;

/** Stable row identity, independent of the existing first-page Stat enum. Append only. */
UENUM(BlueprintType)
enum class EUmbraCombatStat : uint8
{
	None,
	BaseWeaponDamage,
	AttackPower,
	AbilityPower,
	AttackSpeed,
	CriticalChance,
	CriticalDamage,
	VulnerableDamage,
	AllDamage,
	SlashingDamage,
	BluntDamage,
	PiercingDamage,
	FireDamage,
	LightningDamage,
	ColdDamage,
	RadiantDamage,
	PoisonDamage,
	ShadowDamage,
	Thorn,
	MaxHealth,
	Armor,
	MagicResist,
	SlashingResistance,
	BluntResistance,
	PiercingResistance,
	FireResistance,
	LightningResistance,
	ColdResistance,
	RadiantResistance,
	PoisonResistance,
	ShadowResistance,
	DodgeChance,
	BlockChance,
	BlockReduction,
	ShieldStrength,
	MaxResource,
	ResourceRegeneration,
	HealthRegeneration,
	MoveSpeed,
	AbilityHaste,
	EquipLoad,
	MaxEquipLoad,
	HealingBonus,
	ShieldGeneration
};

UENUM(BlueprintType)
enum class EUmbraCombatStatStatus : uint8 { Unavailable, Live, StoredOnly, Placeholder };

USTRUCT(BlueprintType)
struct FUmbraCombatStatValue
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) EUmbraCombatStatStatus Status = EUmbraCombatStatStatus::Unavailable;
	/** Raw units: fractions for percent rows; attacks/sec for AttackSpeed; ratings for resistances. */
	UPROPERTY(BlueprintReadOnly) double Value = 0.;
	UPROPERTY(BlueprintReadOnly) FText Text;
	UPROPERTY(BlueprintReadOnly) FText Explanation;
	UPROPERTY(BlueprintReadOnly) bool bHasAdditionalConditionalBonuses = false;
	UPROPERTY(BlueprintReadOnly) bool bHasMultiplicativeBonuses = false;
};

namespace UmbraCombatStats
{
	/** Shared localized numeric rendering. Percent inputs remain fractions, never factors. */
	UMBRA_API FText FormatNumber(double Value, bool bPercent, int32 MinDigits = 0, int32 MaxDigits = 1);
	FGameplayAttribute AttributeFor(EUmbraCombatStat Stat);
	FUmbraCombatStatValue Read(const AUmbraPlayerState* PlayerState, EUmbraCombatStat Stat);
}

