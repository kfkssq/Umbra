#pragma once

#include "CoreMinimal.h"
#include "Items/UmbraItemDefinition.h"
#include "UmbraItemInstance.generated.h"

/** Complete non-stacking instance payload; only authority creation may roll it. */
USTRUCT(BlueprintType)
struct FUmbraItemInstance
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UUmbraItemDefinition> Definition;
	UPROPERTY(BlueprintReadOnly) FGuid InstanceId;
	UPROPERTY(BlueprintReadOnly) TArray<FUmbraRolledAffix> RolledAffixes;
	/** Zero identifies a legacy payload; its rolls must never be regenerated during adaptation. */
	UPROPERTY(BlueprintReadOnly) int32 SchemaVersion = 0;
	UPROPERTY(BlueprintReadOnly) FName DefinitionId;
	UPROPERTY(BlueprintReadOnly) int32 ItemLevel = 1;
	UPROPERTY(BlueprintReadOnly) EUmbraItemRarity Rarity = EUmbraItemRarity::Common;
	UPROPERTY(BlueprintReadOnly) int32 GenerationSeed = 0;
	UPROPERTY(BlueprintReadOnly) int32 GenerationRulesVersion = 0;
	bool UpgradeLegacy();
	int32 GetItemLevel() const;
	EUmbraItemRarity GetRarity() const;
	FLinearColor GetRarityColor() const;
	bool IsValid() const;
	static bool Create(const AActor* Authority, UUmbraItemDefinition* Definition, FGuid Id, FUmbraItemInstance& Out);
};

/** In-memory persistence boundary, resolved by stable definition ID. No save-slot implementation. */
USTRUCT()
struct FUmbraItemInstanceRecord
{
	GENERATED_BODY()
	UPROPERTY() int32 SchemaVersion = 1;
	UPROPERTY() FName DefinitionId;
	UPROPERTY() FGuid InstanceId;
	UPROPERTY() int32 ItemLevel = 1;
	UPROPERTY() EUmbraItemRarity Rarity = EUmbraItemRarity::Common;
	UPROPERTY() int32 GenerationSeed = 0;
	UPROPERTY() int32 GenerationRulesVersion = 0;
	UPROPERTY() TArray<FUmbraRolledAffix> RolledAffixes;
	static bool Capture(FUmbraItemInstance Value, FUmbraItemInstanceRecord& Out);
	bool Restore(UUmbraItemDefinition* Resolved, FUmbraItemInstance& Out) const;
};
