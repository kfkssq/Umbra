#include "UI/UmbraEnemyTargetPanel.h"
#include "UmbraPlayerController.h"
#include "Characters/UmbraEnemyCharacter.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

void UUmbraEnemyTargetPanel::NativeConstruct()
{
 Super::NativeConstruct();
 // Presentation is authored entirely in the copied WBP tree; runtime only supplies data.
 EnemyName=Cast<UTextBlock>(GetWidgetFromName(TEXT("TextBlock_0")));
 EnemyLevel=Cast<UTextBlock>(GetWidgetFromName(TEXT("TextBlock_5")));
 if(auto* Icon=Cast<UUserWidget>(GetWidgetFromName(TEXT("W_EnemyIcon_2"))))
  Portrait=Cast<UImage>(Icon->GetWidgetFromName(TEXT("item_icon")));
 TemplateHealthBar=Cast<UUserWidget>(GetWidgetFromName(TEXT("W_ProgressBar_Parent")));
 ObservedController = Cast<AUmbraPlayerController>(GetOwningPlayer());
 if (ObservedController.IsValid()) ObservedController->OnHighlightedTargetChanged.AddUniqueDynamic(this, &ThisClass::RefreshTarget);
 RefreshTarget();
}
void UUmbraEnemyTargetPanel::NativeDestruct()
{
 if (ObservedController.IsValid()) ObservedController->OnHighlightedTargetChanged.RemoveDynamic(this, &ThisClass::RefreshTarget);
 ObservedController.Reset();
 Super::NativeDestruct();
}
void UUmbraEnemyTargetPanel::RefreshTarget()
{
 SetEnemy(ObservedController.IsValid() ? Cast<AUmbraEnemyCharacter>(ObservedController->GetHighlightedEnemyForUI()) : nullptr);
}
void UUmbraEnemyTargetPanel::ApplyNativeViewState(const FUmbraEnemyHealthBarViewState& State)
{
 const auto* Target = GetObservedEnemy();
 if (!Target) return;
 if (EnemyName) EnemyName->SetText(Target->EnemyDisplayName);
 if (EnemyLevel) EnemyLevel->SetText(FText::AsNumber(Target->EnemyDisplayLevel));
 if (TemplateHealthBar)
 {
  // Use the supplied Blueprint event so its fluid material and edge-glow behavior stay intact.
  if(auto* Function=TemplateHealthBar->FindFunction(TEXT("SetProgressValue")))
  {
   FStructOnScope Parameters(Function);
   if(auto* Value=FindFProperty<FNumericProperty>(Function,TEXT("Value"));Value&&Value->IsFloatingPoint())
   {
    Value->SetFloatingPointPropertyValue(Value->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()),State.HealthNormalized);
    TemplateHealthBar->ProcessEvent(Function,Parameters.GetStructMemory());
   }
  }
 }

 if (HealthText) HealthText->SetText(FText::Format(NSLOCTEXT("Umbra", "TargetHealth", "{0} / {1}"), FText::AsNumber(FMath::CeilToInt(State.Health)), FText::AsNumber(FMath::CeilToInt(State.MaxHealth))));
 if (Portrait)
 {
  auto* Texture=Target->EnemyPortrait ? Target->EnemyPortrait.Get() : FallbackPortrait.Get();
  if(Texture)
  {
   if(auto* Material=Portrait->GetDynamicMaterial()) Material->SetTextureParameterValue(TEXT("IconTexture"),Texture);
   else Portrait->SetBrushFromTexture(Texture);
  }
 }
}
