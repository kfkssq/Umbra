#include "UI/UmbraStatEntry.h"
#include "UI/UmbraStatTooltip.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UUmbraStatEntry::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshDisplay();
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
	RefreshDisplay();
}

void UUmbraStatEntry::SetStatDisplay(UTexture2D* Icon, const FText& DisplayName, const FText& Value)
{
	DisplayIcon = Icon;
	bHasDisplayIcon = true;
	StatName = DisplayName;
	SetDisplayValue(Value);
}

void UUmbraStatEntry::RefreshDisplay()
{
	// Legacy HUD entries keep their Designer brushes unless the parent supplies an icon.
	if (StatIcon && bHasDisplayIcon)
	{
		StatIcon->SetBrushFromTexture(DisplayIcon);
		StatIcon->SetVisibility(DisplayIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (StatNameText) StatNameText->SetText(StatName);
	const FText Value = DisplayValue.IsEmpty() ? FText::FromString(TEXT("—")) : DisplayValue;
	if (StatValueText) StatValueText->SetText(Value);
	if (StatTooltip) StatTooltip->SetContent(StatName, Description);
	BP_ApplyValue(Value);
}
