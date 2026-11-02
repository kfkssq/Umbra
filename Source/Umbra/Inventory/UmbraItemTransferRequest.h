#pragma once
#include "CoreMinimal.h"
#include "Player/UmbraPlayerState.h"
#include "UmbraItemTransferRequest.generated.h"

UENUM(BlueprintType)
enum class EUmbraItemTransferOperation : uint8 { Equip, Unequip, Move };

/** Client identifies its own item and expected view only; never supplies gameplay values or another player. */
USTRUCT(BlueprintType)
struct FUmbraItemTransferRequest
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 RequestId = 0;
	UPROPERTY(BlueprintReadOnly) EUmbraItemTransferOperation Operation = EUmbraItemTransferOperation::Equip;
	UPROPERTY(BlueprintReadOnly) FGuid InstanceId;
	UPROPERTY(BlueprintReadOnly) EUmbraEquipmentSlot EquipmentSlot = EUmbraEquipmentSlot::Head;
	UPROPERTY(BlueprintReadOnly) int32 TargetCell = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) int32 InventoryRevision = 0;
	UPROPERTY(BlueprintReadOnly) int32 EquipmentRevision = 0;
};

USTRUCT(BlueprintType)
struct FUmbraItemTransferResponse
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FUmbraItemTransferRequest Request;
	UPROPERTY(BlueprintReadOnly) EUmbraTransferResult Result = EUmbraTransferResult::NotReady;
	UPROPERTY(BlueprintReadOnly) FUmbraItemInstance Instance;
};
