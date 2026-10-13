#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/UmbraItemTooltipData.h"
#include "UmbraTooltipAffixEntry.generated.h"

class UTextBlock;

/** WBP owns font, colors, spacing, alignment and line wrapping. No gameplay queries or Tick. */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraTooltipAffixEntry : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Item Tooltip") void SetAffixData(const FUmbraTooltipAffixDisplayData& Data);
	UFUNCTION(BlueprintPure, Category = "Item Tooltip") FUmbraTooltipAffixDisplayData GetAffixData() const { return AffixData; }
	virtual void SetVisibility(ESlateVisibility InVisibility) override;
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> AffixName;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> AffixValue;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AffixRange;
private:
	void RefreshDisplay();
	UPROPERTY(Transient) FUmbraTooltipAffixDisplayData AffixData;
};
