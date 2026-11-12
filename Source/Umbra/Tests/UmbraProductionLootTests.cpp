#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Items/UmbraItemGeneration.h"
#include "Loot/UmbraLootTable.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Player/UmbraPlayerState.h"
#include "Items/UmbraItemTooltipData.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "Stats/UmbraDerivedStatsComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraProductionLootTest, "Umbra.Items.Generation.ProductionLoot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraProductionLootTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	auto* Owner = World->SpawnActor<AActor>();
	auto* Profile = NewObject<UUmbraItemGenerationProfile>(Owner);
	Profile->bRarityFromAffixCount = true;
	Profile->Pool = NewObject<UUmbraAffixPool>(Owner);
	for (int32 I = 0; I < 6; ++I)
	{
		FUmbraGenerationRarity Row; Row.Rarity = EUmbraItemRarity(I); Row.AffixCount = 0;
		Row.Weight = I >= 4 ? 10000 : 0; Profile->Rarities.Add(Row);
	}
	for (int32 I = 0; I < 4; ++I)
	{
		auto* Affix = NewObject<UUmbraAffixDefinition>(Owner);
		Affix->AffixId = FName(*FString::Printf(TEXT("Primary%d"), I));
		FUmbraAffixTier Tier; Tier.TierId = TEXT("T1"); Tier.Values.Kind = EUmbraAffixKind::Primary;
		Tier.Values.Primary = EUmbraPrimaryAttribute(I); Tier.Values.MinValue = 2; Tier.Values.MaxValue = 12;
		Affix->Tiers.Add(Tier);
		FUmbraAffixPoolEntry Entry; Entry.Affix = Affix; Profile->Pool->Entries.Add(Entry);
	}
	auto MakeItem = [&](int32 Index, EUmbraWeaponType Type)
	{
		auto* Item = NewObject<UUmbraItemDefinition>(Owner);
		Item->DefinitionId = FName(*FString::Printf(TEXT("Production.%03d"), Index));
		Item->ItemCategory = EUmbraItemCategory::Weapon; Item->WeaponType = Type;
		Item->AllowedSlots = {EUmbraEquipmentSlot::MainHand}; Item->GenerationProfile = Profile;
		Item->Weapon = NewObject<UUmbraWeaponProfile>(Owner); return Item;
	};
	auto* Sword = MakeItem(0, EUmbraWeaponType::OneHandedSword);
	FUmbraGenerationContext Context; Context.SourceLevel = 1;
	FString Error; FUmbraItemInstance Result, Again;
	for (int32 Count = 1; Count <= 4; ++Count)
	{
		Profile->AffixCountWeights = {0,0,0,0}; Profile->AffixCountWeights[Count - 1] = 1;
		TestTrue(TEXT("Forced count succeeds"), UmbraItemGeneration::Generate(Owner,Sword,Context,42,FGuid(1,2,3,4),Result,Error));
		TestEqual(TEXT("Count drives rarity, ignores legacy rarity weights"), int32(Result.Rarity), Count - 1);
		TestEqual(TEXT("Exact affix count"), Result.RolledAffixes.Num(), Count);
		UmbraItemGeneration::Generate(Owner,Sword,Context,42,FGuid(1,2,3,4),Again,Error);
		TestTrue(TEXT("Deterministic complete payload"), FUmbraItemInstance::StaticStruct()->CompareScriptStruct(&Result,&Again,0));
	}
	// Source restrictions are applied before sampling; banned weights never leak through.
	Profile->AffixCountWeights = {50,30,15,5};
	Profile->Rarities[4].AffixCount = Profile->Rarities[5].AffixCount = 4;
	for (int32 Source = 0; Source <= 3; ++Source)
	{
		Sword->SourceRarity = Source;
		TSet<EUmbraItemRarity> Seen;
		for (int32 Seed = 0; Seed < 128; ++Seed)
		{
			if (!TestTrue(TEXT("Restricted source generates"), UmbraItemGeneration::Generate(Owner,Sword,Context,Seed,FGuid::NewGuid(),Result,Error))) continue;
			Seen.Add(Result.Rarity);
			TestTrue(TEXT("Source rarity gate"), Source == 0 ? Result.Rarity <= EUmbraItemRarity::Rare : Source == 1 ? Result.Rarity <= EUmbraItemRarity::Epic
				: Source == 2 ? Result.Rarity >= EUmbraItemRarity::Epic && Result.Rarity <= EUmbraItemRarity::Legendary : Result.Rarity >= EUmbraItemRarity::Legendary);
			TestEqual(TEXT("High quality retains four ordinary affixes"), Result.RolledAffixes.Num(), FMath::Min(int32(Result.Rarity)+1,4));
		}
		TestEqual(TEXT("Every allowed rank reachable"), Seen.Num(), Source == 0 ? 3 : Source == 1 ? 4 : 2);
	}
	Sword->SourceRarity = 0; Profile->AffixCountWeights = {0,0,0,1};
	TestFalse(TEXT("No fallback to banned rarity if allowed weights are zero"), UmbraItemGeneration::Generate(Owner,Sword,Context,42,FGuid::NewGuid(),Result,Error));
	Profile->AffixCountWeights = {50,30,15,5};
	for (int32 Source : {1,2})
	{
		Sword->SourceRarity = Source;
		int32 Promotions = 0;
		for (int32 Seed = 0; Seed < 2000; ++Seed)
		{
			if (!TestTrue(TEXT("Promotion sampling succeeds"), UmbraItemGeneration::Generate(Owner,Sword,Context,Seed,FGuid::NewGuid(),Result,Error))) break;
			Promotions += int32(Result.Rarity) == Source + 2 ? 1 : 0;
		}
		const int32 Expected = Source == 1 ? 100 : 500;
		TestTrue(TEXT("Configured promotion probability respected"), FMath::Abs(Promotions - Expected) <= 40);
	}
	for (float Chance : {0.f,1.f})
	{
		Sword->SourceRarity = 2; Profile->SourceTwoLegendaryChance = Chance;
		TestTrue(TEXT("Configured promotion boundary generates"), UmbraItemGeneration::Generate(Owner,Sword,Context,42,FGuid::NewGuid(),Result,Error));
		TestEqual(TEXT("Boundary chooses exact quality"), Result.Rarity, Chance == 0.f ? EUmbraItemRarity::Epic : EUmbraItemRarity::Legendary);
	}
	Profile->SourceTwoLegendaryChance = -0.1f;
	TestFalse(TEXT("Invalid probability rejected"), UmbraItemGeneration::Validate(Sword,Error));
	Profile->SourceTwoLegendaryChance = 0.25f;
	Sword->SourceRarity = -1; Profile->AffixCountWeights = {0,0,0,1};
	Profile->Pool->Entries[0].Affix->Categories = {EUmbraItemCategory::Armor};
	TestFalse(TEXT("Incompatible category cannot create a partial epic"), UmbraItemGeneration::Generate(Owner,Sword,Context,42,FGuid::NewGuid(),Result,Error));
	TestFalse(TEXT("Failed output empty"), Result.IsValid());
	Profile->Pool->Entries[0].Affix->Categories.Reset();
	// Trap: A shares a target with B and an exclusive group with C. Only B+C can complete.
	Profile->AffixCountWeights = {0,1,0,0};
	auto SavedEntries = Profile->Pool->Entries;
	Profile->Pool->Entries.SetNum(3);
	Profile->Pool->Entries[0].Affix->ExclusiveGroup = TEXT("Trap");
	Profile->Pool->Entries[1].Affix->Tiers[0].Values.Primary = EUmbraPrimaryAttribute::Strength;
	Profile->Pool->Entries[2].Affix->ExclusiveGroup = TEXT("Trap");
	for (int32 Seed = 0; Seed < 32; ++Seed)
	{
		TestTrue(TEXT("Completion-aware weighted selection"), UmbraItemGeneration::Generate(Owner,Sword,Context,Seed,FGuid::NewGuid(),Result,Error));
		TestFalse(TEXT("Trap excluded"), Result.RolledAffixes.ContainsByPredicate([](const auto& R) { return R.AffixId == TEXT("Primary0"); }));
	}
	Profile->Pool->Entries = SavedEntries;
	Profile->Pool->Entries[0].Affix->ExclusiveGroup = NAME_None;
	Profile->Pool->Entries[1].Affix->Tiers[0].Values.Primary = EUmbraPrimaryAttribute::Dexterity;
	Profile->Pool->Entries[2].Affix->ExclusiveGroup = NAME_None;
	auto* Loot = NewObject<UUmbraLootTable>(Owner); Loot->bUseTypeGroups = true;
	FUmbraLootCategoryGroup Category;
	FUmbraLootTypeGroup SwordGroup; SwordGroup.GroupId = TEXT("Sword"); SwordGroup.WeaponType = Sword->WeaponType;
	FUmbraLootTemplate Entry; Entry.ItemDefinition = Sword; SwordGroup.Templates.Add(Entry);
	FUmbraLootTypeGroup DaggerGroup; DaggerGroup.GroupId = TEXT("Dagger"); DaggerGroup.WeaponType = EUmbraWeaponType::Dagger;
	Entry.ItemDefinition = MakeItem(1, DaggerGroup.WeaponType); DaggerGroup.Templates.Add(Entry);
	Category.Types = {SwordGroup,DaggerGroup}; Loot->Categories = {Category};
	TestTrue(TEXT("Valid grouped table"), Loot->ValidateTypeGroups(Error));
	TArray<EUmbraWeaponType> Baseline;
	FRandomStream Random(12345); TArray<UUmbraItemDefinition*> Picks;
	for (int32 I = 0; I < 2000; ++I) { Loot->Select(Owner,Random,Picks); if (Picks.Num() == 1) Baseline.Add(Picks[0]->WeaponType); }
	for (int32 I = 2; I < 30; ++I) { Entry.ItemDefinition = MakeItem(I,Sword->WeaponType); Loot->Categories[0].Types[0].Templates.Add(Entry); }
	Random.Initialize(12345); int32 Swords = 0;
	for (int32 I = 0; I < Baseline.Num(); ++I)
	{
		Loot->Select(Owner,Random,Picks);
		if (TestEqual(TEXT("One piece"),Picks.Num(),1))
		{
			TestEqual(TEXT("Asset count cannot affect type sequence"), Picks[0]->WeaponType, Baseline[I]);
			Swords += Picks[0]->WeaponType == Sword->WeaponType;
		}
	}
	TestTrue(TEXT("Balanced types despite 29:1 templates"), Swords > 850 && Swords < 1150);
	Loot->EquipmentDropChance = 0; Loot->Select(Owner,Random,Picks); TestTrue(TEXT("Zero drop chance"), Picks.IsEmpty());
	Loot->Categories[0].Types[1].Templates.Reset(); TestFalse(TEXT("Empty enabled type blocks publishing"),Loot->ValidateTypeGroups(Error));
	World->DestroyWorld(false); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraProductionAssetsTest, "Umbra.Items.Generation.ProductionAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraProductionAssetsTest::RunTest(const FString& Parameters)
{
	auto* Table = LoadObject<UUmbraLootTable>(nullptr,TEXT("/Game/Items/GeneratedEquipment/V2/DA_Loot_Equipment.DA_Loot_Equipment"));
	if (!TestNotNull(TEXT("Production table"),Table)) return false;
	FString Error;
	if (!TestTrue(TEXT("Table valid"),Table->ValidateTypeGroups(Error))) { AddError(Error); return false; }
	UWorld* World = UWorld::CreateWorld(EWorldType::Game,false);
	auto* Owner = World->SpawnActor<AActor>();
	int32 Checked = 0, Staged = 0;
	for (const auto& Category : Table->Categories) for (const auto& Group : Category.Types) for (const auto& Entry : Group.Templates)
	{
		auto* Item = Entry.ItemDefinition.Get(); auto* Profile = Item->GenerationProfile.Get();
		const bool bShield = Item->WeaponType == EUmbraWeaponType::SmallShield || Item->WeaponType == EUmbraWeaponType::MediumShield || Item->WeaponType == EUmbraWeaponType::Greatshield;
		if (!Item->bEquippingEnabled)
		{
			++Staged;
			const auto Tooltip = UUmbraItemTooltipDataBuilder::FromDefinition(Item);
			TestTrue(TEXT("Staged item has visible availability status"),Tooltip.bValid && !Tooltip.bEquippingEnabled && !Tooltip.EquippingUnavailableReason.IsEmpty());
			TestTrue(TEXT("Staged item does not pretend to have active attack damage"),Item->Weapon == nullptr);
		}
		const auto Weights = Profile->AffixCountWeights;
		const auto SavedRarities = Profile->Rarities;
		const float SavedChance = Profile->SourceTwoLegendaryChance;
		TestTrue(TEXT("Production source rarity imported"), Item->SourceRarity >= 0 && Item->SourceRarity <= 3);
		for (int32 Rank = 0; Rank < 6; ++Rank)
		{
			if (Item->SourceRarity == 0 ? Rank > 2 : Item->SourceRarity == 1 ? Rank > 3 : Item->SourceRarity == 2 ? Rank < 3 || Rank > 4 : Rank < 4) continue;
			const int32 Count = FMath::Min(Rank+1,4);
			if (Item->SourceRarity == 2) Profile->SourceTwoLegendaryChance = Rank == 4 ? 1.f : 0.f;
			Profile->AffixCountWeights = {0,0,0,0};
			if (Rank < 4) Profile->AffixCountWeights[Rank] = 1;
			for (auto& Row : Profile->Rarities) Row.Weight = int32(Row.Rarity) == Rank ? 1.f : 0.f;
			for (int32 Level : {1,9,10,100}) for (int32 Seed = 0; Seed < 8; ++Seed)
			{
				FUmbraGenerationContext Context; Context.SourceLevel = Level;
				FUmbraItemInstance Instance;
				if (!UmbraItemGeneration::Generate(Owner,Item,Context,Seed,FGuid::NewGuid(),Instance,Error))
					AddError(Item->GetPathName()+TEXT(": ")+Error);
				else
				{
					TestEqual(TEXT("Production count"),Instance.RolledAffixes.Num(),Count);
					TestEqual(TEXT("Production rarity"),int32(Instance.Rarity),Rank);
					TestEqual(TEXT("Reserved special effect marker"), Instance.HasSpecialEffect(), Rank >= 4);
					TestEqual(TEXT("Unique marker"), Instance.IsUnique(), Rank == 5);
					FUmbraItemInstanceRecord Record; FUmbraItemInstance Restored;
					TestTrue(TEXT("Quality marker payload captures"), FUmbraItemInstanceRecord::Capture(Instance,Record));
					TestTrue(TEXT("Quality marker payload restores"), Record.Restore(Item,Restored));
					TestEqual(TEXT("Special marker survives restore"), Restored.HasSpecialEffect(), Instance.HasSpecialEffect());
					TestEqual(TEXT("Unique marker survives restore"), Restored.IsUnique(), Instance.IsUnique());

					for (const auto& Roll : Instance.RolledAffixes)
					{
						if (Category.Category == EUmbraItemCategory::Armor || bShield) TestTrue(TEXT("Armor/shield excludes offensive affixes"),Roll.Kind != EUmbraAffixKind::Damage && Roll.Attribute != EUmbraEquipmentAffixStat::CriticalChance);
						if (Category.Category == EUmbraItemCategory::Weapon && !bShield) TestTrue(TEXT("Weapon excludes defense affixes"),Roll.Kind != EUmbraAffixKind::Attribute || (Roll.Attribute != EUmbraEquipmentAffixStat::Armor && Roll.Attribute != EUmbraEquipmentAffixStat::MagicResistance));
					}
				}
			}
		}
		Profile->AffixCountWeights = Weights; Profile->Rarities = SavedRarities; Profile->SourceTwoLegendaryChance = SavedChance; ++Checked;
	}
	AddInfo(FString::Printf(TEXT("Validated %d templates, 32 generations per allowed rarity"),Checked));
	TestEqual(TEXT("Complete catalog"),Checked,580);
	TestEqual(TEXT("Shield/ranged/catalyst staged"),Staged,92);
	World->DestroyWorld(false); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraStagedEquipmentTest, "Umbra.Items.Generation.StagedEquipment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraStagedEquipmentTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game,false);
	auto* Owner = World->SpawnActor<AUmbraPlayerState>();
	auto* ASC = Owner->GetUmbraAbilitySystemComponent(); ASC->InitializeComponent(); ASC->InitAbilityActorInfo(Owner,Owner);
	auto* Equipment = Owner->GetEquipmentComponent(); Equipment->bEnableEquipment = true;
	Owner->FindComponentByClass<UUmbraDerivedStatsComponent>()->bUseWeaponDerivedPower = true; Owner->InitializeAttributes();
	auto* Item = NewObject<UUmbraItemDefinition>(Owner);
	Item->DefinitionId = TEXT("Test.StagedShield"); Item->ItemCategory = EUmbraItemCategory::Weapon;
	Item->WeaponType = EUmbraWeaponType::SmallShield; Item->AllowedSlots = {EUmbraEquipmentSlot::OffHand};
	Item->bEquippingEnabled = false; Item->EquippingUnavailableReason = FText::FromString(TEXT("Shield not yet available"));
	Item->Requirements.Strength = 12;
	FUmbraItemInstance Instance;
	TestTrue(TEXT("Staged instance valid"),FUmbraItemInstance::Create(Owner,Item,FGuid::NewGuid(),Instance));
	auto* Inventory = Owner->GetInventoryComponent();
	TestEqual(TEXT("May carry staged equipment"),Inventory->AddInstance(Instance),EUmbraInventoryResult::Success);
	const int32 Revision = Inventory->GetSnapshot().Revision;
	const auto ActiveEffects = ASC->GetActiveEffects(FGameplayEffectQuery()).Num();
	TestEqual(TEXT("Direct equip rejected"),Equipment->EquipInstance(EUmbraEquipmentSlot::OffHand,Instance),EUmbraEquipResult::EquippingUnavailable);
	TestEqual(TEXT("Transfer returns distinct unavailable reason"),Owner->EquipFromInventory(EUmbraEquipmentSlot::OffHand,Instance.InstanceId),EUmbraTransferResult::EquippingUnavailable);
	TestEqual(TEXT("Rejected transfer preserves inventory revision"),Inventory->GetSnapshot().Revision,Revision);
	TestEqual(TEXT("No equipment GE created"),ASC->GetActiveEffects(FGameplayEffectQuery()).Num(),ActiveEffects);
	const auto Tooltip = UUmbraItemTooltipDataBuilder::FromDefinition(Item);
	TestTrue(TEXT("Tooltip explicitly unavailable"),Tooltip.bValid && !Tooltip.bEquippingEnabled && Tooltip.RequirementStatusText.EqualTo(Item->EquippingUnavailableReason));
	TestEqual(TEXT("Requirements retained"),Tooltip.RequiredPrimaries.Strength,12.f);
	World->DestroyWorld(false); return true;
}
#endif
