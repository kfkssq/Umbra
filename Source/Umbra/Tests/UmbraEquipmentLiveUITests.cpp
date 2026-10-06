#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/UmbraStatWidgetTestTypes.h"
#include "UI/Equipment/UmbraEquipmentSlotWidget.h"
#include "Equipment/UmbraEquipmentComponent.h"
#include "Stats/UmbraDerivedStatsComponent.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Player/UmbraPlayerState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraEquipmentLiveUITest, "Umbra.UI.Equipment.LiveSnapshot",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraEquipmentLiveUITest::RunTest(const FString& Parameters)
{
 UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 World->InitializeActorsForPlay(FURL());
 auto* PC = World->SpawnActor<APlayerController>();
 PC->SetPlayer(NewObject<ULocalPlayer>(GEngine));
 auto MakeState = [&]()
 {
  auto* State = World->SpawnActor<AUmbraPlayerState>();
  auto* ASC = State->GetUmbraAbilitySystemComponent();
  if (!ASC->HasBeenInitialized()) ASC->InitializeComponent();
  ASC->InitAbilityActorInfo(State, State);
  State->FindComponentByClass<UUmbraDerivedStatsComponent>()->bUseWeaponDerivedPower = true;
  State->FindComponentByClass<UUmbraEquipmentComponent>()->bEnableEquipment = true;
  State->InitializeAttributes();
  return State;
 };
 auto* State = MakeState(); PC->SetPlayerState(State);
 auto* ASC = State->GetUmbraAbilitySystemComponent();
 auto* Equipment = State->FindComponentByClass<UUmbraEquipmentComponent>();
 auto* Menu = NewObject<UUmbraEquipmentMenuTestWidget>(PC);
 Menu->SetOwningPlayer(PC);
 Menu->WidgetTree = NewObject<UWidgetTree>(Menu);
 auto* Root = Menu->WidgetTree->ConstructWidget<UVerticalBox>();
 Menu->WidgetTree->RootWidget = Root;
 for (int32 Index = 0; Index < 10; ++Index)
 {
  auto* Slot = Menu->WidgetTree->ConstructWidget<UUmbraEquipmentSlotTestWidget>();
  Slot->SetSlotType(EUmbraEquipmentSlot(Index)); Root->AddChild(Slot);
 }
 Menu->ConstructForTest(); Menu->ConstructForTest();
 TestTrue(TEXT("Ready empty equipment distinct from unavailable"), Menu->bEquipmentDataReady);
 auto* Ring = NewObject<UUmbraItemDefinition>(State);
 Ring->AllowedSlots = {EUmbraEquipmentSlot::Ring1, EUmbraEquipmentSlot::Ring2};
 Ring->DisplayName = FText::FromString(TEXT("Test Ring")); Ring->Icon = NewObject<UTexture2D>(Ring);
 Ring->RarityColor = FLinearColor::Green; Ring->Requirements.Strength = 5.f;
 const auto FirstId = FGuid::NewGuid(), SecondId = FGuid::NewGuid();
 TestEqual(TEXT("Equip Ring1"), Equipment->Equip(EUmbraEquipmentSlot::Ring1, Ring, FirstId), EUmbraEquipResult::Success);
 auto* First = Menu->GetEquipmentSlot(EUmbraEquipmentSlot::Ring1);
 auto* Second = Menu->GetEquipmentSlot(EUmbraEquipmentSlot::Ring2);
 TestTrue(TEXT("Component event fills existing slot"), First->HasItem());
 auto Display = First->GetItemDisplay();
 TestTrue(TEXT("GUID projected"), Display.InstanceId == FirstId);
 TestTrue(TEXT("Snapshot source marked"), Display.bFromEquipmentSnapshot);
 TestFalse(TEXT("Requirements come from component"), Display.bRequirementsMet);
 TestEqual(TEXT("Name projected"), Display.DisplayName.ToString(), FString(TEXT("Test Ring")));
 TestEqual(TEXT("Icon projected"), Display.Icon.GetResourceObject(), static_cast<UObject*>(Ring->Icon.Get()));
 TestTrue(TEXT("Color projected"), Display.RarityColor == FLinearColor::Green);
 TestFalse(TEXT("Manual view data blocked while bound"), Menu->SetSlotItem(EUmbraEquipmentSlot::Ring2, Display));
 First->RequestSelection();
 Equipment->Refresh();
 TestTrue(TEXT("Same instance preserves selection"), First->GetVisualState() == EUmbraEquipmentSlotState::Selected);
 ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetStrengthAttribute(), 10.f);
 TestTrue(TEXT("Requirement changes refresh view"), First->GetItemDisplay().bRequirementsMet);
 TestEqual(TEXT("Same definition in second slot"), Equipment->Equip(EUmbraEquipmentSlot::Ring2, Ring, SecondId), EUmbraEquipResult::Success);
 TestTrue(TEXT("Second instance identity independent"), Second->GetItemDisplay().InstanceId == SecondId);
 const auto ReplacementId = FGuid::NewGuid();
 Equipment->Equip(EUmbraEquipmentSlot::Ring1, Ring, ReplacementId);
 TestTrue(TEXT("Replacing same definition updates GUID"), First->GetItemDisplay().InstanceId == ReplacementId);
 TestTrue(TEXT("Replacement clears stale selection"), First->GetVisualState() == EUmbraEquipmentSlotState::Equipped);
 Equipment->Unequip(EUmbraEquipmentSlot::Ring1);
 TestFalse(TEXT("Unequip clears only first slot"), First->HasItem());
 TestTrue(TEXT("Other ring retained"), Second->HasItem());
 // Exercise the real slot -> menu -> component path, including stale UI and closed-page guards.
 TestEqual(TEXT("Inactive page cannot mutate"), Menu->RequestUnequipSlot(Second), EUmbraUnequipResult::NotReady);
 Menu->SetPageActive(true);
 Second->SetLocked(true);
 TestEqual(TEXT("Locked slot cannot mutate"), Menu->RequestUnequipSlot(Second), EUmbraUnequipResult::Locked);
 Second->SetLocked(false);
 auto* ForeignSlot = NewObject<UUmbraEquipmentSlotWidget>(Menu);
 TestEqual(TEXT("Foreign widget rejected"), Menu->RequestUnequipSlot(ForeignSlot), EUmbraUnequipResult::InvalidSlot);
 const auto OldDisplay = Second->GetItemDisplay();
 const auto NewId = FGuid::NewGuid();
 Equipment->Equip(EUmbraEquipmentSlot::Ring2, Ring, NewId);
 Second->SetItem(OldDisplay);
 TestEqual(TEXT("Old display cannot remove replacement"), Menu->RequestUnequipSlot(Second), EUmbraUnequipResult::StaleInstance);
 TestTrue(TEXT("Rejected removal retains current instance"), Equipment->GetSnapshot().Items[0].InstanceId == NewId);
 Equipment->Refresh();
 auto ManualDisplay = Second->GetItemDisplay(); ManualDisplay.bFromEquipmentSnapshot = false;
 Second->SetItem(ManualDisplay);
 TestEqual(TEXT("Manual preview cannot mutate"), Menu->RequestUnequipSlot(Second), EUmbraUnequipResult::StaleInstance);
 Equipment->Refresh();
 TSet<FKey> PressedButtons; PressedButtons.Add(EKeys::RightMouseButton);
 const FPointerEvent RightClick(0, FVector2D::ZeroVector, FVector2D::ZeroVector, PressedButtons, EKeys::RightMouseButton, 0.f, FModifierKeysState());
 TestTrue(TEXT("Right click handled"), CastChecked<UUmbraEquipmentSlotTestWidget>(Second)->MouseDownForTest(RightClick).IsEventHandled());
 TestFalse(TEXT("Right click removes via snapshot notification"), Second->HasItem());
 TestTrue(TEXT("Right click returns same identity to inventory"), State->GetInventoryComponent()->GetSnapshot().Items.ContainsByPredicate([NewId](const auto& Item) { return Item.InstanceId == NewId; }));
 TestTrue(TEXT("Result feedback available"), !Menu->LastUnequipMessage.IsEmpty());
 TestEqual(TEXT("Repeated empty action is safe"), Menu->RequestUnequipSlot(Second), EUmbraUnequipResult::EmptySlot);
 Equipment->Equip(EUmbraEquipmentSlot::Ring2, Ring, SecondId);
 FUmbraEquippedItem RemovedItem;
 TestEqual(TEXT("Invalid GUID rejected"), Equipment->TryUnequipInstance(EUmbraEquipmentSlot::Ring2, FGuid(), RemovedItem), EUmbraUnequipResult::StaleInstance);
 TestNull(TEXT("Rejected removal returns no item"), RemovedItem.Definition.Get());
 TestEqual(TEXT("Matching instance removed"), Equipment->TryUnequipInstance(EUmbraEquipmentSlot::Ring2, SecondId, RemovedItem), EUmbraUnequipResult::Success);
 TestEqual(TEXT("Removed definition returned"), RemovedItem.Definition.Get(), Ring);
 TestTrue(TEXT("Removed identity returned unchanged"), RemovedItem.InstanceId == SecondId);
 Equipment->Equip(EUmbraEquipmentSlot::Ring2, Ring, SecondId);
 Second->RequestSelection();
 Menu->DestructForTest();
 TestFalse(TEXT("Destruct clears ready state"), Menu->bEquipmentDataReady);
 TestFalse(TEXT("Destruct clears data"), Second->HasItem());
 Equipment->Refresh();
 TestFalse(TEXT("Closed view detached"), Second->HasItem());
 Menu->ConstructForTest();
 TestTrue(TEXT("Reopen restores current snapshot"), Second->HasItem());
 TestEqual(TEXT("Reopen restores same identity selection"), Second->GetVisualState(), EUmbraEquipmentSlotState::Selected);
 ASC->ClearActorInfo();
 TestFalse(TEXT("ASC loss marks unavailable"), Menu->bEquipmentDataReady);
 TestFalse(TEXT("ASC loss clears stale item"), Second->HasItem());
 ASC->InitAbilityActorInfo(State, State);
 TestTrue(TEXT("ASC recovery reconnects"), Second->HasItem());
 auto* Replacement = MakeState(); PC->SetPlayerState(Replacement);
 Menu->SetPageActive(false);
 // Re-open before switching contexts, then switch without the usual notification.
 PC->SetPlayerState(State); Menu->SetPageActive(true); PC->SetPlayerState(Replacement);
 TestEqual(TEXT("Unnotified owner change cannot mutate old player"), Menu->RequestUnequipSlot(Second), EUmbraUnequipResult::NotReady);
 TestTrue(TEXT("Old player's equipment preserved"), !Equipment->GetSnapshot().Items.IsEmpty());
 Menu->NotifyPlayerContextChanged();
 TestFalse(TEXT("PS change clears old equipment"), Second->HasItem());
 Equipment->Refresh();
 TestFalse(TEXT("Old component detached"), Second->HasItem());
 auto* NewEquipment = Replacement->FindComponentByClass<UUmbraEquipmentComponent>();
 NewEquipment->Equip(EUmbraEquipmentSlot::Ring1, Ring, FGuid::NewGuid());
 TestTrue(TEXT("Replacement component observed"), First->HasItem());
 NewEquipment->UnregisterComponent();
 TestFalse(TEXT("Component shutdown invalidates view"), Menu->bEquipmentDataReady);
 TestFalse(TEXT("Component shutdown clears slot"), First->HasItem());
 Menu->DestructForTest();
 GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
 return true;
}
#endif

