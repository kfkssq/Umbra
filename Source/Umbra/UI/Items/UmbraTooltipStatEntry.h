#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/UmbraItemTooltipData.h"
#include "UmbraTooltipStatEntry.generated.h"

class UTextBlock;

/** Generic preformatted text row. WBP owns typography, spacing and decoration. */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraTooltipStatEntry : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Item Tooltip")
	void SetEntryText(const FText& Text, EUmbraTooltipRowStyle Style = EUmbraTooltipRowStyle::Neutral);
	UFUNCTION(BlueprintPure, Category = "Item Tooltip") FText GetEntryText() const { return DisplayText; }
	UFUNCTION(BlueprintPure, Category = "Item Tooltip") EUmbraTooltipRowStyle GetEntryStyle() const { return DisplayStyle; }
	virtual void SetVisibility(ESlateVisibility InVisibility) override;
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> EntryText;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Tooltip|Style") FLinearColor MetColor = FLinearColor(0.35f, 0.8f, 0.4f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Tooltip|Style") FLinearColor UnmetColor = FLinearColor(0.95f, 0.3f, 0.25f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Tooltip|Style") FLinearColor UnknownColor = FLinearColor(0.65f, 0.65f, 0.65f);
	UFUNCTION(BlueprintImplementableEvent, Category = "Item Tooltip") void BP_EntryChanged(const FText& Text, EUmbraTooltipRowStyle Style);
private:
	void RefreshDisplay();
	UPROPERTY(Transient) FText DisplayText;
	UPROPERTY(Transient) EUmbraTooltipRowStyle DisplayStyle = EUmbraTooltipRowStyle::Neutral;
	FSlateColor NeutralColor;
	bool bCapturedColor = false;
};
