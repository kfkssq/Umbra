#include "Inventory/UmbraInventoryComponent.h"

#include "Items/UmbraItemDefinition.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UUmbraInventoryComponent::FLifecycleChanged UUmbraInventoryComponent::OnLifecycleChanged;

UUmbraInventoryComponent::UUmbraInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UUmbraInventoryComponent::OnRegister()
{
	Super::OnRegister();
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (!bInitialized)
		{
			Snapshot.Capacity = FMath::Clamp(InitialCapacity, 0, 512);
			bInitialized = true;
		}
		Snapshot.bReady = true;
	}
	OnLifecycleChanged.Broadcast(this);
}

void UUmbraInventoryComponent::OnUnregister()
{
	Super::OnUnregister();
	// Keep contents when temporarily re-registering; views must stop showing them until ready.
	OnLifecycleChanged.Broadcast(this);
}

FUmbraInventorySnapshot UUmbraInventoryComponent::GetSnapshot() const
{
	auto Result = Snapshot;
	Result.bReady &= IsRegistered();
	return Result;
}

EUmbraInventoryResult UUmbraInventoryComponent::CheckMutation() const
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return EUmbraInventoryResult::AuthorityRequired;
	if (!IsRegistered() || !Snapshot.bReady || bPublishing) return EUmbraInventoryResult::NotReady;
	return EUmbraInventoryResult::Success;
}

EUmbraInventoryResult UUmbraInventoryComponent::AddItem(UUmbraItemDefinition* Definition, FGuid InstanceId)
{
	const auto Check = CheckMutation();
	if (Check != EUmbraInventoryResult::Success) return Check;
	if (!IsValid(Definition)) return EUmbraInventoryResult::InvalidItem;
	if (!InstanceId.IsValid()) return EUmbraInventoryResult::InvalidIdentity;
	if (Snapshot.Items.ContainsByPredicate([InstanceId](const auto& Item) { return Item.InstanceId == InstanceId; })) return EUmbraInventoryResult::DuplicateInstance;
	if (Snapshot.Items.Num() >= Snapshot.Capacity) return EUmbraInventoryResult::Full;
	int32 FreeIndex = 0;
	while (Snapshot.Items.ContainsByPredicate([FreeIndex](const auto& Item) { return Item.SlotIndex == FreeIndex; })) ++FreeIndex;
	auto& Entry = Snapshot.Items.AddDefaulted_GetRef();
	Entry.SlotIndex = FreeIndex;
	Entry.InstanceId = InstanceId;
	Entry.Definition = Definition;
	Publish();
	return EUmbraInventoryResult::Success;
}

EUmbraInventoryResult UUmbraInventoryComponent::RemoveItem(FGuid InstanceId, FUmbraInventoryItem& RemovedItem)
{
	RemovedItem = FUmbraInventoryItem();
	const auto Check = CheckMutation();
	if (Check != EUmbraInventoryResult::Success) return Check;
	if (!InstanceId.IsValid()) return EUmbraInventoryResult::InvalidIdentity;
	const int32 Index = Snapshot.Items.IndexOfByPredicate([InstanceId](const auto& Item) { return Item.InstanceId == InstanceId; });
	if (Index == INDEX_NONE) return EUmbraInventoryResult::NotFound;
	RemovedItem = Snapshot.Items[Index];
	Snapshot.Items.RemoveAt(Index);
	Publish();
	return EUmbraInventoryResult::Success;
}

void UUmbraInventoryComponent::Publish()
{
	TGuardValue<bool> Guard(bPublishing, true);
	OnInventoryChanged.Broadcast(GetSnapshot());
	if (GetOwner()) GetOwner()->ForceNetUpdate();
}

void UUmbraInventoryComponent::OnRep_Snapshot()
{
	Publish();
	OnLifecycleChanged.Broadcast(this);
}

void UUmbraInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UUmbraInventoryComponent, Snapshot, COND_OwnerOnly);
}
