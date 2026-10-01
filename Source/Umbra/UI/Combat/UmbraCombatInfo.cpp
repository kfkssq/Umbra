#include "UI/Combat/UmbraCombatInfo.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

void UUmbraCombatInfo::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshSections();
}

void UUmbraCombatInfo::NativeConstruct()
{
	Super::NativeConstruct();
	if (AttackToggle) AttackToggle->OnClicked.AddUniqueDynamic(this, &ThisClass::ToggleAttack);
	if (DefenseToggle) DefenseToggle->OnClicked.AddUniqueDynamic(this, &ThisClass::ToggleDefense);
	RefreshSections();
}

void UUmbraCombatInfo::NativeDestruct()
{
	if (AttackToggle) AttackToggle->OnClicked.RemoveDynamic(this, &ThisClass::ToggleAttack);
	if (DefenseToggle) DefenseToggle->OnClicked.RemoveDynamic(this, &ThisClass::ToggleDefense);
	Super::NativeDestruct();
}

void UUmbraCombatInfo::ToggleAttack()
{
	bAttackExpanded = !bAttackExpanded;
	RefreshSections();
}

void UUmbraCombatInfo::ToggleDefense()
{
	bDefenseExpanded = !bDefenseExpanded;
	RefreshSections();
}

void UUmbraCombatInfo::RefreshSections()
{
	if (AttackRows) AttackRows->SetVisibility(bAttackExpanded ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (DefenseRows) DefenseRows->SetVisibility(bDefenseExpanded ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (AttackArrow) AttackArrow->SetText(FText::FromString(bAttackExpanded ? TEXT("\u25bc") : TEXT("\u25b6")));
	if (DefenseArrow) DefenseArrow->SetText(FText::FromString(bDefenseExpanded ? TEXT("\u25bc") : TEXT("\u25b6")));
}
