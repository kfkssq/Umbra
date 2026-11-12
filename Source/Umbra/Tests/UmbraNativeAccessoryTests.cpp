#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Items/UmbraItemGeneration.h"
#include "Items/UmbraItemTooltipData.h"
#include "Loot/UmbraLootTable.h"
#include "Player/UmbraPlayerState.h"
#include "Equipment/UmbraEquipmentComponent.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "Stats/UmbraDerivedStatsComponent.h"
#include "Inventory/UmbraInventoryComponent.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraMaterialRarityTest, "Umbra.Items.Generation.MaterialRarity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraMaterialRarityTest::RunTest(const FString& Parameters)
{
    auto* World = UWorld::CreateWorld(EWorldType::Game, false);
    auto* Owner = World->SpawnActor<AUmbraPlayerState>();
    auto* Item = NewObject<UUmbraItemDefinition>(Owner);
    auto* Profile = NewObject<UUmbraItemGenerationProfile>(Owner);
    Item->DefinitionId = TEXT("Test.Stone"); Item->ItemCategory = EUmbraItemCategory::Material;
    Item->bEquippingEnabled = false; Item->GenerationProfile = Profile;
    Profile->bMaterialRarityOnly = true;
    for (int32 I = 0; I < 6; ++I) { FUmbraGenerationRarity Row; Row.Rarity = EUmbraItemRarity(I); Row.AffixCount = 4; Profile->Rarities.Add(Row); }
    FUmbraGenerationContext Context; Context.SourceLevel = 1;
    FString Error;
    for (int32 Source = 0; Source <= 3; ++Source)
    for (int32 GradeRarity = 0; GradeRarity < 6; ++GradeRarity)
    {
        Item->SourceRarity = Source;
        Item->Rarity = EUmbraItemRarity(GradeRarity);
        TSet<int32> Seen;
        for (int32 Seed = 0; Seed < 32; ++Seed)
        {
            FUmbraItemInstance Instance, Again;
            const FGuid Id(1,2,3,4);
            if (!TestTrue(TEXT("Material generates without an affix pool or slots"), UmbraItemGeneration::Generate(Owner, Item, Context, Seed, Id, Instance, Error))) break;
            UmbraItemGeneration::Generate(Owner, Item, Context, Seed, Id, Again, Error);
            TestEqual(TEXT("Deterministic rarity"), Instance.Rarity, Again.Rarity);
            TestTrue(TEXT("Material never generates combat affixes"), Instance.RolledAffixes.IsEmpty());
            TestFalse(TEXT("No pending legendary combat power on materials"), Instance.HasSpecialEffect());
            const int32 Rank = int32(Instance.Rarity);
            TestEqual(TEXT("Material grade overrides source rarity and seed"), Rank, GradeRarity);
            Seen.Add(Rank);
            FUmbraItemInstanceRecord Record; FUmbraItemInstance Restored;
            TestTrue(TEXT("Capture"), FUmbraItemInstanceRecord::Capture(Instance, Record));
            TestTrue(TEXT("Restore"), Record.Restore(Item, Restored));
            TestEqual(TEXT("Restored rarity preserved"), Restored.Rarity, Instance.Rarity);
        }
        TestEqual(TEXT("Only the authored material rarity reached"), Seen.Num(), 1);
    }
    Item->AllowedSlots = {EUmbraEquipmentSlot::Amulet};
    TestFalse(TEXT("Material cannot gain equip slots"), UmbraItemGeneration::Validate(Item, Error));
    Item->AllowedSlots.Reset(); Item->SourceRarity = 4;
    TestFalse(TEXT("Invalid source rarity rejected"), UmbraItemGeneration::Validate(Item, Error));
    Item->SourceRarity = 1;
    Item->Rarity = EUmbraItemRarity::Unique;
    Profile->Rarities.RemoveAll([](const auto& Row) { return Row.Rarity == EUmbraItemRarity::Unique; });
    TestFalse(TEXT("Missing fixed-rarity style rejected"), UmbraItemGeneration::Validate(Item, Error));
    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraNativeAccessoryAssetsTest, "Umbra.Items.Generation.NativeAccessoryAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraNativeAccessoryAssetsTest::RunTest(const FString& Parameters)
{
    auto* Table = LoadObject<UUmbraLootTable>(nullptr, TEXT("/Game/Items/NativeAccessoriesV1/DA_Loot_Prototype.DA_Loot_Prototype"));
    if (!TestNotNull(TEXT("Imported production table"), Table)) return false;
    FString Error;
    if (!TestTrue(TEXT("Mixed table validates"), Table->ValidateTypeGroups(Error))) { AddError(Error); return false; }
    auto* World = UWorld::CreateWorld(EWorldType::Game, false);
    auto* Owner = World->SpawnActor<AUmbraPlayerState>();
    auto* ASC = Owner->GetUmbraAbilitySystemComponent(); ASC->InitializeComponent(); ASC->InitAbilityActorInfo(Owner, Owner);
    auto* Equipment = Owner->FindComponentByClass<UUmbraEquipmentComponent>();
    Owner->FindComponentByClass<UUmbraDerivedStatsComponent>()->bUseWeaponDerivedPower = true;
    Equipment->bEnableEquipment = true; Owner->InitializeAttributes();
    FUmbraGenerationContext Context; Context.SourceLevel = 1;
    int32 Accessories = 0, Materials = 0;
    for (const auto& Category : Table->Categories)
    {
        if (Category.Category != EUmbraItemCategory::Accessory && Category.Category != EUmbraItemCategory::Material) continue;
        for (const auto& Type : Category.Types) for (const auto& Template : Type.Templates)
        {
            auto* Item = Template.ItemDefinition.Get();
            const bool bMaterial = Category.Category == EUmbraItemCategory::Material;
            bMaterial ? ++Materials : ++Accessories;
            TestNotNull(TEXT("Icon"), Item->Icon.Get());
            TestFalse(TEXT("Shared world model"), Item->WorldDropMesh.IsNull());
            for (int32 Seed = 0; Seed < 24; ++Seed)
            {
                FUmbraItemInstance Instance;
                if (!TestTrue(TEXT("Imported source generates"), UmbraItemGeneration::Generate(Owner, Item, Context, Seed, FGuid::NewGuid(), Instance, Error))) { AddError(Error); break; }
                if (Seed == 0)
                {
                    auto* Inventory = Owner->GetInventoryComponent();
                    TestEqual(TEXT("Generated item stored"), Inventory->AddInstance(Instance, 0), EUmbraInventoryResult::Success);
                    const auto Tooltip = UUmbraItemTooltipDataBuilder::FromInventory(Inventory->GetSnapshot(), 0, Instance.InstanceId, Item, Equipment, false, EUmbraEquipmentSlot::Head);
                    TestTrue(TEXT("Live inventory tooltip works for material and amulet"), Tooltip.bValid);
                    TestEqual(TEXT("Tooltip retains instance rarity"), Tooltip.Rarity, Instance.Rarity);
                    FUmbraInventoryItem Removed;
                    TestEqual(TEXT("Remove test item"), Inventory->RemoveItem(Instance.InstanceId, Removed), EUmbraInventoryResult::Success);
                }
                if (bMaterial)
                {
                    TestEqual(TEXT("Stone grade fixes generated rarity"), Instance.Rarity, Item->Rarity);
                    TestTrue(TEXT("Stone has no affixes"), Instance.RolledAffixes.IsEmpty());
                    TestEqual(TEXT("Stone cannot equip"), Equipment->EquipInstance(EUmbraEquipmentSlot::Amulet, Instance), EUmbraEquipResult::EquippingUnavailable);
                }
                else if (Seed < 4)
                {
                    const auto Slot = Item->GetAllowedEquipmentSlots()[Seed];
                    TestEqual(TEXT("Generated amulet equips"), Equipment->EquipInstance(Slot, Instance), EUmbraEquipResult::Success);
                    TestTrue(TEXT("Generated amulet unequips"), Equipment->Unequip(Slot));
                }
            }
            TestTrue(TEXT("Definition tooltip"), UUmbraItemTooltipDataBuilder::FromDefinition(Item).bValid);
        }
    }
    TestEqual(TEXT("Eligible amulets"), Accessories, 116);
    TestEqual(TEXT("Only nineteen smithing stones"), Materials, 19);
    FRandomStream Random(109); TSet<EUmbraItemCategory> Seen;
    for (int32 I=0; I<2000; ++I) { TArray<UUmbraItemDefinition*> Drops; Table->Select(Owner, Random, Drops); for (auto* Item:Drops) Seen.Add(Item->ItemCategory); }
    TestEqual(TEXT("All four loot categories reachable"), Seen.Num(), 4);
    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraRelicSlotsTest, "Umbra.Equipment.FourRelicSlots",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraRelicSlotsTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Owner=World->SpawnActor<AUmbraPlayerState>();
    auto* ASC=Owner->GetUmbraAbilitySystemComponent(); ASC->InitializeComponent(); ASC->InitAbilityActorInfo(Owner,Owner);
    auto* Equipment=Owner->GetEquipmentComponent(); Equipment->bEnableEquipment=true;
    Owner->FindComponentByClass<UUmbraDerivedStatsComponent>()->bUseWeaponDerivedPower=true; Owner->InitializeAttributes();
    auto* Relic=NewObject<UUmbraItemDefinition>(Owner); Relic->ItemCategory=EUmbraItemCategory::Accessory;
    Relic->AllowedSlots={EUmbraEquipmentSlot::Amulet};
    const auto Slots=Relic->GetAllowedEquipmentSlots();
    TestEqual(TEXT("A legacy amulet exposes four relic slots"),Slots.Num(),4);
    TArray<FGuid> Ids;
    for (auto Slot:Slots)
    {
        FUmbraItemInstance Instance; FUmbraItemInstance::Create(Owner,Relic,FGuid::NewGuid(),Instance); Ids.Add(Instance.InstanceId);
        TestEqual(TEXT("Four independent relics equip simultaneously"),Equipment->EquipInstance(Slot,Instance),EUmbraEquipResult::Success);
    }
    TestEqual(TEXT("Four occupied slots"),Equipment->GetSnapshot().Items.Num(),4);
    for (int32 Index=0;Index<Slots.Num();++Index) TestTrue(TEXT("Relic unequips"),Equipment->Unequip(Slots[Index]));
    Relic->AllowedSlots={EUmbraEquipmentSlot::Ring1};
    for (auto Slot:Slots) TestTrue(TEXT("A ring relic fits the amulet and ring backgrounds"),Relic->CanEquipInSlot(Slot));
    FUmbraItemInstance Instance; FUmbraItemInstance::Create(Owner,Relic,FGuid::NewGuid(),Instance);
    TestEqual(TEXT("Relic cannot equip as armor"),Equipment->EquipInstance(EUmbraEquipmentSlot::Head,Instance),EUmbraEquipResult::WrongSlot);
    TestEqual(TEXT("Relic cannot equip as weapon"),Equipment->EquipInstance(EUmbraEquipmentSlot::MainHand,Instance),EUmbraEquipResult::WrongSlot);
    auto* Armor=NewObject<UUmbraItemDefinition>(Owner); Armor->ItemCategory=EUmbraItemCategory::Armor; Armor->AllowedSlots={EUmbraEquipmentSlot::Head};
    for (auto Slot:Slots) TestFalse(TEXT("Head armor cannot enter relic slots"),Armor->CanEquipInSlot(Slot));
    World->DestroyWorld(false); return true;
}
#endif
