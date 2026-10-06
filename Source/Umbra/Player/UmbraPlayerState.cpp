// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/UmbraPlayerState.h"
#include "Umbra.h"
#include "Stats/UmbraDerivedStatsComponent.h"
#include "Equipment/UmbraEquipmentComponent.h"
#include "Inventory/UmbraInventoryComponent.h"

#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/UmbraGameplayAbility.h"
#include "AbilitySystem/Abilities/UmbraBasicAttackAbility.h"

AUmbraPlayerState::AUmbraPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UUmbraAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UUmbraAttributeSet>(TEXT("AttributeSet"));
	DerivedStatsComponent = CreateDefaultSubobject<UUmbraDerivedStatsComponent>(TEXT("DerivedStatsComponent"));
	EquipmentComponent = CreateDefaultSubobject<UUmbraEquipmentComponent>(TEXT("EquipmentComponent"));
	InventoryComponent = CreateDefaultSubobject<UUmbraInventoryComponent>(TEXT("InventoryComponent"));

	SetNetUpdateFrequency(100.0f);
}

UAbilitySystemComponent* AUmbraPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AUmbraPlayerState::BeginPlay()
{
	Super::BeginPlay();
	InitializeInventory();
}

void AUmbraPlayerState::InitializeInventory()
{
	if (!HasAuthority() || bInitialInventoryGranted || !InventoryComponent
		|| !InventoryComponent->GetSnapshot().bReady || InventoryComponent->IsPublishing()) return;
	// Mark before broadcasting AddItem events; repeated initialization cannot grant another copy.
	bInitialInventoryGranted = true;
	for (const auto& Reference : InitialInventoryItems)
	{
		auto* Definition = Reference.LoadSynchronous();
		if (!Definition)
		{
			UE_LOG(LogUmbra, Warning, TEXT("Initial inventory item could not load: %s"), *Reference.ToString());
			continue;
		}
		const auto Result = InventoryComponent->AddItem(Definition, FGuid::NewGuid());
		if (Result != EUmbraInventoryResult::Success)
			UE_LOG(LogUmbra, Warning, TEXT("Initial inventory grant failed for %s: %s"), *Reference.ToString(), *UEnum::GetValueAsString(Result));
	}
}

const UUmbraBasicAttackAbility* AUmbraPlayerState::GetPrimaryAttackAbilityDefaults() const
{
	for (const TSubclassOf<UUmbraGameplayAbility>& AbilityClass : InitialAbilities)
	{
		if (AbilityClass && AbilityClass->IsChildOf(UUmbraBasicAttackAbility::StaticClass()))
		{
			return Cast<UUmbraBasicAttackAbility>(AbilityClass.GetDefaultObject());
		}
	}
	return nullptr;
}

void AUmbraPlayerState::GrantInitialAbilities()
{
	if (!HasAuthority() || bInitialAbilitiesGranted || !AbilitySystemComponent)
	{
		return;
	}

	for (const TSubclassOf<UUmbraGameplayAbility>& AbilityClass : InitialAbilities)
	{
		if (AbilityClass)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
		}
	}

	bInitialAbilitiesGranted = true;
}

void AUmbraPlayerState::InitializeAttributes()
{
	AbilitySystemComponent->InitializeAttributes(InitialAttributesEffect, bUseDebugInitialAttributes ? &DebugInitialAttributes : nullptr);
	DerivedStatsComponent->Initialize(AbilitySystemComponent);
	EquipmentComponent->Initialize(AbilitySystemComponent);
}

