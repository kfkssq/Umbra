#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Loot/UmbraWorldItem.h"
#include "Loot/UmbraWorldItemVisualComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Player/UmbraPlayerState.h"
#include "Inventory/UmbraInventoryComponent.h"
#include "UmbraPlayerController.h"
#include "Tests/UmbraLootTestTypes.h"
#include "Characters/UmbraPlayerCharacter.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "Equipment/UmbraEquipmentComponent.h"
#include "Stats/UmbraDerivedStatsComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraArmorLootTest, "Umbra.Loot.Armor1730Definitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraArmorLootTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	auto* Owner = World->SpawnActor<AUmbraPlayerState>();
	auto* Inventory = Owner->GetInventoryComponent();
	auto* Controller = World->SpawnActor<AUmbraLootTestController>();
	if (!TestNotNull(TEXT("Controller spawned"), Controller)) { World->DestroyWorld(false); return false; }
	Controller->SetPlayerState(Owner);
	auto* Pawn = World->SpawnActor<AUmbraPlayerCharacter>();
	Owner->FindComponentByClass<UUmbraDerivedStatsComponent>()->bUseWeaponDerivedPower = true;
	Owner->FindComponentByClass<UUmbraEquipmentComponent>()->bEnableEquipment = true;
	Owner->GetUmbraAbilitySystemComponent()->InitializeComponent();
	Controller->Possess(Pawn);
	Pawn->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	TestTrue(TEXT("Player can pick up"), Controller->CanPickupItems());
	const TCHAR* Names[] = {TEXT("Head"), TEXT("Chest"), TEXT("Hands"), TEXT("Legs")};
	const float Armor[] = {8, 20, 6, 12}, Resistance[] = {4, 8, 3, 5}, Weight[] = {3, 8, 2, 5};
	TSet<FGuid> Ids;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FString Path = FString::Printf(TEXT("/Game/Items/Armor/DA_Armor_1730_%s.DA_Armor_1730_%s"), Names[Index], Names[Index]);
		auto* Definition = LoadObject<UUmbraItemDefinition>(nullptr, *Path);
		if (!TestNotNull(*Path, Definition)) continue;
		TestEqual(TEXT("Armor"), Definition->Armor, Armor[Index]);
		TestEqual(TEXT("Magic resistance"), Definition->MagicResistance, Resistance[Index]);
		TestEqual(TEXT("Weight"), Definition->Weight, Weight[Index]);
		TestEqual(TEXT("Level"), Definition->RequiredLevel, 1);
		TestTrue(TEXT("Single correct slot"), Definition->AllowedSlots == TArray<EUmbraEquipmentSlot>{static_cast<EUmbraEquipmentSlot>(Index)});
		TestNull(TEXT("No weapon profile"), Definition->Weapon.Get());
		TestTrue(TEXT("No affixes"), Definition->AffixRollConfigs.IsEmpty() && Definition->AttributeBonuses.IsEmpty() && Definition->DamageBonuses.IsEmpty());
		for (float Value : Definition->Requirements.ToArray()) TestEqual(TEXT("No primary requirement"), Value, 0.f);
		for (float Value : Definition->PrimaryBonuses.ToArray()) TestEqual(TEXT("No primary grant"), Value, 0.f);
		TestNotNull(TEXT("Real skeletal mesh loads"), Definition->WorldDropSkeletalMesh.LoadSynchronous());
		FUmbraItemInstance Instance;
		TestTrue(TEXT("Create authority instance"), FUmbraItemInstance::Create(Owner, Definition, FGuid::NewGuid(), Instance));
		TestFalse(TEXT("Distinct identity"), Ids.Contains(Instance.InstanceId));
		Ids.Add(Instance.InstanceId);
		auto* Drop = World->SpawnActor<AUmbraWorldItem>();
		TestTrue(TEXT("Initialize drop"), Drop->InitializeInstance(Instance));
		TestFalse(TEXT("Cannot initialize twice"), Drop->InitializeInstance(Instance));
		Drop->SetActorLocation(Pawn->GetActorLocation());
		TestEqual(TEXT("Authority pickup through PlayerState"), Drop->TryPickup(Controller), EUmbraPickupResult::Success);
		TestTrue(TEXT("Drop removed after success"), Drop->IsActorBeingDestroyed());
		TestEqual(TEXT("Second pickup rejected"), Drop->TryPickup(Controller), EUmbraPickupResult::Unavailable);
		TestEqual(TEXT("Duplicate rejected"), Inventory->AddInstance(Instance), EUmbraInventoryResult::DuplicateInstance);
		TestTrue(TEXT("Identity preserved in inventory"), Inventory->GetSnapshot().Items.ContainsByPredicate(
			[&](const auto& Entry) { return Entry.InstanceId == Instance.InstanceId && Entry.Definition == Definition; }));
	}
	TestEqual(TEXT("Four inventory entries"), Inventory->GetSnapshot().Items.Num(), 4);
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraLootVisualCompatibilityTest, "Umbra.Loot.VisualMeshCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraLootVisualCompatibilityTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	auto* Owner = World->SpawnActor<AActor>();
	auto* Visual = NewObject<UUmbraWorldItemVisualComponent>(Owner);
	auto* Static = NewObject<UStaticMeshComponent>(Owner);
	auto* Skeletal = NewObject<USkeletalMeshComponent>(Owner);
	Static->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Skeletal->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Asset/Equipments/Head/head_01.head_01")));
	TestNotNull(TEXT("Skeletal source"), Skeletal->GetSkeletalMeshAsset());
	const FTransform Rest(FRotator(0, 30, 0), FVector(1, 2, 3), FVector(0.5));
	const FGuid Id = FGuid::NewGuid();
	Visual->InitializeVisual(Static, Id, Rest, FLinearColor::White, -1, true);
	Visual->SetHighlighted(true);
	TestTrue(TEXT("Static outline preserved"), Static->bRenderCustomDepth);
	Visual->InitializeVisual(Skeletal, Id, Rest, FLinearColor::White, -1, true);
	Visual->SetHighlighted(true);
	TestFalse(TEXT("Old static outline cleared"), Static->bRenderCustomDepth);
	TestTrue(TEXT("Skeletal outline"), Skeletal->bRenderCustomDepth);
	TestEqual(TEXT("Shared stencil"), Skeletal->CustomDepthStencilValue, 2);
	TestTrue(TEXT("Reference transform"), Skeletal->GetRelativeTransform().Equals(Rest));
	Visual->ClearVisual();
	TestFalse(TEXT("Skeletal cleanup"), Skeletal->bRenderCustomDepth);
	TestFalse(TEXT("No idle animation tick"), Visual->IsComponentTickEnabled());
	World->DestroyWorld(false);
	return true;
}
#endif
