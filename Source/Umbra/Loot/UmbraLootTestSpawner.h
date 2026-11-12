#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/UmbraItemGeneration.h"
#include "UmbraLootTestSpawner.generated.h"

class UUmbraItemDefinition;
class AUmbraWorldItem;

/** Place only in a test map. One authority-side batch per actor lifetime. */
UCLASS()
class UMBRA_API AUmbraLootTestSpawner : public AActor
{
	GENERATED_BODY()
public:
	AUmbraLootTestSpawner();
	UPROPERTY(EditAnywhere, Category = "Loot Test") bool bEnabled = true;
	/** Sample the normal generator until each rarity is represented, without changing profiles. */
	UPROPERTY(EditAnywhere, Category = "Loot Test") bool bOneOfEachRarity = false;
	UPROPERTY(EditAnywhere, Category = "Loot Test") bool bRandomizeSeedEachPlay = false;
	/** Pick one definition from Items; normal source-rarity generation still applies. Ignored by six-rarity mode. */
	UPROPERTY(EditAnywhere, Category = "Loot Test") bool bRandomSingleItem = false;
	/** Distance between drops in cm along this actor's local right vector. */
	UPROPERTY(EditAnywhere, Category = "Loot Test", meta=(ClampMin="1", Units="cm")) float Spacing = 100.f;
	UPROPERTY(EditAnywhere, Category = "Loot Test") TArray<TObjectPtr<UUmbraItemDefinition>> Items;
	UPROPERTY(EditAnywhere, Category = "Loot Test") TSubclassOf<AUmbraWorldItem> WorldItemClass;
	UPROPERTY(EditAnywhere, Category = "Loot Test") FUmbraGenerationContext GenerationContext;
	UPROPERTY(EditAnywhere, Category = "Loot Test") int32 GenerationSeed = 12345;
	/** Development fixture only. Returns a complete batch or no items, with bounded sampling. */
	bool GenerateRarityBatch(int32 Seed, TArray<FUmbraItemInstance>& Out) const;
protected:
	virtual void BeginPlay() override;
private:
	bool bSpawned = false;
};
