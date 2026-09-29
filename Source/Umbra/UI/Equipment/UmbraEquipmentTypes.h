#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "UmbraEquipmentTypes.generated.h"

UENUM(BlueprintType)
enum class EUmbraEquipmentSlot : uint8
{
	Head, Chest, Hands, Legs, Feet, Amulet, Ring1, Ring2, MainHand, OffHand
};

UENUM(BlueprintType)
enum class EUmbraEquipmentSlotState : uint8
{
	Empty, Equipped, Hovered, Selected, Locked
};

/** Presentation snapshot supplied by a future equipment data owner; contains no combat rules. */
USTRUCT(BlueprintType)
struct UMBRA_API FUmbraEquipmentItemDisplay
{
	GENERATED_BODY()

	/** Opaque identity/reference only. Widgets never interpret gameplay fields on this object. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	TObjectPtr<UObject> Item = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FSlateBrush Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FLinearColor RarityColor = FLinearColor::White;
};
