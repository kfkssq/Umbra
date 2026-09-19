#pragma once

#include "CoreMinimal.h"
#include "UmbraAttributeDebugTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EUmbraAttributeDebugOperation : uint8
{
	AddEffect,
	RemoveEffect,
	Damage,
	Heal
};

/** Selection result passed to Blueprint so the WBP owns the displayed wording. */
UENUM(BlueprintType)
enum class EUmbraAttributeDebugFeedback : uint8
{
	Instructions,
	ViewingPlayer,
	EnemyLocked,
	EnemyLockFailed
};

/**
 * Read-only data for the attribute debug WBP. C++ observes GAS and supplies
 * values; percentage fields use display units while Blueprint owns presentation.
 */
USTRUCT(BlueprintType)
struct UMBRA_API FUmbraAttributeDebugViewState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug")
	FString TargetName;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug")
	bool bViewingPlayer = true;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug")
	bool bReady = false;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug")
	EUmbraAttributeDebugFeedback Feedback = EUmbraAttributeDebugFeedback::Instructions;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug|Attributes")
	float Health = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug|Attributes")
	float MaxHealth = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug|Attributes")
	float HealthRegen = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug|Attributes")
	float Resource = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug|Attributes")
	float MaxResource = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug|Attributes")
	float ResourceRegen = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug|Attributes")
	float AttackPower = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug|Attributes")
	float AbilityPower = 0.f;

	/** Percentage points; for example GAS ratio 0.2 is supplied as 20. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug|Attributes")
	float AttackSpeedBonus = 0.f;

	/** Percentage in [0, 100]; for example GAS ratio 1 is supplied as 100. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug|Attributes")
	float CriticalChance = 0.f;

	/** Total multiplier expressed as a percentage; for example GAS value 2 is supplied as 200. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug|Attributes")
	float CriticalDamageMultiplier = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug|Attributes")
	float Armor = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug|Attributes")
	float MagicResistance = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug|Attributes")
	float AbilityHaste = 0.f;

	/** Centimeters per second. */
	UPROPERTY(BlueprintReadOnly, Category = "Attribute Debug|Attributes")
	float MoveSpeed = 0.f;
};
