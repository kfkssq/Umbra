#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Blueprint/UserWidget.h"
#include "Items/UmbraItemTooltipData.h"
#include "UmbraItemTooltip.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UUniformGridPanel;
class UUmbraTooltipStatEntry;
class UUmbraTooltipKeyValueEntry;
class UUmbraTooltipScalingEntry;

/** Pure projection of builder output. No gameplay lookups, mouse routing or parent-window changes. */
UCLASS(Abstract, Blueprintable)
class UMBRA_API UUmbraItemTooltip : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Item Tooltip") void SetTooltipData(const FUmbraItemTooltipData& Data);
	UFUNCTION(BlueprintCallable, Category = "Item Tooltip") void ClearTooltipData();
	UFUNCTION(BlueprintPure, Category = "Item Tooltip") FUmbraItemTooltipData GetTooltipData() const { return TooltipData; }
	/** Visible states are always hit-test invisible for this widget and all its descendants. */
	virtual void SetVisibility(ESlateVisibility InVisibility) override;
	/** Horizontal distance from the hovered slot's left edge to the tooltip's right edge; 0 is flush, negative overlaps. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Tooltip") float HorizontalOffsetFromSlot = 0.f;
	/** Present to the left of an anchor (viewport space) and clamp the final position inside the viewport. */
	UFUNCTION(BlueprintCallable, Category = "Item Tooltip") void PresentAt(const FVector2D& Anchor, const FVector2D& ViewportSize);
	/** Collapse without removing from the viewport; the menu owns the cached instance. */
	UFUNCTION(BlueprintCallable, Category = "Item Tooltip") void HideTooltip();
	/** Follow the slot using desktop-to-viewport geometry conversion, including live resize. */
	void PresentBeside(UWidget* AnchorWidget);

	/** Optional WBP_TooltipKeyValueEntry; unset falls back to a native auto-wrapped TextBlock. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Tooltip") TSubclassOf<UUmbraTooltipKeyValueEntry> KeyValueEntryClass;
	/** Optional WBP_TooltipScalingEntry; unset falls back to a native TextBlock cell. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Tooltip") TSubclassOf<UUmbraTooltipScalingEntry> ScalingEntryClass;
	/** Optional WBP_TooltipStatEntry; unset uses plain native TextBlocks. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Tooltip") TSubclassOf<UUmbraTooltipStatEntry> StatEntryClass;
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UFUNCTION(BlueprintImplementableEvent, Category = "Item Tooltip") void BP_TooltipDataChanged(const FUmbraItemTooltipData& Data);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> ItemName;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UVerticalBox> StatRows;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UImage> ItemIcon;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UImage> quality_bg;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UImage> SlotBackground;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ItemTypeText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> RarityText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ItemLevelText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TotalDamageText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ScalingNote;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PenaltyText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> WeightText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> FlavorTextBlock;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> DamageRows;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UUniformGridPanel> ScalingRows;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> RequirementRows;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> SpecialEffectRows;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UWidget> HeaderSection;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UWidget> DamageSection;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UWidget> ScalingSection;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UWidget> StatsSection;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UWidget> RequirementsSection;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UWidget> WeightSection;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UWidget> SpecialEffectsSection;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UWidget> FlavorTextSection;

private:
	friend class FUmbraTooltipViewportFitTest;
	void RefreshDisplay(bool bNotify);
	void ClearRows();
	void CaptureBackgrounds();
	void ApplyBackground(UImage* Image, UTexture2D* Texture, const FSlateBrush& Default, bool bCaptured);
	void AddKeyValueEntry(UVerticalBox* Container, const FText& Label, const FText& Value);
	void AddFullTextEntry(UVerticalBox* Container, const FText& Text, EUmbraTooltipRowStyle Style);
	void AddScalingEntry(UUniformGridPanel* Panel, const FText& Attribute, const FText& Grade, int32 Index);
	void ApplyViewportPosition(const FVector2D& Anchor, const FVector2D& ViewportSize);
	EUmbraTooltipRowStyle ToRowStyle(EUmbraTooltipEntryStyle Style) const;
	void ClearEntryWidget(UWidget* Child);

	UPROPERTY(Transient) FUmbraItemTooltipData TooltipData;
	TSharedPtr<SWidget> TooltipContent;
	TWeakObjectPtr<UWidget> TrackedAnchor;
	FVector2D LastAnchor = FVector2D::ZeroVector;
	bool bPresented = false;
	FSlateBrush QualityBackgroundBrush;
	FSlateBrush SlotBackgroundBrush;
	bool bQualityBackgroundCaptured = false;
	bool bSlotBackgroundCaptured = false;
};
