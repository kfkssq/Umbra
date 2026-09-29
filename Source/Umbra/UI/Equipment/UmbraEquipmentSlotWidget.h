#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Equipment/UmbraEquipmentTypes.h"
#include "UmbraEquipmentSlotWidget.generated.h"

class UUmbraEquipmentSlotWidget;
class UImage;
class UTexture2D;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUmbraEquipmentSlotEvent, UUmbraEquipmentSlotWidget*, Slot);

/** One parent for all WBP_EquipmentSlot instances. Blueprint owns brushes and layout. */
UCLASS(Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraEquipmentSlotWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void SetSlotType(EUmbraEquipmentSlot InSlotType);
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void SetItem(const FUmbraEquipmentItemDisplay& InItem);
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void ClearItem();
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void SetLocked(bool bInLocked);
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void SetSelected(bool bInSelected);
	/** Also usable by an existing inner UButton's hover events. */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void SetHovered(bool bInHovered);
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void RequestSelection();
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void RefreshVisual();
	UFUNCTION(BlueprintPure, Category = "Equipment")
	EUmbraEquipmentSlotState GetVisualState() const;
	UFUNCTION(BlueprintPure, Category = "Equipment")
	EUmbraEquipmentSlot GetSlotType() const { return SlotType; }
	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool HasItem() const { return CurrentItem.Item != nullptr; }

	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FUmbraEquipmentSlotEvent OnSelectionRequested;
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FUmbraEquipmentSlotEvent OnSlotTypeChanged;

protected:
	/** Shared by all instances through WBP_EquipmentSlot Class Defaults; no gameplay meaning. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Visual")
	TMap<EUmbraEquipmentSlot, TObjectPtr<UTexture2D>> EmptySlotIcons;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> EmptyIcon;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemIcon;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> RarityFrame;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> LockedOverlay;
	virtual void NativePreConstruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (ExposeOnSpawn = true))
	EUmbraEquipmentSlot SlotType = EUmbraEquipmentSlot::Head;
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Equipment")
	FUmbraEquipmentItemDisplay CurrentItem;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	bool bLocked = false;
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Equipment")
	bool bSelected = false;
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Equipment")
	bool bHovered = false;

	/** State priority: Locked > Selected > Hovered > Equipped/Empty; HasItem remains independent. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment")
	void BP_RefreshVisual(EUmbraEquipmentSlotState State, const FUmbraEquipmentItemDisplay& ItemDisplay, bool bHasItem);
};
