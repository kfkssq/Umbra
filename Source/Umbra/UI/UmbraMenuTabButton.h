#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UmbraMenuTabButton.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUmbraMenuTabClicked, int32, TabIndex);

/** C++ click and selection state for the existing menu button WBP. The WBP owns its appearance. */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraMenuTabButton : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Menu|Tab")
	FUmbraMenuTabClicked OnTabClicked;

	UFUNCTION(BlueprintCallable, Category = "Menu|Tab")
	void SetSelected(bool bInSelected);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Set on each embedded Button_page instance; the menu uses 0, 1, 2 in switcher order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu|Tab", meta = (ExposeOnSpawn = "true"))
	int32 TabIndex = INDEX_NONE;

	/** Existing W_ButtonBrownSquare_Icon inner UButton; verify this name in Designer after reparenting. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_0;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Menu|Tab")
	bool bIsSelected = false;

	UFUNCTION(BlueprintImplementableEvent, Category = "Menu|Tab")
	void BP_OnSelectedChanged(bool bSelected);

private:
	UFUNCTION()
	void HandleButtonClicked();
};
