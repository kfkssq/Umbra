#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Equipment/UmbraEquipmentSlot.h"
#include "Audio/UmbraAudioTypes.h"
#include "UmbraEquipmentTypes.generated.h"

class UTexture2D;
class UUmbraItemDefinition;
struct FUmbraItemInstance;

UENUM(BlueprintType)
enum class EUmbraEquipmentSlotState : uint8
{
	Empty, Equipped, Hovered, Selected, Locked
};

/** Presentation projection of an equipment snapshot; contains no combat rules. */
USTRUCT(BlueprintType)
struct UMBRA_API FUmbraEquipmentItemDisplay
{
	GENERATED_BODY()

	/** Opaque identity/reference only. Widgets never interpret gameplay fields on this object. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	TObjectPtr<UObject> Item = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "Equipment") FGuid InstanceId;
	UPROPERTY(BlueprintReadOnly, Category = "Inventory") int32 Quantity = 1;
	UPROPERTY(BlueprintReadOnly, Category = "Inventory") bool bShowQuantity = false;
	UPROPERTY(BlueprintReadOnly, Category = "Equipment") FUmbraAudioContext AudioContext;
	UPROPERTY(BlueprintReadOnly, Category = "Equipment") bool bFromEquipmentSnapshot = false;
	UPROPERTY(BlueprintReadOnly, Category = "Equipment") bool bRequirementsMet = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FSlateBrush Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment") TObjectPtr<UTexture2D> SlotBackgroundTexture;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment") TObjectPtr<UTexture2D> RarityFrameTexture;
	/** Shared projection for both slot families; no gameplay calculation. */
	static FUmbraEquipmentItemDisplay FromDefinition(UUmbraItemDefinition* Definition, FGuid Id = FGuid());
	static FUmbraEquipmentItemDisplay FromInstance(const FUmbraItemInstance& Instance);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FLinearColor RarityColor = FLinearColor::White;
	UPROPERTY(BlueprintReadOnly, Category = "Equipment") bool bUseRarityBackgroundColor = false;
};
