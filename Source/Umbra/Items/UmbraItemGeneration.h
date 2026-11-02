#pragma once
#include "CoreMinimal.h"
#include "Items/UmbraItemInstance.h"
#include "UmbraItemGeneration.generated.h"

USTRUCT(BlueprintType)
struct FUmbraAffixTier
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName TierId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MinimumLevel = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FUmbraAffixRollConfig Values;
};

/** Identity/target and damage conditions in each tier must agree. */
UCLASS(BlueprintType)
class UMBRA_API UUmbraAffixDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName AffixId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ExclusiveGroup;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<EUmbraItemCategory> Categories;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<EUmbraItemRarity> Rarities;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<EUmbraWeaponType> WeaponTypes;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<EUmbraEquipmentSlot> Slots;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTagContainer RequiredEquipmentTags;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTagContainer BlockedEquipmentTags;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTagContainer RequiredSourceTags;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MaximumLevel = 100;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FUmbraAffixTier> Tiers;
};

USTRUCT(BlueprintType)
struct FUmbraAffixPoolEntry
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UUmbraAffixDefinition> Affix;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Weight = 1.f;
};

UCLASS(BlueprintType)
class UMBRA_API UUmbraAffixPool : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FUmbraAffixPoolEntry> Entries;
};

USTRUCT(BlueprintType)
struct FUmbraGenerationRarity
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EUmbraItemRarity Rarity = EUmbraItemRarity::Common;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Weight = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AffixCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Color = FLinearColor::White;
	/** Enable for neutral artwork; precolored artwork should leave this disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bTintBackgrounds = false;
	/** Instance rarity owns presentation; never inherit art from a differently ranked template. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UTexture2D> SlotBackgroundTexture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UTexture2D> RarityFrameTexture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UTexture2D> TooltipTitleBackgroundTexture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UTexture2D> TooltipIconBackgroundTexture;
};

UCLASS(BlueprintType)
class UMBRA_API UUmbraItemGenerationProfile : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RulesVersion = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MinimumLevel = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MaximumLevel = 100;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 LevelOffset = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UUmbraAffixPool> Pool;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FUmbraGenerationRarity> Rarities;
};

USTRUCT(BlueprintType)
struct FUmbraGenerationContext
{
	GENERATED_BODY()
	/** Zero is missing, never implicitly replaced with the player's level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 SourceLevel = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTagContainer SourceTags;
};

namespace UmbraItemGeneration
{
	UMBRA_API bool Validate(const UUmbraItemDefinition* Definition, FString& Error);
	/** Same seed/rules/context yield the same payload except caller-owned GUID. No world side effects. */
	UMBRA_API bool Generate(const AActor* Authority, UUmbraItemDefinition* Definition,
		const FUmbraGenerationContext& Context, int32 Seed, FGuid Id, FUmbraItemInstance& Out, FString& Error);
}
