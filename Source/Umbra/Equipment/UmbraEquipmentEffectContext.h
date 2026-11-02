#pragma once
#include "CoreMinimal.h"
#include "AbilitySystem/Damage/UmbraDamageBonusComponent.h"
#include "UmbraEquipmentEffectContext.generated.h"

/** One immutable damage snapshot per equipped GE. Scalars never retain authoring curves. */
USTRUCT()
struct FUmbraEquipmentEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()
	UPROPERTY() TArray<FUmbraDamageBonus> DamageBonuses;
	virtual UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }
	virtual FUmbraEquipmentEffectContext* Duplicate() const override;
	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
	static const FUmbraEquipmentEffectContext* Get(const FGameplayEffectContextHandle& Handle);
};

template<> struct TStructOpsTypeTraits<FUmbraEquipmentEffectContext> : TStructOpsTypeTraitsBase2<FUmbraEquipmentEffectContext>
{
	enum { WithNetSerializer = true, WithCopy = true };
};
