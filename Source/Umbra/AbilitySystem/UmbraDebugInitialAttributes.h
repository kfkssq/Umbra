#pragma once

#include "CoreMinimal.h"
#include "UmbraDebugInitialAttributes.generated.h"

/** Editor tuning data, applied once through an Instant GE. Not progression base stats. */
USTRUCT(BlueprintType)
struct UMBRA_API FUmbraDebugInitialAttributes
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0"))
	float MaxResource = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0"))
	float HealthRegen = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0"))
	float ResourceRegen = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0"))
	float AttackPower = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0"))
	float AbilityPower = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0"))
	float AttackSpeedBonus = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CriticalChance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "1.0"))
	float CriticalDamageMultiplier = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0"))
	float Armor = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0"))
	float MagicResistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0"))
	float AbilityHaste = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0"))
	float MoveSpeed = 600.f;
};

