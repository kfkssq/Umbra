#include "AbilitySystem/Damage/UmbraPhysicalDamageExecution.h"
#include "AbilitySystem/Damage/UmbraPhysicalDamage.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Umbra.h"

namespace
{
	struct FPhysicalCaptures
	{
		DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);
        DECLARE_ATTRIBUTE_CAPTUREDEF(AbilityPower);
        DECLARE_ATTRIBUTE_CAPTUREDEF(MagicResistance);
		DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalChance);
		DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalDamageMultiplier);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);
		FPhysicalCaptures()
		{
			// No snapshots: evaluate the live attributes when the hit spec executes.
			DEFINE_ATTRIBUTE_CAPTUREDEF(UUmbraAttributeSet, AttackPower, Source, false);
            DEFINE_ATTRIBUTE_CAPTUREDEF(UUmbraAttributeSet, AbilityPower, Source, false);
            DEFINE_ATTRIBUTE_CAPTUREDEF(UUmbraAttributeSet, MagicResistance, Target, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UUmbraAttributeSet, CriticalChance, Source, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UUmbraAttributeSet, CriticalDamageMultiplier, Source, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UUmbraAttributeSet, Armor, Target, false);
		}
	};
	const FPhysicalCaptures& Captures()
	{
		static FPhysicalCaptures Value;
		return Value;
	}
	float Nonnegative(float Value) { return FMath::IsFinite(Value) ? FMath::Max(0.f, Value) : 0.f; }
}

UUmbraPhysicalDamageExecution::UUmbraPhysicalDamageExecution()
{
	RelevantAttributesToCapture.Add(Captures().AttackPowerDef);
    RelevantAttributesToCapture.Add(Captures().AbilityPowerDef);
    RelevantAttributesToCapture.Add(Captures().MagicResistanceDef);
	RelevantAttributesToCapture.Add(Captures().CriticalChanceDef);
	RelevantAttributesToCapture.Add(Captures().CriticalDamageMultiplierDef);
	RelevantAttributesToCapture.Add(Captures().ArmorDef);
}

void UUmbraPhysicalDamageExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& Parameters, FGameplayEffectCustomExecutionOutput& Output) const
{
	UAbilitySystemComponent* Source = Parameters.GetSourceAbilitySystemComponent();
	UAbilitySystemComponent* Target = Parameters.GetTargetAbilitySystemComponent();
	if (!Source || !Target || !Source->IsOwnerActorAuthoritative() || !Target->IsOwnerActorAuthoritative())
	{
		return;
	}
	const FGameplayEffectSpec& Spec = Parameters.GetOwningSpec();
	const float Type = Spec.GetSetByCallerMagnitude(UmbraGameplayTags::Damage_Type, false, -1.f);
    if (Type != float(EUmbraDamageType::Physical) && Type != float(EUmbraDamageType::Magical))
    {
        UE_LOG(LogUmbra, Error, TEXT("Damage: missing or invalid Damage.Type (%g); hit rejected."), Type);
        return;
    }
    const bool bMagical = Type == float(EUmbraDamageType::Magical);
    FAggregatorEvaluateParameters Evaluation;
	Evaluation.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	Evaluation.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	float AttackPower = 0.f, AbilityPower = 0.f, Chance = 0.f, CriticalMultiplier = 1.f, Resistance = 0.f;
	const bool bCaptured =
		Parameters.AttemptCalculateCapturedAttributeMagnitude(Captures().AttackPowerDef, Evaluation, AttackPower)
		&& Parameters.AttemptCalculateCapturedAttributeMagnitude(Captures().CriticalChanceDef, Evaluation, Chance)
		&& Parameters.AttemptCalculateCapturedAttributeMagnitude(Captures().CriticalDamageMultiplierDef, Evaluation, CriticalMultiplier)
		&& Parameters.AttemptCalculateCapturedAttributeMagnitude(Captures().AbilityPowerDef, Evaluation, AbilityPower)
        && Parameters.AttemptCalculateCapturedAttributeMagnitude(bMagical ? Captures().MagicResistanceDef : Captures().ArmorDef, Evaluation, Resistance);
	if (!bCaptured)
	{
		UE_LOG(LogUmbra, Error, TEXT("Physical damage: missing source/target Umbra attributes."));
		return;
	}
	AttackPower = Nonnegative(AttackPower);
	Resistance = Nonnegative(Resistance);
    AbilityPower = Nonnegative(AbilityPower);
	Chance = FMath::Clamp(Nonnegative(Chance), 0.f, 1.f);
	CriticalMultiplier = FMath::Max(1.f, Nonnegative(CriticalMultiplier));
	const float Coefficient = Spec.GetSetByCallerMagnitude(UmbraGameplayTags::Damage_AttackPowerCoefficient, false, 0.f);
	const float APCoefficient = Spec.GetSetByCallerMagnitude(UmbraGameplayTags::Damage_AbilityPowerCoefficient, false, 0.f);
    if (!FMath::IsFinite(Coefficient) || !FMath::IsFinite(APCoefficient))
    {
        UE_LOG(LogUmbra, Error, TEXT("Damage: non-finite scaling coefficient; hit rejected."));
        return;
    }
	const bool bCritical = Chance >= 1.f || (Chance > 0.f && FMath::FRand() < Chance);
	const double Raw = FMath::Max(0.0, double(AttackPower) * Coefficient + double(AbilityPower) * APCoefficient);
	const double Final = Raw * (bCritical ? CriticalMultiplier : 1.f) * 100.0 / (100.0 + Resistance);
	const float Damage = float(FMath::Min(Final, double(MAX_flt)));
	if (UmbraPhysicalDamage::IsLoggingEnabled())
	{
		UE_LOG(LogUmbra, Log, TEXT("[Damage] Source=%s Target=%s GE=%s Type=%s AD=%.3f ADCoefficient=%.3f AP=%.3f APCoefficient=%.3f Chance=%.3f CriticalMultiplier=%.3f Crit=%d ResistanceType=%s Resistance=%.3f Raw=%.3f Final=%.3f"),
			*GetNameSafe(Source->GetAvatarActor()), *GetNameSafe(Target->GetAvatarActor()), *GetNameSafe(Spec.Def),
			bMagical ? TEXT("Magical") : TEXT("Physical"), AttackPower, Coefficient, AbilityPower, APCoefficient, Chance, CriticalMultiplier, bCritical, bMagical ? TEXT("MagicResistance") : TEXT("Armor"), Resistance, Raw, Damage);
	}
	// Include zero damage so the settlement log also reports a zero-health delta.
	// Store this hit's result on its own spec, never on the shared execution object.
	if (FGameplayEffectSpec* MutableSpec = Parameters.GetOwningSpecForPreExecuteMod())
		MutableSpec->SetSetByCallerMagnitude(UmbraGameplayTags::Damage_ResultCritical, bCritical ? 1.f : 0.f);
	Output.AddOutputModifier(FGameplayModifierEvaluatedData(UUmbraAttributeSet::GetIncomingDamageAttribute(),
		EGameplayModOp::Additive, Damage));
}


