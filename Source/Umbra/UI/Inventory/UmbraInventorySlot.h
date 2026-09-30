#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UmbraInventorySlot.generated.h"

class UImage;
class UTextBlock;
class UUmbraInventoryMenu;
class UUmbraInventorySlot;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUmbraInventorySlotEvent, UUmbraInventorySlot*, Slot);

/** Empty inventory cell. No item identity, rarity, or gameplay data exists in this phase. */
UCLASS(Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraInventorySlot : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RequestSelection();
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshVisual();
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool ShouldHighlight() const { return bSelected || bHovered; }
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsSelected() const { return bSelected; }
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetSlotIndex() const { return SlotIndex; }

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FUmbraInventorySlotEvent OnSelectionRequested;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> SlotBackground;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> RarityFrame;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> StackCountText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> EquippedMarker;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> HighlightFrame;

	/** Blueprint applies brushes/colors using this C++ decision; never changes selection here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void BP_RefreshVisual(bool bShouldHighlight);

private:
	friend class UUmbraInventoryMenu;
	void SetSelected(bool bInSelected);
	bool bSelected = false;
	bool bHovered = false;
	int32 SlotIndex = INDEX_NONE;
};