namespace
{
	EUmbraTransferResult MapEquipResult(EUmbraEquipResult Result)
	{
		switch (Result)
		{
		case EUmbraEquipResult::Success: return EUmbraTransferResult::Success;
		case EUmbraEquipResult::AppliedInvalid: return EUmbraTransferResult::AppliedInvalid;
		case EUmbraEquipResult::NotReady: return EUmbraTransferResult::NotReady;
		case EUmbraEquipResult::InvalidItem: return EUmbraTransferResult::InvalidItem;
		case EUmbraEquipResult::WrongSlot: return EUmbraTransferResult::InvalidSlot;
		case EUmbraEquipResult::LevelTooLow: return EUmbraTransferResult::LevelTooLow;
		case EUmbraEquipResult::DuplicateInstance: return EUmbraTransferResult::DuplicateInstance;
		case EUmbraEquipResult::EffectRejected: return EUmbraTransferResult::EffectRejected;
		}
		return EUmbraTransferResult::TransferFailed;
	}

	EUmbraTransferResult MapUnequipResult(EUmbraUnequipResult Result)
	{
		switch (Result)
		{
		case EUmbraUnequipResult::Success: return EUmbraTransferResult::Success;
		case EUmbraUnequipResult::AppliedInvalid: return EUmbraTransferResult::AppliedInvalid;
		case EUmbraUnequipResult::NotReady: return EUmbraTransferResult::NotReady;
		case EUmbraUnequipResult::AuthorityRequired: return EUmbraTransferResult::AuthorityRequired;
		case EUmbraUnequipResult::InvalidSlot: return EUmbraTransferResult::InvalidSlot;
		case EUmbraUnequipResult::EmptySlot: return EUmbraTransferResult::NotFound;
		case EUmbraUnequipResult::StaleInstance: return EUmbraTransferResult::NotFound;
		case EUmbraUnequipResult::Locked: return EUmbraTransferResult::NotFound;
		case EUmbraUnequipResult::Failed: return EUmbraTransferResult::TransferFailed;
		}
		return EUmbraTransferResult::TransferFailed;
	}
}

EUmbraTransferResult AUmbraPlayerState::EquipFromInventory(EUmbraEquipmentSlot Slot, FGuid InstanceId)
{
	if (!HasAuthority()) return EUmbraTransferResult::AuthorityRequired;
	if (!InventoryComponent || !EquipmentComponent) return EUmbraTransferResult::NotReady;
	if (bTransferInProgress || InventoryComponent->IsPublishing()) return EUmbraTransferResult::NotReady;
	TGuardValue<bool> TransferGuard(bTransferInProgress, true);
	if (uint8(Slot) > uint8(EUmbraEquipmentSlot::OffHand)) return EUmbraTransferResult::InvalidSlot;
	if (!InstanceId.IsValid()) return EUmbraTransferResult::InvalidIdentity;

	const FUmbraInventorySnapshot InvSnap = InventoryComponent->GetSnapshot();
	if (!InvSnap.bReady || !EquipmentComponent->IsReadyForCombat()) return EUmbraTransferResult::NotReady;

	const FUmbraInventoryItem* Item = InvSnap.Items.FindByPredicate(
		[InstanceId](const FUmbraInventoryItem& Entry) { return Entry.InstanceId == InstanceId; });
	if (!Item) return EUmbraTransferResult::NotFound;
	if (!IsValid(Item->Definition)) return EUmbraTransferResult::InvalidItem;

	const FUmbraEquipmentSnapshot EquipSnap = EquipmentComponent->GetSnapshot();
	if (EquipSnap.Items.ContainsByPredicate([Slot](const FUmbraEquippedItem& Entry) { return Entry.Slot == Slot; }))
	{
		return EUmbraTransferResult::SlotOccupied;
	}

	const EUmbraEquipResult EquipResult = EquipmentComponent->Equip(Slot, Item->Definition, Item->InstanceId);
	const EUmbraTransferResult Mapped = MapEquipResult(EquipResult);
	if (Mapped != EUmbraTransferResult::Success && Mapped != EUmbraTransferResult::AppliedInvalid) return Mapped;

	FUmbraInventoryItem Removed;
	if (InventoryComponent->RemoveItem(InstanceId, Removed) != EUmbraInventoryResult::Success)
	{
		if (!EquipmentComponent->Unequip(Slot))
		{
			UE_LOG(LogUmbra, Error, TEXT("EquipFromInventory rollback failed; instance %s may be stranded in equipment."), *InstanceId.ToString());
		}
		return EUmbraTransferResult::TransferFailed;
	}
	return Mapped;
}

