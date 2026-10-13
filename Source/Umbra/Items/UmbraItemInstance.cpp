#include "Items/UmbraItemInstance.h"
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
	if (!Authority || !Authority->HasAuthority() || !::IsValid(Item) || !Id.IsValid()) return false;
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
	Out = MoveTemp(Next);
	return true;
}

bool FUmbraItemInstance::IsValid() const
{
	if (!::IsValid(Definition) || !InstanceId.IsValid()) return false;
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
