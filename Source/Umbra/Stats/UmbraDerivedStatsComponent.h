#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttributeSet.h"
#include "Items/UmbraWeaponProfile.h"
#include "UmbraDerivedStatsComponent.generated.h"

class UUmbraAbilitySystemComponent;
class UGameplayEffect;
class UUmbraEquipmentComponent;
struct FOnAttributeChangeData;
struct FGameplayEffectSpec;
struct FActiveGameplayEffectsContainer;

USTRUCT(BlueprintType)
struct FUmbraDerivedDamageChannel
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) EUmbraWeaponDamageType Type = EUmbraWeaponDamageType::Blunt;
	UPROPERTY(BlueprintReadOnly) float BaseDamage = 0.f;
	UPROPERTY(BlueprintReadOnly) float ScalingDamage = 0.f;
	UPROPERTY(BlueprintReadOnly) float Damage = 0.f;
};

USTRUCT(BlueprintType)
struct FUmbraDerivedStatsSnapshot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) bool bValid = false;
	UPROPERTY(BlueprintReadOnly) int32 Revision = 0;
	UPROPERTY(BlueprintReadOnly) bool bUnarmed = true;
	UPROPERTY(BlueprintReadOnly) TArray<FUmbraDerivedDamageChannel> Channels;
	UPROPERTY(BlueprintReadOnly) float AttackPower = 0.f;
	UPROPERTY(BlueprintReadOnly) float AbilityPower = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUmbraDerivedStatsChanged, const FUmbraDerivedStatsSnapshot&, Snapshot);

/** Server-owned projection of primary attributes and weapon data, never an additional AD/AP bonus. */
UCLASS(ClassGroup = (Umbra), meta = (BlueprintSpawnableComponent))
class UMBRA_API UUmbraDerivedStatsComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UUmbraDerivedStatsComponent();
	/** Startup opt-in. Legacy is the default; changing modes at runtime is not supported. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Derived Stats")
	bool bUseWeaponDerivedPower = false;
	/** C++ fallback: Blunt 10, no scaling. BP defaults override this whole profile. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Derived Stats")
	FUmbraWeaponDamageProfile UnarmedProfile;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Derived Stats")
	TObjectPtr<UUmbraWeaponProfile> InitialWeapon;

	/** Called after the owning ASC's one-time attribute initialization; safe on avatar rebind. */
	void Initialize(UUmbraAbilitySystemComponent* ASC);
	bool IsDerivedPowerActive() const { return BoundASC.IsValid(); }
	bool IsPublishing() const { return bRefreshing || bEquipmentUpdating; }
	/** Server only. Null selects unarmed. Invalid profiles leave the previous selection intact. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Derived Stats")
	bool SetWeaponProfile(UUmbraWeaponProfile* Profile);
	UFUNCTION(BlueprintPure, Category = "Derived Stats")
	FUmbraDerivedStatsSnapshot GetSnapshot() const { return Snapshot; }
	UPROPERTY(BlueprintAssignable, Category = "Derived Stats")
	FUmbraDerivedStatsChanged OnDerivedStatsChanged;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void OnUnregister() override;

private:
	friend class UUmbraEquipmentComponent;
	bool SetEquipmentWeapon(UUmbraWeaponProfile* Profile, float BaseMultiplier, float ScalingMultiplier);
	TWeakObjectPtr<UUmbraEquipmentComponent> EquipmentOwner;
	bool bEquipmentUpdating = false;
	float WeaponBaseMultiplier = 1.f;
	float WeaponScalingMultiplier = 1.f;
	void Shutdown();
	void Refresh();
	bool Calculate(const FUmbraWeaponDamageProfile& Profile, FUmbraDerivedStatsSnapshot& Out, float BaseFactor = 1.f, float ScalingFactor = 1.f) const;
	void OnPrimaryChanged(const FOnAttributeChangeData& Data);
	bool CanApplyEffect(const FActiveGameplayEffectsContainer& Container, const FGameplayEffectSpec& Spec) const;
	UFUNCTION() void OnRep_Snapshot();
	UPROPERTY(ReplicatedUsing = OnRep_Snapshot)
	FUmbraDerivedStatsSnapshot Snapshot;
	UPROPERTY(Transient) TObjectPtr<UUmbraWeaponProfile> Weapon;
	UPROPERTY(Transient) TObjectPtr<UGameplayEffect> PublishingEffect;
	TWeakObjectPtr<UUmbraAbilitySystemComponent> BoundASC;
	TMap<FGameplayAttribute, FDelegateHandle> PrimaryHandles;
	FDelegateHandle ApplicationQueryHandle;
	bool bPublishing = false;
	bool bRefreshing = false;
	bool bDirty = false;
};
