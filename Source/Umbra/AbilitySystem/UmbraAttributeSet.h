#pragma once
#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "UmbraAttributeSet.generated.h"

#define UMBRA_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/** Shared player/enemy attributes. Change through GEs.
	* Regen: points/sec; MoveSpeed: cm/sec; attack-speed bonus and crit chance: fractions (0.2 = 20%).
	* Crit multiplier: total damage (2 = double); haste: numeric rating (50).
	* GAS BaseValue is an aggregation input, not a character progression stat.
	*/
UCLASS()
class UMBRA_API UUmbraAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	static constexpr float MinAttackSpeedMultiplier = 0.2f;
	static constexpr float MaxAttackSpeedMultiplier = 10.f;
	static constexpr float MinAttackSpeedBonus = MinAttackSpeedMultiplier - 1.f;
	static constexpr float MaxAttackSpeedBonus = MaxAttackSpeedMultiplier - 1.f;

	UUmbraAttributeSet();
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Attributes", meta = (ClampMin = "0.0", DisplayName = "Health (Debug Base)"))
	FGameplayAttributeData Health;
	UMBRA_ATTRIBUTE_ACCESSORS(UUmbraAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Attributes", meta = (ClampMin = "1.0", DisplayName = "Max Health (Debug Base)"))
	FGameplayAttributeData MaxHealth;
	UMBRA_ATTRIBUTE_ACCESSORS(UUmbraAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HealthRegen, Category = "Attributes", meta = (ClampMin = "0.0"))
	FGameplayAttributeData HealthRegen;
	UMBRA_ATTRIBUTE_ACCESSORS(UUmbraAttributeSet, HealthRegen)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Resource, Category = "Attributes", meta = (ClampMin = "0.0"))
	FGameplayAttributeData Resource;
	UMBRA_ATTRIBUTE_ACCESSORS(UUmbraAttributeSet, Resource)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxResource, Category = "Attributes", meta = (ClampMin = "0.0"))
	FGameplayAttributeData MaxResource;
	UMBRA_ATTRIBUTE_ACCESSORS(UUmbraAttributeSet, MaxResource)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ResourceRegen, Category = "Attributes", meta = (ClampMin = "0.0"))
	FGameplayAttributeData ResourceRegen;
	UMBRA_ATTRIBUTE_ACCESSORS(UUmbraAttributeSet, ResourceRegen)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackPower, Category = "Attributes", meta = (ClampMin = "0.0"))
	FGameplayAttributeData AttackPower;
	UMBRA_ATTRIBUTE_ACCESSORS(UUmbraAttributeSet, AttackPower)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AbilityPower, Category = "Attributes", meta = (ClampMin = "0.0"))
	FGameplayAttributeData AbilityPower;
	UMBRA_ATTRIBUTE_ACCESSORS(UUmbraAttributeSet, AbilityPower)

	/** Additive fraction converted to the logical attack speed multiplier as 1 + AttackSpeedBonus. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackSpeedBonus, Category = "Attributes", meta = (ClampMin = "-0.8", ClampMax = "9.0"))
	FGameplayAttributeData AttackSpeedBonus;
	UMBRA_ATTRIBUTE_ACCESSORS(UUmbraAttributeSet, AttackSpeedBonus)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalChance, Category = "Attributes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	FGameplayAttributeData CriticalChance;
	UMBRA_ATTRIBUTE_ACCESSORS(UUmbraAttributeSet, CriticalChance)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalDamageMultiplier, Category = "Attributes", meta = (ClampMin = "1.0"))
	FGameplayAttributeData CriticalDamageMultiplier;
	UMBRA_ATTRIBUTE_ACCESSORS(UUmbraAttributeSet, CriticalDamageMultiplier)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Armor, Category = "Attributes", meta = (ClampMin = "0.0"))
	FGameplayAttributeData Armor;
	UMBRA_ATTRIBUTE_ACCESSORS(UUmbraAttributeSet, Armor)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MagicResistance, Category = "Attributes", meta = (ClampMin = "0.0"))
	FGameplayAttributeData MagicResistance;
	UMBRA_ATTRIBUTE_ACCESSORS(UUmbraAttributeSet, MagicResistance)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AbilityHaste, Category = "Attributes", meta = (ClampMin = "0.0"))
	FGameplayAttributeData AbilityHaste;
	UMBRA_ATTRIBUTE_ACCESSORS(UUmbraAttributeSet, AbilityHaste)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeed, Category = "Attributes", meta = (ClampMin = "0.0"))
	FGameplayAttributeData MoveSpeed;
	UMBRA_ATTRIBUTE_ACCESSORS(UUmbraAttributeSet, MoveSpeed)

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Attributes")
	FGameplayAttributeData IncomingDamage;
	UMBRA_ATTRIBUTE_ACCESSORS(UUmbraAttributeSet, IncomingDamage)

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_HealthRegen(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Resource(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxResource(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_ResourceRegen(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_AttackPower(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_AbilityPower(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_AttackSpeedBonus(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_CriticalChance(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_CriticalDamageMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Armor(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MagicResistance(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_AbilityHaste(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);
private:
	void ClampAttribute(const FGameplayAttribute& Attribute, float& Value) const;
};

