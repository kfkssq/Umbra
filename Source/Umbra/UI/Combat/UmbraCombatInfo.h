#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "UI/Combat/UmbraCombatStatData.h"
#include "Stats/UmbraDerivedStatsComponent.h"
#include "Equipment/UmbraEquipmentComponent.h"
#include "UmbraCombatInfo.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UUmbraCombatStatEntry;
class AUmbraPlayerState;
class UUmbraAbilitySystemComponent;
struct FOnAttributeChangeData;

/** Existing second page: observes gameplay-owned data, preserves Designer rows and layout. */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraCombatInfo : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Combat UI")
	void NotifyPlayerContextChanged();
	/** For optional dynamically inserted rows. Existing Designer rows are discovered on construction. */
	UFUNCTION(BlueprintCallable, Category = "Combat UI")
	void RegisterCombatEntry(UUmbraCombatStatEntry* Entry);
	void Shutdown();

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> AttackToggle;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> DefenseToggle;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> AttackArrow;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> DefenseArrow;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> AttackRows;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> DefenseRows;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UButton> UtilityToggle;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> UtilityArrow;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> UtilityRows;

private:
	UFUNCTION()
	void ToggleAttack();
	UFUNCTION()
	void ToggleDefense();
	UFUNCTION() void ToggleUtility();
	void RefreshSections();
	void BindPlayer();
	void UnbindData();
	void RefreshValues();
	void ObserveEffect(FActiveGameplayEffectHandle Handle);
	void OnAttribute(const FOnAttributeChangeData& Data);
	void OnEffectAdded(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);
	void OnEffectRemoved(const FActiveGameplayEffect& Effect);
	void OnLifecycle(UUmbraAbilitySystemComponent* ASC, bool bReady);
	UFUNCTION() void OnPawnChanged(APawn* OldPawn, APawn* NewPawn);
	UFUNCTION() void OnDerived(const FUmbraDerivedStatsSnapshot& Snapshot);
	UFUNCTION() void OnEquipment(const FUmbraEquipmentSnapshot& Snapshot);
	TArray<TWeakObjectPtr<UUmbraCombatStatEntry>> Entries;
	TWeakObjectPtr<AUmbraPlayerState> BoundPlayerState;
	TWeakObjectPtr<UUmbraAbilitySystemComponent> BoundASC;
	TWeakObjectPtr<UUmbraDerivedStatsComponent> BoundDerived;
	TWeakObjectPtr<UUmbraEquipmentComponent> BoundEquipment;
	TMap<FGameplayAttribute, FDelegateHandle> AttributeHandles;
	TSet<FActiveGameplayEffectHandle> ObservedEffects;
	FDelegateHandle AddedHandle, RemovedHandle, LifecycleHandle;
	bool bObserving = false;

	// Retain expansion when the same menu instance is removed and re-added.
	bool bAttackExpanded = true;
	bool bDefenseExpanded = true;
	bool bUtilityExpanded = true;
};
