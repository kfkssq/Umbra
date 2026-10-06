#include "UI/Combat/UmbraCombatStatEntry.h"

void UUmbraCombatStatEntry::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (IsDesignTime()) SetDisplayValue(TestValue);
	else ApplyCombatValue(CombatValue);
}

EUmbraCombatStat UUmbraCombatStatEntry::ResolveCombatStat() const
{
	if (CombatStat != EUmbraCombatStat::None) return CombatStat;
	const int64 Value = StaticEnum<EUmbraCombatStat>()->GetValueByNameString(GetName());
	return Value == INDEX_NONE ? EUmbraCombatStat::None : EUmbraCombatStat(Value);
}

void UUmbraCombatStatEntry::ApplyCombatValue(const FUmbraCombatStatValue& Value)
{
	CombatValue = Value;
	Description = Value.Explanation;
	SetDisplayValue(Value.Text);
}
