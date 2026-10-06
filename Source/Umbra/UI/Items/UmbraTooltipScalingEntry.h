#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UmbraTooltipScalingEntry.generated.h"

class UTextBlock;

/** Lightweight attribute/grade cell for the two-column scaling grid. WBP owns layout and typography. */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraTooltipScalingEntry : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Item Tooltip")
	void SetEntry(const FText& InAttribute, const FText& InGrade);
	UFUNCTION(BlueprintPure, Category = "Item Tooltip") FText GetAttribute() const { return Attribute; }
	UFUNCTION(BlueprintPure, Category = "Item Tooltip") FText GetGrade() const { return Grade; }
	virtual void SetVisibility(ESlateVisibility InVisibility) override;
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> AttributeText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> GradeText;
	UFUNCTION(BlueprintImplementableEvent, Category = "Item Tooltip") void BP_EntryChanged(const FText& InAttribute, const FText& InGrade);
private:
	void RefreshDisplay();
	UPROPERTY(Transient) FText Attribute;
	UPROPERTY(Transient) FText Grade;
};