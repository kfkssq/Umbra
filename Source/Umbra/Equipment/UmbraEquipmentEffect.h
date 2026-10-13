#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AbilitySystem/Damage/UmbraDamageBonusComponent.h"
#include "UmbraEquipmentEffect.generated.h"

class UUmbraItemDefinition;
enum class EUmbraEquipmentAffixStat : uint8;
struct FUmbraRolledAffix;

/** Static GE component; each spec refers to its own immutable ItemDefinition, never mutates the CDO. */
UCLASS()
class UMBRA_API UUmbraEquipmentDamageBonusComponent : public UUmbraDamageBonusComponent
{
	GENERATED_BODY()
public:
	virtual TArray<FUmbraDamageBonus> GetBonuses(const FGameplayEffectSpec& Spec) const override;
	virtual bool Validate(const FGameplayEffectSpec& Spec) const override;
};

/** Stable replicated GE definition. Each equipped instance owns exactly one handle. */
UCLASS()
class UMBRA_API UUmbraEquipmentEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UUmbraEquipmentEffect();
	static TArray<FGameplayAttribute> Attributes();
	static TArray<FGameplayTag> MagnitudeTags();
	static int32 AffixIndex(EUmbraEquipmentAffixStat Stat);
	static bool ValidateBonuses(const UUmbraItemDefinition* Item);
	static bool ApplyRolledAffixes(FGameplayEffectSpec& Spec, const TArray<FUmbraRolledAffix>& Rolls);
};