EUmbraTransferResult AUmbraPlayerState::UnequipToInventory(EUmbraEquipmentSlot Slot, FGuid ExpectedInstanceId, FUmbraInventoryItem& OutReturnedItem)
{
	OutReturnedItem = FUmbraInventoryItem();
	if (!HasAuthority()) return EUmbraTransferResult::AuthorityRequired;
	if (!InventoryComponent || !EquipmentComponent) return EUmbraTransferResult::NotReady;
	if (bTransferInProgress || InventoryComponent->IsPublishing()) return EUmbraTransferResult::NotReady;
	TGuardValue<bool> TransferGuard(bTransferInProgress, true);
	if (uint8(Slot) > uint8(EUmbraEquipmentSlot::OffHand)) return EUmbraTransferResult::InvalidSlot;
	if (!ExpectedInstanceId.IsValid()) return EUmbraTransferResult::InvalidIdentity;

	const FUmbraInventorySnapshot InvSnap = InventoryComponent->GetSnapshot();
	if (!InvSnap.bReady || !EquipmentComponent->IsReadyForCombat()) return EUmbraTransferResult::NotReady;
	if (InvSnap.Items.Num() >= InvSnap.Capacity) return EUmbraTransferResult::InventoryFull;

	const FUmbraEquipmentSnapshot EquipSnap = EquipmentComponent->GetSnapshot();
	const FUmbraEquippedItem* Entry = EquipSnap.Items.FindByPredicate(
		[Slot](const FUmbraEquippedItem& Equipped) { return Equipped.Slot == Slot; });
	if (!Entry || !Entry->InstanceId.IsValid() || Entry->InstanceId != ExpectedInstanceId) return EUmbraTransferResult::NotFound;
	if (InvSnap.Items.ContainsByPredicate([Entry](const FUmbraInventoryItem& Existing) { return Existing.InstanceId == Entry->InstanceId; }))
	{
		return EUmbraTransferResult::DuplicateInstance;
	}

	FUmbraEquippedItem RemovedEquip;
	const EUmbraUnequipResult UnequipResult = EquipmentComponent->TryUnequipInstance(Slot, ExpectedInstanceId, RemovedEquip);
	const EUmbraTransferResult Mapped = MapUnequipResult(UnequipResult);
	if (Mapped != EUmbraTransferResult::Success && Mapped != EUmbraTransferResult::AppliedInvalid) return Mapped;

	if (InventoryComponent->AddItem(RemovedEquip.Definition, RemovedEquip.InstanceId) != EUmbraInventoryResult::Success)
	{
		if (EquipmentComponent->Equip(Slot, RemovedEquip.Definition, RemovedEquip.InstanceId) != EUmbraEquipResult::Success)
		{
			UE_LOG(LogUmbra, Error, TEXT("UnequipToInventory rollback failed; instance %s may be lost."), *RemovedEquip.InstanceId.ToString());
		}
		return EUmbraTransferResult::TransferFailed;
	}

	// Keep the snapshot alive until its entry is copied; a pointer into a temporary snapshot dangles.
	const auto ReturnedSnapshot = InventoryComponent->GetSnapshot();
	const FUmbraInventoryItem* Stored = ReturnedSnapshot.Items.FindByPredicate(
		[&RemovedEquip](const FUmbraInventoryItem& Existing) { return Existing.InstanceId == RemovedEquip.InstanceId; });
	if (Stored) OutReturnedItem = *Stored;
	else { OutReturnedItem.Definition = RemovedEquip.Definition; OutReturnedItem.InstanceId = RemovedEquip.InstanceId; }
	return Mapped;
}
