#include "Items/UmbraItemGeneration.h"
#include "GameFramework/Actor.h"

namespace
{
TArray<FUmbraGenerationRarity> RarityChoices(const UUmbraItemDefinition& Item)
{
	const auto& Profile = *Item.GenerationProfile;
	auto Rows = Profile.Rarities;
	if (Profile.bMaterialRarityOnly)
	{
		// Material grade is authored on the definition; source rarity and RNG only govern equipment.
		Rows.RemoveAll([&](const auto& Row) { return Row.Rarity != Item.Rarity; });
		for (auto& Row : Rows) { Row.Weight = 1.f; Row.AffixCount = 0; }
		return Rows;
	}
	Rows.RemoveAll([&](const auto& Row)
	{
		if (Item.SourceRarity == 0) return Row.Rarity > EUmbraItemRarity::Rare;
		if (Item.SourceRarity == 1) return Row.Rarity > EUmbraItemRarity::Epic;
		if (Item.SourceRarity == 2) return Row.Rarity < EUmbraItemRarity::Epic || Row.Rarity > EUmbraItemRarity::Legendary;
		if (Item.SourceRarity == 3) return Row.Rarity < EUmbraItemRarity::Legendary;
		return Profile.bRarityFromAffixCount && Row.Rarity > EUmbraItemRarity::Epic;
	});
	Rows.Sort([](const auto& A, const auto& B) { return uint8(A.Rarity) < uint8(B.Rarity); });
	if (Profile.bRarityFromAffixCount)
		for (auto& Row : Rows)
			if (Row.Rarity <= EUmbraItemRarity::Epic)
			{
				Row.AffixCount = int32(Row.Rarity) + 1;
				Row.Weight = Profile.AffixCountWeights[int32(Row.Rarity)];
			}
	// Source 2 has its own promotion probability; the source 3 Legendary/Unique mix stays independent.
	if (Item.SourceRarity == 2)
		for (auto& Row : Rows)
			Row.Weight = Row.Rarity == EUmbraItemRarity::Legendary ? Profile.SourceTwoLegendaryChance : 1.f - Profile.SourceTwoLegendaryChance;
	return Rows;
}
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
	if (Item->DefinitionId.IsNone() || Item->ItemCategory == EUmbraItemCategory::Unknown
		|| (!Profile.bMaterialRarityOnly && Item->GetAllowedEquipmentSlots().IsEmpty()))
		return Fail(TEXT("Generated template needs DefinitionId, category and slots"));
	if (Profile.bMaterialRarityOnly && (Item->ItemCategory != EUmbraItemCategory::Material
		|| !Item->AllowedSlots.IsEmpty() || Item->bEquippingEnabled || Item->Weapon
		|| !Item->AttributeBonuses.IsEmpty() || !Item->DamageBonuses.IsEmpty()
		|| Item->PrimaryBonuses.ToArray().ContainsByPredicate([](float Value) { return Value != 0.f; })
		|| Item->Armor != 0.f || Item->MagicResistance != 0.f))
		return Fail(TEXT("Rarity-only materials must be non-equippable and have no combat bonuses"));
	if (!Item->AffixRollConfigs.IsEmpty()) return Fail(TEXT("Pool and legacy AffixRollConfigs cannot coexist"));
	if (Profile.RulesVersion < 1 || Profile.MinimumLevel < 1 || Profile.MaximumLevel < Profile.MinimumLevel
		|| (!Profile.bMaterialRarityOnly && !IsValid(Profile.Pool)) || Profile.Rarities.IsEmpty()) return Fail(TEXT("Invalid profile level/version/pool/rarities"));
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
	if (Profile.bRarityFromAffixCount)
	{
		if (Profile.AffixCountWeights.Num() != 4) return Fail(TEXT("Count-driven generation requires four weights"));
		Total = 0;
		for (int32 I = 0; I < 4; ++I)
		{
			const float Weight = Profile.AffixCountWeights[I];
			if (!FMath::IsFinite(Weight) || Weight < 0 || !Rarities.Contains(EUmbraItemRarity(I)))
				return Fail(TEXT("Invalid count weight or missing Common..Epic presentation"));
			Total += Weight;
		}
	}
	if (!FMath::IsFinite(Profile.SourceTwoLegendaryChance) || Profile.SourceTwoLegendaryChance < 0.f || Profile.SourceTwoLegendaryChance > 1.f)
		return Fail(TEXT("Source 2 Legendary chance must be in [0,1]"));
	if (Item->SourceRarity < -1 || Item->SourceRarity > 3) return Fail(TEXT("Invalid imported source rarity"));
	Total = 0;
	for (const auto& Row : RarityChoices(*Item)) Total += Row.Weight;
	if (Total <= 0) return Fail(TEXT("No positive generation weight within source rarity restrictions"));
	if (Profile.bMaterialRarityOnly) return true;
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
	const auto Rarities = RarityChoices(*Item);
	double Total = 0; for (const auto& Rarity : Rarities) Total += Rarity.Weight;
	double Choice = Random.FRand() * Total;
	const FUmbraGenerationRarity* Selected = nullptr;
	for (const auto& Rarity : Rarities) { if (Rarity.Weight > 0 && Choice < Rarity.Weight) { Selected = &Rarity; break; } Choice -= Rarity.Weight; }
	if (!Selected) { Error = TEXT("Rarity selection failed"); return false; }
	Next.Rarity = Selected->Rarity;
	if (Profile.bMaterialRarityOnly)
	{
		if (!Next.IsValid()) { Error = TEXT("Generated material payload invalid"); return false; }
		Out = MoveTemp(Next);
		return true;
	}
	struct FCandidate { const UUmbraAffixDefinition* Affix; const FUmbraAffixTier* Tier; double Weight; };
	TArray<FCandidate> Candidates;
	for (const auto& Entry : Profile.Pool->Entries)
	{
		const auto* Affix = Entry.Affix.Get();
		if (Entry.Weight <= 0 || Next.ItemLevel > Affix->MaximumLevel
			|| (!Affix->Rarities.IsEmpty() && !Affix->Rarities.Contains(Next.Rarity))
			|| (!Affix->Categories.IsEmpty() && !Affix->Categories.Contains(Item->ItemCategory))
			|| (!Affix->WeaponTypes.IsEmpty() && !Affix->WeaponTypes.Contains(Item->WeaponType))
			|| (!Affix->Slots.IsEmpty() && !Affix->Slots.ContainsByPredicate([&](auto Slot) { return Item->CanEquipInSlot(Slot); }))
			|| !Item->EquipmentTags.HasAll(Affix->RequiredEquipmentTags) || Item->EquipmentTags.HasAny(Affix->BlockedEquipmentTags)
			|| !Context.SourceTags.HasAll(Affix->RequiredSourceTags)) continue;
		const FUmbraAffixTier* Best = nullptr;
		for (const auto& Tier : Affix->Tiers)
			if (Tier.MinimumLevel <= Next.ItemLevel && (!Best || Tier.MinimumLevel > Best->MinimumLevel)) Best = &Tier;
		if (Best) Candidates.Add({Affix, Best, Entry.Weight});
	}
	Candidates.Sort([](const auto& A, const auto& B) { return A.Affix->AffixId.LexicalLess(B.Affix->AffixId); });
	auto Compatible = [](const FCandidate& A, const FCandidate& B)
	{
		return A.Affix != B.Affix && !SameTarget(A.Tier->Values, B.Tier->Values)
			&& (A.Affix->ExclusiveGroup.IsNone() || A.Affix->ExclusiveGroup != B.Affix->ExclusiveGroup);
	};
	// A greedy first pick can consume two otherwise compatible choices. Count-driven rules
	// only pick candidates that still permit completion; never silently lower rarity.
	auto CanComplete = [&](auto&& Self, const TArray<FCandidate>& Remaining, int32 Needed) -> bool
	{
		if (Needed <= 0) return true;
		if (Remaining.Num() < Needed) return false;
		for (int32 I = 0; I <= Remaining.Num() - Needed; ++I)
		{
			TArray<FCandidate> Rest;
			for (int32 J = I + 1; J < Remaining.Num(); ++J)
				if (Compatible(Remaining[I], Remaining[J])) Rest.Add(Remaining[J]);
			if (Self(Self, Rest, Needed - 1)) return true;
		}
		return false;
	};
	for (int32 Index = 0; Index < Selected->AffixCount; ++Index)
	{
		if (Profile.bRarityFromAffixCount)
		{
			const auto Available = Candidates;
			Candidates.RemoveAll([&](const FCandidate& Candidate)
			{
				TArray<FCandidate> Rest;
				for (const auto& Other : Available) if (Compatible(Candidate, Other)) Rest.Add(Other);
				return !CanComplete(CanComplete, Rest, Selected->AffixCount - Index - 1);
			});
		}
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
