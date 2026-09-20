#include "AbilitySystem/Effects/UmbraDebugEffects.h"
#include "AbilitySystem/UmbraAttributeSet.h"

namespace
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	constexpr float DebugAttributeIncrease = 20.f;
	constexpr float DebugRatioIncrease = 0.2f;

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
	// Raise caps before their current pools so a full pool receives the same increase.
	AddModifier(*this, UUmbraAttributeSet::GetMaxHealthAttribute(), DebugAttributeIncrease);
	AddModifier(*this, UUmbraAttributeSet::GetHealthAttribute(), DebugAttributeIncrease);
	AddModifier(*this, UUmbraAttributeSet::GetHealthRegenAttribute(), DebugAttributeIncrease);
	AddModifier(*this, UUmbraAttributeSet::GetMaxResourceAttribute(), DebugAttributeIncrease);
	AddModifier(*this, UUmbraAttributeSet::GetResourceAttribute(), DebugAttributeIncrease);
	AddModifier(*this, UUmbraAttributeSet::GetResourceRegenAttribute(), DebugAttributeIncrease);
	AddModifier(*this, UUmbraAttributeSet::GetAttackPowerAttribute(), DebugAttributeIncrease);
	AddModifier(*this, UUmbraAttributeSet::GetAbilityPowerAttribute(), DebugAttributeIncrease);
	AddModifier(*this, UUmbraAttributeSet::GetAttackSpeedAttribute(), DebugRatioIncrease);
	AddModifier(*this, UUmbraAttributeSet::GetCriticalChanceAttribute(), DebugRatioIncrease);
	AddModifier(*this, UUmbraAttributeSet::GetCriticalDamageMultiplierAttribute(), DebugRatioIncrease);
	AddModifier(*this, UUmbraAttributeSet::GetArmorAttribute(), DebugAttributeIncrease);
	AddModifier(*this, UUmbraAttributeSet::GetMagicResistanceAttribute(), DebugAttributeIncrease);
	AddModifier(*this, UUmbraAttributeSet::GetAbilityHasteAttribute(), DebugAttributeIncrease);
	AddModifier(*this, UUmbraAttributeSet::GetMoveSpeedAttribute(), DebugAttributeIncrease);
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
