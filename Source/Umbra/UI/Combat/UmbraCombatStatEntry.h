#pragma once

#include "CoreMinimal.h"
#include "UI/UmbraStatEntry.h"
#include "UmbraCombatStatEntry.generated.h"

/** UI prototype row. Designer text is already formatted and never represents a GAS attribute. */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraCombatStatEntry : public UUmbraStatEntry
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;

	/** WBP_CombatInfo row instance overrides the WBP row default; includes units such as %. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat UI|Test Data", meta = (ExposeOnSpawn = "true"))
	FText TestValue;
};
