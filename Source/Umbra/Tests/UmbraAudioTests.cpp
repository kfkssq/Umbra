#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Audio/UmbraAudioLibrary.h"
#include "Audio/UmbraAudioSubsystem.h"
#include "Audio/UmbraAudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundCue.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraAudioSelectionTest, "Umbra.Audio.ContextSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraAudioSelectionTest::RunTest(const FString& Parameters)
{
	auto* Library = NewObject<UUmbraAudioLibrary>();
	FUmbraAudioEntry Base;
	Base.Event = EUmbraAudioEvent::Hit;
	Library->Events.Add(Base);
	FUmbraAudioEntry Weapon = Base;
	Weapon.Match.Weapon = EUmbraWeaponType::OneHandedSword;
	Library->Events.Add(Weapon);
	FUmbraAudioEntry Surface = Weapon;
	Surface.Match.Surface = SurfaceType1;
	Library->Events.Add(Surface);
	FUmbraAudioContext Context;
	TestTrue(TEXT("Generic fallback"), Library->Find(Base.Event, Context) == &Library->Events[0]);
	Context.Weapon = EUmbraWeaponType::OneHandedSword;
	TestTrue(TEXT("Weapon variant"), Library->Find(Base.Event, Context) == &Library->Events[1]);
	Context.Surface = SurfaceType1;
	TestTrue(TEXT("Weapon plus material wins"), Library->Find(Base.Event, Context) == &Library->Events[2]);
	Library->Events.Add(Surface);
	TestTrue(TEXT("Equal specificity first row is stable"), Library->Find(Base.Event, Context) == &Library->Events[2]);
	TestNull(TEXT("Unconfigured event silent"), Library->Find(EUmbraAudioEvent::Footstep, Context));
	auto* Item = NewObject<UUmbraItemDefinition>();
	Item->Rarity = EUmbraItemRarity::Unique;
	Item->WeaponType = EUmbraWeaponType::OneHandedMace;
	const auto ItemContext = FUmbraAudioContext::FromItem(Item);
	TestTrue(TEXT("Actual definition rarity captured"), ItemContext.bHasRarity && ItemContext.Rarity == EUmbraItemRarity::Unique);
	TestEqual(TEXT("Actual weapon captured"), ItemContext.Weapon, EUmbraWeaponType::OneHandedMace);
	FUmbraAudioEntry Rarity;
	Rarity.Match.bHasRarity = true;
	Rarity.Match.Rarity = EUmbraItemRarity::Common;
	TestEqual(TEXT("Unknown item is not Common"), Rarity.MatchScore(Rarity.Event, {}), INDEX_NONE);
	for (EUmbraAudioEvent Event : { EUmbraAudioEvent::LootSpawn, EUmbraAudioEvent::LootRarity })
	{
		Library->Events.Reset();
		Base.Event = Event;
		Library->Events.Add(Base);
		Rarity.Event = Event;
		Rarity.Match.Rarity = EUmbraItemRarity::Unique;
		Library->Events.Add(Rarity);
		TestTrue(TEXT("Loot uses actual rarity override, including intentionally empty rows"), Library->Find(Event, ItemContext) == &Library->Events[1]);
		TestTrue(TEXT("Unmapped rarity uses generic fallback"), Library->Find(Event, {}) == &Library->Events[0]);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraAudioPolicyTest, "Umbra.Audio.SilentAndRatePolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraAudioPolicyTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	auto* Audio = World->GetSubsystem<UUmbraAudioSubsystem>();
	auto* First = World->SpawnActor<AActor>();
	auto* Second = World->SpawnActor<AActor>();
	auto* Library = NewObject<UUmbraAudioLibrary>(World);
	FUmbraAudioEntry Entry;
	Entry.Event = EUmbraAudioEvent::Hit;
	Entry.Sounds.Add(nullptr);
	Library->Events.Add(Entry);
	TestFalse(TEXT("Null emitter"), Audio->Play(nullptr, Entry.Event, {}, FVector::ZeroVector, Library));
	TestFalse(TEXT("Configured empty resource is silent"), Audio->Play(First, Entry.Event, {}, FVector::ZeroVector, Library));
	TestFalse(TEXT("Loot ambience cannot escape through fire-and-forget playback"), Audio->Play(First, EUmbraAudioEvent::LootRarity, {}, FVector::ZeroVector, Library));
	// A transient empty cue exercises dispatch policy, not audible playback or any saved asset.
	Library->Events[0].Sounds.Add(NewObject<USoundCue>(Library));
	Library->Events[0].Probability = 0.f;
	TestFalse(TEXT("Probability zero"), Audio->Play(First, Entry.Event, {}, FVector::ZeroVector, Library));
	Library->Events[0].Probability = 1.f;
	Library->Events[0].Cooldown = 10.f;
	Library->Events[0].GlobalCooldown = 0.f;
	TestTrue(TEXT("Null rows ignored; valid cue dispatched"), Audio->Play(First, Entry.Event, {}, FVector::ZeroVector, Library));
	TestFalse(TEXT("Per-emitter repeated hit suppressed"), Audio->Play(First, Entry.Event, {}, FVector::ZeroVector, Library));
	Library->Events[0].GlobalCooldown = 10.f;
	TestTrue(TEXT("Independent emitter allowed"), Audio->Play(Second, Entry.Event, {}, FVector::ZeroVector, Library));
	auto* Third = World->SpawnActor<AActor>();
	TestFalse(TEXT("Many-target burst global suppression"), Audio->Play(Third, Entry.Event, {}, FVector::ZeroVector, Library));
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
