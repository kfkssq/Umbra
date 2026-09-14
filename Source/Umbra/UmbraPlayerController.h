// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffectTypes.h"
#include "UmbraPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;
class UUmbraDamageNumber;
class UUmbraAttributeDebugPanel;
class UAbilitySystemComponent;

UENUM()
enum class EUmbraAttributeDebugOperation : uint8
{
	AddEffect,
	RemoveEffect,
	Damage,
	Heal
};

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
	UFUNCTION(Client, Unreliable)
	void ClientShowDamageNumber(FVector WorldPosition, float Damage, uint8 Type, bool bCritical);
	void ReleaseDamageNumber(UUmbraDamageNumber* Number);
	virtual void Tick(float DeltaSeconds) override;

	void RequestAttributeDebugOperation(AActor* Target, EUmbraAttributeDebugOperation Operation);
	void StopPointerActionsForDebugUI();
	bool IsPointerOverAttributeDebugPanel() const;
	void AttributeDebugPanelRemoved(UUmbraAttributeDebugPanel* RemovedPanel);

	UFUNCTION(BlueprintCallable, Category = "Debug|Attributes", meta = (DevelopmentOnly))
	void RemoveAttributeDebugPanel();

	/** Enables the fixed-key on-screen diagnostics for cursor attack highlighting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowAttackHighlightDebug = true;

	/** Forces the Pawn currently under the cursor to write stencil 1 for isolation testing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bForceHighlightDebug = false;

	bool ShouldShowAttackHighlightDebug() const { return bShowAttackHighlightDebug; }

	/** Returns the current visibility-channel hit under the mouse cursor. */
	bool GetCursorGroundHit(FHitResult& OutHitResult) const;

	/** Stops following the current navigation path. */
	void CancelAutoMove();

	/** Returns whether manual movement may proceed, canceling pending primary movement when appropriate. */
	bool TryBeginManualMovement();

protected:
	UPROPERTY(EditDefaultsOnly, Category="UI|Damage Numbers")
	TSubclassOf<UUmbraDamageNumber> DamageNumberClass;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRep_PlayerState() override;

	/** Must also be enabled on the server's controller class. Ignored in Shipping/Test. */
	UPROPERTY(EditDefaultsOnly, Category = "Debug|Attributes")
	bool bEnableAttributeDebugPanel = false;

	UPROPERTY(EditDefaultsOnly, Category = "Debug|Attributes")
	TSubclassOf<UUmbraAttributeDebugPanel> AttributeDebugPanelClass;

	UPROPERTY(EditDefaultsOnly, Category = "Debug|Attributes")
	TObjectPtr<UInputMappingContext> AttributeDebugMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Debug|Attributes")
	TObjectPtr<UInputAction> ViewPlayerAttributesAction;

	UPROPERTY(EditDefaultsOnly, Category = "Debug|Attributes")
	TObjectPtr<UInputAction> LockHoveredAttributesAction;

	/** Context-sensitive primary action, configured as left mouse button in the mapping context. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Primary Action")
	TObjectPtr<UInputAction> PrimaryAction;

	/** Existing IA_Attack_Primary action used to select and attack cursor targets. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Primary Action")
	TObjectPtr<UInputAction> PrimaryAttackAction;

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
	void ClearDamageNumbers();
	UPROPERTY(Transient)
	TArray<TObjectPtr<UUmbraDamageNumber>> ActiveDamageNumbers;
	void CreateAttributeDebugPanel();
	void SetupAttributeDebugInput();
	void CleanupAttributeDebugInput();
	void ViewPlayerAttributes();
	void LockHoveredAttributes();
	void ClearAttributeDebugEffects();
	static bool IsAttackableTarget(AActor* Target);

	UFUNCTION(Server, Reliable)
	void ServerAttributeDebugOperation(AActor* Target, EUmbraAttributeDebugOperation Operation);

	UFUNCTION(Server, Reliable)
	void ServerClearAttributeDebugEffects();

	UPROPERTY(Transient)
	TObjectPtr<UUmbraAttributeDebugPanel> AttributeDebugPanel;

	// Authority-only handles; each controller can remove only its own effects.
	TMap<TWeakObjectPtr<UAbilitySystemComponent>, FActiveGameplayEffectHandle> AttributeDebugEffects;
	TArray<uint32> AttributeDebugInputBindingHandles;
	bool bAddedAttributeDebugMapping = false;
	bool bEndingPlay = false;

	void PrimaryActionStarted();
	void PrimaryActionCompleted();
	void PrimaryAttackStarted();
	void UpdateHeldMovement(float DeltaSeconds);
	void UpdateAutoMove();
	void UpdatePendingAttack();
	void UpdateAttackHover();
	void UpdateAttackHighlightDebug();
	void ClearAttackHighlightDebugMessages() const;
	void StartAutoMoveToCursor();
	bool StartAutoMoveToLocation(const FVector& Destination);
	void BeginAttackTarget(AActor* TargetActor);
	void CancelPendingAttack();
	bool GetAttackableUnderCursor(AActor*& OutTargetActor, FHitResult* OutCursorHit = nullptr) const;
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
	TWeakObjectPtr<AActor> PendingAttackTarget;
	TWeakObjectPtr<AActor> HoveredAttackTarget;
	FHitResult AttackHighlightDebugHit;
	bool bAttackHighlightDebugHasPawnHit = false;
	EUmbraPrimaryActionContext ActivePrimaryActionContext = EUmbraPrimaryActionContext::Ground;

};
