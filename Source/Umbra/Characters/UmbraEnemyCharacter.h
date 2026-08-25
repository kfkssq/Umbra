// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/UmbraAttackable.h"
#include "UmbraEnemyCharacter.generated.h"

class UAnimMontage;
class UUmbraAbilitySystemComponent;
class UUmbraAttributeSet;

/** Reusable GAS-enabled base character for enemy Blueprints. */
UCLASS()
class UMBRA_API AUmbraEnemyCharacter : public ACharacter, public IUmbraAttackable, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AUmbraEnemyCharacter();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UAnimMontage* GetHitReactMontage() const { return HitReactMontage; }
	virtual bool CanBeAttacked_Implementation() const override;
	virtual void SetAttackHighlighted_Implementation(bool bHighlighted) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
	TObjectPtr<UUmbraAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
	TObjectPtr<UUmbraAttributeSet> AttributeSet;

	/** Montage played when the enemy ASC receives Event.Combat.HitReact. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Hit React")
	TObjectPtr<UAnimMontage> HitReactMontage;

	/** Custom-depth stencil used by the project's enemy-highlight post-process material. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Highlight", meta = (ClampMin = "0", ClampMax = "255", UIMin = "0", UIMax = "255"))
	int32 AttackHighlightStencilValue = 1;
};
