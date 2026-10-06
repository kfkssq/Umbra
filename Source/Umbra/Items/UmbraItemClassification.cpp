#include "Items/UmbraItemClassification.h"
#include "Items/UmbraItemDefinition.h"

#define LOCTEXT_NAMESPACE "UmbraItemClassification"

FText UmbraItemClassification::CategoryName(EUmbraItemCategory Category)
{
	switch (Category)
	{
	case EUmbraItemCategory::Weapon: return LOCTEXT("Weapon", "武器");
	case EUmbraItemCategory::Armor: return LOCTEXT("Armor", "防具");
	case EUmbraItemCategory::Accessory: return LOCTEXT("Accessory", "饰品");
	case EUmbraItemCategory::Consumable: return LOCTEXT("Consumable", "消耗品");
	case EUmbraItemCategory::Material: return LOCTEXT("Material", "材料");
	case EUmbraItemCategory::Misc: return LOCTEXT("Misc", "杂项");
	default: return LOCTEXT("Unknown", "物品");
	}
}

FText UmbraItemClassification::WeaponTypeName(EUmbraWeaponType Type)
{
	switch (Type)
	{
	case EUmbraWeaponType::OneHandedSword: return LOCTEXT("OneHandedSword", "单手剑");
	case EUmbraWeaponType::TwoHandedSword: return LOCTEXT("TwoHandedSword", "双手剑");
	case EUmbraWeaponType::Dagger: return LOCTEXT("Dagger", "匕首");
	case EUmbraWeaponType::Rapier: return LOCTEXT("Rapier", "刺剑");
	case EUmbraWeaponType::OneHandedAxe: return LOCTEXT("OneHandedAxe", "单手斧");
	case EUmbraWeaponType::TwoHandedAxe: return LOCTEXT("TwoHandedAxe", "双手斧");
	case EUmbraWeaponType::OneHandedMace: return LOCTEXT("OneHandedMace", "单手锤");
	case EUmbraWeaponType::TwoHandedMace: return LOCTEXT("TwoHandedMace", "双手锤");
	case EUmbraWeaponType::Spear: return LOCTEXT("Spear", "长枪");
	case EUmbraWeaponType::Halberd: return LOCTEXT("Halberd", "长戟");
	case EUmbraWeaponType::Staff: return LOCTEXT("Staff", "法杖");
	case EUmbraWeaponType::Wand: return LOCTEXT("Wand", "魔杖");
	case EUmbraWeaponType::Bow: return LOCTEXT("Bow", "弓");
	case EUmbraWeaponType::Crossbow: return LOCTEXT("Crossbow", "弩");
	default: return LOCTEXT("GenericWeapon", "武器");
	}
}

FText UmbraItemClassification::Describe(const UUmbraItemDefinition& Item, TArray<FText>& Warnings)
{
	Warnings.Reset();
	const bool bValidCategory = uint8(Item.ItemCategory) <= uint8(EUmbraItemCategory::Misc);
	const bool bValidType = uint8(Item.WeaponType) <= uint8(EUmbraWeaponType::Crossbow);
	const bool bUnknown = !bValidCategory || Item.ItemCategory == EUmbraItemCategory::Unknown;
	const bool bProfile = IsValid(Item.Weapon);
	const bool bSpecific = bValidType && Item.WeaponType != EUmbraWeaponType::Unknown;
	const bool bWeaponCategory = Item.ItemCategory == EUmbraItemCategory::Weapon;
	if (!bValidCategory || !bValidType) Warnings.Add(LOCTEXT("InvalidEnum", "分类枚举值无效；使用可靠的通用类型显示。"));
	if (bWeaponCategory && !bProfile) Warnings.Add(LOCTEXT("MissingProfile", "ItemCategory为Weapon，但缺少WeaponProfile；分类不会生成武器伤害。"));
	if (bSpecific && ((!bUnknown && !bWeaponCategory) || (bUnknown && !bProfile)))
		Warnings.Add(LOCTEXT("WrongWeaponType", "非武器分类配置了具体WeaponType；忽略具体武器标题，请核对资产分类。"));
	if (bProfile && !bUnknown && !bWeaponCategory)
		Warnings.Add(LOCTEXT("ConflictingProfile", "非武器分类配置了WeaponProfile；分类仅展示，不改变现有装备或伤害规则。"));
	if (bSpecific && (bWeaponCategory || (bUnknown && bProfile))) return WeaponTypeName(Item.WeaponType);
	if (!bUnknown) return CategoryName(Item.ItemCategory);
	return bProfile ? CategoryName(EUmbraItemCategory::Weapon)
		: !Item.AllowedSlots.IsEmpty() ? LOCTEXT("Equipment", "装备") : CategoryName(EUmbraItemCategory::Unknown);
}

#undef LOCTEXT_NAMESPACE
