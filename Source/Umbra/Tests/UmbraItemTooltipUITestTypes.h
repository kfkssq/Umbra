#pragma once

#include "CoreMinimal.h"
#include "UI/Items/UmbraItemTooltip.h"
#include "UI/Items/UmbraTooltipStatEntry.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/UniformGridPanel.h"
#include "UmbraItemTooltipUITestTypes.generated.h"

UCLASS(Transient, NotBlueprintable)
class UUmbraTooltipEntryTestWidget : public UUmbraTooltipStatEntry
{
	GENERATED_BODY()
public:
	UTextBlock* GetTextBlock() const { return EntryText; }
protected:
	virtual void NativeOnInitialized() override
	{
		Super::NativeOnInitialized();
		EntryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EntryText"));
		EntryText->SetColorAndOpacity(FLinearColor::Blue);
		WidgetTree->RootWidget = EntryText;
	}
};

UCLASS(Transient, NotBlueprintable)
class UUmbraItemTooltipTestWidget : public UUmbraItemTooltip
{
	GENERATED_BODY()
public:
	void PreConstructForTest() { NativePreConstruct(); }
	void ConstructForTest() { NativeConstruct(); }
	void DestructForTest() { NativeDestruct(); }
	void Configure(bool bOptional = true)
	{
		auto* Root = WidgetTree->ConstructWidget<UVerticalBox>();
		WidgetTree->RootWidget = Root;
		const auto Section = [&](FName Name)
		{
			auto* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name);
			Root->AddChild(Box);
			return Box;
		};
		const auto Text = [&](UVerticalBox* Parent, FName Name)
		{
			auto* Block = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
			Parent->AddChild(Block);
			return Block;
		};
		const auto Rows = [&](UVerticalBox* Parent, FName Name)
		{
			auto* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name);
			Parent->AddChild(Box);
			return Box;
		};
		const auto Grid = [&](UVerticalBox* Parent, FName Name)
		{
			auto* Panel = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), Name);
			Parent->AddChild(Panel);
			return Panel;
		};
		ItemName = Text(Root, TEXT("ItemName"));
		StatRows = Rows(Root, TEXT("StatRows"));
		if (!bOptional) return;
		quality_bg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("quality_bg"));
		SlotBackground = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SlotBackground"));
		Root->AddChild(quality_bg); Root->AddChild(SlotBackground);
		HeaderSection = Section(TEXT("HeaderSection"));
		ItemIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ItemIcon"));
		Cast<UVerticalBox>(HeaderSection)->AddChild(ItemIcon);
		ItemTypeText = Text(Root, TEXT("ItemTypeText"));
		RarityText = Text(Root, TEXT("RarityText"));
		ItemLevelText = Text(Root, TEXT("ItemLevelText"));
		auto* Damage = Section(TEXT("DamageSection")); DamageSection = Damage;
		TotalDamageText = Text(Damage, TEXT("TotalDamageText")); DamageRows = Rows(Damage, TEXT("DamageRows"));
		auto* Scaling = Section(TEXT("ScalingSection")); ScalingSection = Scaling;
		ScalingRows = Grid(Scaling, TEXT("ScalingRows")); ScalingNote = Text(Scaling, TEXT("ScalingNote"));
		StatsSection = Section(TEXT("StatsSection"));
		auto* Requirements = Section(TEXT("RequirementsSection")); RequirementsSection = Requirements;
		RequirementRows = Rows(Requirements, TEXT("RequirementRows")); PenaltyText = Text(Requirements, TEXT("PenaltyText"));
		auto* Weight = Section(TEXT("WeightSection")); WeightSection = Weight;
		WeightText = Text(Weight, TEXT("WeightText"));
		auto* Special = Section(TEXT("SpecialEffectsSection")); SpecialEffectsSection = Special;
		SpecialEffectRows = Rows(Special, TEXT("SpecialEffectRows"));
		auto* Flavor = Section(TEXT("FlavorTextSection")); FlavorTextSection = Flavor;
		FlavorTextBlock = Text(Flavor, TEXT("FlavorTextBlock"));
	}
};

/** Menu-created fixture builds the same optional contract without loading or saving WBP assets. */
UCLASS(Transient, NotBlueprintable)
class UUmbraHoverTooltipTestWidget : public UUmbraItemTooltipTestWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeOnInitialized() override
	{
		Super::NativeOnInitialized();
		Configure();
	}
};
