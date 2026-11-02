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
	UPROPERTY(EditAnywhere, Category = "Loot Test") TArray<TObjectPtr<UUmbraItemDefinition>> Items;
	UPROPERTY(EditAnywhere, Category = "Loot Test") TSubclassOf<AUmbraWorldItem> WorldItemClass;
	UPROPERTY(EditAnywhere, Category = "Loot Test") FUmbraGenerationContext GenerationContext;
	UPROPERTY(EditAnywhere, Category = "Loot Test") int32 GenerationSeed = 12345;
protected:
	virtual void BeginPlay() override;
private:
	bool bSpawned = false;
};
