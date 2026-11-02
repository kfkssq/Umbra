#include "UI/Equipment/UmbraEquipmentTypes.h"
#include "Items/UmbraItemDefinition.h"
#include "Items/UmbraItemInstance.h"
#include "Items/UmbraItemGeneration.h"
#include "Engine/Texture2D.h"

FUmbraEquipmentItemDisplay FUmbraEquipmentItemDisplay::FromDefinition(UUmbraItemDefinition* Definition, FGuid Id)
{
	FUmbraEquipmentItemDisplay Display;
	if (!IsValid(Definition)) return Display;
	Display.Item = Definition;
	Display.AudioContext = FUmbraAudioContext::FromItem(Definition);
	Display.InstanceId = Id;
	Display.DisplayName = Definition->DisplayName.IsEmpty() ? FText::FromName(Definition->GetFName()) : Definition->DisplayName;
	Display.Icon.SetResourceObject(Definition->Icon);
	Display.Icon.DrawAs = ESlateBrushDrawType::Image;
	Display.SlotBackgroundTexture = Definition->SlotBackgroundTexture;
	Display.RarityFrameTexture = Definition->RarityFrameTexture;
	Display.RarityColor = Definition->RarityColor;
	Display.bUseRarityBackgroundColor = Definition->bTintRarityBackgrounds;
	return Display;
}

FUmbraEquipmentItemDisplay FUmbraEquipmentItemDisplay::FromInstance(const FUmbraItemInstance& Instance)
{
	auto Display = FromDefinition(Instance.Definition, Instance.InstanceId);
	Display.AudioContext = FUmbraAudioContext::FromInstance(Instance);
	Display.RarityColor = Instance.GetRarityColor();
	// Generated quality uses the shared color palette; legacy authored frame art stays unchanged.
	if (Instance.GenerationRulesVersion > 0 && Instance.Definition && Instance.Definition->GenerationProfile)
		for (const auto& Row : Instance.Definition->GenerationProfile->Rarities)
			if (Row.Rarity == Instance.GetRarity())
			{
				Display.SlotBackgroundTexture = Row.SlotBackgroundTexture;
				Display.bUseRarityBackgroundColor = Row.bTintBackgrounds;
				Display.RarityFrameTexture = Row.RarityFrameTexture;
				break;
			}
	return Display;
}
