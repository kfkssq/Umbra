#include "Items/UmbraItemTooltipData.h"
#include "UI/Combat/UmbraCombatStatData.h"

#define LOCTEXT_NAMESPACE "UmbraItemTooltipDisplay"

FText UmbraTooltipFormatting::DamageRange(double Min, double Max)
{
	const FText Low = UmbraCombatStats::FormatNumber(Min, false, 0, 2);
	return Min == Max ? Low : FText::Format(LOCTEXT("DamageRange", "{0}–{1}"), Low,
		UmbraCombatStats::FormatNumber(Max, false, 0, 2));
}

FText UmbraTooltipFormatting::RarityName(EUmbraItemRarity Rarity)
{
	switch (Rarity)
	{
	case EUmbraItemRarity::Common: return LOCTEXT("Common", "普通");
	case EUmbraItemRarity::Magic: return LOCTEXT("Magic", "魔法");
	case EUmbraItemRarity::Rare: return LOCTEXT("Rare", "稀有");
	case EUmbraItemRarity::Epic: return LOCTEXT("Epic", "史诗");
	case EUmbraItemRarity::Legendary: return LOCTEXT("Legendary", "传奇");
	case EUmbraItemRarity::Unique: return LOCTEXT("Unique", "独特");
	default: return FText::GetEmpty();
	}
}

