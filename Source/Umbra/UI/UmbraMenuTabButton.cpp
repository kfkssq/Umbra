#include "UI/UmbraMenuTabButton.h"
#include "Audio/UmbraAudioFunctionLibrary.h"

#include "Components/Button.h"

void UUmbraMenuTabButton::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button_0)
	{
		Button_0->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleButtonClicked);
		Button_0->OnHovered.AddUniqueDynamic(this, &ThisClass::HandleButtonHovered);
		Button_0->OnUnhovered.AddUniqueDynamic(this, &ThisClass::HandleButtonUnhovered);
		// Native event mapping owns these two sounds, including imported button styles.
		auto Style = Button_0->GetStyle();
		Style.HoveredSlateSound = FSlateSound();
		Style.PressedSlateSound = FSlateSound();
		Button_0->SetStyle(Style);
	}
}

void UUmbraMenuTabButton::NativeDestruct()
{
	if (Button_0)
	{
		Button_0->OnClicked.RemoveDynamic(this, &ThisClass::HandleButtonClicked);
		Button_0->OnHovered.RemoveDynamic(this, &ThisClass::HandleButtonHovered);
		Button_0->OnUnhovered.RemoveDynamic(this, &ThisClass::HandleButtonUnhovered);
	}
	Super::NativeDestruct();
	bAudioHovered = false;
}

void UUmbraMenuTabButton::HandleButtonClicked()
{
	UUmbraAudioFunctionLibrary::PlayUI(GetOwningPlayer(), EUmbraAudioEvent::UIClick, {});
	OnTabClicked.Broadcast(TabIndex);
}

void UUmbraMenuTabButton::HandleButtonHovered()
{
	if (!bAudioHovered) UUmbraAudioFunctionLibrary::PlayUI(GetOwningPlayer(), EUmbraAudioEvent::UIHover, {});
	bAudioHovered = true;
}

void UUmbraMenuTabButton::HandleButtonUnhovered() { bAudioHovered = false; }

void UUmbraMenuTabButton::SetSelected(bool bInSelected)
{
	bIsSelected = bInSelected;
	// Also initializes the WBP appearance when opening on tab 0 or reusing this widget.
	BP_OnSelectedChanged(bIsSelected);
}
