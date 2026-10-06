#include "UI/Combat/UmbraCombatStatData.h"
#include "Player/UmbraPlayerState.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/Abilities/UmbraBasicAttackAbility.h"
#include "AbilitySystem/Damage/UmbraDamageBonusComponent.h"
#include "Equipment/UmbraEquipmentComponent.h"
#include "Stats/UmbraDerivedStatsComponent.h"

FText UmbraCombatStats::FormatNumber(double Value, bool bPercent, int32 MinDigits, int32 MaxDigits)
{
	FNumberFormattingOptions Options;
	Options.SetMinimumFractionalDigits(MinDigits);
	Options.SetMaximumFractionalDigits(MaxDigits);
	return bPercent ? FText::Format(NSLOCTEXT("UmbraCombat", "Percent", "{0}%"), FText::AsNumber(Value * 100., &Options))
		: FText::AsNumber(Value, &Options);
}

FGameplayAttribute UmbraCombatStats::AttributeFor(EUmbraCombatStat Stat)
{
	switch (Stat)
	{
	case EUmbraCombatStat::AttackPower: return UUmbraAttributeSet::GetAttackPowerAttribute();
	case EUmbraCombatStat::AbilityPower: return UUmbraAttributeSet::GetAbilityPowerAttribute();
	case EUmbraCombatStat::AttackSpeed: return UUmbraAttributeSet::GetAttackSpeedAttribute();
	case EUmbraCombatStat::CriticalChance: return UUmbraAttributeSet::GetCriticalChanceAttribute();
	case EUmbraCombatStat::MaxHealth: return UUmbraAttributeSet::GetMaxHealthAttribute();
	case EUmbraCombatStat::Armor: return UUmbraAttributeSet::GetArmorAttribute();
	case EUmbraCombatStat::SlashingResistance: return UUmbraAttributeSet::GetSlashingResistanceAttribute();
	case EUmbraCombatStat::BluntResistance: return UUmbraAttributeSet::GetBluntResistanceAttribute();
	case EUmbraCombatStat::PiercingResistance: return UUmbraAttributeSet::GetPiercingResistanceAttribute();
	case EUmbraCombatStat::FireResistance: return UUmbraAttributeSet::GetFireResistanceAttribute();
	case EUmbraCombatStat::LightningResistance: return UUmbraAttributeSet::GetLightningResistanceAttribute();
	case EUmbraCombatStat::ColdResistance: return UUmbraAttributeSet::GetColdResistanceAttribute();
	case EUmbraCombatStat::RadiantResistance: return UUmbraAttributeSet::GetRadiantResistanceAttribute();
	case EUmbraCombatStat::PoisonResistance: return UUmbraAttributeSet::GetPoisonResistanceAttribute();
	case EUmbraCombatStat::ShadowResistance: return UUmbraAttributeSet::GetShadowResistanceAttribute();
	case EUmbraCombatStat::MaxResource: return UUmbraAttributeSet::GetMaxResourceAttribute();
	case EUmbraCombatStat::MoveSpeed: return UUmbraAttributeSet::GetMoveSpeedAttribute();
	case EUmbraCombatStat::AbilityHaste: return UUmbraAttributeSet::GetAbilityHasteAttribute();
	case EUmbraCombatStat::MagicResist: return UUmbraAttributeSet::GetMagicResistanceAttribute();
	case EUmbraCombatStat::ResourceRegeneration: return UUmbraAttributeSet::GetResourceRegenAttribute();
	case EUmbraCombatStat::HealthRegeneration: return UUmbraAttributeSet::GetHealthRegenAttribute();
	default: return FGameplayAttribute();
	}
}

