#include "UI/Combat/UmbraCombatStatEntry.h"

void UUmbraCombatStatEntry::NativePreConstruct()
{
	Super::NativePreConstruct();
	SetDisplayValue(TestValue);
}
