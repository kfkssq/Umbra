#include "AbilitySystem/Damage/UmbraPhysicalDamage.h"
#include "AbilitySystem/Damage/UmbraPhysicalDamageExecution.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "HAL/IConsoleManager.h"
#include "Umbra.h"

static TAutoConsoleVariable<int32> CVarUmbraDamageLog(
	TEXT("umbra.Damage.Log"), 0, TEXT("1: log physical/magical damage inputs and IncomingDamage health settlement; 0: off."));

bool UmbraPhysicalDamage::IsLoggingEnabled()
{
	return CVarUmbraDamageLog.GetValueOnGameThread() != 0;
}

bool UmbraPhysicalDamage::Apply(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target,
	TSubclassOf<UGameplayEffect> EffectClass, const FUmbraPhysicalDamageConfig& Config, float Level,
	UObject* DamageSourceObject)
{
	if (!IsValid(Source) || !IsValid(Target) || !Source->IsOwnerActorAuthoritative() || !Target->IsOwnerActorAuthoritative())
	{
		return false;
	}
	const UGameplayEffect* Effect = EffectClass ? EffectClass.GetDefaultObject() : nullptr;
	if (Config.DamageType != EUmbraDamageType::Physical && Config.DamageType != EUmbraDamageType::Magical)
	{
		UE_LOG(LogUmbra, Error, TEXT("Damage: invalid configured damage type %d; hit rejected."), int32(Config.DamageType));
		return false;
	}
	if (!Effect || Effect->DurationPolicy != EGameplayEffectDurationType::Instant
		|| !Effect->Modifiers.IsEmpty() || Effect->Executions.Num() != 1
		|| Effect->Executions[0].CalculationClass != UUmbraPhysicalDamageExecution::StaticClass())
	{
		UE_LOG(LogUmbra, Error, TEXT("Physical damage GE %s must be Instant, have no Modifiers, and exactly one UmbraPhysicalDamageExecution. Remove legacy Health/IncomingDamage modifiers."),
			*GetNameSafe(EffectClass));
		return false;
	}
	FGameplayEffectContextHandle Context = Source->MakeEffectContext();
	Context.AddSourceObject(DamageSourceObject ? DamageSourceObject : Source->GetAvatarActor());
	const FGameplayEffectSpecHandle Spec = Source->MakeOutgoingSpec(EffectClass, Level, Context);
	if (!Spec.IsValid())
	{
		return false;
	}
	Spec.Data->SetSetByCallerMagnitude(UmbraGameplayTags::Damage_Type, float(Config.DamageType));
	Spec.Data->SetSetByCallerMagnitude(UmbraGameplayTags::Damage_AbilityPowerCoefficient, Config.AbilityPowerCoefficient);
	Spec.Data->SetSetByCallerMagnitude(UmbraGameplayTags::Damage_AttackPowerCoefficient, Config.AttackPowerCoefficient);
	Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Target);
	return true;
}
