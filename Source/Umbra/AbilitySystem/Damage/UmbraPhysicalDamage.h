#pragma once

#include "CoreMinimal.h"
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	EUmbraDamageType DamageType = EUmbraDamageType::Physical;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0"))
	float AbilityPowerCoefficient = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0"))
	float AttackPowerCoefficient = 1.f;
};

namespace UmbraPhysicalDamage
{
	/** Creates a fresh hit spec on authority. Rejects legacy/mixed damage definitions. */
	bool Apply(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target,
		TSubclassOf<UGameplayEffect> EffectClass, const FUmbraPhysicalDamageConfig& Config, float Level,
		bool bPrimaryAttack = false);
	bool IsLoggingEnabled();
}

