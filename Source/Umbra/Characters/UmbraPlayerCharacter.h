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

	/** Sets a horizontal facing target without owning input state. */
	void SetFacingTargetLocation(const FVector& WorldLocation);

	/** Applies movement input toward a world-space location. */
	void MoveTowardWorldLocation(const FVector& WorldLocation);

	/** Maximum two-dimensional distance at which a basic attack may begin. */
	float GetPrimaryAttackRange() const { return PrimaryAttackRange; }

	/** Requests the existing GAS basic-attack ability against the selected target. */
	bool TryActivatePrimaryAttack(AActor* TargetActor);
	/** Authority-side target validation for the owned ASC's attack intent RPC. */
	void ReceivePrimaryAttackIntent(AActor* TargetActor, bool bCombo);

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

	/** How quickly the character interpolates toward movement or clicked facing directions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MouseFacingInterpSpeed = 12.0f;

	/** Two-dimensional movement action configured by the derived Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	/** Ability input actions forwarded to the ASC by gameplay tag. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Abilities")
	TArray<FUmbraTaggedInputAction> AbilityInputActions;

	/** Maximum two-dimensional distance to the selected target before attacking. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Basic Attack", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PrimaryAttackRange = 200.0f;

private:
	void Move(const FInputActionValue& Value);
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	void UpdateFacingRotation(float DeltaSeconds);
	void InitializeAbilitySystem();

	float DesiredFacingYaw = 0.0f;
	bool bHasDesiredFacing = false;
	bool bMovementEnabled = true;
	TWeakObjectPtr<AActor> PrimaryAttackTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCamera;
};
