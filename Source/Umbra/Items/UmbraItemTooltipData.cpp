#include "Items/UmbraItemTooltipData.h"
#include "Equipment/UmbraEquipmentEffect.h"
#include "UI/Combat/UmbraCombatStatData.h"
#include "UI/Equipment/UmbraEquipmentTypes.h"
#include "UI/Inventory/UmbraInventoryMenu.h"
#include "Umbra.h"

#define LOCTEXT_NAMESPACE "UmbraItemTooltip"

bool UUmbraTooltipSettings::IsValidThresholds() const
{
	return FMath::IsFinite(S) && FMath::IsFinite(A) && FMath::IsFinite(B) && FMath::IsFinite(C)
		&& S <= MAX_flt && float(S) > float(A) && float(A) > float(B) && float(B) > float(C) && float(C) > 0.f;
}

EUmbraScalingGrade UUmbraTooltipSettings::GradeFor(double Coefficient) const
{
	if (!IsValidThresholds() || !FMath::IsFinite(Coefficient) || Coefficient <= 0.) return EUmbraScalingGrade::None;
	// Authored coefficients are floats; compare against float-representable thresholds at boundaries.
	return float(Coefficient) >= float(S) ? EUmbraScalingGrade::S
		: float(Coefficient) >= float(A) ? EUmbraScalingGrade::A
		: float(Coefficient) >= float(B) ? EUmbraScalingGrade::B
		: float(Coefficient) >= float(C) ? EUmbraScalingGrade::C : EUmbraScalingGrade::D;
}

FText UmbraTooltipFormatting::DamageName(EUmbraWeaponDamageType Type)
{
	switch (Type)
	{
	case EUmbraWeaponDamageType::Slashing: return LOCTEXT("Slashing", "斩击");
	case EUmbraWeaponDamageType::Blunt: return LOCTEXT("Blunt", "打击");
	case EUmbraWeaponDamageType::Piercing: return LOCTEXT("Piercing", "穿刺");
	case EUmbraWeaponDamageType::Fire: return LOCTEXT("Fire", "火焰");
	case EUmbraWeaponDamageType::Lightning: return LOCTEXT("Lightning", "闪电");
	case EUmbraWeaponDamageType::Cold: return LOCTEXT("Cold", "寒冷");
	case EUmbraWeaponDamageType::Radiant: return LOCTEXT("Radiant", "神圣");
	case EUmbraWeaponDamageType::Poison: return LOCTEXT("Poison", "毒素");
	case EUmbraWeaponDamageType::Shadow: return LOCTEXT("Shadow", "暗影");
	default: return FText::GetEmpty();
	}
}

FText UmbraTooltipFormatting::StatName(FName Id)
{
	if (Id == TEXT("Strength")) return LOCTEXT("Strength", "力量");
	if (Id == TEXT("Dexterity")) return LOCTEXT("Dexterity", "敏捷");
	if (Id == TEXT("Intelligence")) return LOCTEXT("Intelligence", "智力");
	if (Id == TEXT("Faith")) return LOCTEXT("Faith", "信仰");
	if (Id == TEXT("MaxHealth")) return LOCTEXT("MaxHealth", "最大生命");
	if (Id == TEXT("MaxResource")) return LOCTEXT("MaxResource", "最大资源");
	if (Id == TEXT("AttackSpeed")) return LOCTEXT("AttackSpeed", "攻击速度");
	if (Id == TEXT("CriticalChance")) return LOCTEXT("CriticalChance", "暴击率");
	if (Id == TEXT("Armor")) return LOCTEXT("Armor", "护甲");
	if (Id == TEXT("MagicResistance")) return LOCTEXT("MagicResistance", "魔法抗性");
	if (Id == TEXT("MoveSpeed")) return LOCTEXT("MoveSpeed", "移动速度");
	for (uint8 Index = 0; Index < 9; ++Index)
	{
		const FString Name = StaticEnum<EUmbraWeaponDamageType>()->GetNameStringByValue(Index) + TEXT("Resistance");
		if (Id == FName(*Name)) return FText::Format(LOCTEXT("TypeResistance", "{0}抗性"), DamageName(EUmbraWeaponDamageType(Index)));
	}
	return FText::FromName(Id);
}

