#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/UmbraItemInstance.h"
#include "Styling/SlateBrush.h"
#include "UmbraWorldItemLabel.generated.h"

class UTextBlock;
class UImage;
class UBorder;

/** Native data presentation; WBP Designer owns font, brushes and layout. No Graph required. */
UCLASS(Abstract, Blueprintable)
class UMBRA_API UUmbraWorldItemLabel : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetItem(const FUmbraItemInstance& InItem, bool bInHighlighted);
	UPROPERTY(BlueprintReadOnly, Category = "World Item") FUmbraItemInstance Item;
	UPROPERTY(BlueprintReadOnly, Category = "World Item") bool bHighlighted = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Item|Highlight") bool bEnableHoverHighlight = true;
	/** Replaces the border color multiplier while hovered; never modifies its brush resource. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Item|Highlight") FLinearColor HoverBackgroundColor = FLinearColor(0.3f, 0.3f, 0.3f, 0.9f);
protected:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ItemName;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ItemIcon;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UBorder> ItemBackground;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	/** Optional cosmetic hook, called after all native data and highlight updates. */
	UFUNCTION(BlueprintImplementableEvent, Category = "World Item") void BP_RefreshItem();
private:
	void RefreshVisuals();
	void RestoreBackground();
	/** Keep defaults across Slate reconstruction; never recapture an already-highlighted border. */
	TWeakObjectPtr<UBorder> CachedBackground;
	UPROPERTY(Transient) FSlateBrush DefaultBackgroundBrush;
	FLinearColor DefaultBackgroundColor = FLinearColor::White;
	bool bConstructed = false;
};
