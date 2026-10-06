#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/UmbraWeaponProfile.h"
#include "UmbraTypedDamage.generated.h"

class UCurveFloat;
class UAbilitySystemComponent;
struct FGameplayEffectSpec;

UENUM(BlueprintType)
enum class EUmbraDamageModel : uint8 { Legacy, WeaponChannels, ExplicitChannels };

USTRUCT(BlueprintType)
struct FUmbraAttackDamageChannel
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EUmbraWeaponDamageType Type = EUmbraWeaponDamageType::Blunt;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0")) float BaseDamage = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0")) float AttackPowerCoefficient = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0")) float AbilityPowerCoefficient = 0.f;
};

/** Shared tuning asset. Null config uses this class's CDO; level curves use the defender's level. */
UCLASS(BlueprintType)
class UMBRA_API UUmbraDamageRules : public UDataAsset
{
	GENERATED_BODY()
public:
	/** New buckets only: separate base mechanisms, never A bonuses. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Buckets", meta = (ClampMin = "1")) float BaseCriticalMultiplier = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Buckets", meta = (ClampMin = "1")) float BaseVulnerableMultiplier = 1.2f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defense", meta = (ClampMin = "0.001")) float DefenseK = 100.f;
	/** Optional X=defender level, Y=positive K. Overrides DefenseK. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defense") TObjectPtr<UCurveFloat> DefenseKByLevel;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defense", meta = (ClampMin = "0.001")) float TypeResistanceK = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defense", meta = (ClampMin = "0", ClampMax = "1")) float MaxTypeReduction = 0.3f;
};

USTRUCT(BlueprintType)
struct FUmbraTypedDamageConfig
{
	GENERATED_BODY()
	/** Legacy preserves all existing GE and coefficient behavior. Opt in per attack ability. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) EUmbraDamageModel Model = EUmbraDamageModel::Legacy;
	/** WeaponChannels ONLY: multiplies the already-derived nine-channel profile once. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0")) float WeaponMultiplier = 1.f;
	/** ExplicitChannels ONLY: no implicit weapon damage is added. Unique types, at most nine. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TArray<FUmbraAttackDamageChannel> Channels;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) bool bCanCritical = true;
	/** Opt-in migration. False retains phase-four total crit multiplier and ignores A/X/vulnerability. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) bool bUseDamageBuckets = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UUmbraDamageRules> Rules;
};

namespace UmbraTypedDamage
{
	/** Private spec payload names; every channel is explicitly supplied, including zeros. */
	FName BaseName(int32 Index);
	FName ADName(int32 Index);
	FName APName(int32 Index);
	inline const FName DefenseKName(TEXT("Umbra.Typed.DefenseK"));
	inline const FName ResistanceKName(TEXT("Umbra.Typed.ResistanceK"));
	inline const FName ReductionCapName(TEXT("Umbra.Typed.ReductionCap"));
	inline const FName CanCriticalName(TEXT("Umbra.Typed.CanCritical"));
	inline const FName ReadyName(TEXT("Umbra.Typed.Ready"));
	inline const FName BucketsName(TEXT("Umbra.Typed.Buckets"));
	inline const FName BaseCriticalName(TEXT("Umbra.Typed.BaseCritical"));
	inline const FName BaseVulnerableName(TEXT("Umbra.Typed.BaseVulnerable"));
	bool Prepare(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target,
		const FUmbraTypedDamageConfig& Config, FGameplayEffectSpec& Spec);
}
