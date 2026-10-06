#include "UI/Items/UmbraTooltipStatEntry.h"
#include "Components/TextBlock.h"
#include "Umbra.h"

void UUmbraTooltipStatEntry::SetEntryText(const FText& Text, EUmbraTooltipRowStyle Style)
{
	DisplayText = Text;
	DisplayStyle = Style;
	RefreshDisplay();
	BP_EntryChanged(DisplayText, DisplayStyle);
}

void UUmbraTooltipStatEntry::SetVisibility(ESlateVisibility InVisibility)
{
	Super::SetVisibility(InVisibility == ESlateVisibility::Hidden || InVisibility == ESlateVisibility::Collapsed
		? InVisibility : ESlateVisibility::HitTestInvisible);
}

void UUmbraTooltipStatEntry::RefreshDisplay()
{
	SetIsFocusable(false);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (!EntryText) return;
	if (!bCapturedColor) { NeutralColor = EntryText->GetColorAndOpacity(); bCapturedColor = true; }
	EntryText->SetText(DisplayText);
	EntryText->SetAutoWrapText(true);
	EntryText->SetColorAndOpacity(DisplayStyle == EUmbraTooltipRowStyle::Met ? FSlateColor(MetColor)
		: DisplayStyle == EUmbraTooltipRowStyle::Unmet ? FSlateColor(UnmetColor)
		: DisplayStyle == EUmbraTooltipRowStyle::Unknown ? FSlateColor(UnknownColor) : NeutralColor);
}

void UUmbraTooltipStatEntry::NativePreConstruct() { Super::NativePreConstruct(); RefreshDisplay(); }
void UUmbraTooltipStatEntry::NativeConstruct()
{
	Super::NativeConstruct();
	if (!EntryText) UE_LOG(LogUmbra, Warning, TEXT("Tooltip entry %s requires EntryText (TextBlock)."), *GetName());
	RefreshDisplay();
}
void UUmbraTooltipStatEntry::NativeDestruct()
{
	Super::NativeDestruct();
	DisplayText = FText::GetEmpty();
	DisplayStyle = EUmbraTooltipRowStyle::Neutral;
	RefreshDisplay();
}
