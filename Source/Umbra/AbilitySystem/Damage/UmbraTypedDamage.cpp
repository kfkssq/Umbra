#include "AbilitySystem/Damage/UmbraTypedDamage.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Stats/UmbraDerivedStatsComponent.h"
#include "Equipment/UmbraEquipmentComponent.h"
#include "Curves/CurveFloat.h"
#include "GameplayEffect.h"

FName UmbraTypedDamage::BaseName(int32 Index) { return FName(*FString::Printf(TEXT("Umbra.Typed.Base.%d"), Index)); }
FName UmbraTypedDamage::ADName(int32 Index) { return FName(*FString::Printf(TEXT("Umbra.Typed.AD.%d"), Index)); }
FName UmbraTypedDamage::APName(int32 Index) { return FName(*FString::Printf(TEXT("Umbra.Typed.AP.%d"), Index)); }

bool UmbraTypedDamage::Prepare(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target,
	const FUmbraTypedDamageConfig& Config, FGameplayEffectSpec& Spec)
{
	if (!Source || !Target || !Source->GetSet<UUmbraAttributeSet>() || !Target->GetSet<UUmbraAttributeSet>()) return false;
	const auto Nonnegative = [](float Value) { return FMath::IsFinite(Value) && Value >= 0.f; };
	const UUmbraDamageRules* Rules = Config.Rules ? Config.Rules.Get() : GetDefault<UUmbraDamageRules>();
	if (Config.bUseDamageBuckets && (!FMath::IsFinite(Rules->BaseCriticalMultiplier) || Rules->BaseCriticalMultiplier < 1.f
		|| !FMath::IsFinite(Rules->BaseVulnerableMultiplier) || Rules->BaseVulnerableMultiplier < 1.f)) return false;
	int32 DefenderLevel = 1;
	if (const AActor* Owner = Target->GetOwnerActor())
		if (const auto* Equipment = Owner->FindComponentByClass<UUmbraEquipmentComponent>()) DefenderLevel = Equipment->CharacterLevel;
	const float K = Rules->DefenseKByLevel ? Rules->DefenseKByLevel->GetFloatValue(float(DefenderLevel)) : Rules->DefenseK;
	if (DefenderLevel < 1 || !FMath::IsFinite(K) || K <= 0.f
		|| !FMath::IsFinite(Rules->TypeResistanceK) || Rules->TypeResistanceK <= 0.f
		|| !Nonnegative(Rules->MaxTypeReduction) || Rules->MaxTypeReduction > 1.f) return false;
	TArray<FUmbraAttackDamageChannel> Channels;
	if (Config.Model == EUmbraDamageModel::WeaponChannels)
	{
		// Explicit terms would otherwise be silently ignored: reject ambiguous configurations.
		if (!Config.Channels.IsEmpty() || !Nonnegative(Config.WeaponMultiplier)) return false;
		const auto* Derived = Source->GetOwnerActor() ? Source->GetOwnerActor()->FindComponentByClass<UUmbraDerivedStatsComponent>() : nullptr;
		if (!Derived || !Derived->IsDerivedPowerActive() || Derived->IsPublishing()) return false;
		const auto Snapshot = Derived->GetSnapshot();
		if (!Snapshot.bValid) return false;
		// Catch unsupported direct writes that would diverge GAS AD/AP from the channel projection.
		if (Source->GetNumericAttribute(UUmbraAttributeSet::GetAttackPowerAttribute()) != Snapshot.AttackPower
			|| Source->GetNumericAttribute(UUmbraAttributeSet::GetAbilityPowerAttribute()) != Snapshot.AbilityPower) return false;
		for (const auto& DerivedChannel : Snapshot.Channels)
		{
			const double Damage = double(DerivedChannel.Damage) * Config.WeaponMultiplier;
			if (!FMath::IsFinite(Damage) || Damage < 0. || Damage > MAX_flt) return false;
			auto& Channel = Channels.AddDefaulted_GetRef();
			Channel.Type = DerivedChannel.Type;
			Channel.BaseDamage = float(Damage);
		}
	}
	else if (Config.Model == EUmbraDamageModel::ExplicitChannels) Channels = Config.Channels;
	else return false;
	if (Channels.Num() > 9) return false;
	for (int32 Index = 0; Index < 9; ++Index)
	{
		Spec.SetSetByCallerMagnitude(BaseName(Index), 0.f);
		Spec.SetSetByCallerMagnitude(ADName(Index), 0.f);
		Spec.SetSetByCallerMagnitude(APName(Index), 0.f);
	}
	TSet<EUmbraWeaponDamageType> Seen;
	for (const auto& Channel : Channels)
	{
		if (uint8(Channel.Type) > uint8(EUmbraWeaponDamageType::Shadow) || Seen.Contains(Channel.Type)
			|| !Nonnegative(Channel.BaseDamage) || !Nonnegative(Channel.AttackPowerCoefficient)
			|| !Nonnegative(Channel.AbilityPowerCoefficient)) return false;
		Seen.Add(Channel.Type);
		Spec.SetSetByCallerMagnitude(BaseName(uint8(Channel.Type)), Channel.BaseDamage);
		Spec.SetSetByCallerMagnitude(ADName(uint8(Channel.Type)), Channel.AttackPowerCoefficient);
		Spec.SetSetByCallerMagnitude(APName(uint8(Channel.Type)), Channel.AbilityPowerCoefficient);
	}
	Spec.SetSetByCallerMagnitude(DefenseKName, K);
	Spec.SetSetByCallerMagnitude(ResistanceKName, Rules->TypeResistanceK);
	Spec.SetSetByCallerMagnitude(ReductionCapName, Rules->MaxTypeReduction);
	Spec.SetSetByCallerMagnitude(CanCriticalName, Config.bCanCritical ? 1.f : 0.f);
	Spec.SetSetByCallerMagnitude(ReadyName, 1.f);
	Spec.SetSetByCallerMagnitude(BucketsName, Config.bUseDamageBuckets ? 1.f : 0.f);
	Spec.SetSetByCallerMagnitude(BaseCriticalName, Config.bUseDamageBuckets ? Rules->BaseCriticalMultiplier : 1.f);
	Spec.SetSetByCallerMagnitude(BaseVulnerableName, Config.bUseDamageBuckets ? Rules->BaseVulnerableMultiplier : 1.f);
	return true;
}
