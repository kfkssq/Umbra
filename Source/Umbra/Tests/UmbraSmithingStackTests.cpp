#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Inventory/UmbraInventoryComponent.h"
#include "Items/UmbraItemGeneration.h"
#include "UI/Inventory/UmbraInventorySlot.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraSmithingStackTest, "Umbra.Inventory.SmithingStacks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraSmithingStackTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    auto* Owner=World->SpawnActor<AActor>();
    auto* Bag=NewObject<UUmbraInventoryComponent>(Owner); Bag->InitialCapacity=3; Bag->RegisterComponent();
    auto* Stone=LoadObject<UUmbraItemDefinition>(nullptr,TEXT("/Game/Items/NativeAccessoriesV1/Items/DA_Item_EquipParamGoods_10100.DA_Item_EquipParamGoods_10100"));
    auto* OtherStone=LoadObject<UUmbraItemDefinition>(nullptr,TEXT("/Game/Items/NativeAccessoriesV1/Items/DA_Item_EquipParamGoods_10101.DA_Item_EquipParamGoods_10101"));
    if (!Stone || !OtherStone) { AddError(TEXT("Missing imported stones")); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return false; }
    TestTrue(TEXT("Stone stack opt-in saved"),Stone->bStackableMaterial && OtherStone->bStackableMaterial);
    auto Make=[&](UUmbraItemDefinition* Definition, EUmbraItemRarity Rarity, int32 Level=1)
    {
        FUmbraItemInstance Item; FUmbraGenerationContext Context; Context.SourceLevel=Level; FString Error;
        TestTrue(TEXT("Real material generation"),UmbraItemGeneration::Generate(Owner,Definition,Context,Level,FGuid::NewGuid(),Item,Error));
        Item.Rarity=Rarity; return Item;
    };
    auto First=Make(Stone,EUmbraItemRarity::Common);
    auto Second=Make(Stone,EUmbraItemRarity::Common,20);
    auto Magic=Make(Stone,EUmbraItemRarity::Magic);
    auto Other=Make(OtherStone,EUmbraItemRarity::Common);
    TestEqual(TEXT("First stone"),Bag->AddInstance(First),EUmbraInventoryResult::Success);
    TestEqual(TEXT("Same kind/quality merges despite different level/seed"),Bag->AddInstance(Second),EUmbraInventoryResult::Success);
    TestEqual(TEXT("One occupied cell"),Bag->GetSnapshot().Items.Num(),1);
    TestEqual(TEXT("Two stones"),Bag->GetSnapshot().Items[0].Instance.Quantity,2);
    TestEqual(TEXT("Receiving identity preserved"),Bag->GetSnapshot().Items[0].InstanceId,First.InstanceId);
    TestEqual(TEXT("Absorbed identity cannot duplicate"),Bag->AddInstance(Second),EUmbraInventoryResult::DuplicateInstance);
    TestEqual(TEXT("Different quality separate"),Bag->AddInstance(Magic),EUmbraInventoryResult::Success);
    TestEqual(TEXT("Different stone separate"),Bag->AddInstance(Other),EUmbraInventoryResult::Success);
    auto Third=Make(Stone,EUmbraItemRarity::Common);
    TestEqual(TEXT("Full bag still merges"),Bag->AddInstance(Third),EUmbraInventoryResult::Success);
    TestEqual(TEXT("Full bag rejects unmatched quality"),Bag->AddInstance(Make(Stone,EUmbraItemRarity::Rare)),EUmbraInventoryResult::Full);
    TestEqual(TEXT("Three stones after merge"),Bag->GetSnapshot().Items[0].Instance.Quantity,3);
    FUmbraInventoryItem Removed;
    TestEqual(TEXT("Remove entire stack"),Bag->RemoveItem(First.InstanceId,Removed),EUmbraInventoryResult::Success);
    TestEqual(TEXT("Removed quantity retained"),Removed.GetInstance().Quantity,3);
    TestEqual(TEXT("Absorbed identity remains retired after removal"),Bag->AddInstance(Second),EUmbraInventoryResult::DuplicateInstance);
    TestEqual(TEXT("Return complete stack"),Bag->AddInstance(Removed.GetInstance()),EUmbraInventoryResult::Success);
    FUmbraItemInstanceRecord Record; FUmbraItemInstance Restored;
    TestTrue(TEXT("Record quantity capture"),FUmbraItemInstanceRecord::Capture(Removed.GetInstance(),Record));
    TestTrue(TEXT("Record quantity restore"),Record.Restore(Stone,Restored));
    TestEqual(TEXT("Record retains quantity"),Restored.Quantity,3);
    // Explicit empty target slots support transfers; dragging like stacks together merges them.
    Bag->RemoveItem(Magic.InstanceId,Removed); Bag->RemoveItem(Other.InstanceId,Removed);
    auto Split=Make(Stone,EUmbraItemRarity::Common); Split.Quantity=4;
    TestEqual(TEXT("Explicit free target"),Bag->AddInstance(Split,2),EUmbraInventoryResult::Success);
    TestEqual(TEXT("Move merges compatible stacks"),Bag->MoveItem(Split.InstanceId,0),EUmbraInventoryResult::Success);
    TestEqual(TEXT("Move quantity conserved"),Bag->GetSnapshot().Items[0].Instance.Quantity,7);
    TestEqual(TEXT("Move frees source cell"),Bag->GetSnapshot().Items.Num(),1);
    TestEqual(TEXT("Moved source cannot replay"),Bag->AddInstance(Split),EUmbraInventoryResult::DuplicateInstance);
    auto Excess=Make(Stone,EUmbraItemRarity::Common); Excess.Quantity=MAX_int32;
    const auto Revision=Bag->GetSnapshot().Revision;
    TestEqual(TEXT("Overflow rejects atomically"),Bag->AddInstance(Excess),EUmbraInventoryResult::Full);
    TestEqual(TEXT("Overflow does not publish"),Bag->GetSnapshot().Revision,Revision);
    Excess.Quantity=0; TestFalse(TEXT("Zero quantity invalid"),Excess.IsValid());
    auto* Plain=NewObject<UUmbraItemDefinition>(Owner);
    FUmbraItemInstance PlainItem; FUmbraItemInstance::Create(Owner,Plain,FGuid::NewGuid(),PlainItem);
    PlainItem.Quantity=2; TestFalse(TEXT("Non-stackable quantity rejected"),PlainItem.IsValid()); PlainItem.Quantity=1;
    TestEqual(TEXT("Ordinary item added"),Bag->AddInstance(PlainItem),EUmbraInventoryResult::Success);
    auto PlainCopy=PlainItem; PlainCopy.InstanceId=FGuid::NewGuid();
    TestEqual(TEXT("Ordinary copies do not stack"),Bag->AddInstance(PlainCopy),EUmbraInventoryResult::Success);
    TestEqual(TEXT("Ordinary copies occupy distinct cells"),Bag->GetSnapshot().Items.Num(),3);
    TestEqual(TEXT("Unlike items still swap"),Bag->MoveItem(PlainItem.InstanceId,0),EUmbraInventoryResult::Success);
    Bag->UnregisterComponent(); Bag->RegisterComponent();
    TestEqual(TEXT("Reregister retains stack"),Bag->GetSnapshot().Items[0].Instance.Quantity,7);
    auto* PC=World->SpawnActor<APlayerController>(); PC->SetPlayer(NewObject<ULocalPlayer>(GEngine));
    auto* Class=LoadClass<UUmbraInventorySlot>(nullptr,TEXT("/Game/UI/CharacterMenu/InventoryMenu/WBP_InventorySlot.WBP_InventorySlot_C"));
    auto* Cell=Class ? CreateWidget<UUmbraInventorySlot>(PC,Class) : nullptr;
    if (TestNotNull(TEXT("Real inventory WBP"),Cell))
    {
        Cell->TakeWidget();
        auto* Count=Cast<UTextBlock>(Cell->GetWidgetFromName(TEXT("StackCountText")));
        if (TestNotNull(TEXT("Quantity text binding"),Count))
        {
            Cell->SetItemInstance(Bag->GetSnapshot().Items[0].Instance);
            TestEqual(TEXT("Real widget quantity"),Count->GetText().ToString(),FString(TEXT("7")));
            TestEqual(TEXT("Quantity visible"),Count->GetVisibility(),ESlateVisibility::HitTestInvisible);
            Cell->SetItemInstance(PlainItem);
            TestEqual(TEXT("Non-material clears count"),Count->GetVisibility(),ESlateVisibility::Collapsed);
            Cell->SetItemInstance(First);
            TestEqual(TEXT("Single stone shows one"),Count->GetText().ToString(),FString(TEXT("1")));
            Cell->ClearItem();
            TestTrue(TEXT("Empty cell clears count"),Count->GetText().IsEmpty());
        }
    }
    World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
    return true;
}
#endif
