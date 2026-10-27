#include "Audio/UmbraAudioSubsystem.h"
#include "Audio/UmbraAudioLibrary.h"
#include "Audio/UmbraAudioSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundConcurrency.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	float Safe(float Value, float Default, float Min, float Max)
	{
		return FMath::Clamp(FMath::IsFinite(Value) ? Value : Default, Min, Max);
	}
}

bool UUmbraAudioSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{
	return Type == EWorldType::Game || Type == EWorldType::PIE;
}

void UUmbraAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (GetWorld()->GetNetMode() == NM_DedicatedServer) return;
	AudioRandom.Initialize(static_cast<int32>(FPlatformTime::Cycles()));
	DefaultLibrary = GetDefault<UUmbraAudioSettings>()->DefaultLibrary.LoadSynchronous();
	DefaultConcurrency = NewObject<USoundConcurrency>(this);
	DefaultConcurrency->Concurrency.MaxCount = 24;
	DefaultConcurrency->Concurrency.bLimitToOwner = false;
	DefaultConcurrency->Concurrency.ResolutionRule = EMaxConcurrentResolutionRule::StopOldest;
	DefaultAttenuation = NewObject<USoundAttenuation>(this);
	DefaultAttenuation->Attenuation.bAttenuate = true;
	DefaultAttenuation->Attenuation.bSpatialize = true;
	DefaultAttenuation->Attenuation.AttenuationShapeExtents = FVector(200.f);
	DefaultAttenuation->Attenuation.FalloffDistance = 2000.f;
	LootLoopConcurrency = NewObject<USoundConcurrency>(this);
	LootLoopConcurrency->Concurrency.MaxCount = 4;
	LootLoopConcurrency->Concurrency.ResolutionRule = EMaxConcurrentResolutionRule::StopFarthestThenPreventNew;
}

UAudioComponent* UUmbraAudioSubsystem::CreateLootLoop(AActor* Owner, const FUmbraAudioContext& Context, UUmbraAudioLibrary* Override)
{
	if (!IsValid(Owner) || !Owner->GetRootComponent() || !GetWorld() || !GetWorld()->IsGameWorld()
		|| GetWorld()->GetNetMode() == NM_DedicatedServer) return nullptr;
	const UUmbraAudioLibrary* Library = IsValid(Override) ? Override : DefaultLibrary.Get();
	const FUmbraAudioEntry* Entry = Library ? Library->Find(EUmbraAudioEvent::LootRarity, Context) : nullptr;
	if (!Entry && Library != DefaultLibrary)
	{
		Library = DefaultLibrary;
		Entry = Library ? Library->Find(EUmbraAudioEvent::LootRarity, Context) : nullptr;
	}
	if (!Entry) return nullptr;
	TArray<USoundBase*, TInlineAllocator<8>> Sounds;
	for (USoundBase* Sound : Entry->Sounds) if (IsValid(Sound) && Sound->IsLooping()) Sounds.Add(Sound);
	const float Chance = Safe(Entry->Probability, 0.f, 0.f, 1.f);
	if (Sounds.IsEmpty() || Chance <= 0.f || (Chance < 1.f && AudioRandom.FRand() >= Chance)) return nullptr;
	const float* LibraryVolume = Library->CategoryVolumes.Find(Entry->Category);
	const float* RuntimeVolume = CategoryVolumes.Find(Entry->Category);
	const float Volume = Safe(Entry->Volume, 1.f, 0.f, 4.f)
		* (LibraryVolume ? Safe(*LibraryVolume, 1.f, 0.f, 4.f) : 1.f) * (RuntimeVolume ? *RuntimeVolume : 1.f);
	if (Volume <= 0.f) return nullptr;
	auto* Component = NewObject<UAudioComponent>(Owner);
	Component->bAutoActivate = false;
	Component->bAutoDestroy = false;
	Component->bStopWhenOwnerDestroyed = true;
	Component->SetupAttachment(Owner->GetRootComponent());
	Component->SetSound(Sounds[AudioRandom.RandHelper(Sounds.Num())]);
	Component->SetVolumeMultiplier(Volume);
	const float Low = Safe(Entry->PitchMin, 1.f, 0.25f, 4.f);
	const float High = Safe(Entry->PitchMax, 1.f, 0.25f, 4.f);
	Component->SetPitchMultiplier(AudioRandom.FRandRange(FMath::Min(Low, High), FMath::Max(Low, High)));
	Component->AttenuationSettings = Entry->Attenuation ? Entry->Attenuation.Get() : DefaultAttenuation.Get();
	Component->ConcurrencySet.Add(Entry->Concurrency ? Entry->Concurrency.Get() : LootLoopConcurrency.Get());
	Component->RegisterComponent();
	return Component;
}

