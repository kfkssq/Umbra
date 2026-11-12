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
	UFUNCTION(BlueprintCallable, Category="Menu") void ShowCharacterPage();
	UFUNCTION(BlueprintCallable, Category="Menu") void ShowSkillsPage();
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void SetMenuOpen(bool bOpen);
	/** Call after dynamically adding/removing pages; Designer trees are bound automatically. */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void RefreshEquipmentPages();
protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<class UButton> CharacterPageButton;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<class UButton> SkillsPageButton;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<class UWidgetSwitcher> MainPageSwitcher;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
private:
	void SetMainPage(int32 Index);
	void VisitWidget(UWidget* Widget, bool bAncestorsVisible);
	void UnbindPageEvents();
	void HandlePageFieldChanged(UObject* Object, UE::FieldNotification::FFieldId Field);
	UFUNCTION()
	void HandleMenuVisibility(ESlateVisibility InVisibility);
	TArray<TWeakObjectPtr<UWidget>> ObservedWidgets;
	bool bMenuOpen = false;
};
