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
		if (!FUmbraItemInstance::Create(this, Items[Index], FGuid::NewGuid(), Instance))
		{
			UE_LOG(LogUmbra, Warning, TEXT("Loot test index %d: invalid definition; skipped."), Index);
			continue;
		}
		const FTransform Transform(FRotator::ZeroRotator, Ground.ImpactPoint + FVector(0, 0, 2));
		auto* Item = GetWorld()->SpawnActorDeferred<AUmbraWorldItem>(
			WorldItemClass ? WorldItemClass.Get() : AUmbraWorldItem::StaticClass(),
			Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Item) continue;
		if (!Item->InitializeInstance(Instance)) { Item->Destroy(); continue; }
		Item->FinishSpawning(Transform);
	}
#endif
}
