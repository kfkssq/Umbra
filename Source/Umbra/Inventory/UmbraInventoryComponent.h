#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/UmbraItemInstance.h"
#include "UmbraInventoryComponent.generated.h"

class UUmbraItemDefinition;

UENUM(BlueprintType)
enum class EUmbraInventoryResult : uint8 { Success, AuthorityRequired, NotReady, InvalidItem, InvalidIdentity, DuplicateInstance, Full, NotFound, InvalidSlot, SlotOccupied };

USTRUCT(BlueprintType)
struct FUmbraInventoryItem
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 SlotIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FGuid InstanceId;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UUmbraItemDefinition> Definition;
	UPROPERTY(BlueprintReadOnly) TArray<FUmbraRolledAffix> RolledAffixes;
	/** Authoritative complete payload; legacy fields above remain Blueprint-compatible projections. */
	UPROPERTY(BlueprintReadOnly) FUmbraItemInstance Instance;
	void SetInstance(const FUmbraItemInstance& Value) { Instance = Value; Definition = Value.Definition; InstanceId = Value.InstanceId; RolledAffixes = Value.RolledAffixes; }
	FUmbraItemInstance GetInstance() const { if (Instance.SchemaVersion > 0) return Instance; FUmbraItemInstance Item; Item.Definition = Definition; Item.InstanceId = InstanceId; Item.RolledAffixes = RolledAffixes; return Item; }
};

USTRUCT(BlueprintType)
struct FUmbraInventorySnapshot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 Revision = 0;
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
	/** Trusted server import/transfer. Never rolls; clients cannot use this as an RPC. */
	EUmbraInventoryResult AddInstance(const FUmbraItemInstance& Instance, int32 TargetSlot = INDEX_NONE);
	/** Move to an empty cell or swap occupied cells, preserving both complete instances. */
	UFUNCTION(BlueprintCallable, Category = "Inventory") EUmbraInventoryResult MoveItem(FGuid InstanceId, int32 TargetSlot);
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
	/** Retained for this component's lifetime so regranting a known GUID cannot reroll it. */
	UPROPERTY(Transient) TMap<FGuid, FUmbraItemInstance> KnownInstances;
	bool bInitialized = false;
	bool bPublishing = false;
};
