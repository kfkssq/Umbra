#include "UI/UmbraDamageNumber.h"
#include "Components/TextBlock.h"
#include "UmbraPlayerController.h"
void UUmbraDamageNumber::Start(float Damage, bool bMagical, bool bCritical, FVector2D Position)
{
 SetIsFocusable(false);
 SetVisibility(ESlateVisibility::HitTestInvisible);
 SetAlignmentInViewport(FVector2D(0.5f,0.5f));
 SetRenderTransformPivot(FVector2D(0.5f,0.5f));
 Origin = Position;
 const float Spread = FMath::IsFinite(SpreadAngleDegrees) ? FMath::Clamp(SpreadAngleDegrees, 0.f, 360.f) : 120.f;
 const float Angle = FMath::DegreesToRadians(FMath::FRandRange(-Spread * 0.5f, Spread * 0.5f));
 const float Distance = FMath::FRandRange(FMath::Max(0.f, MinDistance), FMath::Max(FMath::Max(0.f, MinDistance), MaxDistance));
 Travel = FVector2D(FMath::Sin(Angle),-FMath::Cos(Angle)) * Distance;
 EndScale = bCritical ? CriticalScale : NormalScale;
 if (DamageText)
 {
  FNumberFormattingOptions Options;
  Options.SetUseGrouping(false);
  Options.SetMinimumFractionalDigits(Damage < 1.f ? 1 : 0);
  Options.SetMaximumFractionalDigits(Damage < 1.f ? 1 : 0);
  FString Text = FText::AsNumber(Damage, &Options).ToString();
  if (bCritical) Text += TEXT("!");
  DamageText->SetText(FText::FromString(Text));
  DamageText->SetColorAndOpacity(bMagical ? MagicalColor : PhysicalColor);
 }
 Elapsed = 0.f;
 bStarted = true;
 UpdateAnimation();
}
void UUmbraDamageNumber::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
 Super::NativeTick(Geometry, DeltaTime);
 if (!bStarted) return;
 Elapsed += FMath::Max(0.f, DeltaTime);
 UpdateAnimation();
}
void UUmbraDamageNumber::UpdateAnimation()
{
 const float Appear = FMath::Max(0.f, AppearDuration);
 const float Hold = FMath::Max(0.f, HoldDuration);
 const float Fade = FMath::Max(0.f, FadeDuration);
 // Absolute elapsed time preserves overshoot across all three stages.
 const float Alpha = Appear > 0.f ? FMath::Clamp(Elapsed / Appear,0.f,1.f) : 1.f;
 const float Smooth = Alpha * Alpha * (3.f - 2.f * Alpha);
 SetPositionInViewport(Origin + Travel * Smooth, false);
 const float Pop = FMath::IsFinite(PopDuration) ? FMath::Max(0.001f, PopDuration) : 0.08f;
 const float ScaleAlpha = FMath::Clamp(Elapsed / Pop, 0.f, 1.f);
 const float ScaleSmooth = ScaleAlpha * ScaleAlpha * (3.f - 2.f * ScaleAlpha);
 SetRenderScale(FVector2D(FMath::Lerp(InitialScale,EndScale,ScaleSmooth)));
 const float FadeElapsed = Elapsed - Appear - Hold;
 SetRenderOpacity(FadeElapsed <= 0.f ? 1.f : (Fade > 0.f ? 1.f - FMath::Clamp(FadeElapsed/Fade,0.f,1.f) : 0.f));
 if (Elapsed >= Appear + Hold + Fade)
 {
  bStarted = false;
  if (auto* PC = Cast<AUmbraPlayerController>(GetOwningPlayer())) PC->ReleaseDamageNumber(this);
  RemoveFromParent();
 }
}
void UUmbraDamageNumber::NativeDestruct()
{
 bStarted = false;
 if (auto* PC = Cast<AUmbraPlayerController>(GetOwningPlayer())) PC->ReleaseDamageNumber(this);
 Super::NativeDestruct();
}
