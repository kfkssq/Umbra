#include "UI/Items/UmbraWorldItemLabel.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UUmbraWorldItemLabel::SetItem(const FUmbraItemInstance& InItem, bool bInHighlighted)
{
	Item = InItem;
	bHighlighted = bInHighlighted;
	// WidgetComponent may deliver the payload before Slate construction/Designer PreConstruct.
	if (bConstructed) RefreshVisuals();
}

void UUmbraWorldItemLabel::NativeConstruct()
{
	Super::NativeConstruct();
	bConstructed = true;
	if (ItemBackground && CachedBackground.Get() != ItemBackground)
	{
		CachedBackground = ItemBackground;
		DefaultBackgroundBrush = ItemBackground->Background;
		DefaultBackgroundColor = ItemBackground->GetBrushColor();
	}
	RefreshVisuals();
}

void UUmbraWorldItemLabel::NativeDestruct()
{
	bConstructed = false;
	RestoreBackground();
	Super::NativeDestruct();
}

void UUmbraWorldItemLabel::RestoreBackground()
{
	if (ItemBackground && CachedBackground.Get() == ItemBackground)
	{
		ItemBackground->SetBrush(DefaultBackgroundBrush);
		ItemBackground->SetBrushColor(DefaultBackgroundColor);
	}
}

void UUmbraWorldItemLabel::RefreshVisuals()
{
	const UUmbraItemDefinition* Definition = IsValid(Item.Definition) ? Item.Definition.Get() : nullptr;
	if (ItemName)
	{
		ItemName->SetText(Definition ? Definition->DisplayName : FText::GetEmpty());
		ItemName->SetColorAndOpacity(FSlateColor(Definition ? Definition->RarityColor : FLinearColor::White));
	}
	if (ItemIcon)
	{
		UTexture2D* Icon = Definition && IsValid(Definition->Icon) ? Definition->Icon.Get() : nullptr;
		// false preserves Designer image size; null also clears a previous item's resource.
		ItemIcon->SetBrushFromTexture(Icon, false);
		ItemIcon->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	RestoreBackground();
	if (ItemBackground && Definition && bHighlighted && bEnableHoverHighlight)
		ItemBackground->SetBrushColor(HoverBackgroundColor);
	BP_RefreshItem();
	// Cosmetic extensions must not intercept clicks intended for the world actor.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}
