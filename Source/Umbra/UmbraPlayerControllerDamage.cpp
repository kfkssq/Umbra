#include "UmbraPlayerController.h"
#include "UI/UmbraDamageNumber.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"

void AUmbraPlayerController::ClientShowAttackDamageMeasurement_Implementation(float Seconds, int32 AttackStarts,
	int32 DamagingHits, float HealthDamage, float DamagePerSecond, bool bAborted)
{
	if (!IsLocalController() || GetNetMode() == NM_DedicatedServer || !GEngine) return;
	const FString Message = FString::Printf(
		TEXT("普攻伤害统计%s | %.2f 秒 | 起手 %d | 结算命中 %d | 总伤害 %.1f | DPS %.1f"),
		bAborted ? TEXT("（中断）") : TEXT(""), Seconds, AttackStarts, DamagingHits, HealthDamage, DamagePerSecond);
	// A stable key replaces the previous result for this controller instead of stacking messages.
	const uint64 MessageKey = 0x554D4252414D4553ULL ^ uint64(GetUniqueID());
	GEngine->AddOnScreenDebugMessage(MessageKey, 10.f, bAborted ? FColor::Yellow : FColor::Cyan, Message);
}

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
