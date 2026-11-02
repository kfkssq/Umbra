#include "Items/UmbraItemGeneration.h"
#include "GameFramework/Actor.h"

namespace
{
bool SameTarget(const FUmbraAffixIdentity& A, const FUmbraAffixIdentity& B)
{
	if (A.Kind != B.Kind) return false;
	if (A.Kind == EUmbraAffixKind::Primary) return A.Primary == B.Primary;
	if (A.Kind == EUmbraAffixKind::Attribute) return A.Attribute == B.Attribute;
	auto Left = A.DamageBonus, Right = B.DamageBonus;
	Left.Magnitude = Right.Magnitude = FScalableFloat(0.f);
	Left.Types.Sort(); Right.Types.Sort();
	return FUmbraDamageBonus::StaticStruct()->CompareScriptStruct(&Left, &Right, 0);
}
bool FixedConflict(const UUmbraItemDefinition& Item, const FUmbraAffixIdentity& Identity)
{
	if (Identity.Kind == EUmbraAffixKind::Primary) return Item.PrimaryBonuses.ToArray()[int32(Identity.Primary)] != 0;
	if (Identity.Kind == EUmbraAffixKind::Damage) return Item.DamageBonuses.ContainsByPredicate([&](const auto& Bonus)
	{ FUmbraAffixIdentity Fixed; Fixed.Kind = EUmbraAffixKind::Damage; Fixed.DamageBonus = Bonus; return SameTarget(Fixed, Identity); });
	return Item.AttributeBonuses.ContainsByPredicate([&](const auto& Bonus) { return Bonus.Stat == Identity.Attribute; });
}
}

bool UmbraItemGeneration::Validate(const UUmbraItemDefinition* Item, FString& Error)
{
	Error.Reset();
	auto Fail = [&](const TCHAR* Reason) { Error = Reason; return false; };
	if (!IsValid(Item) || !IsValid(Item->GenerationProfile)) return Fail(TEXT("Missing definition/profile"));
	const auto& Profile = *Item->GenerationProfile;
	if (Item->DefinitionId.IsNone() || Item->ItemCategory == EUmbraItemCategory::Unknown || Item->AllowedSlots.IsEmpty())
		return Fail(TEXT("Generated template needs DefinitionId, category and slots"));
	if (!Item->AffixRollConfigs.IsEmpty()) return Fail(TEXT("Pool and legacy AffixRollConfigs cannot coexist"));
	if (Profile.RulesVersion < 1 || Profile.MinimumLevel < 1 || Profile.MaximumLevel < Profile.MinimumLevel
		|| !IsValid(Profile.Pool) || Profile.Rarities.IsEmpty()) return Fail(TEXT("Invalid profile level/version/pool/rarities"));
	TSet<EUmbraItemRarity> Rarities;
	double Total = 0;
	for (const auto& Rarity : Profile.Rarities)
	{
		if (uint8(Rarity.Rarity) > uint8(EUmbraItemRarity::Unique) || Rarities.Contains(Rarity.Rarity)
			|| !FMath::IsFinite(Rarity.Weight) || Rarity.Weight < 0 || Rarity.AffixCount < 0 || Rarity.AffixCount > 16
			|| !FMath::IsFinite(Rarity.Color.R) || !FMath::IsFinite(Rarity.Color.G) || !FMath::IsFinite(Rarity.Color.B)
			|| !FMath::IsFinite(Rarity.Color.A)) return Fail(TEXT("Invalid/duplicate rarity or count"));
		Rarities.Add(Rarity.Rarity); Total += Rarity.Weight;
	}
	if (Total <= 0) return Fail(TEXT("No positive rarity weight"));
	TSet<FName> Ids;
	for (const auto& Entry : Profile.Pool->Entries)
	{
		const auto* Affix = Entry.Affix.Get();
		if (!IsValid(Affix) || Affix->AffixId.IsNone() || Ids.Contains(Affix->AffixId)
			|| !FMath::IsFinite(Entry.Weight) || Entry.Weight < 0 || Affix->Tiers.IsEmpty()) return Fail(TEXT("Invalid/duplicate pool affix or weight"));
		Ids.Add(Affix->AffixId);
		TSet<int32> Levels; TSet<FName> TierIds;
		const FUmbraAffixIdentity* First = nullptr;
		for (const auto& Tier : Affix->Tiers)
		{
			auto Values = Tier.Values; Values.AffixId = Affix->AffixId;
			TArray<float> Candidates;
			if (Tier.TierId.IsNone() || TierIds.Contains(Tier.TierId) || Tier.MinimumLevel < 1
				|| Tier.MinimumLevel > Affix->MaximumLevel || Levels.Contains(Tier.MinimumLevel)
				|| !Values.BuildCandidates(Candidates))
				return Fail(TEXT("Invalid affix tier/candidates"));
			if (First && !SameTarget(*First, Values)) return Fail(TEXT("Tier targets must agree"));
			First = &Tier.Values; Levels.Add(Tier.MinimumLevel); TierIds.Add(Tier.TierId);
			if (FixedConflict(*Item, Values)) return Fail(TEXT("Pool target duplicates fixed bonus"));
		}
	}
	return true;
}

