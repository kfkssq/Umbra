#pragma once
#include "UmbraPlayerController.h"
#include "UmbraLootTestTypes.generated.h"

/** Concrete controller for authority pickup tests; the production base is abstract. */
UCLASS(Transient, NotBlueprintable)
class AUmbraLootTestController : public AUmbraPlayerController
{
	GENERATED_BODY()
};
