#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/UmbraInventoryComponent.h"
#include "Player/UmbraPlayerState.h"
#include "UmbraInventoryMenu.generated.h"

class UUmbraItemTooltip;
class UUmbraAbilitySystemComponent;
struct FOnAttributeChangeData;

class UTextBlock;
class UUniformGridPanel;
class UUmbraInventorySlot;

UENUM(BlueprintType)
enum class EUmbraInventoryViewState : uint8 { NotReady, Ready, InvalidSnapshot };

/** Projects the owning player's inventory; Blueprint supplies layout and assets. */
UCLASS(Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraInventoryMenu : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Shared WBP_ItemTooltip, configured in the menu's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Tooltip") TSubclassOf<UUmbraItemTooltip> ItemTooltipClass;
	/** CharacterMenu supplies effective ancestor/page visibility. */
	void SetPageActive(bool bActive);
	UFUNCTION(BlueprintCallable, Category = "Inventory|Interaction") EUmbraTransferResult RequestEquipItem(FGuid InstanceId);
	/** Explicit choice after SlotSelectionRequired; ownership and AllowedSlots are revalidated. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Interaction") EUmbraTransferResult RequestEquipInSlot(FGuid InstanceId, EUmbraEquipmentSlot TargetSlot);
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Inventory|Interaction") FText LastEquipMessage;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Inventory|Interaction") EUmbraTransferResult LastEquipResult = EUmbraTransferResult::NotReady;
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Interaction") void BP_EquipFinished(EUmbraTransferResult Result, FGuid InstanceId, const FText& Message);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Data") bool bBindInventoryData = true;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Inventory|Data") bool bInventoryDataReady = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Inventory|Data") EUmbraInventoryViewState InventoryViewState = EUmbraInventoryViewState::NotReady;
	static EUmbraInventoryViewState ValidateSnapshot(const FUmbraInventorySnapshot& Snapshot);
	UFUNCTION(BlueprintCallable, Category = "Inventory|Data") void NotifyPlayerContextChanged();
	/** Idempotent across Construct calls; rebuilds only when configuration/tree differs. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeSlots();
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetInventoryCapacity() const;
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetGeneratedSlotCount() const { return Slots.Num(); }
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetSelectedSlotIndex() const;
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UUmbraInventorySlot* GetInventorySlot(int32 Index) const;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Data") void BP_InventoryDataChanged(bool bReady);
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Designer/standalone preview only. Live capacity comes from the InventoryComponent. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "0", UIMin = "0"))
	int32 InventoryCapacity = 40;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1", UIMin = "1"))
	int32 Columns = 8;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UUmbraInventorySlot> InventorySlotClass;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> InventoryGrid;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CapacityText;

private:
	void HandleTooltipHover(UUmbraInventorySlot* Slot, bool bHovered);
	void RefreshHoveredTooltip();
	void ClearHoveredTooltip(bool bForgetHover = true);
	void BindTooltipContext();
	void UnbindTooltipContext();
	void QueueTooltipRefresh();
	UFUNCTION() void HandleTooltipVisibility(ESlateVisibility InVisibility);
	void OnTooltipPrimaryChanged(const FOnAttributeChangeData& Change);
	void OnTooltipASCLifecycle(UUmbraAbilitySystemComponent* ASC, bool bReady);
	UFUNCTION() void OnTooltipEquipmentChanged(const FUmbraEquipmentSnapshot& Snapshot);
	UPROPERTY(Transient) TObjectPtr<UUmbraItemTooltip> CachedTooltip;
	TWeakObjectPtr<UUmbraInventorySlot> HoveredTooltipSlot;
	TWeakObjectPtr<UUmbraAbilitySystemComponent> TooltipASC;
	TWeakObjectPtr<UUmbraEquipmentComponent> TooltipEquipment;
	FDelegateHandle TooltipASCLifecycleHandle;
	TArray<FDelegateHandle> TooltipAttributeHandles;
	bool bTooltipRefreshQueued = false;
	bool bPageActive = true;
	EUmbraTransferResult TryEquip(FGuid InstanceId, const EUmbraEquipmentSlot* ExplicitSlot);
	EUmbraTransferResult FinishEquip(EUmbraTransferResult Result, FGuid InstanceId);
	UFUNCTION() void HandleEquip(UUmbraInventorySlot* RequestedSlot);
	void BindInventory();
	void UnbindInventory();
	void RefreshInventory();
	void OnInventoryLifecycle(UUmbraInventoryComponent* Component);
	UFUNCTION() void HandleInventoryChanged(const FUmbraInventorySnapshot& Snapshot);
	UFUNCTION() void HandlePawnChanged(APawn* OldPawn, APawn* NewPawn);
	UPROPERTY(Transient) FUmbraInventorySnapshot ViewSnapshot;
	TWeakObjectPtr<UUmbraInventoryComponent> BoundInventory;
	FDelegateHandle LifecycleHandle;
	bool bObserving = false;
	bool bRefreshing = false;
	void RefreshCapacityText();
	void UnbindSlots();
	void ClearSlots();
	UFUNCTION()
	void HandleSelection(UUmbraInventorySlot* RequestedSlot);
	UPROPERTY(Transient)
	TArray<TObjectPtr<UUmbraInventorySlot>> Slots;
	UPROPERTY(Transient)
	TObjectPtr<UUmbraInventorySlot> SelectedSlot;
	FGuid SelectedInstanceId;
	TWeakObjectPtr<UUmbraInventoryComponent> SelectionInventory;
	UPROPERTY(Transient)
	TSubclassOf<UUmbraInventorySlot> BuiltSlotClass;
	int32 BuiltColumns = 0;
};
