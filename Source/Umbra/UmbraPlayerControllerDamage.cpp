#include "UmbraPlayerController.h"
#include "UI/UmbraDamageNumber.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/LocalPlayer.h"

void AUmbraPlayerController::ClientShowDamageNumber_Implementation(FVector WorldPosition, float Damage, uint8 Type, bool bCritical)
{
 if (!IsLocalController() || GetNetMode() == NM_DedicatedServer || !DamageNumberClass
  || !FMath::IsFinite(Damage) || Damage <= 0.f || Type > 1 || WorldPosition.ContainsNaN()) return;
 FVector2D Pixels;
 if (!ProjectWorldLocationToScreen(WorldPosition,Pixels,true)) return;
 int32 Width, Height;
 GetViewportSize(Width,Height);
 const ULocalPlayer* LP = GetLocalPlayer();
 const FVector2D Size = LP ? FVector2D(Width * LP->Size.X, Height * LP->Size.Y) : FVector2D(Width,Height);
 if (Pixels.X < 0 || Pixels.Y < 0 || Pixels.X >= Size.X || Pixels.Y >= Size.Y) return;
 const float DPI = UWidgetLayoutLibrary::GetViewportScale(this);
 if (DPI <= 0.f) return;
 auto* Number = CreateWidget<UUmbraDamageNumber>(this,DamageNumberClass);
 if (!Number) return;
 ActiveDamageNumbers.Add(Number);
 Number->AddToPlayerScreen(20);
 Number->Start(Damage,Type == 1,bCritical,WorldPosition);
}
void AUmbraPlayerController::ReleaseDamageNumber(UUmbraDamageNumber* Number)
{
 ActiveDamageNumbers.Remove(Number);
}
void AUmbraPlayerController::ClearDamageNumbers()
{
 const auto Numbers = ActiveDamageNumbers;
 ActiveDamageNumbers.Reset();
 for (const auto& Number : Numbers) if (IsValid(Number.Get())) Number->RemoveFromParent();
}
