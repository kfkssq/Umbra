#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/UmbraItemInstance.h"
#include "Styling/SlateColor.h"
#include "UmbraWorldItemLabel.generated.h"

class UTextBlock;
class UImage;

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
	/** Applied to the base text RGB each refresh, never to the previously highlighted color. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Item|Highlight", meta = (ClampMin = "1", ClampMax = "4")) float HoverBrightnessMultiplier = 1.25f;
	/** Wrap long names in Slate units; zero preserves Designer wrapping. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Item", meta = (ClampMin = "0")) float NameWrapWidth = 240.f;
protected:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ItemName;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ItemIcon;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	/** Optional cosmetic hook, called after all native data and highlight updates. */
	UFUNCTION(BlueprintImplementableEvent, Category = "World Item") void BP_RefreshItem();
private:
	void RefreshVisuals();
	/** Designer baseline is captured after PreConstruct and restored on Slate teardown. */
	FSlateColor DefaultNameColor;
	bool bConstructed = false;
};
