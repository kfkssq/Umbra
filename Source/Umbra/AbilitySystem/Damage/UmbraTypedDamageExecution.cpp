#include "AbilitySystem/Damage/UmbraTypedDamageExecution.h"
#include "AbilitySystem/Damage/UmbraTypedDamage.h"
#include "AbilitySystem/Damage/UmbraDamageBonusComponent.h"
#include "AbilitySystem/Damage/UmbraPhysicalDamage.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Umbra.h"

namespace
{
	struct FTypedCaptures
	{
		FGameplayEffectAttributeCaptureDefinition AD{UUmbraAttributeSet::GetAttackPowerAttribute(), EGameplayEffectAttributeCaptureSource::Source, false};
		FGameplayEffectAttributeCaptureDefinition AP{UUmbraAttributeSet::GetAbilityPowerAttribute(), EGameplayEffectAttributeCaptureSource::Source, false};
		FGameplayEffectAttributeCaptureDefinition Chance{UUmbraAttributeSet::GetCriticalChanceAttribute(), EGameplayEffectAttributeCaptureSource::Source, false};
		FGameplayEffectAttributeCaptureDefinition Crit{UUmbraAttributeSet::GetCriticalDamageMultiplierAttribute(), EGameplayEffectAttributeCaptureSource::Source, false};
		FGameplayEffectAttributeCaptureDefinition Armor{UUmbraAttributeSet::GetArmorAttribute(), EGameplayEffectAttributeCaptureSource::Target, false};
		FGameplayEffectAttributeCaptureDefinition MR{UUmbraAttributeSet::GetMagicResistanceAttribute(), EGameplayEffectAttributeCaptureSource::Target, false};
		TArray<FGameplayEffectAttributeCaptureDefinition> Resistances;
		FTypedCaptures()
		{
			const TArray<FGameplayAttribute> Attributes = {
				UUmbraAttributeSet::GetSlashingResistanceAttribute(), UUmbraAttributeSet::GetBluntResistanceAttribute(),
				UUmbraAttributeSet::GetPiercingResistanceAttribute(), UUmbraAttributeSet::GetFireResistanceAttribute(),
				UUmbraAttributeSet::GetLightningResistanceAttribute(), UUmbraAttributeSet::GetColdResistanceAttribute(),
				UUmbraAttributeSet::GetRadiantResistanceAttribute(), UUmbraAttributeSet::GetPoisonResistanceAttribute(),
				UUmbraAttributeSet::GetShadowResistanceAttribute()};
			for (const auto& Attribute : Attributes) Resistances.Emplace(Attribute, EGameplayEffectAttributeCaptureSource::Target, false);
		}
	};
	const FTypedCaptures& Captures() { static const FTypedCaptures Value; return Value; }
}

UUmbraTypedDamageEffect::UUmbraTypedDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Executions.AddDefaulted_GetRef().CalculationClass = UUmbraTypedDamageExecution::StaticClass();
}

UUmbraTypedDamageExecution::UUmbraTypedDamageExecution()
{
	const auto& C = Captures();
	RelevantAttributesToCapture = {C.AD, C.AP, C.Chance, C.Crit, C.Armor, C.MR};
	RelevantAttributesToCapture.Append(C.Resistances);
}

void UUmbraTypedDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& Parameters,
	FGameplayEffectCustomExecutionOutput& Output) const
{
	const auto* Source = Parameters.GetSourceAbilitySystemComponent();
	const auto* Target = Parameters.GetTargetAbilitySystemComponent();
	if (!Source || !Target || !Source->IsOwnerActorAuthoritative() || !Target->IsOwnerActorAuthoritative()) return;
	const auto& Spec = Parameters.GetOwningSpec();
	using namespace UmbraTypedDamage;
	const float K = Spec.GetSetByCallerMagnitude(DefenseKName, false, -1.f);
	const float TypeK = Spec.GetSetByCallerMagnitude(ResistanceKName, false, -1.f);
	const float Cap = Spec.GetSetByCallerMagnitude(ReductionCapName, false, -1.f);
	const float CanCrit = Spec.GetSetByCallerMagnitude(CanCriticalName, false, -1.f);
	const float BucketsMode = Spec.GetSetByCallerMagnitude(BucketsName, false, 0.f);
	const bool bBuckets = BucketsMode == 1.f;
	const float BaseCrit = Spec.GetSetByCallerMagnitude(BaseCriticalName, false, 1.f);
	const float BaseVulnerable = Spec.GetSetByCallerMagnitude(BaseVulnerableName, false, 1.f);
	if (Spec.GetSetByCallerMagnitude(ReadyName, false, 0.f) != 1.f || !FMath::IsFinite(K) || K <= 0.f
		|| !FMath::IsFinite(TypeK) || TypeK <= 0.f || !FMath::IsFinite(Cap) || Cap < 0.f || Cap > 1.f
		|| (CanCrit != 0.f && CanCrit != 1.f) || (BucketsMode != 0.f && BucketsMode != 1.f)
		|| (bBuckets && (!FMath::IsFinite(BaseCrit) || BaseCrit < 1.f || !FMath::IsFinite(BaseVulnerable) || BaseVulnerable < 1.f)))
	{
		UE_LOG(LogUmbra, Error, TEXT("Typed damage: missing or invalid prepared payload; hit rejected."));
		return;
	}
	FAggregatorEvaluateParameters Evaluation;
	Evaluation.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	Evaluation.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	const auto Read = [&](const FGameplayEffectAttributeCaptureDefinition& Definition, float& Value)
	{
		if (!Parameters.AttemptCalculateCapturedAttributeMagnitude(Definition, Evaluation, Value) || !FMath::IsFinite(Value)) return false;
		Value = FMath::Max(0.f, Value);
		return true;
	};
	const auto& C = Captures();
	float AD, AP, Chance, Crit = 1.f, Armor, MR;
	if (!Read(C.AD, AD) || !Read(C.AP, AP) || !Read(C.Chance, Chance) || (!bBuckets && !Read(C.Crit, Crit)) || !Read(C.Armor, Armor) || !Read(C.MR, MR))
	{
		UE_LOG(LogUmbra, Error, TEXT("Typed damage: missing or invalid captured attributes; hit rejected."));
		return;
	}
	Chance = FMath::Clamp(Chance, 0.f, 1.f);
	// One roll for the whole hit. New base crit is independent of legacy total crit and A bonuses.
	const bool bCritical = CanCrit == 1.f && (Chance >= 1.f || (Chance > 0.f && FMath::FRand() < Chance));
	const bool bVulnerable = bBuckets && Target->HasMatchingGameplayTag(UmbraGameplayTags::State_Vulnerable);
	const double CritFactor = bCritical ? (bBuckets ? BaseCrit : FMath::Max(1.f, Crit)) : 1.f;
	const double VulnerableFactor = bVulnerable ? BaseVulnerable : 1.f;
	FUmbraDamageBuckets Buckets;
	if (bBuckets && !UmbraDamageBonuses::Evaluate(Source, Target, Spec, bCritical, bVulnerable, Buckets))
	{
		UE_LOG(LogUmbra, Error, TEXT("Typed damage: invalid A/X contributions or source tags; entire hit rejected."));
		return;
	}
	double Physical = 0., Magical = 0.;
	for (int32 Index = 0; Index < 9; ++Index)
	{
		const float Base = Spec.GetSetByCallerMagnitude(BaseName(Index), false, -1.f);
		const float ADCoefficient = Spec.GetSetByCallerMagnitude(ADName(Index), false, -1.f);
		const float APCoefficient = Spec.GetSetByCallerMagnitude(APName(Index), false, -1.f);
		float Rating;
		if (!FMath::IsFinite(Base) || Base < 0.f || !FMath::IsFinite(ADCoefficient) || ADCoefficient < 0.f
			|| !FMath::IsFinite(APCoefficient) || APCoefficient < 0.f || !Read(C.Resistances[Index], Rating))
		{
			UE_LOG(LogUmbra, Error, TEXT("Typed damage: invalid channel %d; entire hit rejected."), Index);
			return;
		}
		const double Raw = double(Base) + double(AD) * ADCoefficient + double(AP) * APCoefficient;
		const double Defense = Index < 3 ? Armor : MR;
		const double Reduction = FMath::Min(double(Cap), double(Rating) / (double(Rating) + TypeK));
		const double Final = Raw * (1. + Buckets.Additive[Index]) * Buckets.Multiplicative[Index]
			* CritFactor * VulnerableFactor * double(K) / (Defense + K) * (1. - Reduction);
		if (!FMath::IsFinite(Final) || !FMath::IsFinite(Physical + Magical + Final))
		{
			UE_LOG(LogUmbra, Error, TEXT("Typed damage: A/X arithmetic overflow; entire hit rejected."));
			return;
		}
		if (Index < 3) Physical += Final; else Magical += Final;
		if (Raw > 0. && UmbraPhysicalDamage::IsLoggingEnabled())
			UE_LOG(LogUmbra, Log, TEXT("[DamageChannel] Type=%s Base=%.3f AD=%.3f ADCoefficient=%.3f AP=%.3f APCoefficient=%.3f Crit=%d Raw=%.3f Defense=%.3f K=%.3f Rating=%.3f DR=%.3f A=%.3f X=%.3f C=%.3f V=%.3f Final=%.3f"),
				*StaticEnum<EUmbraWeaponDamageType>()->GetNameStringByValue(Index), Base, AD, ADCoefficient, AP, APCoefficient,
				bCritical, Raw, Defense, K, Rating, Reduction, Buckets.Additive[Index], Buckets.Multiplicative[Index], CritFactor, VulnerableFactor, Final);
	}
	const float Damage = float(FMath::Min(Physical + Magical, double(MAX_flt)));
	if (auto* MutableSpec = Parameters.GetOwningSpecForPreExecuteMod())
	{
		MutableSpec->SetSetByCallerMagnitude(UmbraGameplayTags::Damage_ResultCritical, bCritical ? 1.f : 0.f);
		// Existing floating number protocol supports two colors. Use the dominant final side, physical on ties.
		MutableSpec->SetSetByCallerMagnitude(UmbraGameplayTags::Damage_Type, Magical > Physical ? 1.f : 0.f);
	}
	if (UmbraPhysicalDamage::IsLoggingEnabled())
		UE_LOG(LogUmbra, Log, TEXT("[DamageTyped] Source=%s Target=%s Buckets=%d Crit=%d Vulnerable=%d Physical=%.3f Magical=%.3f Final=%.3f"),
			*GetNameSafe(Source->GetAvatarActor()), *GetNameSafe(Target->GetAvatarActor()), bBuckets, bCritical, bVulnerable, Physical, Magical, Damage);
	// Exactly one settlement/health event/floating number, including the all-zero payload.
	Output.AddOutputModifier(FGameplayModifierEvaluatedData(UUmbraAttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, Damage));
}
