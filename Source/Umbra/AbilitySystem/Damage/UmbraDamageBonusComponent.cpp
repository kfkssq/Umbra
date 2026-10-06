#include "AbilitySystem/Damage/UmbraDamageBonusComponent.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Umbra.h"

bool UmbraDamageBonuses::ReadPanelSummary(const UAbilitySystemComponent* Source, FPanelSummary& Out)
{
	Out = FPanelSummary();
	if (!Source) return false;
	for (const auto Handle : Source->GetActiveEffects(FGameplayEffectQuery()))
	{
		const auto* Effect = Source->GetActiveGameplayEffect(Handle);
		if (!Effect || Effect->bIsInhibited || !Effect->Spec.Def) continue;
		const auto* Component = Effect->Spec.Def->FindComponent<UUmbraDamageBonusComponent>();
		if (!Component) continue;
		if (!Component->Validate(Effect->Spec) || Effect->Spec.GetStackCount() <= 0) return false;
		for (const auto& Bonus : Component->GetBonuses(Effect->Spec))
		{
			if (Bonus.Bucket == EUmbraDamageBucket::Multiplicative) { Out.bHasMultiplicative = true; continue; }
			const bool bState = Bonus.bRequiresCritical || Bonus.bRequiresVulnerable;
			if (Bonus.AttackSource != EUmbraBonusAttackSource::Any || !Bonus.SourceRequirements.IsEmpty()
				|| !Bonus.TargetRequirements.IsEmpty() || (bState && !Bonus.Types.IsEmpty())
				|| (Bonus.bRequiresCritical && Bonus.bRequiresVulnerable))
			{
				Out.bHasConditional = true;
				continue;
			}
			const double Value = double(Bonus.Magnitude.GetValueAtLevel(Effect->Spec.GetLevel())) * Effect->Spec.GetStackCount();
			if (Bonus.bRequiresCritical) Out.Critical += Value;
			else if (Bonus.bRequiresVulnerable) Out.Vulnerable += Value;
			else if (Bonus.Types.IsEmpty()) Out.All += Value;
			else for (const auto Type : Bonus.Types) Out.Types[uint8(Type)] += Value;
		}
	}
	if (!FMath::IsFinite(Out.All) || !FMath::IsFinite(Out.Critical) || !FMath::IsFinite(Out.Vulnerable)) return false;
	for (const double Value : Out.Types) if (!FMath::IsFinite(Value)) return false;
	return true;
}

bool UUmbraDamageBonusComponent::Validate(const FGameplayEffectSpec& Spec) const
{
	if (!Spec.Def || Spec.Def->DurationPolicy == EGameplayEffectDurationType::Instant || Spec.GetPeriod() != 0.f) return false;
	for (const auto& Bonus : GetBonuses(Spec))
	{
		const float Value = Bonus.Magnitude.GetValueAtLevel(Spec.GetLevel());
		if (!FMath::IsFinite(Value) || Value < 0.f || uint8(Bonus.Bucket) > uint8(EUmbraDamageBucket::Multiplicative)
			|| uint8(Bonus.AttackSource) > uint8(EUmbraBonusAttackSource::Skill) || Bonus.Types.Num() > 9) return false;
		TSet<EUmbraWeaponDamageType> Seen;
		for (const auto Type : Bonus.Types)
		{
			if (uint8(Type) > uint8(EUmbraWeaponDamageType::Shadow) || Seen.Contains(Type)) return false;
			Seen.Add(Type);
		}
	}
	return true;
}

bool UUmbraDamageBonusComponent::CanGameplayEffectApply(const FActiveGameplayEffectsContainer& Container, const FGameplayEffectSpec& Spec) const
{
	if (Validate(Spec)) return true;
	UE_LOG(LogUmbra, Warning, TEXT("Damage bonus GE rejected: invalid A/X data, Instant duration or nonzero period (%s)."), *GetNameSafe(Spec.Def));
	return false;
}

bool UmbraDamageBonuses::Evaluate(const UAbilitySystemComponent* Source, const UAbilitySystemComponent* Target,
	const FGameplayEffectSpec& Hit, bool bCritical, bool bVulnerable, FUmbraDamageBuckets& Out)
{
	Out = FUmbraDamageBuckets();
	if (!Source || !Target) return false;
	const auto& AttackTags = Hit.GetDynamicAssetTags();
	const bool bBasic = AttackTags.HasTagExact(UmbraGameplayTags::Damage_Source_BasicAttack);
	const bool bSkill = AttackTags.HasTagExact(UmbraGameplayTags::Damage_Source_Skill);
	if (bBasic == bSkill) return false;
	FGameplayTagContainer SourceTags, TargetTags;
	Source->GetOwnedGameplayTags(SourceTags);
	Target->GetOwnedGameplayTags(TargetTags);
	for (const auto Handle : Source->GetActiveEffects(FGameplayEffectQuery()))
	{
		const auto* Effect = Source->GetActiveGameplayEffect(Handle);
		if (!Effect || Effect->bIsInhibited || !Effect->Spec.Def) continue;
		const auto* Component = Effect->Spec.Def->FindComponent<UUmbraDamageBonusComponent>();
		if (!Component) continue;
		if (!Component->Validate(Effect->Spec)) return false;
		const int32 Stacks = Effect->Spec.GetStackCount();
		if (Stacks <= 0) return false;
		for (const auto& Bonus : Component->GetBonuses(Effect->Spec))
		{
			if ((Bonus.bRequiresCritical && !bCritical) || (Bonus.bRequiresVulnerable && !bVulnerable)
				|| (Bonus.AttackSource == EUmbraBonusAttackSource::BasicAttack && !bBasic)
				|| (Bonus.AttackSource == EUmbraBonusAttackSource::Skill && !bSkill)
				|| !Bonus.SourceRequirements.RequirementsMet(SourceTags) || !Bonus.TargetRequirements.RequirementsMet(TargetTags)) continue;
			const double Value = Bonus.Magnitude.GetValueAtLevel(Effect->Spec.GetLevel());
			for (int32 Index = 0; Index < 9; ++Index)
			{
				if (!Bonus.Types.IsEmpty() && !Bonus.Types.Contains(EUmbraWeaponDamageType(Index))) continue;
				if (Bonus.Bucket == EUmbraDamageBucket::Additive) Out.Additive[Index] += Value * Stacks;
				else Out.Multiplicative[Index] *= FMath::Pow(Value, double(Stacks));
				if (!FMath::IsFinite(Out.Additive[Index]) || !FMath::IsFinite(Out.Multiplicative[Index])) return false;
			}
		}
	}
	return true;
}
