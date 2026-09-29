#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "UI/Equipment/UmbraEquipmentMenu.h"
#include "UI/Equipment/UmbraEquipmentSlotWidget.h"
#include "UI/Preview/UmbraCharacterPreviewComponent.h"
#include "UI/UmbraCharacterMenu.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraEquipmentSlotTest, "Umbra.UI.Equipment.SlotContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraEquipmentSlotTest::RunTest(const FString& Parameters)
{
	UUmbraEquipmentSlotWidget* Slot = NewObject<UUmbraEquipmentSlotWidget>();
	TestEqual(TEXT("Empty initially"), Slot->GetVisualState(), EUmbraEquipmentSlotState::Empty);
	FUmbraEquipmentItemDisplay Display;
	// Any concrete UObject is sufficient: the widget must not interpret item gameplay data.
	Display.Item = NewObject<UTexture2D>();
	Slot->SetItem(Display);
	TestEqual(TEXT("Item makes equipped"), Slot->GetVisualState(), EUmbraEquipmentSlotState::Equipped);
	Slot->SetHovered(true);
	TestEqual(TEXT("Hover preserves item"), Slot->HasItem(), true);
	TestEqual(TEXT("Hover state"), Slot->GetVisualState(), EUmbraEquipmentSlotState::Hovered);
	Slot->SetSelected(true);
	TestEqual(TEXT("Selection wins hover"), Slot->GetVisualState(), EUmbraEquipmentSlotState::Selected);
	Slot->SetLocked(true);
	Slot->SetSelected(true);
	TestEqual(TEXT("Locked wins selection"), Slot->GetVisualState(), EUmbraEquipmentSlotState::Locked);
	Slot->SetLocked(false);
	Slot->SetHovered(false);
	TestEqual(TEXT("Lock cleared stale selection"), Slot->GetVisualState(), EUmbraEquipmentSlotState::Equipped);
	Slot->ClearItem();
	TestFalse(TEXT("Clear removes reference"), Slot->HasItem());
	Slot->SetItem(FUmbraEquipmentItemDisplay());
	TestEqual(TEXT("Null item is empty"), Slot->GetVisualState(), EUmbraEquipmentSlotState::Empty);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraEquipmentBindingsTest, "Umbra.UI.Equipment.SlotBindings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraEquipmentBindingsTest::RunTest(const FString& Parameters)
{
	UUmbraEquipmentMenu* Menu = NewObject<UUmbraEquipmentMenu>();
	Menu->WidgetTree = NewObject<UWidgetTree>(Menu);
	UVerticalBox* Root = Menu->WidgetTree->ConstructWidget<UVerticalBox>();
	Menu->WidgetTree->RootWidget = Root;
	TArray<UUmbraEquipmentSlotWidget*> Slots;
	for (int32 Index = 0; Index < 10; ++Index)
	{
		UUmbraEquipmentSlotWidget* Slot = Menu->WidgetTree->ConstructWidget<UUmbraEquipmentSlotWidget>();
		Slot->SetSlotType(static_cast<EUmbraEquipmentSlot>(Index));
		Root->AddChild(Slot);
		Slots.Add(Slot);
	}
	Menu->RebuildSlotBindings();
	Menu->RebuildSlotBindings();
	TestEqual(TEXT("Offhand resolved by enum"), Menu->GetEquipmentSlot(EUmbraEquipmentSlot::OffHand), Slots[9]);
	Slots[0]->RequestSelection();
	Slots[1]->RequestSelection();
	TestEqual(TEXT("Only newest selection remains"), Slots[0]->GetVisualState(), EUmbraEquipmentSlotState::Empty);
	TestEqual(TEXT("Selected slot"), Slots[1]->GetVisualState(), EUmbraEquipmentSlotState::Selected);
	Slots[0]->SetLocked(true);
	Slots[0]->RequestSelection();
	TestEqual(TEXT("Locked click does not steal selection"), Slots[1]->GetVisualState(), EUmbraEquipmentSlotState::Selected);
	AddExpectedError(TEXT("duplicate SlotType"), EAutomationExpectedErrorFlags::Contains, 1);
	AddExpectedError(TEXT("8/10 unique slots"), EAutomationExpectedErrorFlags::Contains, 1);
	Slots[9]->SetSlotType(EUmbraEquipmentSlot::Head);
	TestNull(TEXT("Duplicate never silently overwrites"), Menu->GetEquipmentSlot(EUmbraEquipmentSlot::Head));
	TestNull(TEXT("Old key removed after type change"), Menu->GetEquipmentSlot(EUmbraEquipmentSlot::OffHand));
	Slots[9]->SetSlotType(EUmbraEquipmentSlot::OffHand);
	TestEqual(TEXT("Corrected mapping recovers"), Menu->GetEquipmentSlot(EUmbraEquipmentSlot::Head), Slots[0]);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraPreviewLifecycleTest, "Umbra.UI.Equipment.PreviewLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraPreviewLifecycleTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	ACharacter* Player = World->SpawnActor<ACharacter>();
	const FTransform OriginalTransform = Player->GetActorTransform();
	AActor* Actor = World->SpawnActor<AActor>();
	USkeletalMeshComponent* Mesh = NewObject<USkeletalMeshComponent>(Actor);
	Actor->AddInstanceComponent(Mesh);
	Actor->SetRootComponent(Mesh);
	Mesh->RegisterComponent();
	USceneCaptureComponent2D* Capture = NewObject<USceneCaptureComponent2D>(Actor);
	Actor->AddInstanceComponent(Capture);
	Capture->SetupAttachment(Mesh);
	Capture->RegisterComponent();
	UUmbraCharacterPreviewComponent* Preview = NewObject<UUmbraCharacterPreviewComponent>(Actor);
	Actor->AddInstanceComponent(Preview);
	Preview->RegisterComponent();
	FUmbraCharacterPreviewSettings Settings;
	Settings.bUseBlueprintConfiguration = false;
	Settings.RenderTargetTemplate = NewObject<UTextureRenderTarget2D>();
	Settings.RenderTargetTemplate->InitAutoFormat(128, 256);
	AddExpectedError(TEXT("no stable Idle configured"), EAutomationExpectedErrorFlags::Contains, 2);
	TestTrue(TEXT("Adopts independent rig"), Preview->InitializePreview(Player, Mesh, Capture, Settings));
	TestTrue(TEXT("Template duplicated per preview"), Preview->GetRenderTarget() != Settings.RenderTargetTemplate);
	TestEqual(TEXT("Template dimensions kept"), Preview->GetRenderTarget()->SizeY, 256);
	Preview->SetPreviewActive(true);
	TestTrue(TEXT("Opening starts capture"), Preview->IsPreviewActive());
	TestFalse(TEXT("Timer capture does not enable every-frame capture"), Capture->bCaptureEveryFrame);
	Preview->SetPreviewActive(false);
	TestFalse(TEXT("Closing stops capture"), Preview->IsPreviewActive());
	TestFalse(TEXT("Closing stops animation updates"), Mesh->IsComponentTickEnabled());
	TestTrue(TEXT("Real player transform untouched"), Player->GetActorTransform().Equals(OriginalTransform));
	AddExpectedError(TEXT("independent preview actor"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Reject real player mesh"), Preview->InitializePreview(Player, Player->GetMesh(), Capture, Settings));

	// Default path must preserve the editable Blueprint rig and write its dedicated asset RT.
	FUmbraCharacterPreviewSettings BlueprintSettings;
	UTextureRenderTarget2D* AuthoredTarget = NewObject<UTextureRenderTarget2D>();
	AuthoredTarget->InitAutoFormat(256, 512);
	Capture->TextureTarget = AuthoredTarget;
	const FTransform AuthoredCamera(FRotator(-7, 170, 0), FVector(390, 20, 110));
	const FTransform AuthoredCharacter(FRotator(0, -80, 0), FVector(0, 0, 8), FVector(1.1));
	Capture->SetRelativeTransform(AuthoredCamera);
	Capture->FOVAngle = 27.f;
	Mesh->SetRelativeTransform(AuthoredCharacter);
	TestTrue(TEXT("Uses Blueprint defaults"), Preview->InitializePreview(Player, Mesh, Capture, BlueprintSettings));
	TestEqual(TEXT("Writes the authored full-body RT directly"), Preview->GetRenderTarget(), AuthoredTarget);
	TestTrue(TEXT("Keeps Blueprint camera transform"), Capture->GetRelativeTransform().Equals(AuthoredCamera));
	TestTrue(TEXT("Keeps Blueprint mesh transform"), Mesh->GetRelativeTransform().Equals(AuthoredCharacter));
	TestEqual(TEXT("Keeps Blueprint FOV"), Capture->FOVAngle, 27.f);
	Preview->SetPreviewActive(true);
	Preview->SetPreviewActive(false);
	TestFalse(TEXT("Blueprint target also stops capture on close"), Preview->IsPreviewActive());
	TestEqual(TEXT("Closing retains RT assignment"), Capture->TextureTarget.Get(), AuthoredTarget);

	AActor* OtherActor = World->SpawnActor<AActor>();
	USceneCaptureComponent2D* OtherCapture = NewObject<USceneCaptureComponent2D>(OtherActor);
	OtherActor->AddInstanceComponent(OtherCapture);
	OtherCapture->RegisterComponent();
	OtherCapture->TextureTarget = AuthoredTarget;
	AddExpectedError(TEXT("already assigned to another capture"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Cannot overwrite another capture's RT"), Preview->InitializePreview(Player, Mesh, Capture, BlueprintSettings));
	TestEqual(TEXT("Other capture target untouched"), OtherCapture->TextureTarget.Get(), AuthoredTarget);
	Capture->TextureTarget = nullptr;
	AddExpectedError(TEXT("needs a dedicated full-body TextureTarget"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Missing authored RT is diagnosed"), Preview->InitializePreview(Player, Mesh, Capture, BlueprintSettings));
	OtherActor->Destroy();
	Actor->Destroy();
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraEquipmentPageTest, "Umbra.UI.Equipment.PageVisibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraEquipmentPageTest::RunTest(const FString& Parameters)
{
	UUmbraCharacterMenu* Menu = NewObject<UUmbraCharacterMenu>();
	Menu->WidgetTree = NewObject<UWidgetTree>(Menu);
	UWidgetSwitcher* Switcher = Menu->WidgetTree->ConstructWidget<UWidgetSwitcher>();
	Menu->WidgetTree->RootWidget = Switcher;
	UVerticalBox* OtherPage = Menu->WidgetTree->ConstructWidget<UVerticalBox>();
	UUmbraEquipmentMenu* Equipment = Menu->WidgetTree->ConstructWidget<UUmbraEquipmentMenu>();
	Switcher->AddChild(OtherPage);
	Switcher->AddChild(Equipment);
	// Exercise the real cached Slate path: FieldNotify precedes its active-index update.
	const TSharedRef<SWidget> SlateSwitcher = Switcher->TakeWidget();
	Switcher->SetActiveWidgetIndex(0);
	Menu->SetMenuOpen(true);
	TestFalse(TEXT("Other tab keeps preview inactive"), Equipment->IsPageActive());
	Switcher->SetActiveWidgetIndex(1);
	TestTrue(TEXT("Switcher notification activates equipment"), Equipment->IsPageActive());
	Switcher->SetActiveWidgetIndex(0);
	TestFalse(TEXT("Switching away stops preview"), Equipment->IsPageActive());
	Switcher->SetActiveWidgetIndex(1);
	Menu->SetMenuOpen(false);
	TestFalse(TEXT("Menu close stops active page"), Equipment->IsPageActive());
	Menu->SetMenuOpen(true);
	TestTrue(TEXT("Reopen restores active page"), Equipment->IsPageActive());
	Equipment->SetVisibility(ESlateVisibility::Collapsed);
	TestFalse(TEXT("Explicit page collapse also stops preview"), Equipment->IsPageActive());
	Menu->SetMenuOpen(false);
	return true;
}

#endif
