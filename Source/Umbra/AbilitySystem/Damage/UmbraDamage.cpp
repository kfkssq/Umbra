#include "AbilitySystem/Damage/UmbraDamage.h"

#include "AbilitySystem/Damage/UmbraPhysicalDamageExecution.h"
#include "AbilitySystem/Damage/UmbraTypedDamageExecution.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Umbra.h"
#include "Stats/UmbraDerivedStatsComponent.h"
#include "Equipment/UmbraEquipmentComponent.h"

bool UmbraDamage::Apply(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target,
	const FUmbraDamageRequest& Request)
{
	if (!IsValid(Source) || !IsValid(Target) || !Source->IsOwnerActorAuthoritative() || !Target->IsOwnerActorAuthoritative())
	{
		return false;
	}
	if ((Request.Source != EUmbraDamageSource::BasicAttack && Request.Source != EUmbraDamageSource::Skill
		&& Request.Source != EUmbraDamageSource::LegacyUnspecified)
		|| (Request.bRecordPrimaryAttackDamage && Request.Source != EUmbraDamageSource::BasicAttack))
	{
		UE_LOG(LogUmbra, Error, TEXT("Damage: invalid source classification or primary measurement marker; hit rejected."));
		return false;
	}
	const FUmbraPhysicalDamageConfig& Config = Request.Config;
	// Both offensive and defensive attributes can be transient during an equipment transaction.
	for (const UAbilitySystemComponent* Participant : {Source, Target})
	{
		if (const AActor* Owner = Participant->GetOwnerActor())
			if (const auto* Equipment = Owner->FindComponentByClass<UUmbraEquipmentComponent>();
				Equipment && Equipment->bEnableEquipment && !Equipment->IsReadyForCombat()) return false;
	}
	if (const AActor* Owner = Source->GetOwnerActor())
	{
		if (const auto* Derived = Owner->FindComponentByClass<UUmbraDerivedStatsComponent>();
			Derived && Derived->bUseWeaponDerivedPower
			&& (!Derived->IsDerivedPowerActive() || Derived->IsPublishing() || !Derived->GetSnapshot().bValid))
			return false;
	}
	const bool bTyped = Config.Typed.Model != EUmbraDamageModel::Legacy;
	if (!bTyped && Config.DamageType != EUmbraDamageType::Physical && Config.DamageType != EUmbraDamageType::Magical)
	{
		UE_LOG(LogUmbra, Error, TEXT("Damage: invalid configured damage type %d; hit rejected."), int32(Config.DamageType));
		return false;
	}
	const TSubclassOf<UGameplayEffect> EffectClass = bTyped ? TSubclassOf<UGameplayEffect>(UUmbraTypedDamageEffect::StaticClass()) : Request.EffectClass;
	const UGameplayEffect* Effect = EffectClass ? EffectClass.GetDefaultObject() : nullptr;
	if (!Effect || Effect->DurationPolicy != EGameplayEffectDurationType::Instant
		|| !Effect->Modifiers.IsEmpty() || Effect->Executions.Num() != 1
		|| Effect->Executions[0].CalculationClass != (bTyped ? UUmbraTypedDamageExecution::StaticClass() : UUmbraPhysicalDamageExecution::StaticClass()))
	{
		UE_LOG(LogUmbra, Error, TEXT("Physical damage GE %s must be Instant, have no Modifiers, and exactly one UmbraPhysicalDamageExecution. Remove legacy Health/IncomingDamage modifiers."),
			*GetNameSafe(Request.EffectClass));
		return false;
	}
	FGameplayEffectContextHandle Context = Source->MakeEffectContext();
	Context.AddSourceObject(Source->GetAvatarActor());
	const FGameplayEffectSpecHandle Spec = Source->MakeOutgoingSpec(EffectClass, Request.Level, Context);
	if (!Spec.IsValid()) return false;
	if (bTyped)
	{
		if (Request.Source == EUmbraDamageSource::LegacyUnspecified
			|| !UmbraTypedDamage::Prepare(Source, Target, Config.Typed, *Spec.Data.Get()))
		{
			UE_LOG(LogUmbra, Error, TEXT("Typed damage: invalid configuration, source classification or derived inputs; hit rejected."));
			return false;
		}
	}
	else
	{
		Spec.Data->SetSetByCallerMagnitude(UmbraGameplayTags::Damage_Type, float(Config.DamageType));
		Spec.Data->SetSetByCallerMagnitude(UmbraGameplayTags::Damage_AbilityPowerCoefficient, Config.AbilityPowerCoefficient);
		Spec.Data->SetSetByCallerMagnitude(UmbraGameplayTags::Damage_AttackPowerCoefficient, Config.AttackPowerCoefficient);
	}
	if (Request.Source == EUmbraDamageSource::BasicAttack)
		Spec.Data->AddDynamicAssetTag(UmbraGameplayTags::Damage_Source_BasicAttack);
	else if (Request.Source == EUmbraDamageSource::Skill)
		Spec.Data->AddDynamicAssetTag(UmbraGameplayTags::Damage_Source_Skill);
	if (Request.bRecordPrimaryAttackDamage)
		Spec.Data->SetSetByCallerMagnitude(UmbraGameplayTags::Damage_SourcePrimaryAttack, 1.f);
	Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Target);
	return true;
}
