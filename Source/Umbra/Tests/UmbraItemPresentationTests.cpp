#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/UmbraStatWidgetTestTypes.h"
#include "Tests/UmbraInventoryTestTypes.h"
#include "Items/UmbraItemDefinition.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraItemPresentationTest, "Umbra.UI.Items.Presentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraItemPresentationTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	auto* PC = World->SpawnActor<APlayerController>();
	PC->SetPlayer(NewObject<ULocalPlayer>(GEngine));
	auto* Definition = NewObject<UUmbraItemDefinition>();
	Definition->Icon = NewObject<UTexture2D>(Definition);
	Definition->SlotBackgroundTexture = NewObject<UTexture2D>(Definition);
	Definition->RarityFrameTexture = NewObject<UTexture2D>(Definition);
	Definition->RarityColor = FLinearColor::Red;
	auto* Plain = NewObject<UUmbraItemDefinition>();
	Plain->Icon = NewObject<UTexture2D>(Plain);
	auto* Equipment = CreateWidget<UUmbraEquipmentSlotTestWidget>(PC);
	auto* Inventory = CreateWidget<UUmbraInventorySlotTestWidget>(PC);
	for (int32 Index = 0; Index < 2; ++Index)
	{
		auto* Background = NewObject<UImage>();
		auto* Icon = NewObject<UImage>();
		auto* Frame = NewObject<UImage>();
		auto* EmptyTexture = NewObject<UTexture2D>();
		Background->SetBrushFromTexture(EmptyTexture);
		Background->SetColorAndOpacity(FLinearColor::Gray);
		auto Set = [&](UUmbraItemDefinition* Item)
		{
			if (Index == 0) Equipment->SetItem(FUmbraEquipmentItemDisplay::FromDefinition(Item));
			else Inventory->SetItemDefinition(Item, FGuid::NewGuid());
		};
		if (Index == 0)
		{
			Equipment->Configure(nullptr, Icon); Equipment->SetBackgroundForTest(Background); Equipment->SetRarityFrameForTest(Frame);
			Equipment->SetSlotType(EUmbraEquipmentSlot::MainHand);
		}
		else Inventory->Configure(Background, Icon, Frame, nullptr, nullptr, nullptr);
		Set(Definition);
		TestEqual(TEXT("Both slot families show item icon"), Icon->GetBrush().GetResourceObject(), static_cast<UObject*>(Definition->Icon));
		TestEqual(TEXT("Item icon visible"), Icon->GetVisibility(), ESlateVisibility::HitTestInvisible);
		TestEqual(TEXT("Quality background comes from item"), Background->GetBrush().GetResourceObject(), static_cast<UObject*>(Definition->SlotBackgroundTexture));
		TestEqual(TEXT("Quality frame comes from item"), Frame->GetBrush().GetResourceObject(), static_cast<UObject*>(Definition->RarityFrameTexture));
		TestTrue(TEXT("Texture art is not multiplied by rarity color"), Frame->GetColorAndOpacity() == FLinearColor::White);
		Set(Plain);
		TestEqual(TEXT("Replacement without quality art restores default background"), Background->GetBrush().GetResourceObject(), static_cast<UObject*>(EmptyTexture));
		TestTrue(TEXT("Default background tint restored"), Background->GetColorAndOpacity() == FLinearColor::Gray);
		TestNull(TEXT("Previous frame does not leak"), Frame->GetBrush().GetResourceObject());
		Set(Definition);
		if (Index == 0) { Equipment->SetHovered(true); Equipment->SetSelected(true); Equipment->SetLocked(true); Equipment->ClearItem(); }
		else { Inventory->EnterForTest(); Inventory->ClearItem(); }
		TestEqual(TEXT("Clear restores empty background"), Background->GetBrush().GetResourceObject(), static_cast<UObject*>(EmptyTexture));
		TestEqual(TEXT("Clear hides icon"), Icon->GetVisibility(), ESlateVisibility::Collapsed);
		TestEqual(TEXT("Clear hides rarity"), Frame->GetVisibility(), ESlateVisibility::Collapsed);
	}

	// Load the project's real saved widget to catch name/type mismatches and legacy visual graphs.
	auto* Class = LoadClass<UUmbraEquipmentSlotWidget>(nullptr, TEXT("/Game/UI/CharacterMenu/EquipmentMenu/WBP_EquipmentSlot.WBP_EquipmentSlot_C"));
	if (TestNotNull(TEXT("Saved equipment slot class"), Class))
	{
		auto* Actual = CreateWidget<UUmbraEquipmentSlotWidget>(PC, Class);
		Actual->TakeWidget();
		Actual->SetSlotType(EUmbraEquipmentSlot::MainHand);
		Actual->SetItem(FUmbraEquipmentItemDisplay::FromDefinition(Definition));
		auto* Icon = Cast<UImage>(Actual->WidgetTree->FindWidget(TEXT("ItemIcon")));
		if (TestNotNull(TEXT("Real WBP requires UImage named ItemIcon"), Icon))
		{
			TestEqual(TEXT("Actual main-hand icon survives Blueprint refresh"), Icon->GetBrush().GetResourceObject(), static_cast<UObject*>(Definition->Icon));
			TestEqual(TEXT("Actual main-hand icon visible"), Icon->GetVisibility(), ESlateVisibility::HitTestInvisible);
		}
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
