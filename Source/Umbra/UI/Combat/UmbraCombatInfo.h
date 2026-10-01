#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UmbraCombatInfo.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

/** Local expand/collapse interaction for the existing second page. Owns no combat data. */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraCombatInfo : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> AttackToggle;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> DefenseToggle;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> AttackArrow;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> DefenseArrow;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> AttackRows;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> DefenseRows;

private:
	UFUNCTION()
	void ToggleAttack();
	UFUNCTION()
	void ToggleDefense();
	void RefreshSections();

	// Retain expansion when the same menu instance is removed and re-added.
	bool bAttackExpanded = true;
	bool bDefenseExpanded = true;
};
