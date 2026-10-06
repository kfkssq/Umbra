#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "Items/UmbraItemDefinition.h"
#include "UmbraEquipmentComponent.generated.h"

class UUmbraAbilitySystemComponent;
class UUmbraDerivedStatsComponent;

UENUM(BlueprintType)
enum class EUmbraEquipResult : uint8 { Success, NotReady, InvalidItem, WrongSlot, LevelTooLow, DuplicateInstance, EffectRejected, AppliedInvalid };
UENUM(BlueprintType)
enum class EUmbraLoadState : uint8 { Light, Medium, Heavy, Overweight };

UENUM(BlueprintType)
enum class EUmbraUnequipResult : uint8 { Success, NotReady, AuthorityRequired, EmptySlot, StaleInstance, Locked, InvalidSlot, Failed, AppliedInvalid };

USTRUCT(BlueprintType)
struct FUmbraEquippedItem
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) EUmbraEquipmentSlot Slot = EUmbraEquipmentSlot::Head;
	UPROPERTY(BlueprintReadOnly) FGuid InstanceId;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UUmbraItemDefinition> Definition;
	UPROPERTY(BlueprintReadOnly) bool bRequirementsMet = false;
};
USTRUCT(BlueprintType)
struct FUmbraEquipmentSnapshot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) bool bValid = false;
	UPROPERTY(BlueprintReadOnly) int32 Revision = 0;
	UPROPERTY(BlueprintReadOnly) TArray<FUmbraEquippedItem> Items;
	UPROPERTY(BlueprintReadOnly) float EquipLoad = 0.f;
	UPROPERTY(BlueprintReadOnly) float MaxEquipLoad = 0.f;
	UPROPERTY(BlueprintReadOnly) EUmbraLoadState LoadState = EUmbraLoadState::Light;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUmbraEquipmentChanged, const FUmbraEquipmentSnapshot&, Snapshot);

USTRUCT(BlueprintType)
struct FUmbraEquipmentRequirementsPreview
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) bool bKnown = false;
	UPROPERTY(BlueprintReadOnly) int32 CurrentLevel = 0;
	UPROPERTY(BlueprintReadOnly) bool bLevelMet = false;
	UPROPERTY(BlueprintReadOnly) TArray<float> CurrentPrimaries;
	UPROPERTY(BlueprintReadOnly) TArray<bool> PrimaryMet;
	UPROPERTY(BlueprintReadOnly) bool bPrimariesMet = false;
	UPROPERTY(BlueprintReadOnly) float WeaponBaseMultiplier = 1.f;
	UPROPERTY(BlueprintReadOnly) float WeaponScalingMultiplier = 1.f;
	UPROPERTY(BlueprintReadOnly) float DefenseMultiplier = 1.f;
};

/** Server-only mutations; replicated snapshots/events for views. Inventory later validates ownership. */
UCLASS(ClassGroup = (Umbra), meta = (BlueprintSpawnableComponent))
class UMBRA_API UUmbraEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UUmbraEquipmentComponent();
	/** Startup opt-in, requires weapon-derived mode. Disabled preserves InitialWeapon workflow. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment") bool bEnableEquipment = false;
	/** Prototype progression input, fixed for this component's lifetime. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment", meta = (ClampMin = "1")) int32 CharacterLevel = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Penalty", meta = (ClampMin = "0", ClampMax = "1")) float UnmetWeaponBaseMultiplier = 0.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Penalty", meta = (ClampMin = "0", ClampMax = "1")) float UnmetWeaponScalingMultiplier = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Penalty", meta = (ClampMin = "0", ClampMax = "1")) float UnmetDefenseMultiplier = 0.5f;
	/** Capacity in weight units, not a GAS attribute. No movement penalties in this stage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Load", meta = (ClampMin = "0")) float BaseCapacity = 40.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Load", meta = (ClampMin = "0")) float CapacityPerStrength = 0.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Load", meta = (ClampMin = "0", ClampMax = "1")) float LightThreshold = 0.3f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Load", meta = (ClampMin = "0", ClampMax = "1")) float MediumThreshold = 0.7f;
	void Initialize(UUmbraAbilitySystemComponent* ASC);
	/** Trusted authority API. Caller supplies stable item-instance identity; no client RPC. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	EUmbraEquipResult Equip(EUmbraEquipmentSlot Slot, UUmbraItemDefinition* Item, FGuid InstanceId);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment") bool Unequip(EUmbraEquipmentSlot Slot);
	/** Compare the displayed identity before mutation. Does not transfer ownership into an inventory. */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	EUmbraUnequipResult TryUnequipInstance(EUmbraEquipmentSlot Slot, FGuid ExpectedInstanceId, FUmbraEquippedItem& RemovedItem);
	UFUNCTION(BlueprintPure, Category = "Equipment") FUmbraEquipmentSnapshot GetSnapshot() const { return Snapshot; }
	UPROPERTY(BlueprintAssignable, Category = "Equipment") FUmbraEquipmentChanged OnEquipmentChanged;
	bool IsReadyForCombat() const;
	/** Read-only. Excludes the existing target-slot GE, including self when inspecting worn gear.
	 * No target means a generic preview with all currently worn items contributing. */
	bool QueryRequirements(const UUmbraItemDefinition* Item, bool bHasTargetSlot, EUmbraEquipmentSlot TargetSlot,
		FUmbraEquipmentRequirementsPreview& Out) const;
	/** Also called by the derived component, so requirement updates precede AD/AP publication. */
	void Refresh();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void OnUnregister() override;
private:
	bool ReadRequirements(const UUmbraItemDefinition* Item, FActiveGameplayEffectHandle Excluded,
		FUmbraEquipmentRequirementsPreview& Out) const;
	bool ValidateItem(const UUmbraItemDefinition* Item) const;
	bool CanMutate() const;
	void Shutdown();
	void OnEffectRemoved(const FActiveGameplayEffect& Effect);
	UFUNCTION() void OnRep_Snapshot();
	UPROPERTY(ReplicatedUsing = OnRep_Snapshot) FUmbraEquipmentSnapshot Snapshot;
	TWeakObjectPtr<UUmbraAbilitySystemComponent> BoundASC;
	TWeakObjectPtr<UUmbraDerivedStatsComponent> Derived;
	TMap<EUmbraEquipmentSlot, FActiveGameplayEffectHandle> Handles;
	FDelegateHandle RemovedHandle;
	bool bUpdating = false;
	bool bDirty = false;
};
