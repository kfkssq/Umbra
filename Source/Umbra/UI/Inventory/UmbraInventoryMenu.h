#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UmbraInventoryMenu.generated.h"

class UTextBlock;
class UUniformGridPanel;
class UUmbraInventorySlot;

/** Owns empty cell allocation and exclusive selection; Blueprint supplies layout and assets. */
UCLASS(Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraInventoryMenu : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Idempotent across Construct calls; rebuilds only when configuration/tree differs. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeSlots();
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetInventoryCapacity() const { return FMath::Max(0, InventoryCapacity); }
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetGeneratedSlotCount() const { return Slots.Num(); }
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetSelectedSlotIndex() const;
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UUmbraInventorySlot* GetInventorySlot(int32 Index) const;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Class Defaults are the configuration source; zero capacity is a valid empty grid. */
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
	void RefreshCapacityText();
	void UnbindSlots();
	void ClearSlots();
	UFUNCTION()
	void HandleSelection(UUmbraInventorySlot* RequestedSlot);
	UPROPERTY(Transient)
	TArray<TObjectPtr<UUmbraInventorySlot>> Slots;
	UPROPERTY(Transient)
	TObjectPtr<UUmbraInventorySlot> SelectedSlot;
	UPROPERTY(Transient)
	TSubclassOf<UUmbraInventorySlot> BuiltSlotClass;
	int32 BuiltColumns = 0;
};