FText UmbraTooltipFormatting::StatValue(const FUmbraTooltipStatLine& Line)
{
	const FText Number = UmbraCombatStats::FormatNumber(Line.RawValue, Line.Unit == EUmbraTooltipUnit::Percent, 0, 2);
	const FText Signed = Line.RawValue >= 0. ? FText::Format(LOCTEXT("Plus", "+{0}"), Number) : Number;
	const FText Unit = Line.Unit == EUmbraTooltipUnit::CentimetersPerSecond ? LOCTEXT("SpeedUnit", " cm/s")
		: Line.Unit == EUmbraTooltipUnit::Rating ? LOCTEXT("RatingUnit", " 评分") : FText::GetEmpty();
	return FText::Format(LOCTEXT("StatValue", "{0}{1}"), Signed, Unit);
}

FText UmbraTooltipFormatting::Format(const FUmbraTooltipStatLine& Line)
{
	const FText Number = UmbraCombatStats::FormatNumber(Line.RawValue, Line.Unit == EUmbraTooltipUnit::Percent, 0, 2);
	if (!Line.bDamageBonus)
	{
		return FText::Format(LOCTEXT("StatLine", "{0} {1}"), StatValue(Line), StatName(Line.StatId));
	}
	const auto& Bonus = Line.DamageBonus;
	TArray<FString> Conditions;
	if (Bonus.AttackSource == EUmbraBonusAttackSource::BasicAttack) Conditions.Add(LOCTEXT("BasicOnly", "仅普通攻击").ToString());
	if (Bonus.AttackSource == EUmbraBonusAttackSource::Skill) Conditions.Add(LOCTEXT("SkillOnly", "仅技能").ToString());
	if (Bonus.bRequiresCritical) Conditions.Add(LOCTEXT("CriticalOnly", "仅暴击时").ToString());
	if (Bonus.bRequiresVulnerable) Conditions.Add(LOCTEXT("VulnerableOnly", "仅目标易伤时").ToString());
	if (!Bonus.SourceRequirements.IsEmpty())
		Conditions.Add(FText::Format(LOCTEXT("SourceTags", "来源标签条件（高级）：{0}"), FText::FromString(Bonus.SourceRequirements.ToString())).ToString());
	if (!Bonus.TargetRequirements.IsEmpty())
		Conditions.Add(FText::Format(LOCTEXT("TargetTags", "目标标签条件（高级）：{0}"), FText::FromString(Bonus.TargetRequirements.ToString())).ToString());
	TArray<FString> Types;
	for (const auto Type : Bonus.Types) Types.Add(DamageName(Type).ToString());
	const FText TypeText = Types.IsEmpty() ? LOCTEXT("AllDamage", "所有类型伤害")
		: FText::Format(LOCTEXT("Types", "{0}伤害"), FText::FromString(FString::Join(Types, TEXT(" / "))));
	const FText Value = Bonus.Bucket == EUmbraDamageBucket::Multiplicative
		? FText::Format(LOCTEXT("XBonus", "×{0} {1}"), Number, TypeText)
		: FText::Format(LOCTEXT("ABonus", "+{0} {1}"), Number, TypeText);
	return Conditions.IsEmpty() ? Value : FText::Format(LOCTEXT("ConditionalBonus", "{0}；{1}"), Value,
		FText::FromString(FString::Join(Conditions, TEXT("；"))));
}