bool UmbraItemGeneration::Generate(const AActor* Authority, UUmbraItemDefinition* Item,
	const FUmbraGenerationContext& Context, int32 Seed, FGuid Id, FUmbraItemInstance& Out, FString& Error)
{
	Out = {}; Error.Reset();
	if (!Authority || !Authority->HasAuthority() || !Id.IsValid() || Context.SourceLevel < 1)
	{ Error = TEXT("Authority, identity and source level required"); return false; }
	if (!Validate(Item, Error)) return false;
	const auto& Profile = *Item->GenerationProfile;
	FRandomStream Random(Seed);
	FUmbraItemInstance Next;
	Next.Definition = Item; Next.InstanceId = Id; Next.SchemaVersion = 1; Next.DefinitionId = Item->DefinitionId;
	Next.ItemLevel = int32(FMath::Clamp(int64(Context.SourceLevel) + Profile.LevelOffset, int64(Profile.MinimumLevel), int64(Profile.MaximumLevel)));
	Next.GenerationSeed = Seed; Next.GenerationRulesVersion = Profile.RulesVersion;
	auto Rarities = Profile.Rarities;
	Rarities.Sort([](const auto& A, const auto& B) { return uint8(A.Rarity) < uint8(B.Rarity); });
	double Total = 0; for (const auto& Rarity : Rarities) Total += Rarity.Weight;
	double Choice = Random.FRand() * Total;
	const FUmbraGenerationRarity* Selected = nullptr;
	for (const auto& Rarity : Rarities) { if (Rarity.Weight > 0 && Choice < Rarity.Weight) { Selected = &Rarity; break; } Choice -= Rarity.Weight; }
	if (!Selected) { Error = TEXT("Rarity selection failed"); return false; }
	Next.Rarity = Selected->Rarity;
	struct FCandidate { const UUmbraAffixDefinition* Affix; const FUmbraAffixTier* Tier; double Weight; };
	TArray<FCandidate> Candidates;
	for (const auto& Entry : Profile.Pool->Entries)
	{
		const auto* Affix = Entry.Affix.Get();
		if (Entry.Weight <= 0 || Next.ItemLevel > Affix->MaximumLevel
			|| (!Affix->Rarities.IsEmpty() && !Affix->Rarities.Contains(Next.Rarity))
			|| (!Affix->Categories.IsEmpty() && !Affix->Categories.Contains(Item->ItemCategory))
			|| (!Affix->WeaponTypes.IsEmpty() && !Affix->WeaponTypes.Contains(Item->WeaponType))
			|| (!Affix->Slots.IsEmpty() && !Affix->Slots.ContainsByPredicate([&](auto Slot) { return Item->AllowedSlots.Contains(Slot); }))
			|| !Item->EquipmentTags.HasAll(Affix->RequiredEquipmentTags) || Item->EquipmentTags.HasAny(Affix->BlockedEquipmentTags)
			|| !Context.SourceTags.HasAll(Affix->RequiredSourceTags)) continue;
		const FUmbraAffixTier* Best = nullptr;
		for (const auto& Tier : Affix->Tiers)
			if (Tier.MinimumLevel <= Next.ItemLevel && (!Best || Tier.MinimumLevel > Best->MinimumLevel)) Best = &Tier;
		if (Best) Candidates.Add({Affix, Best, Entry.Weight});
	}
	Candidates.Sort([](const auto& A, const auto& B) { return A.Affix->AffixId.LexicalLess(B.Affix->AffixId); });
	for (int32 Index = 0; Index < Selected->AffixCount; ++Index)
	{
		Total = 0; for (const auto& Candidate : Candidates) Total += Candidate.Weight;
		if (Total <= 0) { Error = FString::Printf(TEXT("Insufficient legal affixes: %s level %d rarity %d missing %d"),
			*Item->DefinitionId.ToString(), Next.ItemLevel, int32(Next.Rarity), Selected->AffixCount - Index); return false; }
		Choice = Random.FRand() * Total;
		int32 Pick = Candidates.Num() - 1;
		for (int32 CandidateIndex = 0; CandidateIndex < Candidates.Num(); ++CandidateIndex)
		{ if (Choice < Candidates[CandidateIndex].Weight) { Pick = CandidateIndex; break; } Choice -= Candidates[CandidateIndex].Weight; }
		const auto Chosen = Candidates[Pick];
		auto Config = Chosen.Tier->Values; Config.AffixId = Chosen.Affix->AffixId;
		FUmbraRolledAffix Roll;
		static_cast<FUmbraAffixIdentity&>(Roll) = Config;
		Config.BuildCandidates(Roll.CandidateValues);
		Roll.SelectedTier = Random.RandRange(0, Roll.CandidateValues.Num() - 1);
		Roll.Value = Roll.CandidateValues[Roll.SelectedTier]; Roll.AffixTierId = Chosen.Tier->TierId;
		if (Roll.Kind == EUmbraAffixKind::Damage) Roll.DamageBonus.Magnitude = FScalableFloat(Roll.Value);
		Next.RolledAffixes.Add(Roll);
		Candidates.RemoveAll([&](const auto& Candidate) { return Candidate.Affix == Chosen.Affix
			|| SameTarget(Candidate.Tier->Values, Config)
			|| (!Chosen.Affix->ExclusiveGroup.IsNone() && Candidate.Affix->ExclusiveGroup == Chosen.Affix->ExclusiveGroup); });
	}
	if (!Next.IsValid()) { Error = TEXT("Generated payload invalid"); return false; }
	Out = MoveTemp(Next); return true;
}
