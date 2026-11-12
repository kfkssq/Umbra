#include "Loot/UmbraLootTestSpawner.h"
#include "Loot/UmbraWorldItem.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Umbra.h"

AUmbraLootTestSpawner::AUmbraLootTestSpawner()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	PrimaryActorTick.bCanEverTick = false;
}

void AUmbraLootTestSpawner::BeginPlay()
{
	Super::BeginPlay();
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (!HasAuthority() || GetNetMode() == NM_Client || bSpawned || !bEnabled) return;
	bSpawned = true;
	const int32 Seed = bRandomizeSeedEachPlay ? int32(FGuid::NewGuid().A & MAX_int32) : GenerationSeed;
	FRandomStream ItemSeeds(Seed);
	const int32 SingleIndex = bRandomSingleItem && !Items.IsEmpty() ? ItemSeeds.RandRange(0, Items.Num() - 1) : INDEX_NONE;
	TArray<FUmbraItemInstance> Batch;
	if (bOneOfEachRarity && !GenerateRarityBatch(Seed, Batch))
	{
		UE_LOG(LogUmbra, Error, TEXT("Six-rarity test: incomplete batch; check Items, source level and profile eligibility."));
		return;
	}
	// Test fixture: configurable spacing, static ground within 500 cm, 2 cm clearance.
	for (int32 Index = 0; Index < (bOneOfEachRarity ? Batch.Num() : bRandomSingleItem ? (SingleIndex == INDEX_NONE ? 0 : 1) : Items.Num()); ++Index)
	{
		const FVector Point = GetActorLocation() + GetActorRightVector() * (Index * FMath::Max(1.f, Spacing));
		FHitResult Ground;
		if (!GetWorld()->LineTraceSingleByObjectType(Ground, Point + FVector(0, 0, 500),
			Point - FVector(0, 0, 500), FCollisionObjectQueryParams(ECC_WorldStatic),
			FCollisionQueryParams(SCENE_QUERY_STAT(UmbraLootTest), false, this)) || Ground.ImpactNormal.Z < 0.65f)
		{
			UE_LOG(LogUmbra, Warning, TEXT("Loot test index %d: no suitable ground; skipped."), Index);
			continue;
		}
		FUmbraItemInstance Instance;
		FString GenerationError;
		bool bCreated = true;
		const int32 DefinitionIndex = bRandomSingleItem ? SingleIndex : Index;
		if (bOneOfEachRarity) Instance = Batch[Index];
		else bCreated = Items[DefinitionIndex] && Items[DefinitionIndex]->GenerationProfile
			? UmbraItemGeneration::Generate(this, Items[DefinitionIndex], GenerationContext, int32(ItemSeeds.GetUnsignedInt() & MAX_int32), FGuid::NewGuid(), Instance, GenerationError)
			: FUmbraItemInstance::Create(this, Items[DefinitionIndex], FGuid::NewGuid(), Instance);
		if (!bCreated)
		{
			UE_LOG(LogUmbra, Warning, TEXT("Loot test index %d: generation rejected: %s"), Index, *GenerationError);
			continue;
		}
		const FTransform Transform(FRotator::ZeroRotator, Ground.ImpactPoint + FVector(0, 0, 2));
		auto* Item = GetWorld()->SpawnActorDeferred<AUmbraWorldItem>(
			WorldItemClass ? WorldItemClass.Get() : AUmbraWorldItem::StaticClass(),
			Transform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Item) continue;
		if (!Item->InitializeInstance(Instance)) { Item->Destroy(); continue; }
		Item->FinishSpawning(Transform);
		UE_LOG(LogUmbra, Display, TEXT("RandomMVP item=%s guid=%s seed=%d level=%d rarity=%d affixes=%d"),
			*Instance.DefinitionId.ToString(), *Instance.InstanceId.ToString(), Instance.GenerationSeed, Instance.GetItemLevel(), int32(Instance.GetRarity()), Instance.RolledAffixes.Num());
	}
#endif
}

bool AUmbraLootTestSpawner::GenerateRarityBatch(int32 Seed, TArray<FUmbraItemInstance>& Out) const
{
	Out.Reset();
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (!HasAuthority() || GetNetMode() == NM_Client || GenerationContext.SourceLevel < 1) return false;
	TArray<UUmbraItemDefinition*> Candidates;
	for (const auto& Entry : Items)
	{
		auto* Item = Entry.Get();
		FString Error;
		if (Item && UmbraItemGeneration::Validate(Item, Error)) Candidates.AddUnique(Item);
	}
	if (Candidates.IsEmpty()) return false;
	Candidates.Sort([](const auto& A, const auto& B) { return A.DefinitionId.LexicalLess(B.DefinitionId); });
	FRandomStream Random(Seed);
	TMap<EUmbraItemRarity, FUmbraItemInstance> Found;
	// Rejection sampling preserves source-rarity restrictions and real affix generation.
	// Bounded so a disabled/impossible rarity cannot hang BeginPlay. Never edit shared assets.
	for (int32 Attempt=0; Attempt<4096 && Found.Num()<6; ++Attempt)
	{
		auto* Definition = Candidates[Random.RandRange(0, Candidates.Num()-1)];
		FUmbraItemInstance Instance;
		FString Error;
		if (UmbraItemGeneration::Generate(this, Definition, GenerationContext,
			int32(Random.GetUnsignedInt() & MAX_int32), FGuid::NewGuid(), Instance, Error)
			&& !Found.Contains(Instance.GetRarity())) Found.Add(Instance.GetRarity(), Instance);
	}
	if (Found.Num()!=6) return false;
	for (uint8 Index=0; Index<6; ++Index) Out.Add(Found.FindChecked(EUmbraItemRarity(Index)));
	return true;
#else
	return false;
#endif
}
