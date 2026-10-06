#include "UI/Items/UmbraTooltipKeyValueEntry.h"
#include "Components/TextBlock.h"
#include "Umbra.h"

void UUmbraTooltipKeyValueEntry::SetEntry(const FText& InLabel, const FText& InValue)
{
	Label = InLabel;
	Value = InValue;
	RefreshDisplay();
	BP_EntryChanged(Label, Value);
}

void UUmbraTooltipKeyValueEntry::SetVisibility(ESlateVisibility InVisibility)
{
	Super::SetVisibility(InVisibility == ESlateVisibility::Hidden || InVisibility == ESlateVisibility::Collapsed
		? InVisibility : ESlateVisibility::HitTestInvisible);
}

void UUmbraTooltipKeyValueEntry::RefreshDisplay()
{
	SetIsFocusable(false);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (LabelText) { LabelText->SetText(Label); LabelText->SetAutoWrapText(true); }
	if (ValueText) { ValueText->SetText(Value); ValueText->SetAutoWrapText(true); }
}

void UUmbraTooltipKeyValueEntry::NativePreConstruct() { Super::NativePreConstruct(); RefreshDisplay(); }
void UUmbraTooltipKeyValueEntry::NativeConstruct()
{
	Super::NativeConstruct();
	if (!LabelText) UE_LOG(LogUmbra, Warning, TEXT("Tooltip key/value entry %s requires LabelText (TextBlock)."), *GetName());
	if (!ValueText) UE_LOG(LogUmbra, Warning, TEXT("Tooltip key/value entry %s requires ValueText (TextBlock)."), *GetName());
	RefreshDisplay();
}
void UUmbraTooltipKeyValueEntry::NativeDestruct()
{
	Super::NativeDestruct();
	Label = FText::GetEmpty();
	Value = FText::GetEmpty();
	RefreshDisplay();
}