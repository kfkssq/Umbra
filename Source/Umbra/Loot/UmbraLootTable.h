#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
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

/** One weighted selection per death, followed by chance and quantity. Never modifies inventory. */
UCLASS(BlueprintType)
class UMBRA_API UUmbraLootTable : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FUmbraLootEntry> Entries;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0")) float NoDropWeight = 0.f;
	static constexpr int32 MaximumItems = 16;
	void Select(const AActor* Authority, TArray<UUmbraItemDefinition*>& OutDefinitions) const;
};
