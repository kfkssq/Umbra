#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UmbraWeaponProfile.generated.h"

class UCurveFloat;

UENUM(BlueprintType)
enum class EUmbraScalingGrade : uint8 { None, D, C, B, A, S };

USTRUCT(BlueprintType)
struct FUmbraScalingGradeOverride
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bOverride = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bOverride")) EUmbraScalingGrade Grade = EUmbraScalingGrade::None;
};

/** Weapon data identifiers, not the legacy physical/magical execution enum. */
UENUM(BlueprintType)
enum class EUmbraWeaponDamageType : uint8
{
	Slashing, Blunt, Piercing, Fire, Lightning, Cold, Radiant, Poison, Shadow
};

UENUM(BlueprintType)
enum class EUmbraPrimaryAttribute : uint8
{
	Strength, Dexterity, Intelligence, Faith
};

USTRUCT(BlueprintType)
struct FUmbraWeaponScaling
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EUmbraPrimaryAttribute Attribute = EUmbraPrimaryAttribute::Strength;
	/** Damage points per primary point, or per curve output when PointCurve is assigned. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	float Coefficient = 0.f;
	/** Optional: X = primary points, Y = nonnegative scaling units. Null means linear points. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCurveFloat> PointCurve;
};

USTRUCT(BlueprintType)
struct FUmbraWeaponDamageChannel
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EUmbraWeaponDamageType Type = EUmbraWeaponDamageType::Blunt;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	float BaseDamage = 0.f;
	/** At most one entry per primary attribute in this channel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FUmbraWeaponScaling> Scaling;
};

USTRUCT(BlueprintType)
struct FUmbraWeaponDamageProfile
{
	GENERATED_BODY()
	/** At most one entry per damage type; empty is an explicitly zero-damage profile. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FUmbraWeaponDamageChannel> Channels;
};

/** Damage-only prototype data. Inventory, equip requirements and affixes are later stages. */
UCLASS(BlueprintType)
class UMBRA_API UUmbraWeaponProfile : public UDataAsset
{
	GENERATED_BODY()
public:
	/** Display only. Explicit override permits hiding a grade with None. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tooltip") FUmbraScalingGradeOverride StrengthGrade;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tooltip") FUmbraScalingGradeOverride DexterityGrade;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tooltip") FUmbraScalingGradeOverride IntelligenceGrade;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Tooltip") FUmbraScalingGradeOverride FaithGrade;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FUmbraWeaponDamageProfile Damage;
};
