#include "Equipment/UmbraEquipmentEffect.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "NativeGameplayTags.h"
#include "Items/UmbraItemDefinition.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_Strength, "Equipment.Magnitude.Strength");
UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_Dexterity, "Equipment.Magnitude.Dexterity");
UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_Intelligence, "Equipment.Magnitude.Intelligence");
UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_Faith, "Equipment.Magnitude.Faith");
UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_Armor, "Equipment.Magnitude.Armor");
UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_MagicResistance, "Equipment.Magnitude.MagicResistance");

UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_MaxHealth, "Equipment.Magnitude.MaxHealth");
UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_MaxResource, "Equipment.Magnitude.MaxResource");
UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_AttackSpeed, "Equipment.Magnitude.AttackSpeed");
UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_CriticalChance, "Equipment.Magnitude.CriticalChance");
UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_MoveSpeed, "Equipment.Magnitude.MoveSpeed");
UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_SlashingResistance, "Equipment.Magnitude.SlashingResistance");
UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_BluntResistance, "Equipment.Magnitude.BluntResistance");
UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_PiercingResistance, "Equipment.Magnitude.PiercingResistance");
UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_FireResistance, "Equipment.Magnitude.FireResistance");
UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_LightningResistance, "Equipment.Magnitude.LightningResistance");
UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_ColdResistance, "Equipment.Magnitude.ColdResistance");
UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_RadiantResistance, "Equipment.Magnitude.RadiantResistance");
UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_PoisonResistance, "Equipment.Magnitude.PoisonResistance");
UE_DEFINE_GAMEPLAY_TAG_STATIC(Equipment_ShadowResistance, "Equipment.Magnitude.ShadowResistance");

TArray<FGameplayAttribute> UUmbraEquipmentEffect::Attributes()
{
	return {UUmbraAttributeSet::GetStrengthAttribute(), UUmbraAttributeSet::GetDexterityAttribute(),
		UUmbraAttributeSet::GetIntelligenceAttribute(), UUmbraAttributeSet::GetFaithAttribute(),
		UUmbraAttributeSet::GetArmorAttribute(), UUmbraAttributeSet::GetMagicResistanceAttribute(),
		UUmbraAttributeSet::GetMaxHealthAttribute(),
		UUmbraAttributeSet::GetMaxResourceAttribute(),
		UUmbraAttributeSet::GetAttackSpeedAttribute(),
		UUmbraAttributeSet::GetCriticalChanceAttribute(),
		UUmbraAttributeSet::GetMoveSpeedAttribute(),
		UUmbraAttributeSet::GetSlashingResistanceAttribute(),
		UUmbraAttributeSet::GetBluntResistanceAttribute(),
		UUmbraAttributeSet::GetPiercingResistanceAttribute(),
		UUmbraAttributeSet::GetFireResistanceAttribute(),
		UUmbraAttributeSet::GetLightningResistanceAttribute(),
		UUmbraAttributeSet::GetColdResistanceAttribute(),
		UUmbraAttributeSet::GetRadiantResistanceAttribute(),
		UUmbraAttributeSet::GetPoisonResistanceAttribute(),
		UUmbraAttributeSet::GetShadowResistanceAttribute()};
}
TArray<FGameplayTag> UUmbraEquipmentEffect::MagnitudeTags()
{
	return {Equipment_Strength, Equipment_Dexterity, Equipment_Intelligence, Equipment_Faith, Equipment_Armor, Equipment_MagicResistance,
		Equipment_MaxHealth, Equipment_MaxResource, Equipment_AttackSpeed, Equipment_CriticalChance, Equipment_MoveSpeed, Equipment_SlashingResistance, Equipment_BluntResistance, Equipment_PiercingResistance, Equipment_FireResistance, Equipment_LightningResistance, Equipment_ColdResistance, Equipment_RadiantResistance, Equipment_PoisonResistance, Equipment_ShadowResistance};
}
UUmbraEquipmentEffect::UUmbraEquipmentEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	// AddComponent uses NewObject and cannot be used while constructing the GE CDO.
	GEComponents.Add(CreateDefaultSubobject<UUmbraEquipmentDamageBonusComponent>(TEXT("EquipmentDamageBonuses")));
	const auto Attrs = Attributes();
	const auto Tags = MagnitudeTags();
	for (int32 Index = 0; Index < Attrs.Num(); ++Index)
	{
		FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = Attrs[Index];
		Modifier.ModifierOp = EGameplayModOp::Additive;
		FSetByCallerFloat Magnitude;
		Magnitude.DataTag = Tags[Index];
		Modifier.ModifierMagnitude = Magnitude;
	}
}

int32 UUmbraEquipmentEffect::AffixIndex(EUmbraEquipmentAffixStat Stat)
{
	switch (Stat)
	{
	case EUmbraEquipmentAffixStat::Armor: return 4;
	case EUmbraEquipmentAffixStat::MagicResistance: return 5;
	case EUmbraEquipmentAffixStat::MaxHealth: return 6;
	case EUmbraEquipmentAffixStat::MaxResource: return 7;
	case EUmbraEquipmentAffixStat::AttackSpeed: return 8;
	case EUmbraEquipmentAffixStat::CriticalChance: return 9;
	case EUmbraEquipmentAffixStat::MoveSpeed: return 10;
	case EUmbraEquipmentAffixStat::SlashingResistance: return 11;
	case EUmbraEquipmentAffixStat::BluntResistance: return 12;
	case EUmbraEquipmentAffixStat::PiercingResistance: return 13;
	case EUmbraEquipmentAffixStat::FireResistance: return 14;
	case EUmbraEquipmentAffixStat::LightningResistance: return 15;
	case EUmbraEquipmentAffixStat::ColdResistance: return 16;
	case EUmbraEquipmentAffixStat::RadiantResistance: return 17;
	case EUmbraEquipmentAffixStat::PoisonResistance: return 18;
	case EUmbraEquipmentAffixStat::ShadowResistance: return 19;
	default: return INDEX_NONE;
	}
}

