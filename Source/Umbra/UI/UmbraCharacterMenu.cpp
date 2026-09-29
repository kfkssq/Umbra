#include "UI/UmbraCharacterMenu.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/WidgetSwitcher.h"
#include "UI/Equipment/UmbraEquipmentMenu.h"

namespace
{
	bool HasVisibleState(const UWidget* Widget)
	{
		const ESlateVisibility State = Widget->GetVisibility();
		return State != ESlateVisibility::Hidden && State != ESlateVisibility::Collapsed;
	}
}

void UUmbraCharacterMenu::NativeConstruct()
{
	Super::NativeConstruct();
	OnVisibilityChanged.AddUniqueDynamic(this, &ThisClass::HandleMenuVisibility);
	RefreshEquipmentPages();
}

void UUmbraCharacterMenu::NativeDestruct()
{
	SetMenuOpen(false);
	UnbindPageEvents();
	OnVisibilityChanged.RemoveDynamic(this, &ThisClass::HandleMenuVisibility);
	Super::NativeDestruct();
}

void UUmbraCharacterMenu::SetMenuOpen(bool bOpen)
{
	bMenuOpen = bOpen;
	RefreshEquipmentPages();
}

void UUmbraCharacterMenu::HandleMenuVisibility(ESlateVisibility InVisibility)
{
	RefreshEquipmentPages();
}

void UUmbraCharacterMenu::UnbindPageEvents()
{
	for (const TWeakObjectPtr<UWidget>& Widget : ObservedWidgets)
	{
		if (Widget.IsValid()) Widget->RemoveAllFieldValueChangedDelegates(this);
	}
	ObservedWidgets.Reset();
}

void UUmbraCharacterMenu::HandlePageFieldChanged(UObject* Object, UE::FieldNotification::FFieldId Field)
{
	RefreshEquipmentPages();
}

void UUmbraCharacterMenu::RefreshEquipmentPages()
{
	if (IsDesignTime()) return;
	UnbindPageEvents();
	if (WidgetTree) VisitWidget(WidgetTree->RootWidget, bMenuOpen && HasVisibleState(this));
}

void UUmbraCharacterMenu::VisitWidget(UWidget* Widget, bool bAncestorsVisible)
{
	if (!Widget) return;
	ObservedWidgets.Add(Widget);
	Widget->AddFieldValueChangedDelegate(UWidget::FFieldNotificationClassDescriptor::Visibility,
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &ThisClass::HandlePageFieldChanged));
	const bool bVisible = bAncestorsVisible && HasVisibleState(Widget);
	if (UUmbraEquipmentMenu* Equipment = Cast<UUmbraEquipmentMenu>(Widget)) Equipment->SetPageActive(bVisible);
	if (UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
	{
		if (UserWidget->WidgetTree) VisitWidget(UserWidget->WidgetTree->RootWidget, bVisible);
	}
	else if (UWidgetSwitcher* Switcher = Cast<UWidgetSwitcher>(Widget))
	{
		Switcher->AddFieldValueChangedDelegate(UWidgetSwitcher::FFieldNotificationClassDescriptor::ActiveWidgetIndex,
			INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &ThisClass::HandlePageFieldChanged));
		// UE 5.8 broadcasts FieldNotify BEFORE updating SWidgetSwitcher. The getter reads
		// that stale Slate index during this callback; use the just-written UMG value.
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const int32 Index = FMath::Clamp(Switcher->ActiveWidgetIndex, 0, FMath::Max(0, Switcher->GetNumWidgets() - 1));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
		for (UWidget* Child : Switcher->GetAllChildren()) VisitWidget(Child, bVisible && Child == Switcher->GetWidgetAtIndex(Index));
	}
	else if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
	{
		for (UWidget* Child : Panel->GetAllChildren()) VisitWidget(Child, bVisible);
	}
}

