#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/UmbraItemTooltipUITestTypes.h"
#include "Tests/UmbraInventoryTestTypes.h"
#include "Tests/UmbraStatWidgetTestTypes.h"
#include "Misc/AutomationTest.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Stats/UmbraDerivedStatsComponent.h"
#include "Components/UniformGridPanel.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Async/TaskGraphInterfaces.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraTooltipHoverTest, "Umbra.UI.Items.TooltipHover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraTooltipHoverTest::RunTest(const FString& Parameters)
{
	auto* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	auto* PC = World->SpawnActor<APlayerController>();
	PC->SetPlayer(NewObject<ULocalPlayer>(GEngine));
	auto MakeState = [&]()
	{
		auto* State = World->SpawnActor<AUmbraPlayerState>();
		auto* ASC = State->GetUmbraAbilitySystemComponent();
		if (!ASC->HasBeenInitialized()) ASC->InitializeComponent();
		ASC->InitAbilityActorInfo(State, State);
		State->FindComponentByClass<UUmbraDerivedStatsComponent>()->bUseWeaponDerivedPower = true;
		State->GetEquipmentComponent()->bEnableEquipment = true;
		State->InitializeAttributes();
		return State;
	};
	auto* State = MakeState(); PC->SetPlayerState(State);
	auto* Inventory = State->GetInventoryComponent();
	auto* Equipment = State->GetEquipmentComponent();
	auto* ASC = State->GetUmbraAbilitySystemComponent();
	auto Flush = []() { FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread); };
	auto* Menu = CreateWidget<UUmbraInventoryMenuTestWidget>(PC);
	auto* Grid = Menu->WidgetTree->ConstructWidget<UUniformGridPanel>();
	Menu->WidgetTree->RootWidget = Grid;
	Menu->Configure(Grid, Menu->WidgetTree->ConstructWidget<UTextBlock>());
	Menu->bBindInventoryData = true;
	Menu->ItemTooltipClass = UUmbraHoverTooltipTestWidget::StaticClass();
	TSharedPtr<SWidget> MenuSlate = Menu->TakeWidget();
	Menu->ConstructForTest(); Menu->ConstructForTest();
	auto* EquipmentMenu = CreateWidget<UUmbraEquipmentMenuTestWidget>(PC);
	auto* Root = EquipmentMenu->WidgetTree->ConstructWidget<UVerticalBox>();
	EquipmentMenu->WidgetTree->RootWidget = Root;
	for (int32 Index = 0; Index < 10; ++Index)
	{
		auto* Slot = EquipmentMenu->WidgetTree->ConstructWidget<UUmbraEquipmentSlotTestWidget>();
		Slot->SetSlotType(EUmbraEquipmentSlot(Index)); Root->AddChild(Slot);
	}
	EquipmentMenu->ItemTooltipClass = UUmbraHoverTooltipTestWidget::StaticClass();
	TSharedPtr<SWidget> EquipmentSlate = EquipmentMenu->TakeWidget();
	EquipmentMenu->ConstructForTest(); EquipmentMenu->SetPageActive(true);
	auto* Cell = CastChecked<UUmbraInventorySlotTestWidget>(Menu->GetInventorySlot(0));
	auto* MainHand = EquipmentMenu->GetEquipmentSlot(EUmbraEquipmentSlot::MainHand);
	auto Tip = [](UWidget* Slot) { return Cast<UUmbraItemTooltip>(Slot->GetToolTip()); };
	auto Data = [&](UWidget* Slot) { return Tip(Slot) ? Tip(Slot)->GetTooltipData() : FUmbraItemTooltipData(); };
	Cell->EnterForTest(); MainHand->SetHovered(true);
	TestNull(TEXT("Empty inventory has no tooltip"), Cell->GetToolTip());
	TestNull(TEXT("Empty equipment has no tooltip"), MainHand->GetToolTip());
	auto* Item = NewObject<UUmbraItemDefinition>(State);
	TestNull(TEXT("Title background defaults null"), Item->TooltipTitleBackgroundTexture.Get());
	TestNull(TEXT("Icon background defaults null"), Item->TooltipIconBackgroundTexture.Get());
	Item->DisplayName = FText::FromString(TEXT("Hover Sword"));
	Item->AllowedSlots = {EUmbraEquipmentSlot::MainHand};
	Item->Requirements.Strength = 5.f;
	Item->Icon = NewObject<UTexture2D>(Item);
	Item->RarityColor = FLinearColor::Yellow;
	Item->TooltipTitleBackgroundTexture = NewObject<UTexture2D>(Item);
	Item->TooltipIconBackgroundTexture = NewObject<UTexture2D>(Item);
	const auto Id = FGuid::NewGuid(), OtherId = FGuid::NewGuid();
	Inventory->AddItem(Item, Id); Inventory->AddItem(Item, OtherId);
	Cell->EnterForTest();
	TestTrue(TEXT("Real inventory tooltip valid"), Data(Cell).bValid);
	TestEqual(TEXT("Inventory builder source"), Data(Cell).Source, EUmbraTooltipSource::Inventory);
	TestEqual(TEXT("Inventory identity"), Data(Cell).InstanceId, Id);
	TestEqual(TEXT("Inventory definition"), Data(Cell).ItemDefinition.Get(), Item);
	TestEqual(TEXT("Inventory index"), Data(Cell).InventorySlot, 0);
	TestFalse(TEXT("Hover never chooses equipment target"), Data(Cell).bHasTargetSlot);
	TestEqual(TEXT("Inventory title background copied"), Data(Cell).TooltipTitleBackgroundTexture.Get(), Item->TooltipTitleBackgroundTexture.Get());
	TestEqual(TEXT("Inventory icon background copied"), Data(Cell).TooltipIconBackgroundTexture.Get(), Item->TooltipIconBackgroundTexture.Get());
	TestFalse(TEXT("Initial requirements unmet"), Data(Cell).Requirements.bPrimariesMet);
	auto* InitialTooltip = Tip(Cell);
	if (!TestNotNull(TEXT("Tooltip created"), InitialTooltip)) return false;
	auto* Quality = CastChecked<UImage>(InitialTooltip->GetWidgetFromName(TEXT("quality_bg")));
	auto* Background = CastChecked<UImage>(InitialTooltip->GetWidgetFromName(TEXT("SlotBackground")));
	auto* Icon = CastChecked<UImage>(InitialTooltip->GetWidgetFromName(TEXT("ItemIcon")));
	InitialTooltip->SetTooltipData(Data(Cell));
	TestEqual(TEXT("Title background uses texture"), Quality->GetBrush().GetResourceObject(), static_cast<UObject*>(Item->TooltipTitleBackgroundTexture.Get()));
	TestEqual(TEXT("Icon background uses texture"), Background->GetBrush().GetResourceObject(), static_cast<UObject*>(Item->TooltipIconBackgroundTexture.Get()));
	TestEqual(TEXT("Title background untinted"), Quality->GetColorAndOpacity(), FLinearColor::White);
	TestEqual(TEXT("Icon background untinted"), Background->GetColorAndOpacity(), FLinearColor::White);
	TestEqual(TEXT("Real icon"), Icon->GetBrush().GetResourceObject(), static_cast<UObject*>(Item->Icon));
	TestEqual(TEXT("Icon is not background tinted"), Icon->GetColorAndOpacity(), FLinearColor::White);
	TestEqual(TEXT("Rarity text uses text color"), CastChecked<UTextBlock>(InitialTooltip->GetWidgetFromName(TEXT("RarityText")))->GetColorAndOpacity().GetSpecifiedColor(), Item->RarityColor);
	TestNull(TEXT("No RarityFrame required"), InitialTooltip->GetWidgetFromName(TEXT("RarityFrame")));
	auto* TypeText = CastChecked<UTextBlock>(InitialTooltip->GetWidgetFromName(TEXT("ItemTypeText")));
	auto* LevelText = CastChecked<UTextBlock>(InitialTooltip->GetWidgetFromName(TEXT("ItemLevelText")));
	TypeText->SetColorAndOpacity(FLinearColor::Green); LevelText->SetColorAndOpacity(FLinearColor::Blue);
	const auto OriginalData = Data(Cell);
	auto* OtherDefinition = NewObject<UUmbraItemDefinition>(State);
	// OtherDefinition has no background textures configured.
	InitialTooltip->SetTooltipData(UUmbraItemTooltipDataBuilder::FromDefinition(OtherDefinition));
	TestNull(TEXT("Switching to unconfigured restores default title brush"), Quality->GetBrush().GetResourceObject());
	TestNull(TEXT("Switching to unconfigured restores default icon brush"), Background->GetBrush().GetResourceObject());
	TestEqual(TEXT("Type retains Designer color"), TypeText->GetColorAndOpacity().GetSpecifiedColor(), FLinearColor::Green);
	TestEqual(TEXT("Level retains Designer color"), LevelText->GetColorAndOpacity().GetSpecifiedColor(), FLinearColor::Blue);
	InitialTooltip->ClearTooltipData();
	TestNull(TEXT("Clear restores default title brush"), Quality->GetBrush().GetResourceObject());
	TestNull(TEXT("Clear restores default icon brush"), Background->GetBrush().GetResourceObject());
	TestEqual(TEXT("Invalid tooltip collapses"), InitialTooltip->GetVisibility(), ESlateVisibility::Collapsed);
	InitialTooltip->SetTooltipData(OriginalData);
	TestEqual(TEXT("Tooltip remains noninteractive"), InitialTooltip->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestFalse(TEXT("Tooltip cannot focus"), InitialTooltip->IsFocusable());
	const int32 EffectsBefore = ASC->GetActiveEffects(FGameplayEffectQuery()).Num();
	const int32 RevisionBefore = Equipment->GetSnapshot().Revision;
	for (int32 Index = 0; Index < 3; ++Index) { Cell->LeaveForTest(); Cell->EnterForTest(); }
	TestEqual(TEXT("Hover cannot apply GE"), ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), EffectsBefore);
	TestEqual(TEXT("Hover cannot mutate equipment"), Equipment->GetSnapshot().Revision, RevisionBefore);
	TestEqual(TEXT("Hover cannot add/remove inventory"), Inventory->GetSnapshot().Items.Num(), 2);
	auto* OtherCell = CastChecked<UUmbraInventorySlotTestWidget>(Menu->GetInventorySlot(1));
	Cell->LeaveForTest(); OtherCell->EnterForTest();
	TestEqual(TEXT("Same definition different GUID"), Data(OtherCell).InstanceId, OtherId);
	TestNull(TEXT("Leave detaches old tooltip"), Cell->GetToolTip());
	OtherCell->LeaveForTest(); Cell->EnterForTest();
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetStrengthAttribute(), 10.f); Flush();
	TestTrue(TEXT("Primary context refreshes active tooltip"), Data(Cell).Requirements.bPrimariesMet);
	TestEqual(TEXT("Attribute event never rebuilds grid"), Menu->GetInventorySlot(0), static_cast<UUmbraInventorySlot*>(Cell));
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetDexterityAttribute(), 11.f);
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetIntelligenceAttribute(), 12.f);
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetFaithAttribute(), 13.f); Flush();
	TestEqual(TEXT("Dexterity context current"), Data(Cell).Requirements.CurrentPrimaries[1], 11.f);
	TestEqual(TEXT("Intelligence context current"), Data(Cell).Requirements.CurrentPrimaries[2], 12.f);
	TestEqual(TEXT("Faith context current"), Data(Cell).Requirements.CurrentPrimaries[3], 13.f);
	const int32 RowCount = CastChecked<UVerticalBox>(Tip(Cell)->GetWidgetFromName(TEXT("RequirementRows")))->GetChildrenCount();
	Equipment->Refresh(); Flush();
	TestEqual(TEXT("Refresh does not accumulate rows"), CastChecked<UVerticalBox>(Tip(Cell)->GetWidgetFromName(TEXT("RequirementRows")))->GetChildrenCount(), RowCount);
	Item->TooltipTitleBackgroundTexture = NewObject<UTexture2D>(Item);
	Equipment->Refresh(); Flush();
	TestEqual(TEXT("Same definition new texture refreshed"), Data(Cell).TooltipTitleBackgroundTexture.Get(), Item->TooltipTitleBackgroundTexture.Get());
	Equipment->CharacterLevel = 7; Cell->LeaveForTest(); Cell->EnterForTest();
	TestEqual(TEXT("Latest level read on hover"), Data(Cell).Requirements.CurrentLevel, 7);
	Cell->SetItemDefinition(Item, Id); Cell->EnterForTest();
	TestNull(TEXT("Manual inventory preview is not a snapshot"), Cell->GetToolTip());
	Menu->NotifyPlayerContextChanged(); Cell->EnterForTest();
	TestEqual(TEXT("Equip through real menu"), Menu->RequestEquipItem(Id), EUmbraTransferResult::Success);
	TestNull(TEXT("Equip immediately clears inventory tooltip"), Cell->GetToolTip());
	Flush(); MainHand->SetHovered(true);
	TestTrue(TEXT("Equipment tooltip valid"), Data(MainHand).bValid);
	TestEqual(TEXT("Equipment builder source"), Data(MainHand).Source, EUmbraTooltipSource::Equipment);
	TestEqual(TEXT("Same GUID after transfer"), Data(MainHand).InstanceId, Id);
	TestTrue(TEXT("Equipped requirements from snapshot"), Data(MainHand).bRequirementsMet);
	TestEqual(TEXT("Equipment title background copied"), Data(MainHand).TooltipTitleBackgroundTexture.Get(), Item->TooltipTitleBackgroundTexture.Get());
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetStrengthAttribute(), 0.f); Flush();
	TestFalse(TEXT("Equipped requirement updates while hovered"), Data(MainHand).bRequirementsMet);
	while (Inventory->GetSnapshot().Items.Num() < Inventory->GetSnapshot().Capacity) Inventory->AddItem(Item, FGuid::NewGuid());
	const int32 FullEffects = ASC->GetActiveEffects(FGameplayEffectQuery()).Num();
	TestEqual(TEXT("Full bag blocks unequip"), EquipmentMenu->RequestUnequipSlot(MainHand), EUmbraUnequipResult::Failed);
	TestEqual(TEXT("Specific failure"), EquipmentMenu->LastUnequipTransferResult, EUmbraTransferResult::InventoryFull);
	TestTrue(TEXT("Failed unequip preserves tooltip"), Data(MainHand).bValid && Data(MainHand).InstanceId == Id);
	TestEqual(TEXT("Failed unequip preserves effects"), ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), FullEffects);
	FUmbraInventoryItem Removed;
	Inventory->RemoveItem(OtherId, Removed);
	TestEqual(TEXT("Unequip through real menu"), EquipmentMenu->RequestUnequipSlot(MainHand), EUmbraUnequipResult::Success);
	TestNull(TEXT("Unequip immediately clears tooltip"), MainHand->GetToolTip()); Flush();
	OtherCell->EnterForTest();
	TestEqual(TEXT("Returned GUID tooltip"), Data(OtherCell).InstanceId, Id);
	const auto NewId = FGuid::NewGuid();
	Inventory->RemoveItem(Id, Removed);
	TestNull(TEXT("Removal invalidates open tooltip"), OtherCell->GetToolTip());
	Inventory->AddItem(Item, NewId);
	TestEqual(TEXT("Replacement GUID same cell"), Data(OtherCell).InstanceId, NewId);
	Equipment->Equip(EUmbraEquipmentSlot::MainHand, Item, Id); Flush(); MainHand->SetHovered(true);
	const auto ReplacementId = FGuid::NewGuid();
	Equipment->Equip(EUmbraEquipmentSlot::MainHand, Item, ReplacementId); Flush();
	TestEqual(TEXT("Equipment replacement refreshes identity"), Data(MainHand).InstanceId, ReplacementId);
	// Inject malformed replicated state through reflection; no production test-only setters.
	auto* InventoryProperty = FindFProperty<FStructProperty>(Inventory->GetClass(), TEXT("Snapshot"));
	auto* InventorySnapshot = InventoryProperty->ContainerPtrToValuePtr<FUmbraInventorySnapshot>(Inventory);
	const auto SavedInventory = *InventorySnapshot;
	const auto Duplicate = InventorySnapshot->Items[0]; InventorySnapshot->Items.Add(Duplicate);
	Inventory->OnInventoryChanged.Broadcast(*InventorySnapshot);
	TestNull(TEXT("Invalid inventory snapshot clears tooltip"), OtherCell->GetToolTip());
	*InventorySnapshot = SavedInventory; Inventory->OnInventoryChanged.Broadcast(*InventorySnapshot);
	auto* EquipmentProperty = FindFProperty<FStructProperty>(Equipment->GetClass(), TEXT("Snapshot"));
	auto* EquipmentSnapshot = EquipmentProperty->ContainerPtrToValuePtr<FUmbraEquipmentSnapshot>(Equipment);
	const auto SavedEquipment = *EquipmentSnapshot;
	EquipmentSnapshot->bValid = false; Equipment->OnEquipmentChanged.Broadcast(*EquipmentSnapshot); Flush();
	TestNull(TEXT("Invalid equipment clears tooltip"), MainHand->GetToolTip());
	*EquipmentSnapshot = SavedEquipment; Equipment->OnEquipmentChanged.Broadcast(*EquipmentSnapshot); Flush();
	Cell = CastChecked<UUmbraInventorySlotTestWidget>(Menu->GetInventorySlot(0));
	Cell->EnterForTest(); MainHand->SetHovered(true);
	Menu->SetPageActive(false); EquipmentMenu->SetPageActive(false); Flush();
	TestNull(TEXT("Inventory page close clears"), Cell->GetToolTip());
	TestNull(TEXT("Equipment page close clears"), MainHand->GetToolTip());
	Menu->SetPageActive(true); EquipmentMenu->SetPageActive(true);
	Cell->EnterForTest(); MainHand->SetHovered(true);
	auto* ReplacementState = MakeState(); PC->SetPlayerState(ReplacementState);
	Menu->NotifyPlayerContextChanged(); EquipmentMenu->NotifyPlayerContextChanged(); Flush();
	TestNull(TEXT("PS change clears inventory tooltip"), Cell->GetToolTip());
	TestNull(TEXT("PS change clears equipment tooltip"), MainHand->GetToolTip());
	PC->SetPlayerState(State); Menu->NotifyPlayerContextChanged(); EquipmentMenu->NotifyPlayerContextChanged();
	Cell->EnterForTest(); MainHand->SetHovered(true);
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetStrengthAttribute(), 9.f);
	Menu->DestructForTest(); EquipmentMenu->DestructForTest(); Flush();
	TestNull(TEXT("Queued event cannot reopen destructed menu"), Cell->GetToolTip());
	TestNull(TEXT("Equipment destruct clears reference"), MainHand->GetToolTip());
	Menu->ConstructForTest(); Menu->ConstructForTest();
	EquipmentMenu->ConstructForTest(); EquipmentMenu->ConstructForTest(); EquipmentMenu->SetPageActive(true);
	Cell = CastChecked<UUmbraInventorySlotTestWidget>(Menu->GetInventorySlot(0));
	Cell->EnterForTest(); MainHand->SetHovered(true);
	TestEqual(TEXT("Inventory hover subscribed exactly once"), Cell->OnTooltipHoverChanged.RemoveAll(Menu), 1);
	TestEqual(TEXT("Equipment hover subscribed exactly once"), MainHand->OnTooltipHoverChanged.RemoveAll(EquipmentMenu), 1);
	Menu->ConstructForTest(); EquipmentMenu->ConstructForTest(); EquipmentMenu->SetPageActive(true);
	Cell->EnterForTest(); MainHand->SetHovered(true);
	Inventory->UnregisterComponent();
	TestNull(TEXT("Inventory unregister invalidates"), Cell->GetToolTip());
	Equipment->UnregisterComponent(); Flush();
	TestNull(TEXT("Equipment unregister invalidates"), MainHand->GetToolTip());
	MenuSlate.Reset(); EquipmentSlate.Reset(); Flush();
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
#endif