namespace
{
	FUmbraItemTooltipData Failure(EUmbraTooltipResult Result)
	{
		FUmbraItemTooltipData Out;
		Out.Result = Result;
		return Out;
	}
	bool Nonnegative(float Value) { return FMath::IsFinite(Value) && Value >= 0.f; }
	bool ValidSlot(EUmbraEquipmentSlot Slot) { return uint8(Slot) <= uint8(EUmbraEquipmentSlot::OffHand); }
	void AddStat(FUmbraItemTooltipData& Out, FName Id, double Value, EUmbraTooltipStatSource Source, EUmbraTooltipUnit Unit)
	{
		if (Value == 0.) return;
		auto& Line = Out.Stats.AddDefaulted_GetRef();
		Line.StatId = Id;
		Line.RawValue = Value;
		Line.Source = Source;
		Line.Unit = Unit;
		Line.Text = UmbraTooltipFormatting::Format(Line);
	}
	bool ReadWeapon(const UUmbraWeaponProfile* Weapon, FUmbraItemTooltipData& Out)
	{
		if (!Weapon) return true;
		if (!IsValid(Weapon)) return false;
		const FUmbraScalingGradeOverride Overrides[] = {Weapon->StrengthGrade, Weapon->DexterityGrade, Weapon->IntelligenceGrade, Weapon->FaithGrade};
		Out.Scaling.SetNum(4);
		TSet<EUmbraWeaponDamageType> Types;
		for (const auto& Channel : Weapon->Damage.Channels)
		{
			if (uint8(Channel.Type) > 8 || Types.Contains(Channel.Type) || !Nonnegative(Channel.BaseDamage)) return false;
			Types.Add(Channel.Type);
			TSet<EUmbraPrimaryAttribute> Primaries;
			for (const auto& Term : Channel.Scaling)
			{
				if (uint8(Term.Attribute) > 3 || Primaries.Contains(Term.Attribute) || !Nonnegative(Term.Coefficient)) return false;
				Primaries.Add(Term.Attribute);
				auto& Scaling = Out.Scaling[uint8(Term.Attribute)];
				Scaling.ReferenceCoefficient += double(Channel.BaseDamage) * Term.Coefficient;
				Scaling.bHasCurve |= Term.PointCurve != nullptr;
			}
			if (Channel.BaseDamage == 0.f) continue;
			auto& Damage = Out.DamageChannels.AddDefaulted_GetRef();
			Damage.Type = Channel.Type;
			Damage.Name = UmbraTooltipFormatting::DamageName(Channel.Type);
			Damage.BaseDamage = Channel.BaseDamage;
			Out.TotalBaseDamage += Channel.BaseDamage;
		}
		if (Out.TotalBaseDamage > MAX_flt) return false;
		Out.DamageChannels.Sort([](const auto& L, const auto& R) { return uint8(L.Type) < uint8(R.Type); });
		for (int32 Index = 0; Index < 4; ++Index)
		{
			if (uint8(Overrides[Index].Grade) > uint8(EUmbraScalingGrade::S)) return false;
			auto& Scaling = Out.Scaling[Index];
			Scaling.Attribute = EUmbraPrimaryAttribute(Index);
			Scaling.ReferenceCoefficient = Out.TotalBaseDamage > 0. ? Scaling.ReferenceCoefficient / Out.TotalBaseDamage : 0.;
			Scaling.bManual = Overrides[Index].bOverride;
			Scaling.Grade = Scaling.bManual ? Overrides[Index].Grade : GetDefault<UUmbraTooltipSettings>()->GradeFor(Scaling.ReferenceCoefficient);
		}
		return true;
	}
	bool SetContext(FUmbraItemTooltipData& Out, const UUmbraEquipmentComponent* Equipment)
	{
		if (!Equipment) return true;
		if (!IsValid(Equipment) || !Equipment->QueryRequirements(Out.ItemDefinition, Out.bHasTargetSlot, Out.TargetSlot, Out.Requirements)) return false;
		Out.bPrimaryStatusKnown = true;
		Out.bRequirementsMet = Out.Requirements.bPrimariesMet;
		Out.RequirementState = !Out.Requirements.bLevelMet ? EUmbraTooltipRequirementState::LevelTooLow
			: !Out.bRequirementsMet ? EUmbraTooltipRequirementState::PrimaryPenalty : EUmbraTooltipRequirementState::Met;
		Out.bHasEquipmentPenalty = !Out.bRequirementsMet && (Out.bEquipped || Out.Requirements.bLevelMet);
		if (Out.bHasEquipmentPenalty)
			Out.PenaltyText = FText::Format(LOCTEXT("Penalty", "主属性不足：武器基础伤害×{0}，武器缩放×{1}，固有护甲/魔抗×{2}；固定词缀不受惩罚。"),
				UmbraCombatStats::FormatNumber(Out.Requirements.WeaponBaseMultiplier, false, 0, 3),
				UmbraCombatStats::FormatNumber(Out.Requirements.WeaponScalingMultiplier, false, 0, 3),
				UmbraCombatStats::FormatNumber(Out.Requirements.DefenseMultiplier, false, 0, 3));
		return true;
	}
}

