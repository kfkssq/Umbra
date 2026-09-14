// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "AbilitySystem/UmbraDebugInitialAttributes.h"
#include "GameFramework/Character.h"
#include "Interfaces/UmbraAttackable.h"
#include "UmbraEnemyCharacter.generated.h"

class UAnimMontage;
class UUmbraEnemyHealthBarComponent;
class UGameplayAbility;
class UGameplayEffect;
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
	/** Enemy-owned ASC. Use it to bind attribute change delegates or apply Gameplay Effects. */
	UFUNCTION(BlueprintPure, Category = "Ability System")
	UUmbraAbilitySystemComponent* GetUmbraAbilitySystemComponent() const { return AbilitySystemComponent; }
	/** Read-only shared attribute set for Blueprint UI/inspection. Mutate through Gameplay Effects. */
	UFUNCTION(BlueprintPure, Category = "Ability System|Attributes")
	UUmbraAttributeSet* GetAttributeSet() const { return AttributeSet; }
	UAnimMontage* GetHitReactMontage() const { return HitReactMontage; }
	UAnimMontage* GetDeathMontage() const { return DeathMontage; }
	bool IsDead() const { return bIsDead; }
	bool IsAIBehaviorEnabled() const { return bAIBehaviorEnabled; }
	AActor* GetCombatTarget() const { return CombatTarget.Get(); }
	void SetCombatTarget(AActor* NewTarget) { CombatTarget = NewTarget; }
	bool TryActivateBasicAttack();
	virtual bool CanBeAttacked_Implementation() const override;
	virtual void SetAttackHighlighted_Implementation(bool bHighlighted) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUmbraEnemyHealthBarComponent> HealthBarComponent;

	/** Spawn-time setting. Disable for a passive test target; GAS, hit reactions and death remain active. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (DisplayName = "Enable AI Behavior"))
	bool bAIBehaviorEnabled = true;

	/** Enemy-specific Instant GE; Health/Resource are filled after initialization. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability System")
	TSubclassOf<UGameplayEffect> InitialAttributesEffect;

	/** Development tuning override applied after InitialAttributesEffect, once on authority. */
	UPROPERTY(EditAnywhere, Category = "Ability System|Debug Attributes")
	bool bUseDebugInitialAttributes = false;

	UPROPERTY(EditAnywhere, Category = "Ability System|Debug Attributes", meta = (EditCondition = "bUseDebugInitialAttributes"))
	FUmbraDebugInitialAttributes DebugInitialAttributes;

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