void UUmbraAudioSubsystem::SetCategoryVolume(EUmbraAudioCategory Category, float Volume)
{
	CategoryVolumes.Add(Category, Safe(Volume, 1.f, 0.f, 4.f));
}

bool UUmbraAudioSubsystem::Play(UObject* Emitter, EUmbraAudioEvent Event, const FUmbraAudioContext& Context,
	const FVector& Location, UUmbraAudioLibrary* Override, bool bForce2D)
{
	// Persistent loot ambience is exclusively owned by the world item lifecycle.
	if (Event == EUmbraAudioEvent::LootRarity) return false;
	UWorld* World = GetWorld();
	if (!IsValid(Emitter) || !World || !World->IsGameWorld() || World->GetNetMode() == NM_DedicatedServer
		|| Location.ContainsNaN()) return false;
	const UUmbraAudioLibrary* Library = IsValid(Override) ? Override : DefaultLibrary.Get();
	const FUmbraAudioEntry* Entry = Library ? Library->Find(Event, Context) : nullptr;
	if (!Entry && Library != DefaultLibrary)
	{
		Library = DefaultLibrary;
		Entry = Library ? Library->Find(Event, Context) : nullptr;
	}
	if (!Entry) return false;
	TArray<USoundBase*, TInlineAllocator<8>> Sounds;
	for (USoundBase* Sound : Entry->Sounds) if (IsValid(Sound)) Sounds.Add(Sound);
	if (Sounds.IsEmpty()) return false;
	const double Now = World->GetRealTimeSeconds(); // UI remains usable while the world is paused.
	// Event-driven pruning bounds lifetime without a housekeeping tick or retaining actors/widgets.
	for (auto It = NextEmitterTimes.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
	auto& Times = NextEmitterTimes.FindOrAdd(Emitter);
	if (Now < Times.FindRef(Event) || Now < NextGlobalTimes.FindRef(Event)) return false;
	const float Chance = Safe(Entry->Probability, 0.f, 0.f, 1.f);
	if (Chance <= 0.f || (Chance < 1.f && AudioRandom.FRand() >= Chance)) return false;
	const float* LibraryVolume = Library->CategoryVolumes.Find(Entry->Category);
	const float* RuntimeVolume = CategoryVolumes.Find(Entry->Category);
	const float Volume = Safe(Entry->Volume, 1.f, 0.f, 4.f)
		* (LibraryVolume ? Safe(*LibraryVolume, 1.f, 0.f, 4.f) : 1.f)
		* (RuntimeVolume ? *RuntimeVolume : 1.f);
	if (Volume <= 0.f) return false;
	Times.Add(Event, Now + Safe(Entry->Cooldown, 0.08f, 0.f, 3600.f));
	NextGlobalTimes.Add(Event, Now + Safe(Entry->GlobalCooldown, 0.015f, 0.f, 60.f));
	const float Low = Safe(Entry->PitchMin, 1.f, 0.25f, 4.f);
	const float High = Safe(Entry->PitchMax, 1.f, 0.25f, 4.f);
	const float Pitch = AudioRandom.FRandRange(FMath::Min(Low, High), FMath::Max(Low, High));
	USoundBase* Sound = Sounds[AudioRandom.RandHelper(Sounds.Num())];
	USoundConcurrency* Concurrency = Entry->Concurrency ? Entry->Concurrency.Get() : DefaultConcurrency.Get();
	if (Entry->bSpatial && !bForce2D)
		UGameplayStatics::PlaySoundAtLocation(World, Sound, Location, FRotator::ZeroRotator, Volume, Pitch, 0.f,
			Entry->Attenuation ? Entry->Attenuation.Get() : DefaultAttenuation.Get(), Concurrency, Cast<AActor>(Emitter));
	else
		UGameplayStatics::PlaySound2D(World, Sound, Volume, Pitch, 0.f, Concurrency, nullptr, true);
	return true;
}
