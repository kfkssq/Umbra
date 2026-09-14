#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "UmbraPhysicalDamageExecution.generated.h"

/** Server-only hit-time physical damage. Writes only IncomingDamage. */
UCLASS()
class UMBRA_API UUmbraPhysicalDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
public:
	UUmbraPhysicalDamageExecution();
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& Parameters,
		FGameplayEffectCustomExecutionOutput& Output) const override;
};