FUmbraItemTooltipData UUmbraItemTooltipDataBuilder::FromDefinition(UUmbraItemDefinition* Definition)
{
	if (!IsValid(Definition) || Definition->ItemLevel < 1 || Definition->RequiredLevel < 1
		|| uint8(Definition->Rarity) > uint8(EUmbraItemRarity::Unique)
		|| !Nonnegative(Definition->Armor) || !Nonnegative(Definition->MagicResistance) || !Nonnegative(Definition->Weight)
		|| !UUmbraEquipmentEffect::ValidateBonuses(Definition)) return Failure(EUmbraTooltipResult::InvalidDefinition);
	for (float Value : Definition->Requirements.ToArray()) if (!Nonnegative(Value)) return Failure(EUmbraTooltipResult::InvalidDefinition);
	for (float Value : Definition->PrimaryBonuses.ToArray()) if (!Nonnegative(Value)) return Failure(EUmbraTooltipResult::InvalidDefinition);
	for (auto Slot : Definition->AllowedSlots)
		if (!ValidSlot(Slot) || (Definition->Weapon && Slot != EUmbraEquipmentSlot::MainHand)) return Failure(EUmbraTooltipResult::InvalidDefinition);
	if (!GetDefault<UUmbraTooltipSettings>()->IsValidThresholds()) return Failure(EUmbraTooltipResult::InvalidConfiguration);
	FUmbraItemTooltipData Out;
	Out.ItemDefinition = Definition;
	const auto Display = FUmbraEquipmentItemDisplay::FromDefinition(Definition, FGuid());
	Out.DisplayName = Display.DisplayName;
	Out.Icon = Definition->Icon;
	Out.ItemCategory = Definition->ItemCategory;
	Out.WeaponType = Definition->WeaponType;
	Out.ItemType = UmbraItemClassification::Describe(*Definition, Out.ClassificationWarnings);
	for (const FText& Warning : Out.ClassificationWarnings)
		UE_LOG(LogUmbra, Warning, TEXT("Item classification %s: %s"), *Definition->GetPathName(), *Warning.ToString());
	Out.Rarity = Definition->Rarity;
	Out.ItemLevel = Definition->ItemLevel;
	Out.RarityColor = Definition->RarityColor;
	Out.TooltipTitleBackgroundTexture = Definition->TooltipTitleBackgroundTexture;
	Out.TooltipIconBackgroundTexture = Definition->TooltipIconBackgroundTexture;
	Out.RequiredLevel = Definition->RequiredLevel;
	Out.RequiredPrimaries = Definition->Requirements;
	Out.Weight = Definition->Weight;
	Out.FlavorText = Definition->FlavorText;
	if (!ReadWeapon(Definition->Weapon, Out)) return Failure(EUmbraTooltipResult::InvalidDefinition);
	AddStat(Out, TEXT("Armor"), Definition->Armor, EUmbraTooltipStatSource::Intrinsic, EUmbraTooltipUnit::Points);
	AddStat(Out, TEXT("MagicResistance"), Definition->MagicResistance, EUmbraTooltipStatSource::Intrinsic, EUmbraTooltipUnit::Points);
	const auto Primaries = Definition->PrimaryBonuses.ToArray();
	for (int32 Index = 0; Index < 4; ++Index)
		AddStat(Out, FName(*StaticEnum<EUmbraPrimaryAttribute>()->GetNameStringByValue(Index)), Primaries[Index], EUmbraTooltipStatSource::FixedAffix, EUmbraTooltipUnit::Points);
	for (const auto& Affix : Definition->AttributeBonuses)
	{
		const auto Unit = Affix.Stat == EUmbraEquipmentAffixStat::AttackSpeed || Affix.Stat == EUmbraEquipmentAffixStat::CriticalChance ? EUmbraTooltipUnit::Percent
			: Affix.Stat == EUmbraEquipmentAffixStat::MoveSpeed ? EUmbraTooltipUnit::CentimetersPerSecond
			: Affix.Stat >= EUmbraEquipmentAffixStat::SlashingResistance ? EUmbraTooltipUnit::Rating : EUmbraTooltipUnit::Points;
		AddStat(Out, FName(*StaticEnum<EUmbraEquipmentAffixStat>()->GetNameStringByValue(int64(Affix.Stat))), Affix.Magnitude, EUmbraTooltipStatSource::FixedAffix, Unit);
	}
	for (const auto& Bonus : Definition->DamageBonuses)
	{
		auto& Line = Out.Stats.AddDefaulted_GetRef();
		Line.StatId = TEXT("DamageBonus");
		Line.bDamageBonus = true;
		Line.DamageBonus = Bonus;
		Line.RawValue = Bonus.Magnitude.GetValueAtLevel(1.f);
		Line.Unit = Bonus.Bucket == EUmbraDamageBucket::Additive ? EUmbraTooltipUnit::Percent : EUmbraTooltipUnit::Factor;
		Line.Text = UmbraTooltipFormatting::Format(Line);
	}
	Out.bValid = true;
	Out.Result = EUmbraTooltipResult::Success;
	UmbraTooltipFormatting::BuildDisplayText(Out);
	return Out;
}

