#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/UmbraStatWidgetTestTypes.h"
#include "Misc/AutomationTest.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/Effects/UmbraDebugEffects.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Player/UmbraPlayerState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraEmptySlotIconsTest, "Umbra.UI.Equipment.EmptySlotIcons",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraEmptySlotIconsTest::RunTest(const FString& Parameters)
{
	auto* Slot = NewObject<UUmbraEquipmentSlotTestWidget>();
	auto* Empty = NewObject<UImage>(Slot);
	auto* Item = NewObject<UImage>(Slot);
	Slot->Configure(Empty, Item);
	auto* Rarity = NewObject<UImage>(Slot);
	Slot->SetRarityFrameForTest(Rarity);
	TArray<UTexture2D*> Textures;
	for (int32 Index = 0; Index < 10; ++Index)
	{
		Textures.Add(NewObject<UTexture2D>());
		Slot->SetEmptyIconForTest(static_cast<EUmbraEquipmentSlot>(Index), Textures.Last());
	}
	for (int32 Index = 0; Index < 10; ++Index)
	{
		Slot->SetSlotType(static_cast<EUmbraEquipmentSlot>(Index));
		TestEqual(TEXT("Each type resolves its own icon"), Empty->GetBrush().GetResourceObject(), static_cast<UObject*>(Textures[Index]));
		TestEqual(TEXT("Empty visible"), Empty->GetVisibility(), ESlateVisibility::HitTestInvisible);
		TestEqual(TEXT("Item hidden"), Item->GetVisibility(), ESlateVisibility::Collapsed);
		Slot->SetHovered(true);
		Slot->SetSelected(true);
		TestEqual(TEXT("Empty rarity hidden regardless of type and interaction"), Rarity->GetVisibility(), ESlateVisibility::Collapsed);
	}
	FUmbraEquipmentItemDisplay Display;
	Display.Item = NewObject<UTexture2D>();
	Display.Icon.SetResourceObject(Textures[0]);
	Slot->SetItem(Display);
	Slot->SetHovered(true);
	Slot->SetSelected(true);
	Slot->SetLocked(true);
	TestEqual(TEXT("Lock preserves equipped icon"), Item->GetBrush().GetResourceObject(), Display.Icon.GetResourceObject());
	TestEqual(TEXT("Equipped hides empty"), Empty->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Equipped visible even when locked"), Item->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Existing item rarity still visible"), Rarity->GetVisibility(), ESlateVisibility::HitTestInvisible);
	Slot->ClearItem();
	TestEqual(TEXT("Clear collapses rarity while locked"), Rarity->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Clearing locked item restores current slot icon"), Empty->GetBrush().GetResourceObject(), static_cast<UObject*>(Textures[9]));
	TestEqual(TEXT("Lock state unchanged"), Slot->GetVisualState(), EUmbraEquipmentSlotState::Locked);
	Slot->SetEmptyIconForTest(EUmbraEquipmentSlot::Head, nullptr);
	Slot->SetSlotType(EUmbraEquipmentSlot::Head);
	TestNull(TEXT("Missing mapping clears stale brush"), Empty->GetBrush().GetResourceObject());
	TestEqual(TEXT("Missing mapping hidden"), Empty->GetVisibility(), ESlateVisibility::Collapsed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraPrimaryStatRowsTest, "Umbra.UI.CharacterStats.PrimaryRows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraPrimaryStatRowsTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	APlayerController* PC = World->SpawnActor<APlayerController>();
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	PC->SetPlayer(LocalPlayer);
	AUmbraPlayerState* Owner = World->SpawnActor<AUmbraPlayerState>();
	PC->SetPlayerState(Owner);
	auto* ASC = Owner->GetUmbraAbilitySystemComponent();
	ASC->InitAbilityActorInfo(Owner, Owner);
	ASC->InitializeAttributes(nullptr);
	auto* Panel = NewObject<UUmbraStatsPanelTestWidget>(PC);
	Panel->SetOwningPlayer(PC);
	TestEqual(TEXT("Fixture has a registered owning player"), Panel->GetOwningPlayer(), PC);
	TestEqual(TEXT("Fixture resolves PlayerState"), Panel->GetOwningPlayerState<AUmbraPlayerState>(), Owner);
	Panel->WidgetTree = NewObject<UWidgetTree>(Panel);
	auto* Root = Panel->WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->WidgetTree->RootWidget = Root;
	const EUmbraCharacterStat Stats[] = { EUmbraCharacterStat::Strength, EUmbraCharacterStat::Dexterity,
		EUmbraCharacterStat::Intelligence, EUmbraCharacterStat::Faith };
	const FGameplayAttribute Attributes[] = { UUmbraAttributeSet::GetStrengthAttribute(), UUmbraAttributeSet::GetDexterityAttribute(),
		UUmbraAttributeSet::GetIntelligenceAttribute(), UUmbraAttributeSet::GetFaithAttribute() };
	TArray<UUmbraStatEntryTestWidget*> Rows;
	TArray<UTextBlock*> Values;
	TArray<UImage*> Icons;
	TArray<UTextBlock*> Names;
	auto* IconTexture = NewObject<UTexture2D>();
	for (int32 Index = 0; Index < 4; ++Index)
	{
		auto* Row = Panel->WidgetTree->ConstructWidget<UUmbraStatEntryTestWidget>();
		auto* Icon = NewObject<UImage>(Row);
		auto* Name = NewObject<UTextBlock>(Row);
		auto* Value = NewObject<UTextBlock>(Row);
		Row->Configure(Stats[Index], Icon, Name, Value);
		Panel->SetIconForTest(Stats[Index], IconTexture);
		Root->AddChild(Row);
		Rows.Add(Row);
		Values.Add(Value);
		Icons.Add(Icon);
		Names.Add(Name);
		ASC->SetNumericAttributeBase(Attributes[Index], 10.f + Index);
	}
	Panel->ConstructForTest();
	Panel->NotifyPlayerContextChanged();
	Panel->NotifyPlayerContextChanged();
	for (int32 Index = 0; Index < 4; ++Index)
	{
		TestEqual(TEXT("Initial current GAS value"), Values[Index]->GetText().ToString(), FText::AsNumber(10 + Index).ToString());
		TestTrue(TEXT("Parent owns listener"), ASC->GetGameplayAttributeValueChangeDelegate(Attributes[Index]).IsBoundToObject(Panel));
		TestFalse(TEXT("Row has no GAS listener"), ASC->GetGameplayAttributeValueChangeDelegate(Attributes[Index]).IsBoundToObject(Rows[Index]));
		TestEqual(TEXT("Parent supplies icon"), Icons[Index]->GetBrush().GetResourceObject(), static_cast<UObject*>(IconTexture));
		TestFalse(TEXT("Parent supplies localized name"), Names[Index]->GetText().IsEmpty());
		ASC->SetNumericAttributeBase(Attributes[Index], 20.f + Index);
		TestEqual(TEXT("Each primary attribute updates immediately"), Values[Index]->GetText().ToString(), FText::AsNumber(20 + Index).ToString());
	}
	// A sentinel detects accidental RefreshAll on a single attribute event.
	Rows[1]->SetDisplayValue(FText::FromString(TEXT("unchanged")));
	ASC->SetNumericAttributeBase(Attributes[0], 37.f);
	TestEqual(TEXT("Strength updates immediately"), Values[0]->GetText().ToString(), FText::AsNumber(37).ToString());
	TestEqual(TEXT("Dexterity not refreshed by Strength"), Values[1]->GetText().ToString(), FString(TEXT("unchanged")));
	Panel->NotifyPlayerContextChanged();
	const auto Spec = ASC->MakeOutgoingSpec(UUmbraDebugAttributeEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
	const auto Effect = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	TestEqual(TEXT("GE aggregate is displayed"), Values[0]->GetText().ToString(), FText::AsNumber(57).ToString());
	ASC->RemoveActiveGameplayEffect(Effect);
	TestEqual(TEXT("GE removal restores current value"), Values[0]->GetText().ToString(), FText::AsNumber(37).ToString());
	ASC->ClearActorInfo();
	TestEqual(TEXT("Not ready clears stale value"), Values[0]->GetText().ToString(), FString(TEXT("—")));
	ASC->InitAbilityActorInfo(Owner, Owner);
	TestEqual(TEXT("Ready event immediately resynchronizes"), Values[0]->GetText().ToString(), FText::AsNumber(37).ToString());
	AUmbraPlayerState* Replacement = World->SpawnActor<AUmbraPlayerState>();
	auto* NewASC = Replacement->GetUmbraAbilitySystemComponent();
	NewASC->InitAbilityActorInfo(Replacement, Replacement);
	NewASC->InitializeAttributes(nullptr);
	NewASC->SetNumericAttributeBase(Attributes[0], 9.f);
	PC->SetPlayerState(Replacement);
	Panel->NotifyPlayerContextChanged();
	TestFalse(TEXT("Old ASC unbound"), ASC->GetGameplayAttributeValueChangeDelegate(Attributes[0]).IsBoundToObject(Panel));
	ASC->SetNumericAttributeBase(Attributes[0], 99.f);
	TestEqual(TEXT("Old ASC cannot change replacement display"), Values[0]->GetText().ToString(), FText::AsNumber(9).ToString());
	Panel->DestructForTest();
	for (const auto& Attribute : Attributes)
		TestFalse(TEXT("Destruct removes each listener"), NewASC->GetGameplayAttributeValueChangeDelegate(Attribute).IsBoundToObject(Panel));
	TestFalse(TEXT("Destruct removes lifecycle listener"), UUmbraAbilitySystemComponent::OnLifecycleChanged.IsBoundToObject(Panel));
	Panel->ConstructForTest();
	TestEqual(TEXT("Reconstruct synchronizes immediately"), Values[0]->GetText().ToString(), FText::AsNumber(9).ToString());
	Panel->DestructForTest();
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
