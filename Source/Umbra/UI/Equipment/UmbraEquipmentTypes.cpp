#include "UI/Equipment/UmbraEquipmentTypes.h"
#include "Items/UmbraItemDefinition.h"
#include "Engine/Texture2D.h"

FUmbraEquipmentItemDisplay FUmbraEquipmentItemDisplay::FromDefinition(UUmbraItemDefinition* Definition, FGuid Id)
{
	FUmbraEquipmentItemDisplay Display;
	if (!IsValid(Definition)) return Display;
	Display.Item = Definition;
	Display.InstanceId = Id;
	Display.DisplayName = Definition->DisplayName.IsEmpty() ? FText::FromName(Definition->GetFName()) : Definition->DisplayName;
	Display.Icon.SetResourceObject(Definition->Icon);
	Display.Icon.DrawAs = ESlateBrushDrawType::Image;
	Display.SlotBackgroundTexture = Definition->SlotBackgroundTexture;
	Display.RarityFrameTexture = Definition->RarityFrameTexture;
	Display.RarityColor = Definition->RarityColor;
	return Display;
}
