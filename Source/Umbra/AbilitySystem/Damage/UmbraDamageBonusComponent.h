#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "GameplayEffectTypes.h"
#include "ScalableFloat.h"
#include "Items/UmbraWeaponProfile.h"
#include "UmbraDamageBonusComponent.generated.h"

class UAbilitySystemComponent;

UENUM(BlueprintType)
enum class EUmbraDamageBucket : uint8 { Additive, Multiplicative };
UENUM(BlueprintType)
enum class EUmbraBonusAttackSource : uint8 { Any, BasicAttack, Skill };

USTRUCT(BlueprintType)
struct FUmbraDamageBonus
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) EUmbraDamageBucket Bucket = EUmbraDamageBucket::Additive;
	/** A: fraction (0.1 = +10%). X: factor (1.1 = x1.1). GE level evaluates the curve. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FScalableFloat Magnitude = FScalableFloat(0.f);
	/** Empty means every type. A single matching entry contributes only once per channel. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TArray<EUmbraWeaponDamageType> Types;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) EUmbraBonusAttackSource AttackSource = EUmbraBonusAttackSource::Any;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) bool bRequiresCritical = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) bool bRequiresVulnerable = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FGameplayTagRequirements SourceRequirements;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FGameplayTagRequirements TargetRequirements;
};

/** Immutable GE configuration. GAS active handles own lifetime, stacks and inhibition; no actor Tick. */
UCLASS(DisplayName = "Umbra Damage Bonuses (A/X)")
class UMBRA_API UUmbraDamageBonusComponent : public UGameplayEffectComponent
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage") TArray<FUmbraDamageBonus> Bonuses;
	virtual bool CanGameplayEffectApply(const FActiveGameplayEffectsContainer& Container, const FGameplayEffectSpec& Spec) const override;
	virtual bool Validate(const FGameplayEffectSpec& Spec) const;
	/** Immutable definition data, overridable by a native equipment GE using its replicated source asset. */
	virtual const TArray<FUmbraDamageBonus>& GetBonuses(const FGameplayEffectSpec& Spec) const { return Bonuses; }
};

struct FUmbraDamageBuckets
{
	double Additive[9] = {};
	double Multiplicative[9] = {1., 1., 1., 1., 1., 1., 1., 1., 1.};
};

namespace UmbraDamageBonuses
{
	/** Panel categories, not a hypothetical hit. General/type/critical/vulnerable are disjoint.
	 * Additional source/target/type-state conditions and X stay outside these scalar rows. */
	struct FPanelSummary
	{
		double All = 0.;
		double Types[9] = {};
		double Critical = 0.;
		double Vulnerable = 0.;
		bool bHasConditional = false;
		bool bHasMultiplicative = false;
	};
	bool ReadPanelSummary(const UAbilitySystemComponent* Source, FPanelSummary& Out);
	/** One per-hit evaluation from the attacker's live, non-inhibited GEs. False rejects the hit. */
	bool Evaluate(const UAbilitySystemComponent* Source, const UAbilitySystemComponent* Target,
		const FGameplayEffectSpec& Hit, bool bCritical, bool bVulnerable, FUmbraDamageBuckets& Out);
}
