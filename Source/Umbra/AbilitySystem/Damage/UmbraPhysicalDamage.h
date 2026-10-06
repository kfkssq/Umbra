#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Damage/UmbraTypedDamage.h"
#include "UmbraPhysicalDamage.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

UENUM(BlueprintType)
enum class EUmbraDamageType : uint8
{
	Physical = 0,
	Magical = 1
};

USTRUCT(BlueprintType)
struct FUmbraPhysicalDamageConfig
{
	GENERATED_BODY()

	/** Opt-in typed modes use a native GE; the fields below remain legacy-only. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	FUmbraTypedDamageConfig Typed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	EUmbraDamageType DamageType = EUmbraDamageType::Physical;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0"))
	float AbilityPowerCoefficient = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0"))
	float AttackPowerCoefficient = 1.f;
};

namespace UmbraPhysicalDamage
{
	/** Compatibility adapter to UmbraDamage. False preserves an unspecified source, not Skill.
	 * True preserves the player-primary measurement marker. New callers classify their source explicitly.
	 */
	bool Apply(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target,
		TSubclassOf<UGameplayEffect> EffectClass, const FUmbraPhysicalDamageConfig& Config, float Level,
		bool bPrimaryAttack = false);
	bool IsLoggingEnabled();
}

