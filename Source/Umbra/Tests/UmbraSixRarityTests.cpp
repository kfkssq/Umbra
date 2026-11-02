#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Items/UmbraItemGeneration.h"
#include "Items/UmbraItemTooltipData.h"
#include "UI/Equipment/UmbraEquipmentTypes.h"
#include "Audio/UmbraAudioLibrary.h"
#include "Audio/UmbraAudioSubsystem.h"
#include "Audio/UmbraAudioComponent.h"
#include "Loot/UmbraWorldItem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Sound/SoundCue.h"
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraSixRarityTest, "Umbra.Items.Generation.SixRarities",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraSixRarityTest::RunTest(const FString&)
{
 auto* World=UWorld::CreateWorld(EWorldType::Game,false);
 auto* Owner=World->SpawnActor<AActor>();
 auto* Item=NewObject<UUmbraItemDefinition>(Owner);
 Item->DefinitionId=TEXT("Test.Six"); Item->ItemCategory=EUmbraItemCategory::Armor;
 Item->AllowedSlots={EUmbraEquipmentSlot::Chest};
 Item->GenerationProfile=NewObject<UUmbraItemGenerationProfile>(Owner);
 Item->GenerationProfile->Pool=NewObject<UUmbraAffixPool>(Owner);
 Item->SlotBackgroundTexture=NewObject<UTexture2D>(Owner);
 Item->TooltipIconBackgroundTexture=Item->SlotBackgroundTexture;
 FUmbraGenerationContext Context; Context.SourceLevel=10;
 for(uint8 Index=0;Index<6;++Index)
 {
  FUmbraGenerationRarity Row; Row.Rarity=EUmbraItemRarity(Index);
  Row.Color=FLinearColor(float(Index)/6,0.3f,0.5f,1); Row.bTintBackgrounds=true;
  Row.SlotBackgroundTexture=NewObject<UTexture2D>(Owner);
  Row.TooltipIconBackgroundTexture=Row.SlotBackgroundTexture;
  Item->GenerationProfile->Rarities={Row};
  FUmbraItemInstance Instance; FString Error;
  TestTrue(TEXT("Each of six rarities can generate"),UmbraItemGeneration::Generate(Owner,Item,Context,5,FGuid::NewGuid(),Instance,Error));
  TestEqual(TEXT("Generated rarity preserved"),Instance.GetRarity(),Row.Rarity);
  TestEqual(TEXT("Audio uses generated rarity"),FUmbraAudioContext::FromInstance(Instance).Rarity,Row.Rarity);
  auto Display=FUmbraEquipmentItemDisplay::FromInstance(Instance);
  TestEqual(TEXT("Slot uses rarity art, not template"),Display.SlotBackgroundTexture.Get(),Row.SlotBackgroundTexture.Get());
  TestTrue(TEXT("Neutral slot art tinted"),Display.bUseRarityBackgroundColor);
  FUmbraInventorySnapshot Bag; Bag.bReady=true; Bag.Capacity=1;
  auto& Entry=Bag.Items.AddDefaulted_GetRef(); Entry.SlotIndex=0; Entry.SetInstance(Instance);
  auto Data=UUmbraItemTooltipDataBuilder::FromInventory(Bag,0,Instance.InstanceId,Item,nullptr,false,EUmbraEquipmentSlot::Chest);
  TestTrue(TEXT("Generated tooltip valid"),Data.bValid);
  TestEqual(TEXT("Tooltip instance color"),Data.RarityColor,Row.Color);
  TestEqual(TEXT("Tooltip uses rarity art"),Data.TooltipIconBackgroundTexture.Get(),Row.TooltipIconBackgroundTexture.Get());
  TestTrue(TEXT("Neutral tooltip art tinted"),Data.bTintRarityBackgrounds);
 }
 Item->GenerationProfile->Rarities[0].Rarity=EUmbraItemRarity(6);
 FString Error; TestFalse(TEXT("Unknown rarity rejected"),UmbraItemGeneration::Validate(Item,Error));
 World->DestroyWorld(false);
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraLootSpawnAudioTest, "Umbra.Audio.LootSpawnLifecycle",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraLootSpawnAudioTest::RunTest(const FString&)
{
 auto* World=UWorld::CreateWorld(EWorldType::Game,false);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 World->InitializeActorsForPlay(FURL()); World->BeginPlay();
 for(int32 Frame=0;Frame<180;++Frame){++GFrameCounter;World->Tick(LEVELTICK_All,1.f/60);}
 auto* Owner=World->SpawnActor<AActor>();
 auto* Audio=World->GetSubsystem<UUmbraAudioSubsystem>();
 auto* Library=NewObject<UUmbraAudioLibrary>(World);
 for(uint8 Index=0;Index<6;++Index)
 {
  FUmbraAudioEntry Row; Row.Event=EUmbraAudioEvent::LootSpawn;
  Row.Match.bHasRarity=true; Row.Match.Rarity=EUmbraItemRarity(Index);
  Row.Sounds.Add(NewObject<USoundCue>(Library)); Row.GlobalCooldown=0.015f;
  Library->Events.Add(Row);
 }
 TMap<UObject*,int32> Counts; TSet<EUmbraItemRarity> Heard;
 Audio->OnDispatchedForTest.AddLambda([&](UObject* E,EUmbraAudioEvent Event,const FUmbraAudioContext& C)
 { if(Event==EUmbraAudioEvent::LootSpawn){++Counts.FindOrAdd(E);Heard.Add(C.Rarity);} });
 auto Spawn=[&](uint8 Index,double Age,bool Settled)
 {
  auto* Definition=NewObject<UUmbraItemDefinition>(Owner); Definition->Rarity=EUmbraItemRarity(Index);
  FUmbraItemInstance Instance; FUmbraItemInstance::Create(Owner,Definition,FGuid::NewGuid(),Instance);
  auto* Drop=World->SpawnActorDeferred<AUmbraWorldItem>(AUmbraWorldItem::StaticClass(),FTransform::Identity);
  Drop->DropPhysics.bEnabled=false; Drop->AudioEvents->LibraryOverride=Library;
  Drop->InitializeInstance(Instance);
  Drop->DropSpawnServerTime=World->GetTimeSeconds()-Age; Drop->DropMotion.bSettled=Settled;
  Drop->FinishSpawning(FTransform::Identity);
  if(!Drop->HasActorBegunPlay()) Drop->DispatchBeginPlay();
  return Drop;
 };
 for(uint8 Index=0;Index<6;++Index)
 {
  auto* Drop=Spawn(Index,0,true);
  TestEqual(TEXT("Fresh settled item announces once"),Counts.FindRef(Drop),1);
  Drop->PlaySpawnAudio();
  TestEqual(TEXT("Repeated presentation never repeats announcement"),Counts.FindRef(Drop),1);
 }
 TestEqual(TEXT("Six simultaneous rarities do not suppress each other"),Heard.Num(),6);
 auto* Suppressed=Spawn(0,0,false);
 TestEqual(TEXT("Same rarity retains burst limit"),Counts.FindRef(Suppressed),0);
 auto* Old=Spawn(1,2,false);
 TestEqual(TEXT("Old relevance does not announce"),Counts.FindRef(Old),0);
 auto* Future=Spawn(2,-0.1,false);
 TestFalse(TEXT("Future clock not prematurely consumed"),Future->bSpawnAudioSent);
 for(int32 Frame=0;Frame<30;++Frame){++GFrameCounter;World->Tick(LEVELTICK_All,1.f/60);}
 TestEqual(TEXT("Clock catchup retries once"),Counts.FindRef(Future),1);
 World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
 return true;
}
#endif
