#pragma once

#include "CoreMinimal.h"
#include "UI/Equipment/UmbraEquipmentSlotWidget.h"
#include "UI/UmbraCharacterStatsPanel.h"
#include "UI/Equipment/UmbraEquipmentMenu.h"
#include "UmbraStatWidgetTestTypes.generated.h"

// Transient native fixtures exercise the shared bases without depending on unfinished WBP assets.
UCLASS(Transient, NotBlueprintable)
class UUmbraEquipmentMenuTestWidget : public UUmbraEquipmentMenu
{
	GENERATED_BODY()
public:
	void ConstructForTest() { NativeConstruct(); }
	void DestructForTest() { NativeDestruct(); }
};

UCLASS(Transient, NotBlueprintable)
class UUmbraStatEntryTestWidget : public UUmbraStatEntry
{
	GENERATED_BODY()
public:
	void Configure(EUmbraCharacterStat InStat, UImage* Icon, UTextBlock* Name, UTextBlock* Value)
	{
		Stat = InStat;
		StatIcon = Icon;
		StatNameText = Name;
		StatValueText = Value;
	}
};

UCLASS(Transient, NotBlueprintable)
class UUmbraStatsPanelTestWidget : public UUmbraCharacterStatsPanel
{
	GENERATED_BODY()
public:
	void ConstructForTest() { NativeConstruct(); }
	void DestructForTest() { NativeDestruct(); }
	void SetIconForTest(EUmbraCharacterStat Stat, UTexture2D* Icon) { StatDisplayData.FindChecked(Stat).Icon = Icon; }
};

UCLASS(Transient, NotBlueprintable)
class UUmbraEquipmentSlotTestWidget : public UUmbraEquipmentSlotWidget
{
	GENERATED_BODY()
public:
	FReply MouseDownForTest(const FPointerEvent& Event) { return NativeOnMouseButtonDown(FGeometry(), Event); }
	void Configure(UImage* Empty, UImage* Item)
	{
		EmptyIcon = Empty;
		ItemIcon = Item;
	}
	void SetEmptyIconForTest(EUmbraEquipmentSlot Type, UTexture2D* Texture) { EmptySlotIcons.Add(Type, Texture); }
	void SetRarityFrameForTest(UImage* Frame) { RarityFrame = Frame; }
	void SetBackgroundForTest(UImage* Background) { SlotBackground = Background; }
};
