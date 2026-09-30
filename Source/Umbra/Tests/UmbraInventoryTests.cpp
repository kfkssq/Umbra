#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/UmbraInventoryTestTypes.h"
#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/Equipment/UmbraEquipmentMenu.h"
#include "UI/UmbraCharacterMenu.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraInventoryTest, "Umbra.UI.Inventory.EmptyGridLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraInventoryTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	APlayerController* PC = World->SpawnActor<APlayerController>();
	PC->SetPlayer(NewObject<ULocalPlayer>(GEngine));
	auto* Menu = CreateWidget<UUmbraInventoryMenuTestWidget>(PC);
	Menu->WidgetTree = NewObject<UWidgetTree>(Menu);
	auto* Grid = Menu->WidgetTree->ConstructWidget<UUniformGridPanel>();
	Menu->WidgetTree->RootWidget = Grid;
	auto* Text = Menu->WidgetTree->ConstructWidget<UTextBlock>();
	Menu->Configure(Grid, Text);
	Menu->ConstructForTest();
	TestEqual(TEXT("Default count"), Menu->GetGeneratedSlotCount(), 40);
	TestEqual(TEXT("Default capacity text"), Text->GetText().ToString(), FString(TEXT("0/40")));
	TestEqual(TEXT("No initial selection"), Menu->GetSelectedSlotIndex(), INDEX_NONE);
	for (int32 Index = 0; Index < 40; ++Index)
	{
		auto* Cell = Menu->GetInventorySlot(Index);
		if (!TestNotNull(TEXT("Generated cell"), Cell)) continue;
		TestEqual(TEXT("Stable slot index"), Cell->GetSlotIndex(), Index);
		auto* Layout = Cast<UUniformGridSlot>(Cell->Slot);
		if (!TestNotNull(TEXT("Uniform grid layout"), Layout)) continue;
		TestEqual(TEXT("Row"), Layout->GetRow(), Index / 8);
		TestEqual(TEXT("Column"), Layout->GetColumn(), Index % 8);
	}
	auto* First = Cast<UUmbraInventorySlotTestWidget>(Menu->GetInventorySlot(0));
	auto* Second = Menu->GetInventorySlot(1);
	if (TestNotNull(TEXT("First cell"), First) && TestNotNull(TEXT("Second cell"), Second))
	{
		auto* Background = NewObject<UImage>(First);
		auto* Icon = NewObject<UImage>(First);
		auto* Rarity = NewObject<UImage>(First);
		auto* Count = NewObject<UTextBlock>(First);
		auto* Equipped = NewObject<UImage>(First);
		auto* Highlight = NewObject<UImage>(First);
		First->Configure(Background, Icon, Rarity, Count, Equipped, Highlight);
		First->RefreshVisual();
		TestEqual(TEXT("Normal highlight hidden"), Highlight->GetVisibility(), ESlateVisibility::Collapsed);
		First->EnterForTest();
		TestTrue(TEXT("Hover highlights"), First->ShouldHighlight());
		TestEqual(TEXT("Hover does not intercept pointer"), Highlight->GetVisibility(), ESlateVisibility::HitTestInvisible);
		First->LeaveForTest();
		TestFalse(TEXT("Leave clears unselected highlight"), First->ShouldHighlight());
		First->RequestSelection();
		First->EnterForTest();
		First->LeaveForTest();
		TestTrue(TEXT("Selected persists after leave"), First->ShouldHighlight());
		Second->RequestSelection();
		Second->RequestSelection();
		TestFalse(TEXT("Old selection cleared"), First->IsSelected());
		TestTrue(TEXT("New selection retained"), Second->IsSelected());
		TestEqual(TEXT("Menu owns selection"), Menu->GetSelectedSlotIndex(), 1);
		TestEqual(TEXT("Background visible"), Background->GetVisibility(), ESlateVisibility::Visible);
		for (UWidget* Decoration : TArray<UWidget*>{Icon, Rarity, Count, Equipped})
		{
			TestEqual(TEXT("Empty layers remain collapsed across interactions"), Decoration->GetVisibility(), ESlateVisibility::Collapsed);
		}
		for (int32 Repeat = 0; Repeat < 3; ++Repeat)
		{
			Menu->SetVisibility(ESlateVisibility::Collapsed);
			Menu->SetVisibility(ESlateVisibility::Visible);
			Menu->DestructForTest();
			First->RequestSelection();
			TestEqual(TEXT("Destruct unbinds selection"), Menu->GetSelectedSlotIndex(), 1);
			Menu->ConstructForTest();
			Menu->InitializeSlots();
			TestEqual(TEXT("Reopen does not append"), Grid->GetChildrenCount(), 40);
			TestEqual(TEXT("Reopen reuses same cell"), Menu->GetInventorySlot(0), static_cast<UUmbraInventorySlot*>(First));
			First->RequestSelection();
			TestEqual(TEXT("Reopen restores binding"), Menu->GetSelectedSlotIndex(), 0);
			Second->RequestSelection();
		}
	}
	Menu->Configure(Grid, Text, 10, 3);
	Menu->InitializeSlots();
	TestEqual(TEXT("Configuration rebuild replaces grid"), Grid->GetChildrenCount(), 10);
	TestEqual(TEXT("Rebuild clears selection"), Menu->GetSelectedSlotIndex(), INDEX_NONE);
	TestEqual(TEXT("Configured capacity"), Text->GetText().ToString(), FString(TEXT("0/10")));
	Menu->Configure(Grid, Text, 2, 0);
	Menu->InitializeSlots();
	auto* Last = Cast<UUniformGridSlot>(Menu->GetInventorySlot(1)->Slot);
	TestEqual(TEXT("Invalid columns clamp to one"), Last->GetRow(), 1);
	TestEqual(TEXT("Clamped column"), Last->GetColumn(), 0);
	Menu->Configure(Grid, Text, -1, 8);
	Menu->InitializeSlots();
	TestEqual(TEXT("Negative capacity becomes zero"), Grid->GetChildrenCount(), 0);
	TestEqual(TEXT("Zero capacity label"), Text->GetText().ToString(), FString(TEXT("0/0")));
	TestNull(TEXT("Invalid index safe"), Menu->GetInventorySlot(-1));
	Menu->ClearClassForTest();
	AddExpectedError(TEXT("needs InventoryGrid"), EAutomationExpectedErrorFlags::Contains, 1);
	Menu->InitializeSlots();
	TestEqual(TEXT("Missing class creates no fallback cells"), Grid->GetChildrenCount(), 0);
	Menu->DestructForTest();
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraInventoryCompositionTest, "Umbra.UI.Inventory.CharacterMenuComposition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraInventoryCompositionTest::RunTest(const FString& Parameters)
{
	auto* Menu = NewObject<UUmbraCharacterMenu>();
	Menu->WidgetTree = NewObject<UWidgetTree>(Menu);
	auto* Row = Menu->WidgetTree->ConstructWidget<UHorizontalBox>();
	Menu->WidgetTree->RootWidget = Row;
	auto* Attribute = Menu->WidgetTree->ConstructWidget<UVerticalBox>();
	auto* Equipment = Menu->WidgetTree->ConstructWidget<UUmbraEquipmentMenu>();
	auto* Inventory = Menu->WidgetTree->ConstructWidget<UUmbraInventoryMenu>();
	Row->AddChild(Attribute);
	Row->AddChild(Equipment);
	Row->AddChild(Inventory);
	Attribute->SetVisibility(ESlateVisibility::Visible);
	Equipment->SetVisibility(ESlateVisibility::Visible);
	Inventory->SetVisibility(ESlateVisibility::Visible);
	Menu->SetMenuOpen(true);
	// This fixture has no Slate widgets; verify UMG state, not Slate IsVisible().
	TestEqual(TEXT("Attribute remains visible beside inventory"), Attribute->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Equipment remains visible beside inventory"), Equipment->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Inventory is simultaneously visible"), Inventory->GetVisibility(), ESlateVisibility::Visible);
	TestTrue(TEXT("Equipment preview stays active with inventory present"), Equipment->IsPageActive());
	Inventory->SetVisibility(ESlateVisibility::Collapsed);
	TestTrue(TEXT("Hiding inventory does not stop equipment preview"), Equipment->IsPageActive());
	Inventory->SetVisibility(ESlateVisibility::Visible);
	Menu->SetMenuOpen(false);
	TestFalse(TEXT("Closing the entire menu stops preview"), Equipment->IsPageActive());
	Menu->SetMenuOpen(true);
	TestTrue(TEXT("Reopening restores equipment preview"), Equipment->IsPageActive());
	TestEqual(TEXT("All three sibling panels are retained"), Row->GetChildrenCount(), 3);
	TestEqual(TEXT("Inventory still occupies the third column"), Row->GetChildAt(2), static_cast<UWidget*>(Inventory));
	Menu->SetMenuOpen(false);
	return true;
}

#endif
