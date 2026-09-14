#include "AbilitySystem/Effects/UmbraDebugEffects.h"
#include "AbilitySystem/UmbraAttributeSet.h"

namespace
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	void AddModifier(UGameplayEffect& Effect, const FGameplayAttribute& Attribute, float Magnitude)
	{
		FGameplayModifierInfo& Modifier = Effect.Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FScalableFloat(Magnitude);
	}
#endif
}

UUmbraDebugAttributeEffect::UUmbraDebugAttributeEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	AddModifier(*this, UUmbraAttributeSet::GetAttackPowerAttribute(), 20.f);
	AddModifier(*this, UUmbraAttributeSet::GetMaxHealthAttribute(), 100.f);
#endif
}

UUmbraDebugDamageEffect::UUmbraDebugDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	AddModifier(*this, UUmbraAttributeSet::GetIncomingDamageAttribute(), 10.f);
#endif
}

UUmbraDebugHealEffect::UUmbraDebugHealEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	AddModifier(*this, UUmbraAttributeSet::GetHealthAttribute(), 10.f);
#endif
}
