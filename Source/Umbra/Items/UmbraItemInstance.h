#pragma once

#include "CoreMinimal.h"
#include "Items/UmbraItemDefinition.h"
#include "UmbraItemInstance.generated.h"

/** Complete non-stacking instance payload; only authority creation may roll it. */
USTRUCT(BlueprintType)
struct FUmbraItemInstance
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UUmbraItemDefinition> Definition;
	UPROPERTY(BlueprintReadOnly) FGuid InstanceId;
	UPROPERTY(BlueprintReadOnly) TArray<FUmbraRolledAffix> RolledAffixes;
	bool IsValid() const;
	static bool Create(const AActor* Authority, UUmbraItemDefinition* Definition, FGuid Id, FUmbraItemInstance& Out);
};
