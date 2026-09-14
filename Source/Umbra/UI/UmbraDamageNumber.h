#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UmbraDamageNumber.generated.h"
class UTextBlock;
UCLASS(Abstract, Blueprintable)
class UMBRA_API UUmbraDamageNumber : public UUserWidget
{
 GENERATED_BODY()
public:
 void Start(float Damage, bool bMagical, bool bCritical, FVector2D Position);
protected:
 virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
 virtual void NativeDestruct() override;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> DamageText;
 UPROPERTY(EditDefaultsOnly, Category="Damage Number") float InitialScale = 0.4f;
 UPROPERTY(EditDefaultsOnly, Category="Damage Number") float NormalScale = 1.f;
 UPROPERTY(EditDefaultsOnly, Category="Damage Number") float CriticalScale = 1.3f;
 /** Movement duration; retained under its original name for existing Blueprint defaults. */
 UPROPERTY(EditDefaultsOnly, Category="Damage Number", meta=(ClampMin="0")) float AppearDuration = 0.25f;
 /** Scaling only; does not change movement or the hold/fade schedule. */
 UPROPERTY(EditDefaultsOnly, Category="Damage Number", meta=(ClampMin="0.001", Units="s")) float PopDuration = 0.08f;
 /** Full angular width centered on screen up (negative screen Y). */
 UPROPERTY(EditDefaultsOnly, Category="Damage Number", meta=(ClampMin="0", ClampMax="360")) float SpreadAngleDegrees = 120.f;
 UPROPERTY(EditDefaultsOnly, Category="Damage Number", meta=(ClampMin="0")) float HoldDuration = 0.3f;
 UPROPERTY(EditDefaultsOnly, Category="Damage Number", meta=(ClampMin="0")) float FadeDuration = 0.3f;
 UPROPERTY(EditDefaultsOnly, Category="Damage Number", meta=(ClampMin="0")) float MinDistance = 40.f;
 UPROPERTY(EditDefaultsOnly, Category="Damage Number", meta=(ClampMin="0")) float MaxDistance = 90.f;
 UPROPERTY(EditDefaultsOnly, Category="Damage Number") FLinearColor PhysicalColor = FLinearColor::White;
 UPROPERTY(EditDefaultsOnly, Category="Damage Number") FLinearColor MagicalColor = FLinearColor(0.45f,0.75f,1.f,1.f);
private:
 void UpdateAnimation();
 FVector2D Origin;
 FVector2D Travel;
 float Elapsed = 0.f;
 float EndScale = 1.f;
 bool bStarted = false;
};
