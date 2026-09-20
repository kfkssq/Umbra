#include "UI/UmbraStatEntry.h"
#include "UI/UmbraStatTooltip.h"

void UUmbraStatEntry::NativePreConstruct()
{
	Super::NativePreConstruct();
	BP_ApplyValue(DisplayValue.IsEmpty() ? FText::FromString(TEXT("—")) : DisplayValue);
}

void UUmbraStatEntry::NativeConstruct()
{
	Super::NativeConstruct();
	if (TooltipClass && GetOwningPlayer())
	{
		if (!IsValid(StatTooltip) || StatTooltip->GetClass() != TooltipClass.Get())
		{
			StatTooltip = CreateWidget<UUmbraStatTooltip>(GetOwningPlayer(), TooltipClass);
		}
		if (IsValid(StatTooltip))
		{
			StatTooltip->SetContent(StatName, Description);
			SetToolTip(StatTooltip);
		}
	}
}

void UUmbraStatEntry::NativeDestruct()
{
	SetToolTip(nullptr);
	StatTooltip = nullptr;
	Super::NativeDestruct();
}

void UUmbraStatEntry::SetDisplayValue(const FText& Value)
{
	DisplayValue = Value;
	BP_ApplyValue(DisplayValue);
}
