#include "Items/UmbraWeaponProfile.h"
#include "Umbra.h"

bool FUmbraWeaponDamageChannel::ResolveRange(float& OutMin, float& OutMax) const
{
	OutMin = bUseDamageRange ? MinBaseDamage : BaseDamage;
	OutMax = bUseDamageRange ? MaxBaseDamage : BaseDamage;
	if (FMath::IsFinite(OutMin) && FMath::IsFinite(OutMax) && OutMin >= 0.f && OutMax >= OutMin) return true;
	UE_LOG(LogUmbra, Error, TEXT("Weapon channel %d rejected: invalid base damage range [%g, %g] (range enabled: %d)."), int32(Type), OutMin, OutMax, bUseDamageRange);
	return false;
}
