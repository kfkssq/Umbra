#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UmbraStatTooltip.generated.h"

/** Reusable hover content for a compact character stat cell. */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraStatTooltip : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetContent(const FText& InName, const FText& InDescription);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Character Stats|Tooltip", meta = (DisplayName = "Apply Stat Tooltip Content"))
	void BP_ApplyContent(const FText& InName, const FText& InDescription);
};
