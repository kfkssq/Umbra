#include "Loot/UmbraLootTable.h"
#include "Items/UmbraItemDefinition.h"
#include "GameFramework/Actor.h"
#include "Items/UmbraItemGeneration.h"
#include "Misc/DataValidation.h"
#include "Umbra.h"

namespace
{
bool ValidWeight(float Weight) { return FMath::IsFinite(Weight) && Weight >= 0; }

template<typename T, typename Less>
const T* Pick(const TArray<T>& Rows, FRandomStream& Random, Less Sort)
{
	TArray<const T*> Ordered;
	double Total = 0;
	for (const auto& Row : Rows) if (Row.Weight > 0) { Ordered.Add(&Row); Total += Row.Weight; }
	Ordered.Sort(Sort);
	const double Roll = Random.FRand() * Total;
	double Sum = 0;
	for (const auto* Row : Ordered) { Sum += Row->Weight; if (Roll < Sum) return Row; }
	return nullptr;
}
}

bool UUmbraLootTable::ValidateTypeGroups(FString& Error) const
{
	Error.Reset();
	auto Fail = [&](const TCHAR* Why) { Error = Why; return false; };
	if (!FMath::IsFinite(EquipmentDropChance) || EquipmentDropChance < 0 || EquipmentDropChance > 1
		|| MinEquipmentCount < 1 || MaxEquipmentCount < MinEquipmentCount || MaxEquipmentCount > MaximumItems)
		return Fail(TEXT("Invalid equipment chance/count"));
	TSet<EUmbraItemCategory> SeenCategories;
	TSet<FName> Definitions;
	double CategoryTotal = 0;
	for (const auto& Category : Categories)
	{
		if (!ValidWeight(Category.Weight) || SeenCategories.Contains(Category.Category)
			|| (Category.Category != EUmbraItemCategory::Weapon && Category.Category != EUmbraItemCategory::Armor
				&& Category.Category != EUmbraItemCategory::Accessory && Category.Category != EUmbraItemCategory::Material))
			return Fail(TEXT("Invalid/duplicate equipment category"));
		SeenCategories.Add(Category.Category);
		if (Category.Weight == 0) continue;
		CategoryTotal += Category.Weight;
		TSet<FName> Groups; TSet<int32> TypeKeys;
		double TypeTotal = 0;
		for (const auto& Type : Category.Types)
		{
			const int32 Key = Category.Category == EUmbraItemCategory::Weapon ? int32(Type.WeaponType) : int32(Type.ArmorSlot);
			const bool bEquipmentTypeKey = Category.Category == EUmbraItemCategory::Weapon || Category.Category == EUmbraItemCategory::Armor;
			if (!ValidWeight(Type.Weight) || Type.GroupId.IsNone() || Groups.Contains(Type.GroupId) || (bEquipmentTypeKey && TypeKeys.Contains(Key)))
				return Fail(TEXT("Invalid/duplicate type group"));
			Groups.Add(Type.GroupId); TypeKeys.Add(Key);
			if (Type.Weight == 0) continue;
			if (Category.Category == EUmbraItemCategory::Weapon && Type.WeaponType == EUmbraWeaponType::Unknown)
				return Fail(TEXT("Unknown weapon type"));
			if (Category.Category == EUmbraItemCategory::Armor && Type.ArmorSlot != EUmbraEquipmentSlot::Head
				&& Type.ArmorSlot != EUmbraEquipmentSlot::Chest && Type.ArmorSlot != EUmbraEquipmentSlot::Hands
				&& Type.ArmorSlot != EUmbraEquipmentSlot::Legs)
				return Fail(TEXT("Invalid armor slot"));
			TypeTotal += Type.Weight;
			double TemplateTotal = 0;
			for (const auto& Template : Type.Templates)
			{
				if (!ValidWeight(Template.Weight)) return Fail(TEXT("Invalid template weight"));
				if (Template.Weight == 0) continue;
				const auto* Item = Template.ItemDefinition.Get();
				if (!IsValid(Item) || Item->DefinitionId.IsNone() || Definitions.Contains(Item->DefinitionId)
					|| Item->ItemCategory != Category.Category
					|| (Category.Category != EUmbraItemCategory::Material && Item->GetAllowedEquipmentSlots().IsEmpty()))
					return Fail(TEXT("Invalid/duplicate template identity or category/slots"));
				if (Category.Category == EUmbraItemCategory::Weapon && (Item->WeaponType != Type.WeaponType || (Item->bEquippingEnabled && !Item->Weapon)))
					return Fail(TEXT("Weapon template does not match type or lacks damage profile"));
				if (!Item->bEquippingEnabled && Item->EquippingUnavailableReason.IsEmpty())
					return Fail(TEXT("Staged equipment requires a visible availability reason"));
				if (Category.Category == EUmbraItemCategory::Armor && (!Item->AllowedSlots.Contains(Type.ArmorSlot) || Item->Weapon))
					return Fail(TEXT("Armor template does not match slot"));
				if (Category.Category == EUmbraItemCategory::Accessory && (Item->Weapon
					|| Item->AllowedSlots.ContainsByPredicate([](auto Slot) {
						return Slot != EUmbraEquipmentSlot::Amulet && Slot != EUmbraEquipmentSlot::Amulet2
							&& Slot != EUmbraEquipmentSlot::Ring1 && Slot != EUmbraEquipmentSlot::Ring2;
					}))) return Fail(TEXT("Invalid accessory slots/profile"));
				if (Category.Category == EUmbraItemCategory::Material
					&& (!Item->GenerationProfile || !Item->GenerationProfile->bMaterialRarityOnly))
					return Fail(TEXT("Materials require rarity-only generation"));
				if (!UmbraItemGeneration::Validate(Item, Error)) return false;
				if (!Item->GenerationProfile->bMaterialRarityOnly && !Item->GenerationProfile->bRarityFromAffixCount)
					return Fail(TEXT("Equipment type groups require count-driven generation"));
				Definitions.Add(Item->DefinitionId); TemplateTotal += Template.Weight;
			}
			if (TemplateTotal <= 0) return Fail(TEXT("Enabled equipment type has no templates"));
		}
		if (TypeTotal <= 0) return Fail(TEXT("Enabled category has no types"));
	}
	return CategoryTotal > 0 || Fail(TEXT("No enabled equipment categories"));
}

