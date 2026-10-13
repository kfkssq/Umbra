#include "UI/Items/UmbraTooltipAffixEntry.h"
#include "Components/TextBlock.h"

void UUmbraTooltipAffixEntry::SetAffixData(const FUmbraTooltipAffixDisplayData& Data)
{
	AffixData = Data;
	RefreshDisplay();
}

void UUmbraTooltipAffixEntry::SetVisibility(ESlateVisibility InVisibility)
{
	Super::SetVisibility(InVisibility == ESlateVisibility::Hidden || InVisibility == ESlateVisibility::Collapsed
		? InVisibility : ESlateVisibility::HitTestInvisible);
}

void UUmbraTooltipAffixEntry::RefreshDisplay()
{
	SetIsFocusable(false);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (IsDesignTime()) return;
	if (AffixName) AffixName->SetText(AffixData.DisplayName);
	if (AffixValue) AffixValue->SetText(!AffixRange && !AffixData.RangeText.IsEmpty()
		? FText::Format(NSLOCTEXT("UmbraTooltipAffix", "InlineRange", "{0} {1}"), AffixData.ValueText, AffixData.RangeText) : AffixData.ValueText);
	if (AffixRange)
	{
		AffixRange->SetText(AffixData.RangeText);
		AffixRange->SetVisibility(AffixData.RangeText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UUmbraTooltipAffixEntry::NativePreConstruct() { Super::NativePreConstruct(); RefreshDisplay(); }
void UUmbraTooltipAffixEntry::NativeConstruct() { Super::NativeConstruct(); RefreshDisplay(); }
void UUmbraTooltipAffixEntry::NativeDestruct()
{
	AffixData = FUmbraTooltipAffixDisplayData();
	RefreshDisplay();
	Super::NativeDestruct();
}
