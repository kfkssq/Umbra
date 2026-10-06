#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/UmbraInventoryTestTypes.h"
#include "Tests/UmbraStatWidgetTestTypes.h"
#include "Stats/UmbraDerivedStatsComponent.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraInventoryInteractionTest, "Umbra.UI.Inventory.EquipInteraction",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraInventoryInteractionTest::RunTest(const FString& Parameters)
{
 auto* World = UWorld::CreateWorld(EWorldType::Game, false);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 World->InitializeActorsForPlay(FURL());
 auto* PC = World->SpawnActor<APlayerController>();
 PC->SetPlayer(NewObject<ULocalPlayer>(GEngine));
 auto* State = World->SpawnActor<AUmbraPlayerState>(); PC->SetPlayerState(State);
 auto* Inventory = State->GetInventoryComponent();
 auto* Equipment = State->GetEquipmentComponent();
 auto* ASC = State->GetUmbraAbilitySystemComponent();
 auto* Menu = CreateWidget<UUmbraInventoryMenuTestWidget>(PC);
 Menu->WidgetTree = NewObject<UWidgetTree>(Menu);
 auto* Grid = Menu->WidgetTree->ConstructWidget<UUniformGridPanel>();
 Menu->WidgetTree->RootWidget = Grid;
 auto* Text = Menu->WidgetTree->ConstructWidget<UTextBlock>();
 Menu->Configure(Grid, Text); Menu->bBindInventoryData = true; Menu->ConstructForTest();
 auto* Sword = NewObject<UUmbraItemDefinition>(State);
 Sword->AllowedSlots = {EUmbraEquipmentSlot::MainHand};
 Sword->Weapon = NewObject<UUmbraWeaponProfile>(Sword);
 Sword->Weapon->Damage.Channels.AddDefaulted_GetRef().BaseDamage = 100.f;
 auto& Magic = Sword->Weapon->Damage.Channels.AddDefaulted_GetRef();
 Magic.Type = EUmbraWeaponDamageType::Fire; Magic.BaseDamage = 30.f;
 Sword->Armor = 20.f; Sword->PrimaryBonuses.Strength = 5.f; Sword->Weight = 7.f;
 auto& Affix = Sword->AttributeBonuses.AddDefaulted_GetRef();
 Affix.Stat = EUmbraEquipmentAffixStat::AttackSpeed; Affix.Magnitude = .2f;
 const auto SwordId = FGuid::NewGuid(); Inventory->AddItem(Sword, SwordId);
 TestEqual(TEXT("ASC not initialized blocks equip, not display"), Menu->RequestEquipItem(SwordId), EUmbraTransferResult::NotReady);
 TestTrue(TEXT("Item visible before ASC ready"), Menu->GetInventorySlot(0)->HasItem());
 if (!ASC->HasBeenInitialized()) ASC->InitializeComponent();
 ASC->InitAbilityActorInfo(State, State);
 State->FindComponentByClass<UUmbraDerivedStatsComponent>()->bUseWeaponDerivedPower = true;
 Equipment->bEnableEquipment = true; State->InitializeAttributes();
 const float OldArmor = ASC->GetNumericAttribute(UUmbraAttributeSet::GetArmorAttribute());
 const float OldSpeed = ASC->GetNumericAttribute(UUmbraAttributeSet::GetAttackSpeedAttribute());
 TSet<FKey> Buttons; Buttons.Add(EKeys::LeftMouseButton);
 const FPointerEvent Click(0, FVector2D::ZeroVector, FVector2D::ZeroVector, Buttons, EKeys::LeftMouseButton, 0.f, FModifierKeysState());
 auto* Cell = CastChecked<UUmbraInventorySlotTestWidget>(Menu->GetInventorySlot(0));
 TestTrue(TEXT("Single click handled"), Cell->MouseDownForTest(Click).IsEventHandled());
 TestTrue(TEXT("Single click selects without transfer"), Cell->IsSelected() && Inventory->GetSnapshot().Items.Num() == 1);
 TestTrue(TEXT("Double click handled"), Cell->DoubleClickForTest(Click).IsEventHandled());
 TestEqual(TEXT("Double click transfers"), Menu->LastEquipResult, EUmbraTransferResult::Success);
 TestFalse(TEXT("Successful equip clears cell and selection"), Cell->HasItem() || Cell->IsSelected());
 TestEqual(TEXT("Capacity updated by event"), Text->GetText().ToString(), FString(TEXT("0/96")));
 TestEqual(TEXT("Original identity equipped"), Equipment->GetSnapshot().Items[0].InstanceId, SwordId);
 TestEqual(TEXT("Weapon AD"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetAttackPowerAttribute()), 100.f);
 TestEqual(TEXT("Weapon AP"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetAbilityPowerAttribute()), 30.f);
 TestEqual(TEXT("Armor uses GAS"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetArmorAttribute()), OldArmor + 20.f);
 TestEqual(TEXT("Fixed affix uses GAS"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetAttackSpeedAttribute()), OldSpeed + .2f);
 TestEqual(TEXT("Equipment weight"), Equipment->GetSnapshot().EquipLoad, 7.f);
 Cell->DoubleClickForTest(Click);
 TestEqual(TEXT("Empty double click cannot duplicate"), Equipment->GetSnapshot().Items.Num(), 1);
 TestEqual(TEXT("Repeated GUID cannot equip"), Menu->RequestEquipItem(SwordId), EUmbraTransferResult::NotFound);
 TestEqual(TEXT("Invalid GUID rejected"), Menu->RequestEquipItem(FGuid()), EUmbraTransferResult::InvalidIdentity);
 const auto SecondId = FGuid::NewGuid(); Inventory->AddItem(Sword, SecondId);
 TestEqual(TEXT("Occupied does not replace"), Menu->RequestEquipItem(SecondId), EUmbraTransferResult::SlotOccupied);
 TestEqual(TEXT("Wrong explicit slot"), Menu->RequestEquipInSlot(SecondId, EUmbraEquipmentSlot::Head), EUmbraTransferResult::InvalidSlot);
 auto* Ring = NewObject<UUmbraItemDefinition>(State);
 Ring->AllowedSlots = {EUmbraEquipmentSlot::Ring1, EUmbraEquipmentSlot::Ring2};
 const auto Ring1 = FGuid::NewGuid(), Ring2 = FGuid::NewGuid(), Ring3 = FGuid::NewGuid();
 Inventory->AddItem(Ring, Ring1); Inventory->AddItem(Ring, Ring2);
 TestEqual(TEXT("Ambiguous empty slots need choice"), Menu->RequestEquipItem(Ring1), EUmbraTransferResult::SlotSelectionRequired);
 TestTrue(TEXT("Localized feedback available"), !Menu->LastEquipMessage.IsEmpty());
 TestEqual(TEXT("Explicit choice"), Menu->RequestEquipInSlot(Ring1, EUmbraEquipmentSlot::Ring2), EUmbraTransferResult::Success);
 TestEqual(TEXT("Unique remaining empty slot"), Menu->RequestEquipItem(Ring2), EUmbraTransferResult::Success);
 Inventory->AddItem(Ring, Ring3);
 TestEqual(TEXT("All legal slots occupied"), Menu->RequestEquipItem(Ring3), EUmbraTransferResult::SlotOccupied);
 auto* HighLevel = NewObject<UUmbraItemDefinition>(State);
 HighLevel->AllowedSlots = {EUmbraEquipmentSlot::Head}; HighLevel->RequiredLevel = 99;
 const auto HighId = FGuid::NewGuid(); Inventory->AddItem(HighLevel, HighId);
 TestEqual(TEXT("Level failure preserves item"), Menu->RequestEquipItem(HighId), EUmbraTransferResult::LevelTooLow);
 TestTrue(TEXT("Failed item still owned"), Inventory->GetSnapshot().Items.ContainsByPredicate([HighId](const auto& I) { return I.InstanceId == HighId; }));
 auto* EquipmentMenu = CreateWidget<UUmbraEquipmentMenuTestWidget>(PC);
 EquipmentMenu->WidgetTree = NewObject<UWidgetTree>(EquipmentMenu);
 auto* Root = EquipmentMenu->WidgetTree->ConstructWidget<UVerticalBox>();
 EquipmentMenu->WidgetTree->RootWidget = Root;
 for (int32 Index = 0; Index < 10; ++Index)
 {
  auto* EquipmentCell = EquipmentMenu->WidgetTree->ConstructWidget<UUmbraEquipmentSlotTestWidget>();
  EquipmentCell->SetSlotType(EUmbraEquipmentSlot(Index)); Root->AddChild(EquipmentCell);
 }
 EquipmentMenu->ConstructForTest(); EquipmentMenu->SetPageActive(true);
 auto* MainHand = CastChecked<UUmbraEquipmentSlotTestWidget>(EquipmentMenu->GetEquipmentSlot(EUmbraEquipmentSlot::MainHand));
 TestEqual(TEXT("Equipment UI shows equipped identity"), MainHand->GetItemDisplay().InstanceId, SwordId);
 while (Inventory->GetSnapshot().Items.Num() < Inventory->GetSnapshot().Capacity) Inventory->AddItem(Ring, FGuid::NewGuid());
 TestEqual(TEXT("Full inventory rejects UI return"), EquipmentMenu->RequestUnequipSlot(MainHand), EUmbraUnequipResult::Failed);
 TestEqual(TEXT("Specific return failure exposed"), EquipmentMenu->LastUnequipTransferResult, EUmbraTransferResult::InventoryFull);
 TestEqual(TEXT("Rejected return retains displayed GUID"), MainHand->GetItemDisplay().InstanceId, SwordId);
 TestEqual(TEXT("Rejected return keeps effects"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetArmorAttribute()), OldArmor + 20.f);
 FUmbraInventoryItem Removed;
 Inventory->RemoveItem(HighId, Removed);
 Buttons.Reset(); Buttons.Add(EKeys::RightMouseButton);
 const FPointerEvent RightClick(0, FVector2D::ZeroVector, FVector2D::ZeroVector, Buttons, EKeys::RightMouseButton, 0.f, FModifierKeysState());
 TestTrue(TEXT("Right click handled"), MainHand->MouseDownForTest(RightClick).IsEventHandled());
 TestEqual(TEXT("Right click transfers back"), EquipmentMenu->LastUnequipTransferResult, EUmbraTransferResult::Success);
 TestFalse(TEXT("Snapshot clears equipment"), MainHand->HasItem());
 TestEqual(TEXT("Return uses original GUID in inventory UI"), Menu->GetInventorySlot(Removed.SlotIndex)->GetItemDisplay().InstanceId, SwordId);
 TestEqual(TEXT("Return restores armor"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetArmorAttribute()), OldArmor);
 TestEqual(TEXT("Return restores affix"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetAttackSpeedAttribute()), OldSpeed);
 TestEqual(TEXT("Return restores load"), Equipment->GetSnapshot().EquipLoad, 0.f);
 TestEqual(TEXT("Full count after return"), Text->GetText().ToString(), FString(TEXT("96/96")));
 MainHand->MouseDownForTest(RightClick);
 TestEqual(TEXT("Repeated right click creates nothing"), Inventory->GetSnapshot().Items.Num(), 96);
 TestEqual(TEXT("Same definition second GUID preserved"), Menu->GetInventorySlot(0)->GetItemDisplay().InstanceId, SecondId);
 EquipmentMenu->DestructForTest();
 auto* NewState = World->SpawnActor<AUmbraPlayerState>(); PC->SetPlayerState(NewState);
 TestEqual(TEXT("Stale player context cannot mutate"), Menu->RequestEquipItem(SecondId), EUmbraTransferResult::NotReady);
 Menu->DestructForTest();
 GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraInitialInventoryTest, "Umbra.Inventory.InitialTestItems",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraInitialInventoryTest::RunTest(const FString& Parameters)
{
 auto* World = UWorld::CreateWorld(EWorldType::Game, false);
 auto* StateClass = LoadClass<AUmbraPlayerState>(nullptr, TEXT("/Game/Blueprints/Player/BP_UmbraPlayerState.BP_UmbraPlayerState_C"));
 if (!TestNotNull(TEXT("Existing PlayerState Blueprint loads"), StateClass)) { World->DestroyWorld(false); return false; }
 auto* State = World->SpawnActor<AUmbraPlayerState>(StateClass);
 TestEqual(TEXT("Two configured starting items on real BP"), State->InitialInventoryItems.Num(), 2);
 State->InitializeInventory();
 const auto First = State->GetInventoryComponent()->GetSnapshot();
 TestEqual(TEXT("Both real definitions granted"), First.Items.Num(), 2);
 if (First.Items.Num() == 2)
 {
  TestEqual(TEXT("Sword item asset loaded"), First.Items[0].Definition->GetPathName(), FString(TEXT("/Game/Items/Weapons/DA_Item_TestSword.DA_Item_TestSword")));
  TestEqual(TEXT("Mace item asset loaded"), First.Items[1].Definition->GetPathName(), FString(TEXT("/Game/Items/Weapons/DA_Item_TestMace.DA_Item_TestMace")));
  TestTrue(TEXT("Initial identities distinct and valid"), First.Items[0].InstanceId.IsValid() && First.Items[1].InstanceId.IsValid() && First.Items[0].InstanceId != First.Items[1].InstanceId);
  State->InitializeInventory(); State->InitializeAttributes(); State->InitializeInventory();
  auto* Inventory = State->GetInventoryComponent();
  Inventory->UnregisterComponent(); Inventory->RegisterComponent(); State->InitializeInventory();
  TestEqual(TEXT("Repeated initialization does not duplicate"), Inventory->GetSnapshot().Items.Num(), 2);
  TestEqual(TEXT("Repeated initialization preserves identity"), Inventory->GetSnapshot().Items[0].InstanceId, First.Items[0].InstanceId);
  FUmbraInventoryItem Removed; Inventory->RemoveItem(First.Items[0].InstanceId, Removed); State->InitializeInventory();
  TestEqual(TEXT("Consumed initial item is never respawned"), Inventory->GetSnapshot().Items.Num(), 1);
 }
 TestTrue(TEXT("Starts in bag, not equipped"), State->GetEquipmentComponent()->GetSnapshot().Items.IsEmpty());
 World->DestroyWorld(false);
 return true;
}
#endif
