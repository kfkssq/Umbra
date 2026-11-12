#pragma once

#include "CoreMinimal.h"
#include "Items/UmbraItemDefinition.h"
#include "UmbraItemInstance.generated.h"

/** Complete instance/stack payload; only authority creation may roll it. */
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
	/** Presentation metadata; upgrade actions and bonuses are outside this contract. */
	UPROPERTY(BlueprintReadOnly) int32 UpgradeLevel = 0;
	UPROPERTY(BlueprintReadOnly) EUmbraItemRarity Rarity = EUmbraItemRarity::Common;
	UPROPERTY(BlueprintReadOnly) int32 GenerationSeed = 0;
	UPROPERTY(BlueprintReadOnly) int32 GenerationRulesVersion = 0;
	/** Defaults to one for legacy payloads. Inventory merges only opted-in materials. */
	UPROPERTY(BlueprintReadOnly) int32 Quantity = 1;
	bool CanStack() const;
	bool CanStackWith(const FUmbraItemInstance& Other) const;
	bool UpgradeLegacy();
	int32 GetItemLevel() const;
	FText GetDisplayName() const;
	EUmbraItemRarity GetRarity() const;
	/** Reserved special-effect identity only; no gameplay power is granted by this marker. */
	bool HasSpecialEffect() const { return Definition && Definition->ItemCategory != EUmbraItemCategory::Material && GetRarity() >= EUmbraItemRarity::Legendary; }
	bool IsUnique() const { return GetRarity() == EUmbraItemRarity::Unique; }
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
	UPROPERTY() int32 UpgradeLevel = 0;
	UPROPERTY() EUmbraItemRarity Rarity = EUmbraItemRarity::Common;
	UPROPERTY() int32 GenerationSeed = 0;
	UPROPERTY() int32 GenerationRulesVersion = 0;
	UPROPERTY() int32 Quantity = 1;
	UPROPERTY() TArray<FUmbraRolledAffix> RolledAffixes;
	static bool Capture(FUmbraItemInstance Value, FUmbraItemInstanceRecord& Out);
	bool Restore(UUmbraItemDefinition* Resolved, FUmbraItemInstance& Out) const;
};
