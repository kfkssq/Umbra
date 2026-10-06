#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UmbraInventoryComponent.generated.h"

class UUmbraItemDefinition;

UENUM(BlueprintType)
enum class EUmbraInventoryResult : uint8 { Success, AuthorityRequired, NotReady, InvalidItem, InvalidIdentity, DuplicateInstance, Full, NotFound };

USTRUCT(BlueprintType)
struct FUmbraInventoryItem
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 SlotIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FGuid InstanceId;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UUmbraItemDefinition> Definition;
};

USTRUCT(BlueprintType)
struct FUmbraInventorySnapshot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) bool bReady = false;
	UPROPERTY(BlueprintReadOnly) int32 Capacity = 0;
	UPROPERTY(BlueprintReadOnly) TArray<FUmbraInventoryItem> Items;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUmbraInventoryChanged, const FUmbraInventorySnapshot&, Snapshot);

/** Minimal authority-owned, non-stacking storage. No equipment transfer, loot generation or persistence. */
UCLASS(ClassGroup = (Umbra), meta = (BlueprintSpawnableComponent))
class UMBRA_API UUmbraInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UUmbraInventoryComponent();
	/** Startup capacity; runtime snapshots are authoritative. Clamped to 0..512 cells. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "0", ClampMax = "512")) int32 InitialCapacity = 96;
	/** Trusted grant/import API. Caller must supply an existing identity or create it once at item creation. */
	UFUNCTION(BlueprintCallable, Category = "Inventory") EUmbraInventoryResult AddItem(UUmbraItemDefinition* Definition, FGuid InstanceId);
	/** Removes by identity, preserving other cells' indices. This does not equip or destroy a world actor. */
	UFUNCTION(BlueprintCallable, Category = "Inventory") EUmbraInventoryResult RemoveItem(FGuid InstanceId, FUmbraInventoryItem& RemovedItem);
	UFUNCTION(BlueprintPure, Category = "Inventory") FUmbraInventorySnapshot GetSnapshot() const;
	bool IsPublishing() const { return bPublishing; }
	UPROPERTY(BlueprintAssignable, Category = "Inventory") FUmbraInventoryChanged OnInventoryChanged;
	DECLARE_MULTICAST_DELEGATE_OneParam(FLifecycleChanged, UUmbraInventoryComponent*);
	static FLifecycleChanged OnLifecycleChanged;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
private:
	EUmbraInventoryResult CheckMutation() const;
	void Publish();
	UFUNCTION() void OnRep_Snapshot();
	UPROPERTY(ReplicatedUsing = OnRep_Snapshot) FUmbraInventorySnapshot Snapshot;
	bool bInitialized = false;
	bool bPublishing = false;
};