#if WITH_EDITOR
EDataValidationResult UUmbraLootTable::IsDataValid(FDataValidationContext& Context) const
{
	const auto Base = Super::IsDataValid(Context);
	if (!bUseTypeGroups) return Base;
	FString Error;
	if (!ValidateTypeGroups(Error)) { Context.AddError(FText::FromString(Error)); return EDataValidationResult::Invalid; }
	return Base == EDataValidationResult::Invalid ? Base : EDataValidationResult::Valid;
}
#endif

void UUmbraLootTable::Select(const AActor* Authority, TArray<UUmbraItemDefinition*>& OutDefinitions) const
{
	FRandomStream Random(FMath::Rand());
	Select(Authority, Random, OutDefinitions);
}

void UUmbraLootTable::Select(const AActor* Authority, FRandomStream& Random, TArray<UUmbraItemDefinition*>& OutDefinitions) const
{
	OutDefinitions.Reset();
	if (!Authority || !Authority->HasAuthority()) return;
	if (bUseTypeGroups)
	{
		FString Error;
		if (!ValidateTypeGroups(Error)) { UE_LOG(LogUmbra, Error, TEXT("Loot table %s rejected: %s"), *GetPathName(), *Error); return; }
		if (Random.FRand() >= EquipmentDropChance) return;
		const int32 Count = Random.RandRange(MinEquipmentCount, MaxEquipmentCount);
		for (int32 I = 0; I < Count; ++I)
		{
			const auto* Category = Pick(Categories, Random, [](const auto& A, const auto& B) { return A.Category < B.Category; });
			const auto* Type = Category ? Pick(Category->Types, Random, [](const auto& A, const auto& B) { return A.GroupId.LexicalLess(B.GroupId); }) : nullptr;
			const auto* Template = Type ? Pick(Type->Templates, Random, [](const auto& A, const auto& B) { return A.ItemDefinition->DefinitionId.LexicalLess(B.ItemDefinition->DefinitionId); }) : nullptr;
			if (!Template) { OutDefinitions.Reset(); return; }
			OutDefinitions.Add(Template->ItemDefinition);
		}
		return;
	}
	auto Eligible = [](const FUmbraLootEntry& Entry)
	{
		return IsValid(Entry.ItemDefinition) && FMath::IsFinite(Entry.Weight) && Entry.Weight > 0.f
			&& FMath::IsFinite(Entry.DropChance) && Entry.DropChance >= 0.f && Entry.DropChance <= 1.f
			&& Entry.MinCount >= 0 && Entry.MaxCount >= Entry.MinCount;
	};
	double Total = FMath::IsFinite(NoDropWeight) ? FMath::Max(0.f, NoDropWeight) : 0.f;
	for (const auto& Entry : Entries) if (Eligible(Entry)) Total += Entry.Weight;
	if (Total <= 0.0) return;
	double Choice = Random.FRand() * Total;
	for (const auto& Entry : Entries)
	{
		if (!Eligible(Entry)) continue;
		if (Choice < Entry.Weight)
		{
			if (Entry.DropChance <= 0.f || (Entry.DropChance < 1.f && Random.FRand() >= Entry.DropChance)) return;
			const int32 Count = Random.RandRange(FMath::Min(Entry.MinCount, MaximumItems), FMath::Min(Entry.MaxCount, MaximumItems));
			for (int32 Index = 0; Index < Count; ++Index) OutDefinitions.Add(Entry.ItemDefinition);
			return;
		}
		Choice -= Entry.Weight;
	}
}