FUmbraItemTooltipData UUmbraItemTooltipDataBuilder::FromInventory(const FUmbraInventorySnapshot& Snapshot, int32 SlotIndex,
	FGuid ExpectedId, UUmbraItemDefinition* ExpectedDefinition, const UUmbraEquipmentComponent* Equipment,
	bool bHasTargetSlot, EUmbraEquipmentSlot TargetSlot)
{
	if (UUmbraInventoryMenu::ValidateSnapshot(Snapshot) != EUmbraInventoryViewState::Ready) return Failure(EUmbraTooltipResult::InvalidSnapshot);
	if (SlotIndex < 0 || SlotIndex >= Snapshot.Capacity) return Failure(EUmbraTooltipResult::InvalidSlot);
	const auto* Entry = Snapshot.Items.FindByPredicate([SlotIndex](const auto& Item) { return Item.SlotIndex == SlotIndex; });
	if (!ExpectedId.IsValid() || !Entry || Entry->InstanceId != ExpectedId || Entry->Definition != ExpectedDefinition) return Failure(EUmbraTooltipResult::IdentityMismatch);
	auto Out = FromDefinition(Entry->Definition);
	if (!Out.bValid) return Out;
	if (bHasTargetSlot && (!ValidSlot(TargetSlot) || !Entry->Definition->AllowedSlots.Contains(TargetSlot))) return Failure(EUmbraTooltipResult::InvalidSlot);
	Out.Source = EUmbraTooltipSource::Inventory;
	Out.InstanceId = Entry->InstanceId;
	Out.InventorySlot = SlotIndex;
	Out.bHasTargetSlot = bHasTargetSlot;
	Out.TargetSlot = TargetSlot;
	if (Equipment)
	{
		// Reject a stale inventory identity that has already moved into equipment.
		for (const auto& Worn : Equipment->GetSnapshot().Items)
			if (Worn.InstanceId == ExpectedId) return Failure(EUmbraTooltipResult::IdentityMismatch);
	}
	if (!SetContext(Out, Equipment)) return Failure(EUmbraTooltipResult::ContextUnavailable);
	UmbraTooltipFormatting::BuildDisplayText(Out);
	return Out;
}

