#include "UI/UmbraAttributeMenu.h"

#include "Components/WidgetSwitcher.h"
#include "UI/UmbraMenuTabButton.h"

void UUmbraAttributeMenu::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button_page1)
	{
		Button_page1->OnTabClicked.AddUniqueDynamic(this, &ThisClass::HandleTabClicked);
	}
	if (Button_page2)
	{
		Button_page2->OnTabClicked.AddUniqueDynamic(this, &ThisClass::HandleTabClicked);
	}
	if (Button_page3)
	{
		Button_page3->OnTabClicked.AddUniqueDynamic(this, &ThisClass::HandleTabClicked);
	}
	SetActiveTab(0);
}

void UUmbraAttributeMenu::NativeDestruct()
{
	if (Button_page1)
	{
		Button_page1->OnTabClicked.RemoveDynamic(this, &ThisClass::HandleTabClicked);
	}
	if (Button_page2)
	{
		Button_page2->OnTabClicked.RemoveDynamic(this, &ThisClass::HandleTabClicked);
	}
	if (Button_page3)
	{
		Button_page3->OnTabClicked.RemoveDynamic(this, &ThisClass::HandleTabClicked);
	}
	Super::NativeDestruct();
}

void UUmbraAttributeMenu::HandleTabClicked(int32 Index)
{
	SetActiveTab(Index);
}

void UUmbraAttributeMenu::SetActiveTab(int32 Index)
{
	// Only these three buttons have matching pages; an invalid index must not change selection.
	if (!WidgetSwitcher_0 || !Button_page1 || !Button_page2 || !Button_page3 ||
		Index < 0 || Index > 2 || Index >= WidgetSwitcher_0->GetNumWidgets())
	{
		return;
	}

	WidgetSwitcher_0->SetActiveWidgetIndex(Index);
	Button_page1->SetSelected(Index == 0);
	Button_page2->SetSelected(Index == 1);
	Button_page3->SetSelected(Index == 2);
	CurrentTabIndex = Index;
}
