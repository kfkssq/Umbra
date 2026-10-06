#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UmbraTooltipKeyValueEntry.generated.h"

class UTextBlock;

/** Lightweight label/value row. C++ supplies semantics; WBP owns fonts, colors, padding and alignment. */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraTooltipKeyValueEntry : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Item Tooltip")
	void SetEntry(const FText& InLabel, const FText& InValue);
	UFUNCTION(BlueprintPure, Category = "Item Tooltip") FText GetLabel() const { return Label; }
	UFUNCTION(BlueprintPure, Category = "Item Tooltip") FText GetValue() const { return Value; }
	virtual void SetVisibility(ESlateVisibility InVisibility) override;
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> LabelText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> ValueText;
	UFUNCTION(BlueprintImplementableEvent, Category = "Item Tooltip") void BP_EntryChanged(const FText& InLabel, const FText& InValue);
private:
	void RefreshDisplay();
	UPROPERTY(Transient) FText Label;
	UPROPERTY(Transient) FText Value;
};