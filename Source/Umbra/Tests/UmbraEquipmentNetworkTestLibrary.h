#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Inventory/UmbraItemTransferRequest.h"
#include "UmbraEquipmentNetworkTestLibrary.generated.h"
class AUmbraPlayerState;
class AUmbraPlayerController;
class AUmbraEnemyCharacter;

/** Read-only PIE evidence for the actual replicated GE context, not a reconstructed client roll. */
UCLASS()
class UUmbraEquipmentNetworkTestLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Test driver: uses the real health-death delegate and enemy loot path. */
	UFUNCTION(BlueprintCallable, meta=(DevelopmentOnly)) static bool KillEnemyForLootTest(AUmbraEnemyCharacter* Enemy);
	UFUNCTION(BlueprintCallable, meta=(DevelopmentOnly)) static AUmbraEnemyCharacter* SpawnEnemyForLootTest(AUmbraEnemyCharacter* Template);
	UFUNCTION(BlueprintPure, meta=(DevelopmentOnly)) static int32 CountDamageSnapshots(AUmbraPlayerState* State);
	/** Editor-only reflected settings adapter; returns prior mode/count/process values for restoration. */
	UFUNCTION(BlueprintCallable, meta=(DevelopmentOnly)) static TArray<int32> ConfigurePlaySettings(int32 Mode, int32 Clients, bool bSingleProcess);
	UFUNCTION(BlueprintCallable, meta=(DevelopmentOnly)) static EUmbraTransferResult SubmitRemoteRequest(AUmbraPlayerController* Controller, EUmbraItemTransferOperation Operation, FGuid Id, EUmbraEquipmentSlot Slot, int32 Cell);
};
