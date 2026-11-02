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
	if (!HasAuthority() || bSpawned) return;
	bSpawned = true;
	FRandomStream ItemSeeds(GenerationSeed);
	// Test fixture: 100 cm spacing, static ground within 500 cm, 2 cm clearance.
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		const FVector Point = GetActorLocation() + GetActorRightVector() * (Index * 100.f);
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
		const bool bCreated = Items[Index] && Items[Index]->GenerationProfile
			? UmbraItemGeneration::Generate(this, Items[Index], GenerationContext, int32(ItemSeeds.GetUnsignedInt() & MAX_int32), FGuid::NewGuid(), Instance, GenerationError)
			: FUmbraItemInstance::Create(this, Items[Index], FGuid::NewGuid(), Instance);
		if (!bCreated)
		{
			UE_LOG(LogUmbra, Warning, TEXT("Loot test index %d: generation rejected: %s"), Index, *GenerationError);
			continue;
		}
		const FTransform Transform(FRotator::ZeroRotator, Ground.ImpactPoint + FVector(0, 0, 2));
		auto* Item = GetWorld()->SpawnActorDeferred<AUmbraWorldItem>(
			WorldItemClass ? WorldItemClass.Get() : AUmbraWorldItem::StaticClass(),
			Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Item) continue;
		if (!Item->InitializeInstance(Instance)) { Item->Destroy(); continue; }
		Item->FinishSpawning(Transform);
		UE_LOG(LogUmbra, Display, TEXT("RandomMVP item=%s guid=%s seed=%d level=%d rarity=%d affixes=%d"),
			*Instance.DefinitionId.ToString(), *Instance.InstanceId.ToString(), Instance.GenerationSeed, Instance.GetItemLevel(), int32(Instance.GetRarity()), Instance.RolledAffixes.Num());
	}
#endif
}
