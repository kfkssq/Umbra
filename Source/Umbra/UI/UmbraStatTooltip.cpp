#include "UI/UmbraStatTooltip.h"

void UUmbraStatTooltip::SetContent(const FText& InName, const FText& InDescription)
{
	BP_ApplyContent(InName, InDescription);
}
