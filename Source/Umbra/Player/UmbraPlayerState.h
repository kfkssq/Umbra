// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/UmbraDebugInitialAttributes.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "UmbraPlayerState.generated.h"

class UUmbraAbilitySystemComponent;
class UUmbraAttributeSet;
class UUmbraGameplayAbility;
class UUmbraBasicAttackAbility;
class UGameplayEffect;

/** Persistent replicated owner of a player's ability system and attributes. */
UCLASS(Blueprintable)
class UMBRA_API AUmbraPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AUmbraPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	void GrantInitialAbilities();
	void InitializeAttributes();

	/** Player-owned ASC. Use it to bind attribute change delegates or apply Gameplay Effects. */
	UFUNCTION(BlueprintPure, Category = "Ability System")
	UUmbraAbilitySystemComponent* GetUmbraAbilitySystemComponent() const { return AbilitySystemComponent; }
	/** Read-only shared attribute set for Blueprint UI/inspection. Mutate through Gameplay Effects. */
	UFUNCTION(BlueprintPure, Category = "Ability System|Attributes")
	UUmbraAttributeSet* GetAttributeSet() const { return AttributeSet; }
	/** The configured primary attack CDO supplies the same 1x period used by combat. */
	const UUmbraBasicAttackAbility* GetPrimaryAttackAbilityDefaults() const;

private:
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
};

