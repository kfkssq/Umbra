#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/UmbraInventoryTestTypes.h"
#include "Inventory/UmbraInventoryComponent.h"
#include "Items/UmbraItemDefinition.h"
#include "Player/UmbraPlayerState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraInventoryLiveTest, "Umbra.UI.Inventory.LiveStorage",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraInventoryLiveTest::RunTest(const FString& Parameters)
{
 auto* World = UWorld::CreateWorld(EWorldType::Game, false);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 World->InitializeActorsForPlay(FURL());
 auto* Owner = World->SpawnActor<AActor>();
 auto* Storage = NewObject<UUmbraInventoryComponent>(Owner);
 Storage->InitialCapacity = 2; Storage->RegisterComponent();
 auto* Definition = NewObject<UUmbraItemDefinition>(Owner);
 Definition->DisplayName = FText::FromString(TEXT("Inventory Sword"));
 const auto FirstId = FGuid::NewGuid(), SecondId = FGuid::NewGuid(), ThirdId = FGuid::NewGuid();
 TestEqual(TEXT("Null rejected"), Storage->AddItem(nullptr, FirstId), EUmbraInventoryResult::InvalidItem);
 TestEqual(TEXT("Invalid identity rejected"), Storage->AddItem(Definition, FGuid()), EUmbraInventoryResult::InvalidIdentity);
 TestEqual(TEXT("Add first"), Storage->AddItem(Definition, FirstId), EUmbraInventoryResult::Success);
 TestEqual(TEXT("Duplicate rejected"), Storage->AddItem(Definition, FirstId), EUmbraInventoryResult::DuplicateInstance);
 TestEqual(TEXT("Same definition with another identity allowed"), Storage->AddItem(Definition, SecondId), EUmbraInventoryResult::Success);
 TestEqual(TEXT("Full rejects without replacing"), Storage->AddItem(Definition, ThirdId), EUmbraInventoryResult::Full);
 FUmbraInventoryItem Removed;
 TestEqual(TEXT("Unknown identity rejected"), Storage->RemoveItem(ThirdId, Removed), EUmbraInventoryResult::NotFound);
 TestNull(TEXT("Failed removal returns no item"), Removed.Definition.Get());
 TestEqual(TEXT("Remove existing"), Storage->RemoveItem(FirstId, Removed), EUmbraInventoryResult::Success);
 TestTrue(TEXT("Removed identity preserved"), Removed.InstanceId == FirstId);
 TestEqual(TEXT("Remaining slot stays at one"), Storage->GetSnapshot().Items[0].SlotIndex, 1);
 Storage->AddItem(Definition, ThirdId);
 TestEqual(TEXT("Freed cell reused"), Storage->GetSnapshot().Items[1].SlotIndex, 0);
 Storage->UnregisterComponent();
 TestFalse(TEXT("Unregistered unavailable"), Storage->GetSnapshot().bReady);
 TestEqual(TEXT("Unregistered cannot mutate"), Storage->RemoveItem(SecondId, Removed), EUmbraInventoryResult::NotReady);
 Storage->RegisterComponent();
 TestEqual(TEXT("Reregister retains contents"), Storage->GetSnapshot().Items.Num(), 2);
 auto* NoOwner = NewObject<UUmbraInventoryComponent>();
 TestEqual(TEXT("No authority cannot grant"), NoOwner->AddItem(Definition, FirstId), EUmbraInventoryResult::AuthorityRequired);

 auto* PC = World->SpawnActor<APlayerController>();
 PC->SetPlayer(NewObject<ULocalPlayer>(GEngine));
 auto* Menu = CreateWidget<UUmbraInventoryMenuTestWidget>(PC);
 Menu->WidgetTree = NewObject<UWidgetTree>(Menu);
 auto* Grid = Menu->WidgetTree->ConstructWidget<UUniformGridPanel>();
 Menu->WidgetTree->RootWidget = Grid;
 auto* Text = Menu->WidgetTree->ConstructWidget<UTextBlock>();
 Menu->Configure(Grid, Text, 40, 8); Menu->bBindInventoryData = true;
 Menu->ConstructForTest();
 TestFalse(TEXT("No player state is not an empty bag"), Menu->bInventoryDataReady);
 TestEqual(TEXT("Unavailable capacity"), Text->GetText().ToString(), FString(TEXT("—/—")));
 auto* State = World->SpawnActor<AUmbraPlayerState>(); PC->SetPlayerState(State);
 Menu->NotifyPlayerContextChanged();
 auto* Inventory = State->FindComponentByClass<UUmbraInventoryComponent>();
 TestTrue(TEXT("Ready without requiring GAS initialization"), Menu->bInventoryDataReady);
 TestEqual(TEXT("Component overrides preview capacity"), Menu->GetGeneratedSlotCount(), 96);
 Inventory->AddItem(Definition, FirstId);
 TestEqual(TEXT("Real count"), Text->GetText().ToString(), FString(TEXT("1/96")));
 auto* First = Menu->GetInventorySlot(0);
 TestTrue(TEXT("Event projects identity"), First->GetItemDisplay().InstanceId == FirstId);
 TestEqual(TEXT("Name projected"), First->GetItemDisplay().DisplayName.ToString(), FString(TEXT("Inventory Sword")));
 First->RequestSelection();
 Menu->ConstructForTest();
 TestEqual(TEXT("Repeated construct reuses cells"), Menu->GetInventorySlot(0), First);
 TestTrue(TEXT("Same identity retains selection"), First->IsSelected());
 Inventory->AddItem(Definition, SecondId);
 Inventory->RemoveItem(FirstId, Removed);
 TestFalse(TEXT("Removed cell clears"), First->HasItem());
 TestEqual(TEXT("Removed selection clears"), Menu->GetSelectedSlotIndex(), INDEX_NONE);
 TestTrue(TEXT("Other identity retains cell"), Menu->GetInventorySlot(1)->GetItemDisplay().InstanceId == SecondId);
 Inventory->AddItem(Definition, ThirdId);
 TestTrue(TEXT("Hole refill refreshes view"), First->GetItemDisplay().InstanceId == ThirdId);
 First->RequestSelection();
 Menu->DestructForTest();
 Menu->ConstructForTest();
 TestTrue(TEXT("Reopen preserves selected identity"), Menu->GetInventorySlot(0)->IsSelected());
 auto Snapshot = Inventory->GetSnapshot();
 TestEqual(TEXT("Valid snapshot"), Menu->ValidateSnapshot(Snapshot), EUmbraInventoryViewState::Ready);
 const auto Duplicate = Snapshot.Items[0];
 Snapshot.Items.Add(Duplicate);
 TestEqual(TEXT("Duplicate snapshot rejected"), Menu->ValidateSnapshot(Snapshot), EUmbraInventoryViewState::InvalidSnapshot);
 Snapshot.bReady = false;
 TestEqual(TEXT("Unavailable distinguished"), Menu->ValidateSnapshot(Snapshot), EUmbraInventoryViewState::NotReady);
 Menu->DestructForTest();
 Inventory->RemoveItem(ThirdId, Removed);
 TestFalse(TEXT("Detached not ready"), Menu->bInventoryDataReady);
 Menu->ConstructForTest();
 TestFalse(TEXT("Reopen reads current empty cell"), Menu->GetInventorySlot(0)->HasItem());
 TestTrue(TEXT("Reopen reads remaining item"), Menu->GetInventorySlot(1)->HasItem());
 Inventory->UnregisterComponent();
 TestFalse(TEXT("Shutdown invalidates live view"), Menu->bInventoryDataReady);
 Inventory->RegisterComponent();
 TestTrue(TEXT("Registration reconnects automatically"), Menu->bInventoryDataReady);
 auto* NewState = World->SpawnActor<AUmbraPlayerState>(); PC->SetPlayerState(NewState);
 Menu->NotifyPlayerContextChanged();
 TestEqual(TEXT("New player has empty bag"), Text->GetText().ToString(), FString(TEXT("0/96")));
 Inventory->AddItem(Definition, ThirdId);
 TestFalse(TEXT("Old player no longer updates view"), Menu->GetInventorySlot(0)->HasItem());
 Menu->DestructForTest();
 GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
 return true;
}
#endif
