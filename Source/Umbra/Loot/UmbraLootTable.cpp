#include "Loot/UmbraLootTable.h"
#include "Items/UmbraItemDefinition.h"
#include "GameFramework/Actor.h"

void UUmbraLootTable::Select(const AActor* Authority, TArray<UUmbraItemDefinition*>& OutDefinitions) const
{
	OutDefinitions.Reset();
	if (!Authority || !Authority->HasAuthority()) return;
	auto Eligible = [](const FUmbraLootEntry& Entry)
	{
		return IsValid(Entry.ItemDefinition) && FMath::IsFinite(Entry.Weight) && Entry.Weight > 0.f
			&& FMath::IsFinite(Entry.DropChance) && Entry.DropChance >= 0.f && Entry.DropChance <= 1.f
			&& Entry.MinCount >= 0 && Entry.MaxCount >= Entry.MinCount;
	};
	double Total = FMath::IsFinite(NoDropWeight) ? FMath::Max(0.f, NoDropWeight) : 0.f;
	for (const auto& Entry : Entries) if (Eligible(Entry)) Total += Entry.Weight;
	if (Total <= 0.0) return;
	double Choice = FMath::FRand() * Total;
	for (const auto& Entry : Entries)
	{
		if (!Eligible(Entry)) continue;
		if (Choice < Entry.Weight)
		{
			if (Entry.DropChance <= 0.f || (Entry.DropChance < 1.f && FMath::FRand() >= Entry.DropChance)) return;
			const int32 Count = FMath::RandRange(FMath::Min(Entry.MinCount, MaximumItems), FMath::Min(Entry.MaxCount, MaximumItems));
			for (int32 Index = 0; Index < Count; ++Index) OutDefinitions.Add(Entry.ItemDefinition);
			return;
		}
		Choice -= Entry.Weight;
	}
}
