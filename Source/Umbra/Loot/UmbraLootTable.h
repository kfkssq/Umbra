#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/UmbraItemClassification.h"
#include "Equipment/UmbraEquipmentSlot.h"
#include "UmbraLootTable.generated.h"

class UUmbraItemDefinition;

USTRUCT(BlueprintType)
struct FUmbraLootEntry
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UUmbraItemDefinition> ItemDefinition;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0")) float Weight = 1.f;
	/** Applied after selection; a failed roll is no drop, with no retry. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "1")) float DropChance = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "16")) int32 MinCount = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "16")) int32 MaxCount = 1;
};

USTRUCT(BlueprintType)
struct FUmbraLootTemplate
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UUmbraItemDefinition> ItemDefinition;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Weight = 1.f;
};

USTRUCT(BlueprintType)
struct FUmbraLootTypeGroup
{
	GENERATED_BODY()
	/** Stable identity; ordering in the editor does not change seeded selection. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName GroupId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Weight = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EUmbraWeaponType WeaponType = EUmbraWeaponType::Unknown;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EUmbraEquipmentSlot ArmorSlot = EUmbraEquipmentSlot::Head;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FUmbraLootTemplate> Templates;
};

USTRUCT(BlueprintType)
struct FUmbraLootCategoryGroup
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EUmbraItemCategory Category = EUmbraItemCategory::Weapon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Weight = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FUmbraLootTypeGroup> Types;
};

/** Legacy flat selection or opt-in category/type/template selection. Never modifies inventory. */
UCLASS(BlueprintType)
class UMBRA_API UUmbraLootTable : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bUseTypeGroups = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FUmbraLootCategoryGroup> Categories;
	/** One gate per death, followed by independently selected equipment pieces. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float EquipmentDropChance = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MinEquipmentCount = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MaxEquipmentCount = 1;
	bool ValidateTypeGroups(FString& Error) const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FUmbraLootEntry> Entries;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0")) float NoDropWeight = 0.f;
	static constexpr int32 MaximumItems = 16;
	void Select(const AActor* Authority, TArray<UUmbraItemDefinition*>& OutDefinitions) const;
	void Select(const AActor* Authority, FRandomStream& Random, TArray<UUmbraItemDefinition*>& OutDefinitions) const;
};
