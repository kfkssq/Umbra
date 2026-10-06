// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/UmbraDebugInitialAttributes.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "Equipment/UmbraEquipmentSlot.h"
#include "Equipment/UmbraEquipmentComponent.h"
#include "Inventory/UmbraInventoryComponent.h"
#include "UmbraPlayerState.generated.h"

class UUmbraAbilitySystemComponent;
class UUmbraAttributeSet;
class UUmbraGameplayAbility;
class UUmbraBasicAttackAbility;
class UGameplayEffect;
class UUmbraDerivedStatsComponent;
class UUmbraEquipmentComponent;
class UUmbraInventoryComponent;

/** Result of an atomic inventory <-> equipment transfer. Never leaves an item duplicated or dropped. */
UENUM(BlueprintType)
enum class EUmbraTransferResult : uint8
{
	Success,
	NotReady,
	AuthorityRequired,
	InvalidSlot,
	InvalidIdentity,
	InvalidItem,
	LevelTooLow,
	NotFound,
	SlotOccupied,
	InventoryFull,
	DuplicateInstance,
	EffectRejected,
	AppliedInvalid,
	TransferFailed,
	SlotSelectionRequired
};

/** Persistent replicated owner of a player's ability system and attributes. */
UCLASS(Blueprintable, Config = Game)
class UMBRA_API AUmbraPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AUmbraPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	void GrantInitialAbilities();
	void InitializeAttributes();
	/** Once per authoritative PlayerState, independent of ASC and menu lifetime. */
	void InitializeInventory();
	/** Config defaults -> PlayerState Blueprint override. Each entry creates one distinct instance. */
	UPROPERTY(EditDefaultsOnly, Config, BlueprintReadOnly, Category = "Inventory")
	TArray<TSoftObjectPtr<UUmbraItemDefinition>> InitialInventoryItems;

	/** Player-owned ASC. Use it to bind attribute change delegates or apply Gameplay Effects. */
	UFUNCTION(BlueprintPure, Category = "Ability System")
	UUmbraAbilitySystemComponent* GetUmbraAbilitySystemComponent() const { return AbilitySystemComponent; }
	/** Read-only shared attribute set for Blueprint UI/inspection. Mutate through Gameplay Effects. */
	UFUNCTION(BlueprintPure, Category = "Ability System|Attributes")
	UUmbraAttributeSet* GetAttributeSet() const { return AttributeSet; }
	/** The configured primary attack CDO supplies the same 1x period used by combat. */
	const UUmbraBasicAttackAbility* GetPrimaryAttackAbilityDefaults() const;

	/** Consume an inventory item (by identity) and equip it to an empty slot. Fails without touching either side. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory|Transfer")
	EUmbraTransferResult EquipFromInventory(EUmbraEquipmentSlot Slot, FGuid InstanceId);

	/** Unequip an item and return it to the inventory. Requires an empty cell; rolls back on failure. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory|Transfer")
	EUmbraTransferResult UnequipToInventory(EUmbraEquipmentSlot Slot, FGuid ExpectedInstanceId, FUmbraInventoryItem& OutReturnedItem);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	UUmbraInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }
	UFUNCTION(BlueprintPure, Category = "Equipment")
	UUmbraEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UUmbraInventoryComponent> InventoryComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UUmbraEquipmentComponent> EquipmentComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Derived Stats", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UUmbraDerivedStatsComponent> DerivedStatsComponent;
	/** Instant GE containing initial stats; omit Health/Resource (filled afterwards). */
	UPROPERTY(EditDefaultsOnly, Category = "Ability System")
	TSubclassOf<UGameplayEffect> InitialAttributesEffect;

	/** Development tuning override applied after InitialAttributesEffect, once on authority. */
	UPROPERTY(EditAnywhere, Category = "Ability System|Debug Attributes")
	bool bUseDebugInitialAttributes = false;

	UPROPERTY(EditAnywhere, Category = "Ability System|Debug Attributes", meta = (EditCondition = "bUseDebugInitialAttributes"))
	FUmbraDebugInitialAttributes DebugInitialAttributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UUmbraAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UUmbraAttributeSet> AttributeSet;

	/** Abilities granted once by Authority for this PlayerState. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability System", meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UUmbraGameplayAbility>> InitialAbilities;

	bool bInitialAbilitiesGranted = false;
	bool bTransferInProgress = false;
	bool bInitialInventoryGranted = false;
};