void UmbraTooltipFormatting::BuildDisplayText(FUmbraItemTooltipData& Data)
{
	const auto Number = [](double Value) { return UmbraCombatStats::FormatNumber(Value, false, 0, 2); };
	Data.RarityText = RarityName(Data.Rarity);
	Data.RarityAndTypeText = FText::Format(LOCTEXT("RarityAndType", "{0} · {1}"), Data.RarityText, Data.ItemType);
	Data.ItemLevelText = FText::Format(LOCTEXT("ItemLevel", "物品等级 {0}"), FText::AsNumber(Data.ItemLevel));
	Data.TotalDamageText = Data.DamageChannels.IsEmpty() ? FText::GetEmpty() : DamageRange(Data.TotalMinBaseDamage, Data.TotalMaxBaseDamage);
	Data.DamageEntries.Reset();
	for (auto& Channel : Data.DamageChannels)
	{
		Channel.Text = FText::Format(LOCTEXT("Channel", "{0} {1}"), Channel.Name, DamageRange(Channel.MinBaseDamage, Channel.MaxBaseDamage));
		FUmbraTooltipEntry Entry;
		Entry.DisplayMode = EUmbraTooltipEntryDisplayMode::KeyValue;
		Entry.LabelText = Channel.Name;
		Entry.ValueText = DamageRange(Channel.MinBaseDamage, Channel.MaxBaseDamage);
		Entry.Style = EUmbraTooltipEntryStyle::Damage;
		Data.DamageEntries.Add(Entry);
	}
	bool bHasGrade = false, bCurve = false;
	Data.ScalingEntries.Reset();
	for (auto& Scaling : Data.Scaling)
	{
		Scaling.Text = FText::GetEmpty();
		if (Scaling.Grade == EUmbraScalingGrade::None) continue;
		bHasGrade = true;
		bCurve |= Scaling.bHasCurve;
		const FText Name = StatName(FName(*StaticEnum<EUmbraPrimaryAttribute>()->GetNameStringByValue(int64(Scaling.Attribute))));
		const FText Grade = FText::FromString(StaticEnum<EUmbraScalingGrade>()->GetNameStringByValue(int64(Scaling.Grade)));
		Scaling.Text = FText::Format(LOCTEXT("Grade", "{0}  {1}{2}"), Name, Grade,
			Scaling.bHasCurve ? LOCTEXT("CurveMarker", "（曲线）") : FText::GetEmpty());
		FUmbraTooltipEntry Entry;
		Entry.DisplayMode = EUmbraTooltipEntryDisplayMode::KeyValue;
		Entry.LabelText = Name;
		Entry.ValueText = Grade;
		Entry.Style = EUmbraTooltipEntryStyle::Scaling;
		Data.ScalingEntries.Add(Entry);
	}
	Data.ScalingNote = !bHasGrade ? FText::GetEmpty() : bCurve
		? LOCTEXT("CurveNote", "评级仅表示配置参考强度；曲线使实际收益随主属性变化，不代表精确收益。")
		: LOCTEXT("GradeNote", "评级仅表示配置参考强度，不代表精确收益。");
	// The compatibility array remains empty: ordinary affixes have one dedicated rendering path.
	Data.StatEntries.Reset();
	Data.AffixEntries.Reset();
	for (int32 Index = 0; Index < Data.Stats.Num(); ++Index)
	{
		const auto& Stat = Data.Stats[Index];
		auto& Entry = Data.AffixEntries.AddDefaulted_GetRef();
		Entry.AffixId = Stat.AffixId.IsNone() ? FName(*FString::Printf(TEXT("Fixed.%d.%s"), Index, *Stat.StatId.ToString())) : Stat.AffixId;
		Entry.AttributeId = Stat.StatId;
		Entry.bDamageBonus = Stat.bDamageBonus;
		Entry.DamageBonus = Stat.DamageBonus;
		Entry.Source = Stat.Source;
		Entry.Unit = Stat.Unit;
		Entry.DisplayName = Stat.bDamageBonus ? DamageAffixName(Stat.DamageBonus) : StatName(Stat.StatId);
		Entry.bHasActualValue = !Stat.bConfigurationPreview;
		Entry.ValueText = Entry.bHasActualValue ? StatValue(Stat) : FText::GetEmpty();
		Entry.bRolled = Stat.bRolled;
		Entry.RolledValue = Stat.RawValue;
		Entry.TierCount = Stat.CandidateValues.Num();
		Entry.SelectedTier = Stat.SelectedTier;
		if (!Stat.CandidateValues.IsEmpty())
		{
			Entry.MinPossibleValue = Stat.CandidateValues[0];
			Entry.MaxPossibleValue = Stat.CandidateValues.Last();
			if (Stat.bConfigurationPreview || Entry.MinPossibleValue != Entry.MaxPossibleValue) Entry.RangeText = FText::Format(LOCTEXT("AffixRange", "[{0}–{1}]"),
				UmbraCombatStats::FormatNumber(Entry.MinPossibleValue, Stat.Unit == EUmbraTooltipUnit::Percent, 0, 4),
				UmbraCombatStats::FormatNumber(Entry.MaxPossibleValue, Stat.Unit == EUmbraTooltipUnit::Percent, 0, 4));
		}
	}
	Data.SpecialEffects.Reset();
	for (const auto& Stat : Data.SpecialEffectStats)
	{
		FText Text = Stat.bConfigurationPreview ? DamageAffixName(Stat.DamageBonus) : Format(Stat);
		if (!Stat.CandidateValues.IsEmpty())
			Text = FText::Format(LOCTEXT("SpecialRange", "{0} [{1}–{2}]"), Text,
				UmbraCombatStats::FormatNumber(double(Stat.CandidateValues[0]) - 1., true, 0, 4),
				UmbraCombatStats::FormatNumber(double(Stat.CandidateValues.Last()) - 1., true, 0, 4));
		Data.SpecialEffects.Add(Text);
	}
	Data.RequirementRows.Reset();
	const bool bKnown = Data.Requirements.bKnown && Data.RequirementState != EUmbraTooltipRequirementState::Unknown;
	const auto Style = [](bool bAvailable, bool bMet)
	{
		return !bAvailable ? EUmbraTooltipRowStyle::Unknown : bMet ? EUmbraTooltipRowStyle::Met : EUmbraTooltipRowStyle::Unmet;
	};
	const auto StateText = [](EUmbraTooltipRowStyle State)
	{
		return State == EUmbraTooltipRowStyle::Unknown ? LOCTEXT("Unknown", "未知")
			: State == EUmbraTooltipRowStyle::Met ? LOCTEXT("Met", "已满足") : LOCTEXT("Unmet", "未满足");
	};
	auto& Level = Data.RequirementRows.AddDefaulted_GetRef();
	Level.Style = Style(bKnown, Data.Requirements.bLevelMet);
	Level.Text = FText::Format(LOCTEXT("LevelRequirement", "需求等级 {0}（当前 {1}，{2}）"), FText::AsNumber(Data.RequiredLevel),
		bKnown ? FText::AsNumber(Data.Requirements.CurrentLevel) : LOCTEXT("Unknown", "未知"), StateText(Level.Style));
	const auto Required = Data.RequiredPrimaries.ToArray();
	for (int32 Index = 0; Index < 4; ++Index)
	{
		if (Required[Index] <= 0.f) continue;
		const bool bAvailable = bKnown && Data.Requirements.CurrentPrimaries.IsValidIndex(Index) && Data.Requirements.PrimaryMet.IsValidIndex(Index);
		auto& Row = Data.RequirementRows.AddDefaulted_GetRef();
		Row.Style = Style(bAvailable, bAvailable && Data.Requirements.PrimaryMet[Index]);
		Row.Text = FText::Format(LOCTEXT("PrimaryRequirement", "需求{0} {1}（当前 {2}，{3}）"),
			StatName(FName(*StaticEnum<EUmbraPrimaryAttribute>()->GetNameStringByValue(Index))), Number(Required[Index]),
			bAvailable ? Number(Data.Requirements.CurrentPrimaries[Index]) : LOCTEXT("Unknown", "未知"), StateText(Row.Style));
	}
	switch (Data.RequirementState)
	{
	case EUmbraTooltipRequirementState::LevelTooLow: Data.RequirementStatusText = LOCTEXT("LevelTooLow", "等级不足，不可穿戴。"); break;
	case EUmbraTooltipRequirementState::PrimaryPenalty: Data.RequirementStatusText = LOCTEXT("PrimaryPenalty", "主属性不足，允许穿戴但存在装备惩罚。"); break;
	case EUmbraTooltipRequirementState::Met: Data.RequirementStatusText = LOCTEXT("AllMet", "装备要求已满足。"); break;
	default: Data.RequirementStatusText = LOCTEXT("UnknownRequirements", "装备要求状态未知；尚不能确认可穿戴。"); break;
	}
	Data.WeightText = FText::Format(LOCTEXT("Weight", "重量：{0}"), Number(Data.Weight));
}

#undef LOCTEXT_NAMESPACE
