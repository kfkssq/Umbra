#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Player/UmbraPlayerState.h"
#include "Equipment/UmbraEquipmentComponent.h"
#include "Inventory/UmbraInventoryComponent.h"
#include "Stats/UmbraDerivedStatsComponent.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Items/UmbraItemDefinition.h"
#include "Items/UmbraWeaponProfile.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraInventoryTransferTest, "Umbra.Inventory.Transfer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraInventoryTransferTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	auto* Owner = World->SpawnActor<AUmbraPlayerState>();
	auto* ASC = Owner->GetUmbraAbilitySystemComponent();
	ASC->InitializeComponent();
	ASC->InitAbilityActorInfo(Owner, Owner);
	Owner->InitializeAttributes();
	auto* Derived = Owner->FindComponentByClass<UUmbraDerivedStatsComponent>();
	auto* Equipment = Owner->FindComponentByClass<UUmbraEquipmentComponent>();
	auto* Inventory = Owner->GetInventoryComponent();
	Derived->bUseWeaponDerivedPower = true;
	Equipment->bEnableEquipment = true;
	Owner->InitializeAttributes();
	TestTrue(TEXT("Equipment ready for transfer"), Equipment->IsReadyForCombat());
	TestTrue(TEXT("Inventory ready for transfer"), Inventory->GetSnapshot().bReady);
	TestEqual(TEXT("Transfer requires valid identity"), Owner->EquipFromInventory(EUmbraEquipmentSlot::MainHand, FGuid()), EUmbraTransferResult::InvalidIdentity);

	auto* Sword = NewObject<UUmbraItemDefinition>(Owner);
	Sword->AllowedSlots = {EUmbraEquipmentSlot::MainHand};
	Sword->Weapon = NewObject<UUmbraWeaponProfile>(Sword);
	auto& Channel = Sword->Weapon->Damage.Channels.AddDefaulted_GetRef();
	Channel.BaseDamage = 100.f;
	Channel.Scaling.AddDefaulted_GetRef().Coefficient = 2.f;
	Sword->Requirements.Strength = 5.f;
	Sword->PrimaryBonuses.Strength = 10.f;
	Sword->Weight = 20.f;
	Sword->RequiredLevel = 1;

	const FGuid SwordId = FGuid::NewGuid();
	TestEqual(TEXT("Equip unknown inventory identity"), Owner->EquipFromInventory(EUmbraEquipmentSlot::MainHand, SwordId), EUmbraTransferResult::NotFound);
	TestEqual(TEXT("Import into inventory"), Inventory->AddItem(Sword, SwordId), EUmbraInventoryResult::Success);
	TestEqual(TEXT("Equip from inventory"), Owner->EquipFromInventory(EUmbraEquipmentSlot::MainHand, SwordId), EUmbraTransferResult::Success);
	TestTrue(TEXT("Identity preserved in equipment"), Equipment->GetSnapshot().Items.ContainsByPredicate([SwordId](const FUmbraEquippedItem& E) { return E.InstanceId == SwordId; }));
	TestFalse(TEXT("Identity consumed from inventory"), Inventory->GetSnapshot().Items.ContainsByPredicate([SwordId](const FUmbraInventoryItem& I) { return I.InstanceId == SwordId; }));

	const FGuid SecondId = FGuid::NewGuid();
	Inventory->AddItem(Sword, SecondId);
	TestEqual(TEXT("Occupied slot rejects transfer"), Owner->EquipFromInventory(EUmbraEquipmentSlot::MainHand, SecondId), EUmbraTransferResult::SlotOccupied);
	TestTrue(TEXT("Rejected transfer keeps inventory item"), Inventory->GetSnapshot().Items.ContainsByPredicate([SecondId](const FUmbraInventoryItem& I) { return I.InstanceId == SecondId; }));
	TestEqual(TEXT("Wrong slot rejected"), Owner->EquipFromInventory(EUmbraEquipmentSlot::Head, SecondId), EUmbraTransferResult::InvalidSlot);

	FUmbraInventoryItem Returned;
	TestEqual(TEXT("Unequip to inventory"), Owner->UnequipToInventory(EUmbraEquipmentSlot::MainHand, SwordId, Returned), EUmbraTransferResult::Success);
	TestEqual(TEXT("Returned identity preserved"), Returned.InstanceId, SwordId);
	TestFalse(TEXT("Equipment slot cleared"), Equipment->GetSnapshot().Items.ContainsByPredicate([SwordId](const FUmbraEquippedItem& E) { return E.InstanceId == SwordId; }));
	TestTrue(TEXT("Item returned to inventory"), Inventory->GetSnapshot().Items.ContainsByPredicate([SwordId](const FUmbraInventoryItem& I) { return I.InstanceId == SwordId; }));
	TestEqual(TEXT("Stale unequip identity"), Owner->UnequipToInventory(EUmbraEquipmentSlot::MainHand, FGuid::NewGuid(), Returned), EUmbraTransferResult::NotFound);

	// Duplicate: the identity already lives in the inventory while still equipped.
	Equipment->Equip(EUmbraEquipmentSlot::MainHand, Sword, SwordId);
	TestEqual(TEXT("Duplicate blocks return"), Owner->UnequipToInventory(EUmbraEquipmentSlot::MainHand, SwordId, Returned), EUmbraTransferResult::DuplicateInstance);
	TestTrue(TEXT("Duplicate return keeps equipment"), Equipment->GetSnapshot().Items.ContainsByPredicate([SwordId](const FUmbraEquippedItem& E) { return E.InstanceId == SwordId; }));
	Inventory->RemoveItem(SwordId, Returned);
	TestEqual(TEXT("Unequip after duplicate cleared"), Owner->UnequipToInventory(EUmbraEquipmentSlot::MainHand, SwordId, Returned), EUmbraTransferResult::Success);

	// Full inventory blocks return; equipment keeps the item.
	Equipment->Equip(EUmbraEquipmentSlot::MainHand, Sword, SwordId);
	for (int32 Index = 0; Index < 96; ++Index) Inventory->AddItem(Sword, FGuid::NewGuid());
	TestEqual(TEXT("Full inventory rejects return"), Owner->UnequipToInventory(EUmbraEquipmentSlot::MainHand, SwordId, Returned), EUmbraTransferResult::InventoryFull);
	TestTrue(TEXT("Full inventory keeps equipment"), Equipment->GetSnapshot().Items.ContainsByPredicate([SwordId](const FUmbraEquippedItem& E) { return E.InstanceId == SwordId; }));

	World->DestroyWorld(false);
	return true;
}
#endif
