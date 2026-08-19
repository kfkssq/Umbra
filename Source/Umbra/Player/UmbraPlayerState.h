// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "UmbraPlayerState.generated.h"

class UUmbraAbilitySystemComponent;
class UUmbraAttributeSet;
class UUmbraGameplayAbility;

/** Persistent replicated owner of a player's ability system and attributes. */
UCLASS(Blueprintable)
class UMBRA_API AUmbraPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AUmbraPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	void GrantInitialAbilities();

	UUmbraAbilitySystemComponent* GetUmbraAbilitySystemComponent() const { return AbilitySystemComponent; }
	const UUmbraAttributeSet* GetAttributeSet() const { return AttributeSet; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UUmbraAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UUmbraAttributeSet> AttributeSet;

	/** Abilities granted once by Authority for this PlayerState. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability System", meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UUmbraGameplayAbility>> InitialAbilities;

	bool bInitialAbilitiesGranted = false;
};
