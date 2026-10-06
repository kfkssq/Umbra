#include "UI/Items/UmbraTooltipScalingEntry.h"
#include "Components/TextBlock.h"
#include "Umbra.h"

void UUmbraTooltipScalingEntry::SetEntry(const FText& InAttribute, const FText& InGrade)
{
	Attribute = InAttribute;
	Grade = InGrade;
	RefreshDisplay();
	BP_EntryChanged(Attribute, Grade);
}

void UUmbraTooltipScalingEntry::SetVisibility(ESlateVisibility InVisibility)
{
	Super::SetVisibility(InVisibility == ESlateVisibility::Hidden || InVisibility == ESlateVisibility::Collapsed
		? InVisibility : ESlateVisibility::HitTestInvisible);
}

void UUmbraTooltipScalingEntry::RefreshDisplay()
{
	SetIsFocusable(false);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (AttributeText) AttributeText->SetText(Attribute);
	if (GradeText) GradeText->SetText(Grade);
}

void UUmbraTooltipScalingEntry::NativePreConstruct() { Super::NativePreConstruct(); RefreshDisplay(); }
void UUmbraTooltipScalingEntry::NativeConstruct()
{
	Super::NativeConstruct();
	if (!AttributeText) UE_LOG(LogUmbra, Warning, TEXT("Tooltip scaling entry %s requires AttributeText (TextBlock)."), *GetName());
	if (!GradeText) UE_LOG(LogUmbra, Warning, TEXT("Tooltip scaling entry %s requires GradeText (TextBlock)."), *GetName());
	RefreshDisplay();
}
void UUmbraTooltipScalingEntry::NativeDestruct()
{
	Super::NativeDestruct();
	Attribute = FText::GetEmpty();
	Grade = FText::GetEmpty();
	RefreshDisplay();
}