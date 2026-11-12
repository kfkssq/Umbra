#include "Items/UmbraItemInstance.h"
#include "Items/UmbraItemGeneration.h"
#include "Equipment/UmbraEquipmentEffect.h"
#include "GameFramework/Actor.h"
#include "Umbra.h"

namespace
{
	bool ValidAffixIdentity(const FUmbraAffixIdentity& Affix)
	{
		if (Affix.AffixId.IsNone()) return false;
		if (Affix.Kind == EUmbraAffixKind::Primary) return uint8(Affix.Primary) <= 3;
		if (Affix.Kind == EUmbraAffixKind::Attribute) return UUmbraEquipmentEffect::AffixIndex(Affix.Attribute) != INDEX_NONE;
		if (Affix.Kind != EUmbraAffixKind::Damage) return false;
		const auto& Bonus = Affix.DamageBonus;
		if (uint8(Bonus.Bucket) > 1 || uint8(Bonus.AttackSource) > 2 || Bonus.Types.Num() > 9) return false;
		TSet<EUmbraWeaponDamageType> Types;
		for (const auto Type : Bonus.Types)
		{
			if (uint8(Type) > 8 || Types.Contains(Type)) return false;
			Types.Add(Type);
		}
		return true;
	}
}

bool FUmbraAffixRollConfig::BuildCandidates(TArray<float>& Out) const
{
	Out.Reset();
	if (!ValidAffixIdentity(*this) || !FMath::IsFinite(MinValue) || !FMath::IsFinite(MaxValue)
		|| MinValue < 0.f || MaxValue < MinValue || (TierCount != 5 && TierCount != 6)) return false;
	if (!ExplicitValues.IsEmpty()) Out = ExplicitValues;
	else if (MinValue == MaxValue) Out.Add(MinValue);
	else for (int32 Index = 0; Index < TierCount; ++Index)
	{
		const double Value = FMath::Lerp(double(MinValue), double(MaxValue), double(Index) / (TierCount - 1));
		Out.Add(float(bIntegerValues ? FMath::RoundToDouble(Value) : Value));
	}
	for (int32 Index = 0; Index < Out.Num(); ++Index)
	{
		const float Value = Out[Index];
		if (!FMath::IsFinite(Value) || Value < MinValue || Value > MaxValue
			|| (bIntegerValues && FMath::RoundToDouble(double(Value)) != double(Value))
			|| (Index > 0 && Value <= Out[Index - 1])) { Out.Reset(); return false; }
	}
	return !Out.IsEmpty();
}

bool FUmbraItemInstance::Create(const AActor* Authority, UUmbraItemDefinition* Item, FGuid Id, FUmbraItemInstance& Out)
{
	Out = FUmbraItemInstance();
	if (!Authority || !Authority->HasAuthority() || !::IsValid(Item) || !Id.IsValid() || Item->GenerationProfile) return false;
	FUmbraItemInstance Next;
	Next.Definition = Item;
	Next.InstanceId = Id;
	TSet<FName> Seen;
	for (const auto& Config : Item->AffixRollConfigs)
	{
		FUmbraRolledAffix Roll;
		if (Seen.Contains(Config.AffixId) || !Config.BuildCandidates(Roll.CandidateValues))
		{
			UE_LOG(LogUmbra, Error, TEXT("Item %s: invalid or duplicate affix %s; creation rejected."), *GetNameSafe(Item), *Config.AffixId.ToString());
			return false;
		}
		Seen.Add(Config.AffixId);
		static_cast<FUmbraAffixIdentity&>(Roll) = Config;
		Roll.SelectedTier = FMath::RandRange(0, Roll.CandidateValues.Num() - 1);
		Roll.Value = Roll.CandidateValues[Roll.SelectedTier];
		if (Roll.Kind == EUmbraAffixKind::Damage) Roll.DamageBonus.Magnitude = FScalableFloat(Roll.Value);
		Next.RolledAffixes.Add(MoveTemp(Roll));
	}
	if (!Next.UpgradeLegacy()) return false;
	Out = MoveTemp(Next);
	return true;
}

bool FUmbraItemInstance::IsValid() const
{
	if (!::IsValid(Definition) || !InstanceId.IsValid()) return false;
	if (Quantity < 1 || (Quantity != 1 && !CanStack())) return false;
	if (UpgradeLevel < 0 || SchemaVersion < 0 || SchemaVersion > 1) return false;
	if (Definition->GenerationProfile && (SchemaVersion != 1 || GenerationRulesVersion < 1)) return false;
	if (SchemaVersion == 1 && (DefinitionId.IsNone() || ItemLevel < 1 || uint8(Rarity) > uint8(EUmbraItemRarity::Unique)
		|| GenerationRulesVersion < 0)) return false;
	TSet<FName> Seen;
	for (const auto& Roll : RolledAffixes)
	{
		if (!ValidAffixIdentity(Roll) || Seen.Contains(Roll.AffixId) || !Roll.CandidateValues.IsValidIndex(Roll.SelectedTier)
			|| !FMath::IsFinite(Roll.Value) || Roll.Value < 0.f || Roll.Value != Roll.CandidateValues[Roll.SelectedTier]) return false;
		Seen.Add(Roll.AffixId);
		for (int32 Index = 0; Index < Roll.CandidateValues.Num(); ++Index)
			if (!FMath::IsFinite(Roll.CandidateValues[Index]) || Roll.CandidateValues[Index] < 0.f
				|| (Index && Roll.CandidateValues[Index] <= Roll.CandidateValues[Index - 1])) return false;
	}
	return true;
}

