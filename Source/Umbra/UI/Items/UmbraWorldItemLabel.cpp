#include "UI/Items/UmbraWorldItemLabel.h"
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
	if (ItemName) DefaultNameColor = ItemName->GetColorAndOpacity();
	RefreshVisuals();
}

void UUmbraWorldItemLabel::NativeDestruct()
{
	bConstructed = false;
	if (ItemName) ItemName->SetColorAndOpacity(DefaultNameColor);
	Super::NativeDestruct();
}

void UUmbraWorldItemLabel::RefreshVisuals()
{
	const UUmbraItemDefinition* Definition = IsValid(Item.Definition) ? Item.Definition.Get() : nullptr;
	if (ItemName)
	{
		ItemName->SetText(Definition ? Definition->DisplayName : FText::GetEmpty());
		FSlateColor NameColor = DefaultNameColor;
		if (Definition)
		{
			const FLinearColor& Color = Definition->RarityColor;
			// Transparent/nonfinite/negative colors are unset; opaque black is a valid authored color.
			if (FMath::IsFinite(Color.R) && FMath::IsFinite(Color.G) && FMath::IsFinite(Color.B)
				&& FMath::IsFinite(Color.A) && Color.A > 0.f && Color.R >= 0.f && Color.G >= 0.f && Color.B >= 0.f)
				NameColor = FSlateColor(Color);
		}
		if (bHighlighted && bEnableHoverHighlight)
		{
			FLinearColor Color = NameColor.IsColorSpecified() ? NameColor.GetSpecifiedColor() : GetForegroundColor().GetSpecifiedColor();
			const float Brightness = FMath::IsFinite(HoverBrightnessMultiplier) ? FMath::Clamp(HoverBrightnessMultiplier, 1.f, 4.f) : 1.25f;
			Color.R *= Brightness;
			Color.G *= Brightness;
			Color.B *= Brightness;
			NameColor = FSlateColor(Color);
		}
		ItemName->SetColorAndOpacity(NameColor);
		if (NameWrapWidth > 0.f) ItemName->SetWrapTextAt(NameWrapWidth);
	}
	if (ItemIcon)
	{
		UTexture2D* Icon = Definition && IsValid(Definition->Icon) ? Definition->Icon.Get() : nullptr;
		// false preserves Designer image size; null also clears a previous item's resource.
		ItemIcon->SetBrushFromTexture(Icon, false);
		ItemIcon->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	BP_RefreshItem();
	// Cosmetic extensions must not intercept clicks intended for the world actor.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}
