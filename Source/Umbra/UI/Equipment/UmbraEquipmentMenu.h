#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Equipment/UmbraEquipmentTypes.h"
#include "UI/Preview/UmbraCharacterPreviewComponent.h"
#include "UmbraEquipmentMenu.generated.h"

class UImage;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UUmbraEquipmentSlotWidget;

UCLASS(Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraEquipmentMenu : public UUserWidget
{
	GENERATED_BODY()
public:
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
