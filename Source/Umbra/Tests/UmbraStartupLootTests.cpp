#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_TEST
#include "Misc/AutomationTest.h"
#include "Loot/UmbraLootTestSpawner.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraStartupLootTest, "Umbra.Loot.SixRarityStartup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraStartupLootTest::RunTest(const FString&)
{
	auto* Class=LoadClass<AUmbraLootTestSpawner>(nullptr,TEXT("/Game/Items/Tests/BP_SixRarityStartup.BP_SixRarityStartup_C"));
	if (!TestNotNull(TEXT("Configured startup BP"),Class)) return false;
	auto* World=UWorld::CreateWorld(EWorldType::Game,false);
	auto* Spawner=World->SpawnActor<AUmbraLootTestSpawner>(Class);
	TestTrue(TEXT("Random seed each session enabled"),Spawner->bRandomizeSeedEachPlay);
	TSet<FName> Templates;
	for (int32 Seed=0;Seed<24;++Seed)
	{
		TArray<FUmbraItemInstance> Batch, Repeat;
		if (!TestTrue(TEXT("Complete batch"),Spawner->GenerateRarityBatch(Seed,Batch))) continue;
		TestEqual(TEXT("Exactly six"),Batch.Num(),6);
		Spawner->GenerateRarityBatch(Seed,Repeat);
		if (!TestEqual(TEXT("Repeat complete"),Repeat.Num(),6)) continue;
		for (int32 Index=0;Index<6;++Index)
		{
			TestTrue(TEXT("Valid real generated payload"),Batch[Index].IsValid());
			TestEqual(TEXT("One per rarity in order"),Batch[Index].GetRarity(),EUmbraItemRarity(Index));
			TestEqual(TEXT("Same seed selects same template"),Batch[Index].DefinitionId,Repeat[Index].DefinitionId);
			TestEqual(TEXT("Same seed preserves generation seed"),Batch[Index].GenerationSeed,Repeat[Index].GenerationSeed);
			TestTrue(TEXT("Fresh instance identity"),Batch[Index].InstanceId!=Repeat[Index].InstanceId);
			Templates.Add(Batch[Index].DefinitionId);
		}
	}
	TestTrue(TEXT("Different seeds vary templates"),Templates.Num()>6);
	Spawner->Items.Reset();
	TArray<FUmbraItemInstance> Empty;
	TestFalse(TEXT("Empty pool fails without partial batch"),Spawner->GenerateRarityBatch(0,Empty));
	TestTrue(TEXT("No partial output"),Empty.IsEmpty());
	World->DestroyWorld(false);
	return true;
}
#endif
