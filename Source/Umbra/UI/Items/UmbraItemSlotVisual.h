#pragma once
#include "CoreMinimal.h"
#include "UI/Equipment/UmbraEquipmentTypes.h"
#include "UmbraItemSlotVisual.generated.h"

class UImage;

/** Per-widget authored empty brushes, retained across item replacement and clearing. */
USTRUCT()
struct FUmbraItemSlotVisual
{
	GENERATED_BODY()
	void CaptureDefaults(UImage* Background, UImage* Frame);
	void Apply(const FUmbraEquipmentItemDisplay& Item, UImage* Icon, UImage* Background, UImage* Frame) const;
private:
	UPROPERTY(Transient) FSlateBrush BackgroundBrush;
	UPROPERTY(Transient) FSlateBrush FrameBrush;
	FLinearColor BackgroundColor = FLinearColor::White;
	bool bBackgroundCaptured = false;
	bool bFrameCaptured = false;
};
