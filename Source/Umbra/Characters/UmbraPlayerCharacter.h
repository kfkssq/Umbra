// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "UmbraPlayerCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class USpringArmComponent;
class UAbilitySystemComponent;
struct FInputActionValue;

/** Enhanced Input action associated with a GAS input tag. */
USTRUCT(BlueprintType)
struct FUmbraTaggedInputAction
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (Categories = "Input"))
	FGameplayTag InputTag;
};

/**
 * Minimal top-down player character for the Umbra prototype.
 * Asset references and presentation are configured by a derived Blueprint.
 */
UCLASS()
class UMBRA_API AUmbraPlayerCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AUmbraPlayerCharacter();
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Applies movement input toward a world-space location. */
	void MoveTowardWorldLocation(const FVector& WorldLocation);

	/** Gives one attack instance exclusive stationary-facing ownership. */
	void BeginPrimaryAttackFacing(uint32 AttackInstanceId, const FVector& WorldLocation);

	/** Returns facing to CharacterMovement without changing the actor's current rotation. */
	void EndPrimaryAttackFacing(uint32 AttackInstanceId);

	/** Maximum two-dimensional distance at which a basic attack may begin. */
	float GetPrimaryAttackRange() const { return PrimaryAttackRange; }
	bool IsTargetInPrimaryAttackRange(const AActor* TargetActor, float ExtraTolerance = 0.f) const;

	/**
	 * Play-rate scale for start/stop locomotion clips, derived from the GAS-driven MaxWalkSpeed.
	 * Anim Blueprints should read this value instead of duplicating the MoveSpeed attribute.
	 */
	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	float GetLocomotionAnimationPlayRate() const;

	/** Returns the movement-facing yaw rate for a given MoveSpeed without changing the character. */
	UFUNCTION(BlueprintPure, Category = "Movement|Rotation")
	float CalculateMovementYawRate(float MoveSpeed) const;

	/** Called by the ASC's MoveSpeed delegate; does not take over attack-facing ownership. */
	void ApplyMoveSpeedDrivenYawRate(float MoveSpeed);

	/** Requests the existing GAS basic-attack ability against the selected target. */
	bool TryActivatePrimaryAttack(AActor* TargetActor, bool bContinueAttacking = false);
	/** Authority-side target validation for the owned ASC's attack intent RPC. */
	void ReceivePrimaryAttackIntent(AActor* TargetActor, bool bCombo, bool bContinueAttacking);
	void StopPrimaryAttackContinuation();
	bool ShouldContinuePrimaryAttack() const { return bContinuePrimaryAttack; }

	/** Target consumed by the active basic-attack ability for facing. */
	AActor* GetPrimaryAttackTarget() const { return PrimaryAttackTarget.Get(); }
	void ClearPrimaryAttackTarget() { PrimaryAttackTarget.Reset(); }

	/** Whether movement input is accepted. False while the initial spawn animation is playing. */
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsMovementEnabled() const { return bMovementEnabled; }

	/** Backward-compatible animation hook. Movement now starts enabled, so this normally has no work to do. */
	UFUNCTION(BlueprintCallable, Category = "Animation|Spawn")
	void FinishSpawnAnimation();

	/** Returns the fixed top-down camera boom. */
	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns the top-down camera. */
	UCameraComponent* GetTopDownCamera() const { return TopDownCamera; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Two-dimensional movement action configured by the derived Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	/** Ability input actions forwarded to the ASC by gameplay tag. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Abilities")
	TArray<FUmbraTaggedInputAction> AbilityInputActions;

	/** Maximum two-dimensional distance to the selected target before attacking. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Basic Attack", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PrimaryAttackRange = 200.0f;

	/** MaxWalkSpeed at which locomotion start/stop animations play at their authored 1x rate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion",
		meta = (ClampMin = "1.0", UIMin = "1.0", Units = "cm/s",
			ToolTip = "Set this to the movement speed for which the locomotion clips were authored."))
	float LocomotionAnimationReferenceSpeed = 500.f;

	/** Lower safety bound applied to the Anim Blueprint locomotion play-rate output. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion",
		meta = (ClampMin = "0.05", ClampMax = "10.0", UIMin = "0.1", UIMax = "2.0", Units = "x"))
	float MinLocomotionAnimationPlayRate = 0.25f;

	/** Upper safety bound applied to the Anim Blueprint locomotion play-rate output. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion",
		meta = (ClampMin = "0.05", ClampMax = "10.0", UIMin = "1.0", UIMax = "5.0", Units = "x"))
	float MaxLocomotionAnimationPlayRate = 3.f;

	/** CharacterMovement yaw rate at LocomotionAnimationReferenceSpeed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Rotation",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "deg/s",
			ToolTip = "Yaw turn rate at the shared locomotion reference speed; higher MoveSpeed scales it proportionally."))
	float BaseMovementYawRate = 600.f;

	/** Safety ceiling for speed-driven CharacterMovement yaw rotation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Rotation",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "deg/s",
			ToolTip = "Maximum CharacterMovement RotationRate.Yaw after MoveSpeed scaling."))
	float MaxMovementYawRate = 1800.f;

private:
	void Move(const FInputActionValue& Value);
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	void InitializeAbilitySystem();

	uint32 PrimaryAttackFacingInstanceId = 0;
	bool bMovementEnabled = true;
	bool bContinuePrimaryAttack = false;
	TWeakObjectPtr<AActor> PrimaryAttackTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCamera;
};
