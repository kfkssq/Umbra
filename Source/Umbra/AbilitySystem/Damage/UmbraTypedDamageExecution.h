#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayEffectExecutionCalculation.h"
#include "UmbraTypedDamageExecution.generated.h"

UCLASS()
class UMBRA_API UUmbraTypedDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
public:
	UUmbraTypedDamageExecution();
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& Parameters,
		FGameplayEffectCustomExecutionOutput& Output) const override;
};

/** Single native Instant execution; typed mode requires no replacement Blueprint GE asset. */
UCLASS()
class UMBRA_API UUmbraTypedDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UUmbraTypedDamageEffect();
};
