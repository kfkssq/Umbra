#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UmbraAttributeMenu.generated.h"

class UUmbraMenuTabButton;
class UWidgetSwitcher;

/** Coordinates the three existing attribute-menu pages without owning their presentation. */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraAttributeMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Menu|Tab")
	void SetActiveTab(int32 Index);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher_0;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UUmbraMenuTabButton> Button_page1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UUmbraMenuTabButton> Button_page2;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UUmbraMenuTabButton> Button_page3;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Menu|Tab")
	int32 CurrentTabIndex = INDEX_NONE;

private:
	UFUNCTION()
	void HandleTabClicked(int32 Index);
};
