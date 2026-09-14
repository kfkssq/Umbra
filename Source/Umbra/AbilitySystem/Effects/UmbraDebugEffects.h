#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "UmbraDebugEffects.generated.h"

/** Fixed native classes ensure duration effects have stable definitions on network clients. */
UCLASS(NotBlueprintable)
class UMBRA_API UUmbraDebugAttributeEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UUmbraDebugAttributeEffect();
};

UCLASS(NotBlueprintable)
class UMBRA_API UUmbraDebugDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UUmbraDebugDamageEffect();
};

UCLASS(NotBlueprintable)
class UMBRA_API UUmbraDebugHealEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UUmbraDebugHealEffect();
};
