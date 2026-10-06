#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Equipment/UmbraEquipmentTypes.h"
#include "UI/Preview/UmbraCharacterPreviewComponent.h"
#include "Equipment/UmbraEquipmentComponent.h"
#include "Player/UmbraPlayerState.h"
#include "UmbraEquipmentMenu.generated.h"

class UUmbraItemTooltip;
class UUmbraAbilitySystemComponent;
struct FOnAttributeChangeData;

class UImage;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UUmbraEquipmentSlotWidget;
class UUmbraEquipmentComponent;
class UUmbraAbilitySystemComponent;
struct FUmbraEquipmentSnapshot;

UCLASS(Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraEquipmentMenu : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Shared WBP_ItemTooltip, configured in the menu's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Tooltip") TSubclassOf<UUmbraItemTooltip> ItemTooltipClass;
	// Tooltips use the same page lifecycle as the portrait.
	UFUNCTION(BlueprintCallable, Category = "Equipment|Interaction")
	EUmbraUnequipResult RequestUnequipSlot(UUmbraEquipmentSlotWidget* EquipmentSlot);
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Equipment|Interaction") FText LastUnequipMessage;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Equipment|Interaction") EUmbraTransferResult LastUnequipTransferResult = EUmbraTransferResult::NotReady;
	/** Presentation-only feedback. Successful RemovedItem is ALREADY returned to inventory; never grant it again. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment|Interaction")
	void BP_UnequipFinished(EUmbraUnequipResult Result, const FUmbraEquippedItem& RemovedItem, const FText& Message);
	/** Defaults to gameplay snapshots; disable only for standalone presentation prototypes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Data") bool bBindEquipmentData = true;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Equipment|Data") bool bEquipmentDataReady = false;
	UFUNCTION(BlueprintCallable, Category = "Equipment|Data") void NotifyPlayerContextChanged();
	/** Called by the character menu after visibility/switcher events, never by Tick. */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void SetPageActive(bool bActive);
	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsPageActive() const { return bPageActive; }
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void RebuildSlotBindings();
	UFUNCTION(BlueprintPure, Category = "Equipment")
	UUmbraEquipmentSlotWidget* GetEquipmentSlot(EUmbraEquipmentSlot Type) const;
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool SetSlotItem(EUmbraEquipmentSlot Type, const FUmbraEquipmentItemDisplay& Item);
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void RefreshAppearance();
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void RefreshEquipmentVisuals();
	UFUNCTION(BlueprintPure, Category = "Equipment")
	UUmbraCharacterPreviewComponent* GetPreviewComponent() const { return PreviewComponent; }

protected:
	/** Invoked after all slots have been updated. Blueprint may refresh selected-item details or unavailable styling. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment|Data") void BP_EquipmentDataChanged(bool bReady);
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	/** Existing portrait BP or its child, with one skeletal mesh and one SceneCapture2D. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Preview")
	TSubclassOf<AActor> PreviewActorClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Preview")
	FUmbraCharacterPreviewSettings PreviewSettings;
	/** Separate stage in cm; do not place it near gameplay lighting or geometry. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Preview")
	FTransform PreviewActorTransform = FTransform(FVector(0, 0, -100000));
	/** Optional override. Otherwise use CharacterPreview's Designer material brush. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Preview")
	TObjectPtr<UMaterialInterface> PreviewMaterial;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Preview")
	FName TextureParameterName = TEXT("PortraitTexture");
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> CharacterPreview;

private:
	void HandleTooltipHover(UUmbraEquipmentSlotWidget* Slot, bool bHovered);
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
	TWeakObjectPtr<UUmbraEquipmentSlotWidget> HoveredTooltipSlot;
	TWeakObjectPtr<UUmbraAbilitySystemComponent> TooltipASC;
	TWeakObjectPtr<UUmbraEquipmentComponent> TooltipEquipment;
	FDelegateHandle TooltipASCLifecycleHandle;
	TArray<FDelegateHandle> TooltipAttributeHandles;
	bool bTooltipRefreshQueued = false;
	// A queued callback reads the current hover/context; it never captures snapshot data.
	UFUNCTION() void HandleUnequip(UUmbraEquipmentSlotWidget* EquipmentSlot);
	void BindEquipment();
	void UnbindEquipment();
	void RefreshSlotData();
	void OnASCLifecycle(UUmbraAbilitySystemComponent* ASC, bool bReady);
	UFUNCTION() void OnEquipmentChanged(const FUmbraEquipmentSnapshot& Snapshot);
	TWeakObjectPtr<UUmbraEquipmentComponent> BoundEquipment;
	TWeakObjectPtr<UUmbraEquipmentComponent> SelectionEquipment;
	FGuid SelectedInstanceId;
	TWeakObjectPtr<UUmbraAbilitySystemComponent> BoundASC;
	FDelegateHandle LifecycleHandle;
	bool bObservingEquipment = false;
	bool CreatePreview();
	void ReleasePreview();
	void UnbindSlots();
	UFUNCTION()
	void HandleSlotTypeChanged(UUmbraEquipmentSlotWidget* ChangedSlot);
	UFUNCTION()
	void HandleSelection(UUmbraEquipmentSlotWidget* Selected);
	UFUNCTION()
	void HandlePawnChanged(APawn* OldPawn, APawn* NewPawn);
	UPROPERTY(Transient)
	TMap<EUmbraEquipmentSlot, TObjectPtr<UUmbraEquipmentSlotWidget>> Slots;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UUmbraEquipmentSlotWidget>> BoundSlots;
	UPROPERTY(Transient)
	TObjectPtr<AActor> PreviewActor;
	UPROPERTY(Transient)
	TObjectPtr<UUmbraCharacterPreviewComponent> PreviewComponent;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PreviewMID;
	bool bPageActive = false;
};
