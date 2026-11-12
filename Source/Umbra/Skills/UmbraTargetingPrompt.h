#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UmbraTargetingPrompt.generated.h"
UCLASS()
class UMBRA_API UUmbraTargetingPrompt : public UUserWidget
{
    GENERATED_BODY()
protected:
    virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Culling,
        FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool bEnabled) const override;
};
