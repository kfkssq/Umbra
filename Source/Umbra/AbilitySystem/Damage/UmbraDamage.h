#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Damage/UmbraPhysicalDamage.h"

/** Classification is independent of the GameplayAbility that delivers the hit. */
enum class EUmbraDamageSource : uint8
{
	BasicAttack,
	Skill,
	/** Only for old callers whose false bPrimaryAttack did not identify a skill. */
	LegacyUnspecified
};

/** Server submission data. Config selects legacy or typed rules; one request remains one hit. */
struct UMBRA_API FUmbraDamageRequest
{
	explicit FUmbraDamageRequest(EUmbraDamageSource InSource) : Source(InSource) {}

	EUmbraDamageSource Source;
	TSubclassOf<UGameplayEffect> EffectClass;
	FUmbraPhysicalDamageConfig Config;
	float Level = 1.f;
	/** Existing player-primary measurement marker; enemy basic attacks must not start counting. */
	bool bRecordPrimaryAttackDamage = false;
};

namespace UmbraDamage
{
	/** True means a valid spec was submitted, not that damage landed (GE application can be blocked).
	 * Callers retain hit deduplication. Each call creates and submits exactly one execution spec.
	 */
	UMBRA_API bool Apply(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target,
		const FUmbraDamageRequest& Request);
}
