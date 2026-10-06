#include "UI/Items/UmbraItemSlotVisual.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void FUmbraItemSlotVisual::CaptureDefaults(UImage* Background, UImage* Frame)
{
	if (Background && !bBackgroundCaptured)
	{
		BackgroundBrush = Background->GetBrush(); BackgroundColor = Background->GetColorAndOpacity(); bBackgroundCaptured = true;
	}
	if (Frame && !bFrameCaptured) { FrameBrush = Frame->GetBrush(); bFrameCaptured = true; }
}

void FUmbraItemSlotVisual::Apply(const FUmbraEquipmentItemDisplay& Item, UImage* Icon, UImage* Background, UImage* Frame) const
{
	const bool bHasItem = IsValid(Item.Item);
	if (Icon)
	{
		Icon->SetBrush(bHasItem ? Item.Icon : FSlateBrush());
		Icon->SetColorAndOpacity(FLinearColor::White);
		Icon->SetVisibility(bHasItem && Item.Icon.GetResourceObject() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	auto ApplyArt = [](UImage* Image, UTexture2D* Texture)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Image->SetBrush(Brush);
		Image->SetColorAndOpacity(FLinearColor::White);
	};
	if (Background)
	{
		if (bHasItem && Item.SlotBackgroundTexture) ApplyArt(Background, Item.SlotBackgroundTexture);
		else { Background->SetBrush(BackgroundBrush); Background->SetColorAndOpacity(BackgroundColor); }
		Background->SetVisibility(ESlateVisibility::Visible);
	}
	if (Frame)
	{
		if (bHasItem && Item.RarityFrameTexture) ApplyArt(Frame, Item.RarityFrameTexture);
		else { Frame->SetBrush(FrameBrush); Frame->SetColorAndOpacity(bHasItem ? Item.RarityColor : FLinearColor::White); }
		Frame->SetVisibility(bHasItem ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
