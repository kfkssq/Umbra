#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/UmbraItemTooltipUITestTypes.h"
#include "Misc/AutomationTest.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Components/SizeBox.h"
#include "Widgets/SWidget.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/PlayerController.h"
#include "Player/UmbraPlayerState.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Equipment/UmbraEquipmentEffect.h"
#include "Stats/UmbraDerivedStatsComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraItemClassificationTest, "Umbra.Items.Tooltip.Classification",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraItemClassificationTest::RunTest(const FString& Parameters)
{
	auto* Item = NewObject<UUmbraItemDefinition>();
	TestEqual(TEXT("New category defaults Unknown"), Item->ItemCategory, EUmbraItemCategory::Unknown);
	TestEqual(TEXT("New weapon type defaults Unknown"), Item->WeaponType, EUmbraWeaponType::Unknown);
	TestEqual(TEXT("Legacy generic item"), UUmbraItemTooltipDataBuilder::FromDefinition(Item).ItemType.ToString(), FString(TEXT("物品")));
	Item->AllowedSlots = {EUmbraEquipmentSlot::MainHand};
	TestEqual(TEXT("Slots only imply equipment"), UUmbraItemTooltipDataBuilder::FromDefinition(Item).ItemType.ToString(), FString(TEXT("装备")));
	Item->Weapon = NewObject<UUmbraWeaponProfile>(Item);
	auto& Channel = Item->Weapon->Damage.Channels.AddDefaulted_GetRef();
	Channel.Type = EUmbraWeaponDamageType::Fire;
	Channel.BaseDamage = 35.f;
	TestEqual(TEXT("Unknown with profile is weapon"), UUmbraItemTooltipDataBuilder::FromDefinition(Item).ItemType.ToString(), FString(TEXT("武器")));
	Item->ItemCategory = EUmbraItemCategory::Weapon;
	const TCHAR* Labels[] = {TEXT("武器"), TEXT("单手剑"), TEXT("双手剑"), TEXT("匕首"), TEXT("刺剑"), TEXT("单手斧"), TEXT("双手斧"), TEXT("单手锤"), TEXT("双手锤"), TEXT("长枪"), TEXT("长戟"), TEXT("法杖"), TEXT("魔杖"), TEXT("弓"), TEXT("弩")};
	for (uint8 Index = 0; Index < UE_ARRAY_COUNT(Labels); ++Index)
	{
		Item->WeaponType = EUmbraWeaponType(Index);
		const auto Data = UUmbraItemTooltipDataBuilder::FromDefinition(Item);
		TestEqual(TEXT("All concrete types localized"), Data.ItemType.ToString(), FString(Labels[Index]));
		TestEqual(TEXT("Weapon type copied"), Data.WeaponType, Item->WeaponType);
		TestEqual(TEXT("Weapon types never change damage type"), Data.DamageChannels[0].Type, EUmbraWeaponDamageType::Fire);
		TestEqual(TEXT("Weapon types never change damage"), Data.TotalBaseDamage, 35.);
	}
	Item->WeaponType = EUmbraWeaponType::OneHandedSword;
	const TCHAR* Qualities[] = {TEXT("普通"), TEXT("魔法"), TEXT("稀有"), TEXT("史诗"), TEXT("传奇"), TEXT("独特")};
	for (uint8 Index = 0; Index < 6; ++Index)
	{
		Item->Rarity = EUmbraItemRarity(Index);
		const auto Data = UUmbraItemTooltipDataBuilder::FromDefinition(Item);
		TestEqual(TEXT("Quality mapping including Epic"), Data.RarityText.ToString(), FString(Qualities[Index]));
		TestEqual(TEXT("Combined localized header"), Data.RarityAndTypeText.ToString(), FString(Qualities[Index]) + TEXT(" · 单手剑"));
	}
	Item->Weapon = nullptr;
	TArray<FText> Warnings;
	UmbraItemClassification::Describe(*Item, Warnings);
	TestEqual(TEXT("Weapon missing profile diagnostic"), Warnings.Num(), 1);
	Item->ItemCategory = EUmbraItemCategory::Armor;
	AddExpectedError(TEXT("Item classification"), EAutomationExpectedErrorFlags::Contains, 1);
	const auto Conflict = UUmbraItemTooltipDataBuilder::FromDefinition(Item);
	TestTrue(TEXT("Metadata inconsistency nonfatal"), Conflict.bValid);
	TestEqual(TEXT("Wrong weapon type does not mislabel armor"), Conflict.ItemType.ToString(), FString(TEXT("防具")));
	TestEqual(TEXT("Warning also available to presentation"), Conflict.ClassificationWarnings.Num(), 1);
	TestTrue(TEXT("Classification cannot generate damage"), Conflict.DamageChannels.IsEmpty());
	Item->WeaponType = EUmbraWeaponType::Unknown;
	const TCHAR* Categories[] = {TEXT("装备"), TEXT("武器"), TEXT("防具"), TEXT("饰品"), TEXT("消耗品"), TEXT("材料"), TEXT("杂项")};
	for (uint8 Index = 0; Index < UE_ARRAY_COUNT(Categories); ++Index)
	{
		Item->ItemCategory = EUmbraItemCategory(Index);
		TestEqual(TEXT("All categories localized with stable values"), UmbraItemClassification::Describe(*Item, Warnings).ToString(), FString(Categories[Index]));
	}
	auto* Legacy = LoadObject<UUmbraItemDefinition>(nullptr, TEXT("/Game/Items/Weapons/DA_Item_TestSword.DA_Item_TestSword"));
	if (TestNotNull(TEXT("Pre-classification asset loads without migration"), Legacy))
	{
		TestEqual(TEXT("Legacy category Unknown"), Legacy->ItemCategory, EUmbraItemCategory::Unknown);
		TestEqual(TEXT("Legacy type Unknown"), Legacy->WeaponType, EUmbraWeaponType::Unknown);
		TestTrue(TEXT("Legacy definition builds"), UUmbraItemTooltipDataBuilder::FromDefinition(Legacy).bValid);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraTooltipDesignerTest, "Umbra.UI.Items.TooltipDesignerPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraTooltipDesignerTest::RunTest(const FString& Parameters)
{
	const auto MakePreview = []()
	{
		auto* Widget = NewObject<UUmbraItemTooltipTestWidget>();
		Widget->WidgetTree = NewObject<UWidgetTree>(Widget);
		Widget->Configure();
		return Widget;
	};
	auto* Designer = MakePreview();
	Designer->SetDesignerFlags(EWidgetDesignFlags::Designing);
	const TCHAR* Names[] = {TEXT("ItemName"), TEXT("RarityText"), TEXT("TotalDamageText")};
	const TCHAR* Values[] = {TEXT("Title"), TEXT("Legendary"), TEXT("285")};
	for (int32 Index = 0; Index < 3; ++Index)
		CastChecked<UTextBlock>(Designer->WidgetTree->FindWidget(Names[Index]))->SetText(FText::FromString(Values[Index]));
	auto* Rows = CastChecked<UVerticalBox>(Designer->WidgetTree->FindWidget(TEXT("StatRows")));
	auto* Placeholder = NewObject<UTextBlock>(Designer);
	Placeholder->SetText(FText::FromString(TEXT("Layout only")));
	Rows->AddChild(Placeholder);
	const auto DesignedSectionVisibility = Designer->WidgetTree->FindWidget(TEXT("DamageSection"))->GetVisibility();
	Designer->PreConstructForTest();
	Designer->PreConstructForTest();
	TestEqual(TEXT("Designer visible without data"), Designer->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestFalse(TEXT("Preview never becomes real data"), Designer->GetTooltipData().bValid);
	TestFalse(TEXT("Preview does not invent GUID"), Designer->GetTooltipData().InstanceId.IsValid());
	for (int32 Index = 0; Index < 3; ++Index)
		TestEqual(TEXT("Designer authored text preserved"), CastChecked<UTextBlock>(Designer->WidgetTree->FindWidget(Names[Index]))->GetText().ToString(), FString(Values[Index]));
	TestEqual(TEXT("Designer rows preserved"), Rows->GetChildrenCount(), 1);
	TestEqual(TEXT("Designer section visibility preserved"), Designer->WidgetTree->FindWidget(TEXT("DamageSection"))->GetVisibility(), DesignedSectionVisibility);
	// A fresh runtime instance must never treat designer placeholders as actual item data.
	auto* Runtime = MakePreview();
	auto* Name = CastChecked<UTextBlock>(Runtime->WidgetTree->FindWidget(TEXT("ItemName")));
	Name->SetText(FText::FromString(TEXT("Title")));
	Runtime->PreConstructForTest();
	TestTrue(TEXT("Runtime clears placeholder"), Name->GetText().IsEmpty());
	TestEqual(TEXT("Runtime invalid still collapsed"), Runtime->GetVisibility(), ESlateVisibility::Collapsed);
	auto* Item = NewObject<UUmbraItemDefinition>();
	Item->DisplayName = FText::FromString(TEXT("Real item"));
	Runtime->SetTooltipData(UUmbraItemTooltipDataBuilder::FromDefinition(Item));
	TestEqual(TEXT("Runtime uses supplied data"), Name->GetText().ToString(), Item->DisplayName.ToString());
	Runtime->ClearTooltipData();
	TestTrue(TEXT("Runtime Clear still empties text"), Name->GetText().IsEmpty());
	TestEqual(TEXT("Runtime Clear still collapses"), Runtime->GetVisibility(), ESlateVisibility::Collapsed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraItemTooltipWidgetTest, "Umbra.UI.Items.TooltipLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraItemTooltipWidgetTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	auto* PC = World->SpawnActor<APlayerController>();
	PC->SetPlayer(NewObject<ULocalPlayer>(GEngine));
	auto* Tooltip = CreateWidget<UUmbraItemTooltipTestWidget>(PC);
	Tooltip->Configure();
	Tooltip->StatEntryClass = UUmbraTooltipEntryTestWidget::StaticClass();
	auto* Item = NewObject<UUmbraItemDefinition>();
	Item->DisplayName = FText::FromString(TEXT("Tooltip sword"));
	Item->ItemCategory = EUmbraItemCategory::Weapon;
	Item->WeaponType = EUmbraWeaponType::OneHandedSword;
	Item->Rarity = EUmbraItemRarity::Epic;
	Item->RarityColor = FLinearColor::Red;
	Item->Icon = NewObject<UTexture2D>(Item);
	Item->ItemLevel = 42;
	Item->RequiredLevel = 5;
	Item->Requirements.Strength = 20.f;
	Item->Armor = 7.f;
	Item->PrimaryBonuses.Strength = 4.f;
	Item->Weapon = NewObject<UUmbraWeaponProfile>(Item);
	auto& Channel = Item->Weapon->Damage.Channels.AddDefaulted_GetRef();
	Channel.Type = EUmbraWeaponDamageType::Fire;
	Channel.BaseDamage = 13.f;
	auto& Scale = Channel.Scaling.AddDefaulted_GetRef();
	Scale.Coefficient = .8f;
	Scale.PointCurve = NewObject<UCurveFloat>(Item);
	const auto Data = UUmbraItemTooltipDataBuilder::FromDefinition(Item);
	// Set before Construct must survive initialization, then rebuild exactly once per refresh.
	Tooltip->SetTooltipData(Data);
	const TSharedRef<SWidget> TooltipSlate = Tooltip->TakeWidget();
	const auto Block = [&](const TCHAR* Name) { return CastChecked<UTextBlock>(Tooltip->WidgetTree->FindWidget(Name)); };
	const auto Rows = [&](const TCHAR* Name) { return CastChecked<UVerticalBox>(Tooltip->WidgetTree->FindWidget(Name)); };
	const auto Visible = [&](const TCHAR* Name) { return Tooltip->WidgetTree->FindWidget(Name)->GetVisibility() != ESlateVisibility::Collapsed; };
	TestEqual(TEXT("Name copied"), Block(TEXT("ItemName"))->GetText().ToString(), Data.DisplayName.ToString());
	TestEqual(TEXT("Specific localized type"), Block(TEXT("ItemTypeText"))->GetText().ToString(), FString(TEXT("单手剑")));
	TestEqual(TEXT("Epic localized"), Block(TEXT("RarityText"))->GetText().ToString(), FString(TEXT("史诗")));
	TestEqual(TEXT("Name uses authored color"), Block(TEXT("ItemName"))->GetColorAndOpacity().GetSpecifiedColor(), FLinearColor::Red);
	TestEqual(TEXT("Item level copied independent of requirement"), Block(TEXT("ItemLevelText"))->GetText().ToString(), Data.ItemLevelText.ToString());
	TestEqual(TEXT("Damage total copied"), Block(TEXT("TotalDamageText"))->GetText().ToString(), Data.TotalDamageText.ToString());
	TestEqual(TEXT("Only nonzero damage rows"), Rows(TEXT("DamageRows"))->GetChildrenCount(), 1);
	TestEqual(TEXT("None grades omitted"), Rows(TEXT("ScalingRows"))->GetChildrenCount(), 1);
	TestTrue(TEXT("Curve reference note visible"), Block(TEXT("ScalingNote"))->GetText().ToString().Contains(TEXT("曲线")));
	TestEqual(TEXT("Unified intrinsic and fixed stats"), Rows(TEXT("StatRows"))->GetChildrenCount(), Data.Stats.Num());
	TestFalse(TEXT("Empty special effects folded"), Visible(TEXT("SpecialEffectsSection")));
	TestFalse(TEXT("Empty flavor folded"), Visible(TEXT("FlavorTextSection")));
	TestTrue(TEXT("Zero weight remains valid and visible"), Visible(TEXT("WeightSection")));
	TestTrue(TEXT("Zero weight formatted"), Block(TEXT("WeightText"))->GetText().ToString().Contains(TEXT("0")));
	for (int32 Index = 0; Index < Data.RequirementRows.Num(); ++Index)
	{
		auto* Entry = CastChecked<UUmbraTooltipEntryTestWidget>(Rows(TEXT("RequirementRows"))->GetChildAt(Index));
		TestEqual(TEXT("Unknown is never green/met"), Entry->GetEntryStyle(), EUmbraTooltipRowStyle::Unknown);
		TestTrue(TEXT("Unknown is explicit"), Entry->GetEntryText().ToString().Contains(TEXT("未知")));
	}
	TestEqual(TEXT("Whole subtree ignores mouse"), Tooltip->GetVisibility(), ESlateVisibility::HitTestInvisible);
	Tooltip->SetVisibility(ESlateVisibility::Visible);
	TestEqual(TEXT("Visible cannot enable hit tests"), Tooltip->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestFalse(TEXT("No keyboard focus"), Tooltip->IsFocusable());
	auto* OldRow = CastChecked<UUmbraTooltipEntryTestWidget>(Rows(TEXT("StatRows"))->GetChildAt(0));
	for (int32 Index = 0; Index < 4; ++Index) Tooltip->SetTooltipData(Data);
	TestTrue(TEXT("Detached row text cleared"), OldRow->GetEntryText().IsEmpty());
	TestEqual(TEXT("Refresh never duplicates rows"), Rows(TEXT("StatRows"))->GetChildrenCount(), Data.Stats.Num());
	Tooltip->ConstructForTest();
	TestEqual(TEXT("Repeated Construct never duplicates rows"), Rows(TEXT("StatRows"))->GetChildrenCount(), Data.Stats.Num());
	// Pure projection: deliberately different preformatted text must be used verbatim.
	auto DisplayOnly = Data;
	DisplayOnly.Stats[0].Text = FText::FromString(TEXT("already formatted condition"));
	DisplayOnly.TotalDamageText = FText::FromString(TEXT("authored total caption"));
	DisplayOnly.FlavorText = FText::FromString(TEXT("A saved story"));
	DisplayOnly.SpecialEffects.Add(FText::FromString(TEXT("external presentation extension")));
	Tooltip->SetTooltipData(DisplayOnly);
	TestEqual(TEXT("Stat string not recomputed"), CastChecked<UUmbraTooltipEntryTestWidget>(Rows(TEXT("StatRows"))->GetChildAt(0))->GetEntryText().ToString(), DisplayOnly.Stats[0].Text.ToString());
	TestEqual(TEXT("Damage string not recomputed"), Block(TEXT("TotalDamageText"))->GetText().ToString(), DisplayOnly.TotalDamageText.ToString());
	TestTrue(TEXT("Nonempty extension displayed"), Visible(TEXT("SpecialEffectsSection")));
	TestTrue(TEXT("Nonempty flavor displayed"), Visible(TEXT("FlavorTextSection")));
	auto* Plain = NewObject<UUmbraItemDefinition>();
	Plain->ItemCategory = EUmbraItemCategory::Material;
	const auto PlainData = UUmbraItemTooltipDataBuilder::FromDefinition(Plain);
	Tooltip->SetTooltipData(PlainData);
	TestFalse(TEXT("Nonweapon damage collapsed"), Visible(TEXT("DamageSection")));
	TestFalse(TEXT("Nonweapon scaling collapsed"), Visible(TEXT("ScalingSection")));
	TestFalse(TEXT("No stats collapsed"), Visible(TEXT("StatsSection")));
	TestEqual(TEXT("Previous stat rows removed"), Rows(TEXT("StatRows"))->GetChildrenCount(), 0);
	TestEqual(TEXT("Previous special rows removed"), Rows(TEXT("SpecialEffectRows"))->GetChildrenCount(), 0);
	TestTrue(TEXT("Old story cleared"), Block(TEXT("FlavorTextBlock"))->GetText().IsEmpty());
	TestNull(TEXT("Old icon cleared"), CastChecked<UImage>(Tooltip->WidgetTree->FindWidget(TEXT("ItemIcon")))->GetBrush().GetResourceObject());
	for (int32 Case = 0; Case < 2; ++Case)
	{
		Tooltip->SetTooltipData(Data);
		auto Invalid = Data;
		if (Case == 0) Invalid.bValid = false; else Invalid.Result = EUmbraTooltipResult::IdentityMismatch;
		Tooltip->SetTooltipData(Invalid);
		TestFalse(TEXT("Invalid flag or failure result clears data"), Tooltip->GetTooltipData().bValid);
		TestTrue(TEXT("Invalid clears name"), Block(TEXT("ItemName"))->GetText().IsEmpty());
		TestEqual(TEXT("Invalid clears stats"), Rows(TEXT("StatRows"))->GetChildrenCount(), 0);
		TestEqual(TEXT("Invalid collapses tooltip"), Tooltip->GetVisibility(), ESlateVisibility::Collapsed);
	}
	Tooltip->SetTooltipData(Data);
	Tooltip->DestructForTest();
	Tooltip->ConstructForTest();
	TestFalse(TEXT("Reopen without fresh data stays empty"), Tooltip->GetTooltipData().bValid);
	Tooltip->SetTooltipData(Data);
	Tooltip->ClearTooltipData();
	TestFalse(TEXT("Explicit clear"), Tooltip->GetTooltipData().bValid);
	// Optional art/sections can be absent. Native fallback needs no additional WBP asset.
	auto* Minimal = CreateWidget<UUmbraItemTooltipTestWidget>(PC);
	Minimal->Configure(false);
	Minimal->SetTooltipData(Data);
	const TSharedRef<SWidget> MinimalSlate = Minimal->TakeWidget();
	TestTrue(TEXT("Optional controls absent remain usable"), Minimal->GetTooltipData().bValid);
	auto* MinimalRows = CastChecked<UVerticalBox>(Minimal->WidgetTree->FindWidget(TEXT("StatRows")));
	TestEqual(TEXT("Fallback plain row count"), MinimalRows->GetChildrenCount(), Data.Stats.Num());
	TestNotNull(TEXT("Fallback is native text"), Cast<UTextBlock>(MinimalRows->GetChildAt(0)));
	// Entry neutral restores the WBP color after state-specific styles.
	auto* Entry = CreateWidget<UUmbraTooltipEntryTestWidget>(PC);
	const TSharedRef<SWidget> EntrySlate = Entry->TakeWidget();
	Entry->SetEntryText(FText::FromString(TEXT("unmet")), EUmbraTooltipRowStyle::Unmet);
	Entry->SetEntryText(FText::FromString(TEXT("neutral")));
	TestEqual(TEXT("Entry preserves authored neutral color"), Entry->GetTextBlock()->GetColorAndOpacity().GetSpecifiedColor(), FLinearColor::Blue);
	TestEqual(TEXT("Entry text updated"), Entry->GetTextBlock()->GetText().ToString(), FString(TEXT("neutral")));
	Tooltip->DestructForTest();
	Minimal->DestructForTest();
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraTooltipWidgetReadOnlyTest, "Umbra.UI.Items.TooltipReadOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraTooltipWidgetReadOnlyTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	auto* PC = World->SpawnActor<APlayerController>();
	PC->SetPlayer(NewObject<ULocalPlayer>(GEngine));
	auto* Owner = World->SpawnActor<AUmbraPlayerState>();
	auto* ASC = Owner->GetUmbraAbilitySystemComponent();
	if (!ASC->HasBeenInitialized()) ASC->InitializeComponent();
	ASC->InitAbilityActorInfo(Owner, Owner); Owner->InitializeAttributes();
	auto* Equipment = Owner->GetEquipmentComponent();
	auto* Inventory = Owner->GetInventoryComponent();
	Owner->FindComponentByClass<UUmbraDerivedStatsComponent>()->bUseWeaponDerivedPower = true;
	Equipment->bEnableEquipment = true;
	Owner->InitializeAttributes();
	auto* Item = NewObject<UUmbraItemDefinition>(Owner);
	Item->AllowedSlots = {EUmbraEquipmentSlot::Head}; Item->ItemCategory = EUmbraItemCategory::Armor;
	Item->Armor = 10.f; Item->Requirements.Strength = 999.f;
	const FGuid Id = FGuid::NewGuid();
	Inventory->AddItem(Item, Id);
	const auto InvBefore = Inventory->GetSnapshot();
	const int32 Slot = InvBefore.Items.FindByPredicate([Id](const auto& I) { return I.InstanceId == Id; })->SlotIndex;
	const auto Data = UUmbraItemTooltipDataBuilder::FromInventory(InvBefore, Slot, Id, Item, Equipment, true, EUmbraEquipmentSlot::Head);
	TestEqual(TEXT("Inventory builder carries category"), Data.ItemCategory, EUmbraItemCategory::Armor);
	TestEqual(TEXT("Insufficient primary status built"), Data.RequirementState, EUmbraTooltipRequirementState::PrimaryPenalty);
	TestEqual(TEXT("Requirement row is unmet"), Data.RequirementRows[1].Style, EUmbraTooltipRowStyle::Unmet);
	const auto Effects = ASC->GetActiveEffects(FGameplayEffectQuery());
	const auto Attributes = UUmbraEquipmentEffect::Attributes();
	TArray<float> Values;
	for (const auto& Attribute : Attributes) Values.Add(ASC->GetNumericAttribute(Attribute));
	const int32 Revision = Equipment->GetSnapshot().Revision;
	auto* Tooltip = CreateWidget<UUmbraItemTooltipTestWidget>(PC);
	Tooltip->Configure();
	const TSharedRef<SWidget> TooltipSlate = Tooltip->TakeWidget();
	for (int32 Index = 0; Index < 3; ++Index) { Tooltip->SetTooltipData(Data); Tooltip->ClearTooltipData(); }
	TestTrue(TEXT("No GE side effects"), ASC->GetActiveEffects(FGameplayEffectQuery()) == Effects);
	TestEqual(TEXT("No equipment refresh/mutation"), Equipment->GetSnapshot().Revision, Revision);
	TestEqual(TEXT("No inventory mutation"), Inventory->GetSnapshot().Items.Num(), InvBefore.Items.Num());
	TestEqual(TEXT("Original inventory GUID preserved"), Inventory->GetSnapshot().Items[Slot].InstanceId, Id);
	for (int32 Index = 0; Index < Attributes.Num(); ++Index) TestEqual(TEXT("All equipment GAS attributes unchanged"), ASC->GetNumericAttribute(Attributes[Index]), Values[Index]);
	Owner->EquipFromInventory(EUmbraEquipmentSlot::Head, Id);
	const auto Worn = UUmbraItemTooltipDataBuilder::FromEquipment(Equipment->GetSnapshot(), EUmbraEquipmentSlot::Head, Id, Item, Equipment);
	TestTrue(TEXT("Equipment builder valid"), Worn.bValid);
	TestEqual(TEXT("Equipment localized type"), Worn.ItemType.ToString(), FString(TEXT("防具")));
	Tooltip->SetTooltipData(Worn);
	TestEqual(TEXT("Penalty shown exactly from data"), CastChecked<UTextBlock>(Tooltip->WidgetTree->FindWidget(TEXT("PenaltyText")))->GetText().ToString(), Worn.PenaltyText.ToString());
	Tooltip->DestructForTest();
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraTooltipViewportFitTest, "Umbra.UI.Items.TooltipViewportFit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraTooltipViewportFitTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	auto* PC = World->SpawnActor<APlayerController>();
	PC->SetPlayer(NewObject<ULocalPlayer>(GEngine));
	auto* Tip = CreateWidget<UUmbraItemTooltipTestWidget>(PC);
	Tip->Configure();
	auto* Frame = Tip->WidgetTree->ConstructWidget<USizeBox>();
	Frame->AddChild(Tip->WidgetTree->RootWidget);
	Tip->WidgetTree->RootWidget = Frame;
	auto* Item = NewObject<UUmbraItemDefinition>();
	Item->DisplayName = FText::FromString(TEXT("Viewport regression"));
	Tip->SetTooltipData(UUmbraItemTooltipDataBuilder::FromDefinition(Item));
	TSharedPtr<SWidget> Slate = Tip->TakeWidget();
	const FVector2D Contents[] = { {440, 1500}, {1800, 500}, {1800, 1600}, {300, 200} };
	const FVector2D Viewports[] = { {1348, 950}, {2014, 970}, {640, 360}, {1920, 1080} };
	for (const FVector2D& Content : Contents)
	{
		Frame->SetWidthOverride(Content.X);
		Frame->SetHeightOverride(Content.Y);
		for (const FVector2D& Viewport : Viewports)
		{
			const FVector2D Anchors[] = { {0, 0}, Viewport, {-100, -100}, Viewport * 0.5 };
			for (const FVector2D& Anchor : Anchors)
			{
				Tip->ApplyViewportPosition(Anchor, Viewport);
				const FMargin Rect = UGameViewportSubsystem::Get()->GetWidgetSlot(Tip).Offsets;
				TestTrue(TEXT("Complete tooltip fits inside all four viewport edges"),
					Rect.Left >= 11.99 && Rect.Top >= 11.99
					&& Rect.Left + Rect.Right <= Viewport.X - 11.99
					&& Rect.Top + Rect.Bottom <= Viewport.Y - 11.99);
				TestTrue(TEXT("Uniform scale preserves authored proportions"),
					FMath::IsNearlyEqual(double(Rect.Right / Rect.Bottom), Content.X / Content.Y, 0.001));
				TestTrue(TEXT("Fit never enlarges content"), Rect.Right <= Content.X + 0.01 && Rect.Bottom <= Content.Y + 0.01);
				TestEqual(TEXT("Layout does not discard item content"), Tip->GetTooltipData().DisplayName.ToString(), Item->DisplayName.ToString());
			}
		}
	}
	Tip->HideTooltip();
	TestFalse(TEXT("Hiding stops viewport tracking"), Tip->bPresented);
	Slate.Reset();
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
