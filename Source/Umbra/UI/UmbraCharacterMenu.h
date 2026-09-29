#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UmbraCharacterMenu.generated.h"

/** Event-driven activation of equipment pages in the existing nested WidgetSwitchers. */
UCLASS(Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraCharacterMenu : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void SetMenuOpen(bool bOpen);
	/** Call after dynamically adding/removing pages; Designer trees are bound automatically. */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void RefreshEquipmentPages();
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
private:
	void VisitWidget(UWidget* Widget, bool bAncestorsVisible);
	void UnbindPageEvents();
	void HandlePageFieldChanged(UObject* Object, UE::FieldNotification::FFieldId Field);
	UFUNCTION()
	void HandleMenuVisibility(ESlateVisibility InVisibility);
	TArray<TWeakObjectPtr<UWidget>> ObservedWidgets;
	bool bMenuOpen = false;
};
