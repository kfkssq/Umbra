// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/UmbraAttackable.h"
#include "UmbraEnemyCharacter.generated.h"

class UAnimMontage;
class UGameplayAbility;
class UUmbraAbilitySystemComponent;
class UUmbraAttributeSet;
struct FOnAttributeChangeData;

/** Reusable GAS-enabled base character for enemy Blueprints. */
UCLASS()
class UMBRA_API AUmbraEnemyCharacter : public ACharacter, public IUmbraAttackable, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AUmbraEnemyCharacter();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UAnimMontage* GetHitReactMontage() const { return HitReactMontage; }
	UAnimMontage* GetDeathMontage() const { return DeathMontage; }
	bool IsDead() const { return bIsDead; }
	AActor* GetCombatTarget() const { return CombatTarget.Get(); }
	void SetCombatTarget(AActor* NewTarget) { CombatTarget = NewTarget; }
	bool TryActivateBasicAttack();
	virtual bool CanBeAttacked_Implementation() const override;
	virtual void SetAttackHighlighted_Implementation(bool bHighlighted) override;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Death")
	void OnDeathStarted();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
	TObjectPtr<UUmbraAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
	TObjectPtr<UUmbraAttributeSet> AttributeSet;

	/** Ground speed applied when the enemy begins play. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float EnemyMoveSpeed = 300.0f;

	/** Montage played when the enemy ASC receives Event.Combat.HitReact. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Hit React")
	TObjectPtr<UAnimMontage> HitReactMontage;

	/** Montage played once when Health reaches zero. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Death")
	TObjectPtr<UAnimMontage> DeathMontage;

	/** Enemy attack ability, normally a Blueprint child of UUmbraEnemyBasicAttackAbility. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Attack")
	TSubclassOf<UGameplayAbility> BasicAttackAbilityClass;

	/** Destroy after death; zero keeps the corpse indefinitely. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Death", meta = (ClampMin = "0.0"))
	float CorpseLifetime = 5.0f;

	/** Freeze shortly before the death montage ends so the corpse cannot return to idle. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Death", meta = (ClampMin = "0.0"))
	float DeathPoseFreezeLeadTime = 0.25f;

	/** Custom-depth stencil used by the project's enemy-highlight post-process material. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Highlight", meta = (ClampMin = "0", ClampMax = "255", UIMin = "0", UIMax = "255"))
	int32 AttackHighlightStencilValue = 1;

private:
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	void Die();
	void ApplyDeathState();
	void FreezeDeathPose();

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;

	UFUNCTION()
	void OnRep_IsDead();

	bool bDeathStateApplied = false;
	TWeakObjectPtr<AActor> CombatTarget;
	FTimerHandle DeathPoseTimerHandle;
};
