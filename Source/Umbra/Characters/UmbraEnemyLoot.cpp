#include "Characters/UmbraEnemyCharacter.h"
#include "Loot/UmbraLootTable.h"
#include "Loot/UmbraWorldItem.h"
#include "NavigationSystem.h"
#include "Engine/World.h"
#include "Umbra.h"

void AUmbraEnemyCharacter::SpawnDeathLoot()
{
	if (!HasAuthority() || !bEnableLoot || !IsValid(LootTable)) return;
	TArray<UUmbraItemDefinition*> Definitions;
	LootTable->Select(this, Definitions);
	if (Definitions.IsEmpty()) return;
	auto* Navigation = UNavigationSystemV1::GetCurrent(GetWorld());
	FNavLocation Origin;
	if (!Navigation || !Navigation->ProjectPointToNavigation(GetActorLocation(), Origin))
	{
		UE_LOG(LogUmbra, Warning, TEXT("Loot skipped for %s: no navigation ground."), *GetName());
		return;
	}
	const float Radius = FMath::IsFinite(LootScatterRadius) ? FMath::Clamp(LootScatterRadius, 0.f, 1000.f) : 140.f;
	const float Height = FMath::IsFinite(LootGroundOffset) ? FMath::Clamp(LootGroundOffset, 0.f, 100.f) : 20.f;
	TArray<FVector> UsedLocations;
	for (auto* Definition : Definitions)
	{
		FVector Location;
		bool bFound = false;
		for (int32 Attempt = 0; Attempt < 12 && !bFound; ++Attempt)
		{
			FNavLocation Candidate;
			if (!Navigation->GetRandomReachablePointInRadius(Origin.Location, Radius, Candidate)) continue;
			if (UsedLocations.ContainsByPredicate([&](const FVector& Used) { return FVector::DistSquared2D(Used, Candidate.Location) < FMath::Square(48.f); })) continue;
			FCollisionQueryParams Query(SCENE_QUERY_STAT(UmbraLootGround), false, this);
			FHitResult Ground;
			if (!GetWorld()->LineTraceSingleByObjectType(Ground, Candidate.Location + FVector(0,0,100),
				Candidate.Location - FVector(0,0,200), FCollisionObjectQueryParams(ECC_WorldStatic), Query)
				|| Ground.ImpactNormal.Z < 0.65f || FMath::Abs(Ground.ImpactPoint.Z - Candidate.Location.Z) > 50.f) continue;
			if (GetWorld()->OverlapAnyTestByObjectType(Ground.ImpactPoint + FVector(0,0,30), FQuat::Identity,
				FCollisionObjectQueryParams(ECC_WorldStatic), FCollisionShape::MakeSphere(18.f), Query)) continue;
			Location = Ground.ImpactPoint + FVector(0,0,Height);
			bFound = true;
		}
		if (!bFound)
		{
			UE_LOG(LogUmbra, Warning, TEXT("Loot position unavailable for %s; item skipped."), *GetName());
			continue;
		}
		FUmbraItemInstance Instance;
		if (!FUmbraItemInstance::Create(this, Definition, FGuid::NewGuid(), Instance)) continue;
		const FTransform Transform(FRotator::ZeroRotator, Location);
		auto* Item = GetWorld()->SpawnActorDeferred<AUmbraWorldItem>(WorldItemClass ? WorldItemClass.Get() : AUmbraWorldItem::StaticClass(),
			Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Item || !Item->InitializeInstance(Instance))
		{
			if (Item) Item->Destroy();
			UE_LOG(LogUmbra, Error, TEXT("World item spawn failed for %s."), *GetNameSafe(Definition));
			continue;
		}
		Item->FinishSpawning(Transform);
		UsedLocations.Add(Location);
	}
}
