#include "Skills/UmbraTargetingPrompt.h"
#include "Skills/UmbraSkillTargetingComponent.h"
#include "UmbraPlayerController.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
int32 UUmbraTargetingPrompt::NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Culling,
    FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool bEnabled) const
{
    const auto* PC=Cast<AUmbraPlayerController>(GetOwningPlayer());
    if (!PC || !PC->SkillTargeting || !PC->SkillTargeting->IsTargeting()) return Layer;
    const auto* Aim=PC->SkillTargeting.Get();
    const FString Text=TEXT("技能瞄准：左键确认 / 右键或 Esc 取消\n")+Aim->TargetStatus.ToString();
    FSlateDrawElement::MakeText(Elements,Layer+1,
        Geometry.ToPaintGeometry(FVector2D(600,70),FSlateLayoutTransform(FVector2D(FMath::Max(20.f,Geometry.GetLocalSize().X*.5f-300.f),Geometry.GetLocalSize().Y-340.f))),
        Text,FCoreStyle::GetDefaultFontStyle("Regular",22),ESlateDrawEffect::None,
        Aim->bValidTarget ? FLinearColor(.65f,1.f,.85f) : FLinearColor(1.f,.55f,.4f));
    return Layer+1;
}
