// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UmbraPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;

UENUM(BlueprintType)
enum class EUmbraPrimaryActionContext : uint8
{
	Ground,
	Ability
};

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AUmbraPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaSeconds) override;

	/** Returns the current visibility-channel hit under the mouse cursor. */
	bool GetCursorGroundHit(FHitResult& OutHitResult) const;

	/** Stops following the current navigation path. */
	void CancelAutoMove();

	/** Returns whether manual movement may proceed, canceling pending primary movement when appropriate. */
	bool TryBeginManualMovement();

protected:
	/** Context-sensitive primary action, configured as left mouse button in the mapping context. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Primary Action")
	TObjectPtr<UInputAction> PrimaryAction;

	/** Time in seconds before primary action input becomes a hold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Primary Action", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PrimaryActionHoldThreshold = 0.25f;

	/** Distance from the final navigation target at which automatic movement stops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Auto Move", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float AutoMoveAcceptanceRadius = 60.0f;

	/** Distance at which the next point along an automatic path becomes active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Auto Move", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float PathPointAcceptanceRadius = 50.0f;

	/** Future extension point: enemy hover can return Ability instead of Ground. */
	UFUNCTION(BlueprintNativeEvent, Category = "Input|Primary Action")
	EUmbraPrimaryActionContext DeterminePrimaryActionContext(const FHitResult& CursorHit) const;

	/** Future ability-context hooks. Intentionally empty during the ground-movement phase. */
	virtual void HandlePrimaryAbilityPressed(const FHitResult& CursorHit);
	virtual void HandlePrimaryAbilityReleased();

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

private:
	void PrimaryActionStarted();
	void PrimaryActionCompleted();
	void UpdateHeldMovement(float DeltaSeconds);
	void UpdateAutoMove();
	void StartAutoMoveToCursor();
	bool GetNavigableCursorLocation(FVector& OutLocation) const;
	void MovePawnToward(const FVector& WorldLocation);
	void ResetPrimaryActionState();

	TArray<FVector> AutoMovePathPoints;
	FHitResult PrimaryActionInitialHit;
	FVector AutoMoveTargetLocation = FVector::ZeroVector;
	int32 CurrentPathPointIndex = INDEX_NONE;
	float PrimaryActionHeldTime = 0.0f;
	bool bPrimaryActionHeld = false;
	bool bPrimaryActionIsHold = false;
	bool bAutoMoving = false;
	EUmbraPrimaryActionContext ActivePrimaryActionContext = EUmbraPrimaryActionContext::Ground;

};