FUmbraItemTooltipData UUmbraItemTooltipDataBuilder::FromEquipment(const FUmbraEquipmentSnapshot& Snapshot, EUmbraEquipmentSlot Slot,
	FGuid ExpectedId, UUmbraItemDefinition* ExpectedDefinition, const UUmbraEquipmentComponent* Equipment)
{
	if (!Snapshot.bValid || Snapshot.Revision < 0 || !Nonnegative(Snapshot.EquipLoad)
		|| !Nonnegative(Snapshot.MaxEquipLoad) || uint8(Snapshot.LoadState) > uint8(EUmbraLoadState::Overweight)) return Failure(EUmbraTooltipResult::InvalidSnapshot);
	TSet<FGuid> Ids;
	TSet<EUmbraEquipmentSlot> Slots;
	for (const auto& Entry : Snapshot.Items)
	{
		if (!IsValid(Entry.Definition) || !Entry.InstanceId.IsValid() || !ValidSlot(Entry.Slot)
			|| !Entry.Definition->AllowedSlots.Contains(Entry.Slot) || Ids.Contains(Entry.InstanceId) || Slots.Contains(Entry.Slot)) return Failure(EUmbraTooltipResult::InvalidSnapshot);
		Ids.Add(Entry.InstanceId);
		Slots.Add(Entry.Slot);
	}
	if (!ValidSlot(Slot)) return Failure(EUmbraTooltipResult::InvalidSlot);
	const auto* Entry = Snapshot.Items.FindByPredicate([Slot](const auto& Item) { return Item.Slot == Slot; });
	if (!ExpectedId.IsValid() || !Entry || Entry->InstanceId != ExpectedId || Entry->Definition != ExpectedDefinition) return Failure(EUmbraTooltipResult::IdentityMismatch);
	if (Equipment)
	{
		const auto Live = Equipment->GetSnapshot();
		const auto* LiveEntry = Live.Items.FindByPredicate([Slot](const auto& Item) { return Item.Slot == Slot; });
		if (!Live.bValid || Live.Revision != Snapshot.Revision || !LiveEntry || LiveEntry->InstanceId != ExpectedId
			|| LiveEntry->Definition != ExpectedDefinition || LiveEntry->bRequirementsMet != Entry->bRequirementsMet) return Failure(EUmbraTooltipResult::IdentityMismatch);
	}
	auto Out = FromDefinition(Entry->Definition);
	if (!Out.bValid) return Out;
	Out.Source = EUmbraTooltipSource::Equipment;
	Out.InstanceId = Entry->InstanceId;
	Out.bEquipped = true;
	Out.bHasTargetSlot = true;
	Out.TargetSlot = Slot;
	if (!SetContext(Out, Equipment)) return Failure(EUmbraTooltipResult::ContextUnavailable);
	// Snapshot is authoritative for worn aggregate status. Unavailable details remain explicitly unknown.
	Out.bPrimaryStatusKnown = true;
	Out.bRequirementsMet = Entry->bRequirementsMet;
	Out.bHasEquipmentPenalty = !Entry->bRequirementsMet;
	if (!Equipment && Out.bHasEquipmentPenalty) Out.PenaltyText = LOCTEXT("UnknownPenalty", "主属性不足，存在装备惩罚；当前上下文无法读取惩罚倍率。");
	UmbraTooltipFormatting::BuildDisplayText(Out);
	return Out;
}

#undef LOCTEXT_NAMESPACE
