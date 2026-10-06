#pragma once

#include "CoreMinimal.h"
#include "UI/Combat/UmbraCombatInfo.h"
#include "UI/Combat/UmbraCombatStatEntry.h"
#include "UmbraCombatInfoTestTypes.generated.h"

UCLASS(Transient, NotBlueprintable)
class UUmbraCombatInfoTestWidget : public UUmbraCombatInfo
{
	GENERATED_BODY()
public:
	void Configure(UButton* Attack, UButton* Defense, UTextBlock* AttackText, UTextBlock* DefenseText,
		UVerticalBox* AttackBody, UVerticalBox* DefenseBody)
	{
		AttackToggle = Attack; DefenseToggle = Defense;
		AttackArrow = AttackText; DefenseArrow = DefenseText;
		AttackRows = AttackBody; DefenseRows = DefenseBody;
	}
	void ConstructForTest() { NativeConstruct(); }
	void ConfigureUtility(UButton* Toggle, UTextBlock* Arrow, UVerticalBox* Rows)
	{
		UtilityToggle = Toggle; UtilityArrow = Arrow; UtilityRows = Rows;
	}
	void DestructForTest() { NativeDestruct(); }
};

UCLASS(Transient, NotBlueprintable)
class UUmbraCombatStatEntryTestWidget : public UUmbraCombatStatEntry
{
	GENERATED_BODY()
public:
	void Configure(UTextBlock* Name, UTextBlock* Value, const FText& Label, const FText& TestText)
	{
		StatNameText = Name; StatValueText = Value; StatName = Label; TestValue = TestText;
	}
	void PreConstructForTest() { NativePreConstruct(); }
};