FUmbraCombatStatValue UmbraCombatStats::Read(const AUmbraPlayerState* PlayerState, EUmbraCombatStat Stat)
{
	FUmbraCombatStatValue Out;
	Out.Text = FText::FromString(TEXT("—"));
	Out.Explanation = NSLOCTEXT("UmbraCombat", "Unavailable", "数据未就绪或尚未绑定。");
	if (Stat == EUmbraCombatStat::None) return Out;
	if (Stat == EUmbraCombatStat::Thorn || Stat == EUmbraCombatStat::DodgeChance
		|| Stat == EUmbraCombatStat::BlockChance || Stat == EUmbraCombatStat::BlockReduction
		|| Stat == EUmbraCombatStat::ShieldStrength || Stat == EUmbraCombatStat::HealingBonus
		|| Stat == EUmbraCombatStat::ShieldGeneration)
	{
		Out.Status = EUmbraCombatStatStatus::Placeholder;
		Out.Text = NSLOCTEXT("UmbraCombat", "Placeholder", "—（未实现）");
		Out.Explanation = NSLOCTEXT("UmbraCombat", "PlaceholderNote", "规划中的属性，尚未接入玩法。");
		return Out;
	}
	const auto* ASC = PlayerState ? PlayerState->GetUmbraAbilitySystemComponent() : nullptr;
	if (!ASC || !ASC->IsActorInfoReady() || !ASC->GetSet<UUmbraAttributeSet>()) return Out;
	const auto* Derived = PlayerState->FindComponentByClass<UUmbraDerivedStatsComponent>();
	const auto* Equipment = PlayerState->FindComponentByClass<UUmbraEquipmentComponent>();
	bool bPercent = Stat == EUmbraCombatStat::CriticalChance;
	int32 Digits = 0;
	Out.Explanation = NSLOCTEXT("UmbraCombat", "GAS", "读取GAS当前属性值。");
	if (Stat == EUmbraCombatStat::BaseWeaponDamage ||
		((Stat == EUmbraCombatStat::AttackPower || Stat == EUmbraCombatStat::AbilityPower) && Derived && Derived->bUseWeaponDerivedPower))
	{
		if (!Derived) return Out;
		const auto Snapshot = Derived->GetSnapshot();
		if (!Snapshot.bValid) return Out;
		if (Stat == EUmbraCombatStat::AttackPower) Out.Value = Snapshot.AttackPower;
		else if (Stat == EUmbraCombatStat::AbilityPower) Out.Value = Snapshot.AbilityPower;
		else for (const auto& Channel : Snapshot.Channels) Out.Value += Channel.BaseDamage;
		Out.Explanation = Stat == EUmbraCombatStat::BaseWeaponDamage
			? NSLOCTEXT("UmbraCombat", "BaseWeapon", "武器或徒手各类型有效基础伤害之和，包含装备需求惩罚，不含主属性补正；不是单次攻击伤害。")
			: NSLOCTEXT("UmbraCombat", "DerivedPower", "武器派生快照：包含主属性补正与装备需求惩罚，不含A/X或防御结算。");
	}
	else if (Stat == EUmbraCombatStat::EquipLoad || Stat == EUmbraCombatStat::MaxEquipLoad)
	{
		if (!Equipment) return Out;
		const auto Snapshot = Equipment->GetSnapshot();
		if (!Snapshot.bValid) return Out;
		Out.Value = Stat == EUmbraCombatStat::EquipLoad ? Snapshot.EquipLoad : Snapshot.MaxEquipLoad;
		Digits = 1;
		Out.Explanation = NSLOCTEXT("UmbraCombat", "Load", "读取装备快照；仅统计已穿戴装备，容量由装备组件规则提供，当前没有负重移动惩罚。");
	}
	else if (Stat >= EUmbraCombatStat::CriticalDamage && Stat <= EUmbraCombatStat::ShadowDamage)
	{
		UmbraDamageBonuses::FPanelSummary Summary;
		if (!UmbraDamageBonuses::ReadPanelSummary(ASC, Summary)) return Out;
		Out.bHasAdditionalConditionalBonuses = Summary.bHasConditional;
		Out.bHasMultiplicativeBonuses = Summary.bHasMultiplicative;
		if (Stat == EUmbraCombatStat::CriticalDamage) Out.Value = Summary.Critical;
		else if (Stat == EUmbraCombatStat::VulnerableDamage) Out.Value = Summary.Vulnerable;
		else if (Stat == EUmbraCombatStat::AllDamage) Out.Value = Summary.All;
		else Out.Value = Summary.Types[uint8(Stat) - uint8(EUmbraCombatStat::SlashingDamage)];
		bPercent = true;
		Out.Explanation = NSLOCTEXT("UmbraCombat", "Additive", "A区分类加成，仅新伤害规则使用。类型行不包含全伤；暴击/易伤行仅在相应状态下生效，不含基础倍率或旧总暴击倍率。额外条件项及X不计入此值，不代表本次命中的最终增伤。");
	}
	else
	{
		const auto Attribute = AttributeFor(Stat);
		if (!Attribute.IsValid()) return Out;
		Out.Value = ASC->GetNumericAttribute(Attribute);
		if (Stat == EUmbraCombatStat::AttackSpeed)
		{
			const auto* Attack = PlayerState->GetPrimaryAttackAbilityDefaults();
			const float Period = Attack ? Attack->GetConfiguredBaseAttackInterval() : 0.f;
			if (!FMath::IsFinite(Period) || Period <= 0.f) return Out;
			Out.Value = FMath::Clamp(Out.Value, double(UUmbraAttributeSet::MinAttackSpeedMultiplier),
				double(UUmbraAttributeSet::MaxAttackSpeedMultiplier)) / Period;
			Digits = 2;
			Out.Explanation = NSLOCTEXT("UmbraCombat", "AttackSpeed", "理论攻击次数/秒，按现有普攻基础周期与GAS攻速倍率读取；不是百分比或实测DPS。");
		}
		if (Stat >= EUmbraCombatStat::SlashingResistance && Stat <= EUmbraCombatStat::ShadowResistance)
			Out.Explanation = NSLOCTEXT("UmbraCombat", "Resistance", "类型抗性数值，不是减伤百分比；实际减伤由本次攻击的规则资产结算。");
		if (Stat == EUmbraCombatStat::MoveSpeed)
			Out.Explanation = NSLOCTEXT("UmbraCombat", "MoveSpeed", "移动速度，单位cm/s。");
	}
	if (!FMath::IsFinite(Out.Value)) return FUmbraCombatStatValue();
	Out.Status = EUmbraCombatStatStatus::Live;
	Out.Text = FormatNumber(Out.Value, bPercent, Digits, bPercent ? 1 : Digits);
	if (Out.bHasAdditionalConditionalBonuses || Out.bHasMultiplicativeBonuses)
		Out.Text = FText::Format(NSLOCTEXT("UmbraCombat", "Conditional", "{0}*"), Out.Text);
	if (Stat == EUmbraCombatStat::AbilityHaste || Stat == EUmbraCombatStat::HealthRegeneration || Stat == EUmbraCombatStat::ResourceRegeneration)
	{
		Out.Status = EUmbraCombatStatStatus::StoredOnly;
		Out.Text = FText::Format(NSLOCTEXT("UmbraCombat", "Stored", "{0}（仅存储）"), Out.Text);
		Out.Explanation = NSLOCTEXT("UmbraCombat", "StoredNote", "GAS已存储此值，但尚未实现对应的冷却或持续恢复逻辑。");
	}
	return Out;
}