bool FUmbraItemInstance::CanStack() const
{
	return ::IsValid(Definition) && Definition->bStackableMaterial
		&& Definition->ItemCategory == EUmbraItemCategory::Material && !Definition->bEquippingEnabled
		&& Definition->AllowedSlots.IsEmpty() && !Definition->Weapon && RolledAffixes.IsEmpty() && UpgradeLevel == 0;
}

bool FUmbraItemInstance::CanStackWith(const FUmbraItemInstance& Other) const
{
	// Material level and generation seed have no effect on its use; the receiving stack keeps its metadata.
	return CanStack() && Other.CanStack() && Definition == Other.Definition && GetRarity() == Other.GetRarity();
}

bool FUmbraItemInstance::UpgradeLegacy()
{
	if (!IsValid()) return false;
	if (SchemaVersion == 1) return true;
	if (Definition->GenerationProfile || Definition->ItemLevel < 1 || uint8(Definition->Rarity) > uint8(EUmbraItemRarity::Unique)) return false;
	DefinitionId = Definition->DefinitionId.IsNone() ? FName(*Definition->GetPathName()) : Definition->DefinitionId;
	ItemLevel = Definition->ItemLevel; Rarity = Definition->Rarity;
	SchemaVersion = 1;
	return true;
}

FText FUmbraItemInstance::GetDisplayName() const
{
	if (!Definition) return FText::GetEmpty();
	const FText Name = Definition->DisplayName.IsEmpty() ? FText::FromName(Definition->GetFName()) : Definition->DisplayName;
	return UpgradeLevel > 0 ? FText::Format(NSLOCTEXT("UmbraItem", "UpgradedName", "{0} +{1}"), Name, FText::AsNumber(UpgradeLevel)) : Name;
}

int32 FUmbraItemInstance::GetItemLevel() const { return SchemaVersion == 0 && Definition ? Definition->ItemLevel : ItemLevel; }
EUmbraItemRarity FUmbraItemInstance::GetRarity() const { return SchemaVersion == 0 && Definition ? Definition->Rarity : Rarity; }
FLinearColor FUmbraItemInstance::GetRarityColor() const
{
	if (!Definition) return FLinearColor::White;
	if (GenerationRulesVersion > 0 && Definition->GenerationProfile)
		for (const auto& Row : Definition->GenerationProfile->Rarities) if (Row.Rarity == GetRarity()) return Row.Color;
	return Definition->RarityColor;
}

bool FUmbraItemInstanceRecord::Capture(FUmbraItemInstance Value, FUmbraItemInstanceRecord& Out)
{
	if (!Value.UpgradeLegacy()) return false;
	Out.SchemaVersion = Value.SchemaVersion; Out.DefinitionId = Value.DefinitionId; Out.InstanceId = Value.InstanceId;
	Out.UpgradeLevel = Value.UpgradeLevel;
	Out.Quantity = Value.Quantity;
	Out.ItemLevel = Value.ItemLevel; Out.Rarity = Value.Rarity; Out.GenerationSeed = Value.GenerationSeed;
	Out.GenerationRulesVersion = Value.GenerationRulesVersion; Out.RolledAffixes = Value.RolledAffixes;
	// Rolled damage stores a scalar; never persist its authoring curve reference.
	for (auto& Roll : Out.RolledAffixes) if (Roll.Kind == EUmbraAffixKind::Damage) Roll.DamageBonus.Magnitude = FScalableFloat(Roll.Value);
	return true;
}

bool FUmbraItemInstanceRecord::Restore(UUmbraItemDefinition* Resolved, FUmbraItemInstance& Out) const
{
	Out = {};
	if (!::IsValid(Resolved) || SchemaVersion != 1) return false;
	const FName Expected = Resolved->DefinitionId.IsNone() ? FName(*Resolved->GetPathName()) : Resolved->DefinitionId;
	if (Expected != DefinitionId) return false;
	FUmbraItemInstance Next;
	Next.Definition = Resolved; Next.SchemaVersion = SchemaVersion; Next.DefinitionId = DefinitionId;
	Next.UpgradeLevel = UpgradeLevel;
	Next.Quantity = Quantity;
	Next.InstanceId = InstanceId; Next.ItemLevel = ItemLevel; Next.Rarity = Rarity;
	Next.GenerationSeed = GenerationSeed; Next.GenerationRulesVersion = GenerationRulesVersion; Next.RolledAffixes = RolledAffixes;
	if (!Next.IsValid()) return false;
	Out = MoveTemp(Next); return true;
}
