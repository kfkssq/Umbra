#include "AbilitySystem/Damage/UmbraPhysicalDamage.h"
#include "AbilitySystem/Damage/UmbraDamage.h"

#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarUmbraDamageLog(
	TEXT("umbra.Damage.Log"), 0, TEXT("1: log physical/magical damage inputs and IncomingDamage health settlement; 0: off."));

bool UmbraPhysicalDamage::IsLoggingEnabled()
{
	return CVarUmbraDamageLog.GetValueOnGameThread() != 0;
}

bool UmbraPhysicalDamage::Apply(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target,
	TSubclassOf<UGameplayEffect> EffectClass, const FUmbraPhysicalDamageConfig& Config, float Level,
	bool bPrimaryAttack)
{
	FUmbraDamageRequest Request(bPrimaryAttack ? EUmbraDamageSource::BasicAttack : EUmbraDamageSource::LegacyUnspecified);
	Request.EffectClass = EffectClass;
	Request.Config = Config;
	Request.Level = Level;
	Request.bRecordPrimaryAttackDamage = bPrimaryAttack;
	return UmbraDamage::Apply(Source, Target, Request);
}
