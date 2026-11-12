#pragma once
#include "CoreMinimal.h"
#include "UI/UmbraEnemyHealthBar.h"
#include "UmbraEnemyTargetPanel.generated.h"
class UTextBlock;
class UImage;
class AUmbraPlayerController;

/** Screen-space target details; all health values are observed from the existing GAS chain. */
UCLASS(Blueprintable, meta=(DisableNativeTick))
class UMBRA_API UUmbraEnemyTargetPanel : public UUmbraEnemyHealthBar
{
 GENERATED_BODY()
protected:
 virtual void NativeConstruct() override;
 virtual void NativeDestruct() override;
 virtual void ApplyNativeViewState(const FUmbraEnemyHealthBarViewState& State) override;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> EnemyName;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> EnemyLevel;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> HealthText;
 UPROPERTY(Transient) TObjectPtr<UImage> Portrait;
 UPROPERTY(EditDefaultsOnly, Category="Presentation") TObjectPtr<UTexture2D> FallbackPortrait;
private:
 UPROPERTY(Transient) TObjectPtr<UUserWidget> TemplateHealthBar;
 UFUNCTION() void RefreshTarget();
 TWeakObjectPtr<AUmbraPlayerController> ObservedController;
};
