#include "UI/UmbraMenuTabButton.h"

#include "Components/Button.h"

void UUmbraMenuTabButton::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button_0)
	{
		Button_0->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleButtonClicked);
	}
}

void UUmbraMenuTabButton::NativeDestruct()
{
	if (Button_0)
	{
		Button_0->OnClicked.RemoveDynamic(this, &ThisClass::HandleButtonClicked);
	}
	Super::NativeDestruct();
}

void UUmbraMenuTabButton::HandleButtonClicked()
{
	OnTabClicked.Broadcast(TabIndex);
}

void UUmbraMenuTabButton::SetSelected(bool bInSelected)
{
	bIsSelected = bInSelected;
	// Also initializes the WBP appearance when opening on tab 0 or reusing this widget.
	BP_OnSelectedChanged(bIsSelected);
}