TArray<FUmbraDamageBonus> UUmbraEquipmentDamageBonusComponent::GetBonuses(const FGameplayEffectSpec& Spec) const
{
	const auto* Item = Cast<UUmbraItemDefinition>(Spec.GetContext().GetSourceObject());
	TArray<FUmbraDamageBonus> Result;
	if (!Item) return Result;
	Result = Item->DamageBonuses;
	for (const auto& Config : Item->AffixRollConfigs)
	{
		if (Config.Kind != EUmbraAffixKind::Damage) continue;
		const FName Key(*FString::Printf(TEXT("Umbra.Affix.%s"), *Config.AffixId.ToString()));
		if (const float* Value = Spec.SetByCallerNameMagnitudes.Find(Key))
		{
			auto Bonus = Config.DamageBonus;
			Bonus.Magnitude = FScalableFloat(*Value);
			Result.Add(MoveTemp(Bonus));
		}
	}
	return Result;
}

bool UUmbraEquipmentEffect::ApplyRolledAffixes(FGameplayEffectSpec& Spec, const TArray<FUmbraRolledAffix>& Rolls)
{
	const auto Tags = MagnitudeTags();
	const auto* Item = Cast<UUmbraItemDefinition>(Spec.GetContext().GetSourceObject());
	if (!Item) return false;
	for (const auto& Roll : Rolls)
	{
		if (Roll.Kind == EUmbraAffixKind::Damage)
		{
			// Definitions are immutable during play; only the spec's magnitude is instance-specific.
			const auto* Config = Item->AffixRollConfigs.FindByPredicate([&Roll](const auto& Entry)
				{ return Entry.AffixId == Roll.AffixId && Entry.Kind == EUmbraAffixKind::Damage; });
			if (!Config || Config->DamageBonus.Bucket != Roll.DamageBonus.Bucket
				|| Config->DamageBonus.Types != Roll.DamageBonus.Types
				|| Config->DamageBonus.AttackSource != Roll.DamageBonus.AttackSource
				|| Config->DamageBonus.bRequiresCritical != Roll.DamageBonus.bRequiresCritical
				|| Config->DamageBonus.bRequiresVulnerable != Roll.DamageBonus.bRequiresVulnerable
				|| !(Config->DamageBonus.SourceRequirements == Roll.DamageBonus.SourceRequirements)
				|| !(Config->DamageBonus.TargetRequirements == Roll.DamageBonus.TargetRequirements)) return false;
			Spec.SetSetByCallerMagnitude(FName(*FString::Printf(TEXT("Umbra.Affix.%s"), *Roll.AffixId.ToString())), Roll.Value);
			continue;
		}
		const int32 Index = Roll.Kind == EUmbraAffixKind::Primary ? int32(Roll.Primary) : AffixIndex(Roll.Attribute);
		if (!Tags.IsValidIndex(Index)) return false;
		const double Value = double(Spec.GetSetByCallerMagnitude(Tags[Index], false, 0.f)) + Roll.Value;
		if (!FMath::IsFinite(Value) || Value > MAX_flt) return false;
		Spec.SetSetByCallerMagnitude(Tags[Index], float(Value));
	}
	// Intrinsic defense is added later by Refresh; reject overflow before committing a handle.
	if (double(Item->Armor) + Spec.GetSetByCallerMagnitude(Tags[4], false, 0.f) > MAX_flt
		|| double(Item->MagicResistance) + Spec.GetSetByCallerMagnitude(Tags[5], false, 0.f) > MAX_flt) return false;
	return true;
}

bool UUmbraEquipmentDamageBonusComponent::Validate(const FGameplayEffectSpec& Spec) const
{
	return IsValid(Cast<UUmbraItemDefinition>(Spec.GetContext().GetSourceObject())) && Super::Validate(Spec);
}

bool UUmbraEquipmentEffect::ValidateBonuses(const UUmbraItemDefinition* Item)
{
	if (!IsValid(Item)) return false;
	TSet<EUmbraEquipmentAffixStat> Seen;
	for (const auto& Affix : Item->AttributeBonuses)
	{
		if (AffixIndex(Affix.Stat) == INDEX_NONE || Seen.Contains(Affix.Stat)
			|| !FMath::IsFinite(Affix.Magnitude) || Affix.Magnitude < 0.f) return false;
		if (Affix.Stat == EUmbraEquipmentAffixStat::Armor && double(Item->Armor) + Affix.Magnitude > MAX_flt) return false;
		if (Affix.Stat == EUmbraEquipmentAffixStat::MagicResistance && double(Item->MagicResistance) + Affix.Magnitude > MAX_flt) return false;
		Seen.Add(Affix.Stat);
	}
	FGameplayEffectContextHandle Context(new FGameplayEffectContext());
	Context.AddSourceObject(Item);
	const FGameplayEffectSpec Spec(GetDefault<UUmbraEquipmentEffect>(), Context, 1.f);
	return GetDefault<UUmbraEquipmentDamageBonusComponent>()->Validate(Spec);
}
