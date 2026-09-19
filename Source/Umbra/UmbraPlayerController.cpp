// Copyright Epic Games, Inc. All Rights Reserved.

#include "UmbraPlayerController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Characters/UmbraPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/HUD.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "InputMappingContext.h"
#include "Interfaces/UmbraAttackable.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Umbra.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "UI/UmbraAttributeDebugPanel.h"

namespace UmbraAttackHighlightDebug
{
	constexpr uint64 StatusMessageKey = 1001;
	constexpr uint64 HoverEventMessageKey = 1002;
	constexpr uint64 RequestEventMessageKey = 1003;
	constexpr uint64 ForceTestMessageKey = 1004;
	constexpr uint64 FunctionEventMessageKey = 1010;
}

void AUmbraPlayerController::BeginPlay()
{
	Super::BeginPlay();
	InitializeCombatHUD();

	if (IsLocalPlayerController())
	{
		bShowMouseCursor = true;
		DefaultMouseCursor = EMouseCursor::Default;

		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		UE_LOG(LogUmbra, Log, TEXT("Attack highlight debug: local controller %s is updating cursor hover."), *GetNameSafe(this));
	}

	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);
		if (MobileControlsWidget)
		{
			MobileControlsWidget->AddToPlayerScreen(0);
		}
		else
		{
			UE_LOG(LogUmbra, Error, TEXT("Could not spawn mobile controls widget."));
		}
	}
	CreateAttributeDebugPanel();
}

void AUmbraPlayerController::SetPawn(APawn* InPawn)
{
	if (InPawn != GetPawn())
	{
		// Command state belongs to one avatar and must never leak across respawn/possession.
		CancelCombatCommandState();
	}
	Super::SetPawn(InPawn);
}

void AUmbraPlayerController::InitializeCombatHUD()
{
	if (!IsLocalController() || IsValid(CombatHUD))
	{
		return;
	}

	if (!CombatHUDClass)
	{
		UE_LOG(LogUmbra, Warning, TEXT("Combat HUD class is not configured on %s."), *GetNameSafe(this));
		return;
	}

	CombatHUD = CreateWidget<UUserWidget>(this, CombatHUDClass);
	if (!IsValid(CombatHUD))
	{
		UE_LOG(LogUmbra, Error, TEXT("Failed to create combat HUD %s for %s."),
			*GetNameSafe(CombatHUDClass), *GetNameSafe(this));
		return;
	}

	CombatHUD->AddToViewport();
	UE_LOG(LogUmbra, Log, TEXT("Created combat HUD %s for local controller %s."),
		*GetNameSafe(CombatHUD), *GetNameSafe(this));
}

void AUmbraPlayerController::ShowCombatHUD()
{
	if (!IsValid(CombatHUD))
	{
		InitializeCombatHUD();
	}
	if (IsValid(CombatHUD))
	{
		CombatHUD->SetVisibility(ESlateVisibility::Visible);
	}
}

void AUmbraPlayerController::HideCombatHUD()
{
	if (IsValid(CombatHUD))
	{
		CombatHUD->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AUmbraPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsLocalPlayerController())
	{
		return;
	}

	UpdateAttackHover();
	UpdateAttackHighlightDebug();

	if (bPrimaryActionHeld && ActivePrimaryActionContext == EUmbraPrimaryActionContext::Ground)
	{
		UpdateHeldMovement(DeltaSeconds);
	}
	else if (PendingAttackTarget.IsValid())
	{
		UpdatePendingAttack();
	}
	else if (bAutoMoving)
	{
		UpdateAutoMove();
	}
}

void AUmbraPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(CombatHUD))
	{
		CombatHUD->RemoveFromParent();
		CombatHUD = nullptr;
	}
	ClearDamageNumbers();
	bEndingPlay = true;
	RemoveAttributeDebugPanel();
	ClearAttributeDebugEffects();
	ClearAttackHighlightDebugMessages();
	if (AActor* HoveredActor = HoveredAttackTarget.Get())
	{
		IUmbraAttackable::Execute_SetAttackHighlighted(HoveredActor, false);
	}
	HoveredAttackTarget.Reset();
	CancelCombatCommandState();

	Super::EndPlay(EndPlayReason);
}

bool AUmbraPlayerController::GetCursorGroundHit(FHitResult& OutHitResult) const
{
	return IsLocalPlayerController()
		&& !IsPointerOverAttributeDebugPanel()
		&& GetHitResultUnderCursor(ECC_Visibility, false, OutHitResult)
		&& OutHitResult.bBlockingHit;
}

void AUmbraPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	SetupAttributeDebugInput();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (PrimaryAction)
		{
			EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Started, this, &AUmbraPlayerController::PrimaryActionStarted);
			EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Completed, this, &AUmbraPlayerController::PrimaryActionCompleted);
			EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Canceled, this, &AUmbraPlayerController::PrimaryActionCompleted);
		}
		else
		{
			UE_LOG(LogUmbra, Warning, TEXT("No PrimaryAction is configured for %s."), *GetNameSafe(this));
		}

		if (PrimaryAttackAction)
		{
			EnhancedInputComponent->BindAction(PrimaryAttackAction, ETriggerEvent::Started, this, &AUmbraPlayerController::PrimaryAttackStarted);
		}
		else
		{
			UE_LOG(LogUmbra, Warning, TEXT("No PrimaryAttackAction is configured for %s."), *GetNameSafe(this));
		}
	}

	if (IsLocalPlayerController())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool AUmbraPlayerController::ShouldUseTouchControls() const
{
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

EUmbraPrimaryActionContext AUmbraPlayerController::DeterminePrimaryActionContext_Implementation(const FHitResult& CursorHit) const
{
	const AActor* HitActor = CursorHit.GetActor();
	return HitActor && HitActor->Implements<UUmbraAttackable>()
		? EUmbraPrimaryActionContext::Ability
		: EUmbraPrimaryActionContext::Ground;
}

void AUmbraPlayerController::HandlePrimaryAbilityPressed(const FHitResult& CursorHit)
{
}

void AUmbraPlayerController::HandlePrimaryAbilityReleased()
{
}

void AUmbraPlayerController::PrimaryActionStarted()
{
	if (IsPointerOverAttributeDebugPanel())
	{
		StopPointerActionsForDebugUI();
		return;
	}
	bPrimaryActionHeld = false;
	bPrimaryActionIsHold = false;
	PrimaryActionHeldTime = 0.0f;
	AActor* AttackableActor = nullptr;
	if (GetAttackableUnderCursor(AttackableActor))
	{
		ActivePrimaryActionContext = EUmbraPrimaryActionContext::Ability;
		return;
	}

	if (!GetCursorGroundHit(PrimaryActionInitialHit))
	{
		return;
	}

	AActor* HitActor = PrimaryActionInitialHit.GetActor();
	ActivePrimaryActionContext = HitActor && HitActor->Implements<UUmbraAttackable>()
		? EUmbraPrimaryActionContext::Ability
		: DeterminePrimaryActionContext(PrimaryActionInitialHit);
	if (ActivePrimaryActionContext == EUmbraPrimaryActionContext::Ability)
	{
		HandlePrimaryAbilityPressed(PrimaryActionInitialHit);
		return;
	}
	if (const AUmbraPlayerCharacter* UmbraCharacter = Cast<AUmbraPlayerCharacter>(GetPawn());
		!UmbraCharacter || !UmbraCharacter->IsMovementEnabled())
	{
		return;
	}

	bPrimaryActionHeld = true;
	if (!IsPrimaryAttackActive())
	{
		CancelPendingAttack();
		CancelAutoMove();
	}
}

void AUmbraPlayerController::PrimaryAttackStarted()
{
	if (IsPointerOverAttributeDebugPanel())
	{
		return;
	}
	AActor* AttackableActor = nullptr;
	if (GetAttackableUnderCursor(AttackableActor))
	{
		BeginAttackTarget(AttackableActor);
	}
}

void AUmbraPlayerController::PrimaryActionCompleted()
{
	if (IsPointerOverAttributeDebugPanel())
	{
		StopPointerActionsForDebugUI();
		return;
	}
	if (ActivePrimaryActionContext == EUmbraPrimaryActionContext::Ability)
	{
		HandlePrimaryAbilityReleased();
	}
	else if (bPrimaryActionHeld && !bPrimaryActionIsHold)
	{
		if (IsPrimaryAttackActive())
		{
			QueueMoveCommand(PrimaryActionInitialHit.ImpactPoint);
		}
		else
		{
			StartAutoMoveToCursor();
		}
	}

	ResetPrimaryActionState();
}

void AUmbraPlayerController::UpdateHeldMovement(float DeltaSeconds)
{
	PrimaryActionHeldTime += DeltaSeconds;
	if (PrimaryActionHeldTime < PrimaryActionHoldThreshold)
	{
		return;
	}

	bPrimaryActionIsHold = true;
	FVector CursorLocation;
	if (GetNavigableCursorLocation(CursorLocation))
	{
		if (IsPrimaryAttackActive())
		{
			QueueMoveCommand(CursorLocation);
		}
		else
		{
			MovePawnToward(CursorLocation);
		}
	}
}

void AUmbraPlayerController::StartAutoMoveToCursor()
{
	FVector Destination;
	if (!GetNavigableCursorLocation(Destination))
	{
		return;
	}

	StartAutoMoveToLocation(Destination);
}

bool AUmbraPlayerController::StartAutoMoveToLocation(const FVector& Destination)
{
	APawn* ControlledPawn = GetPawn();
	UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!ControlledPawn || !NavigationSystem)
	{
		UE_LOG(LogUmbra, Warning, TEXT("Cannot start auto move for %s: no pawn or navigation system."), *GetNameSafe(this));
		return false;
	}

	FNavLocation ProjectedDestination;
	if (!NavigationSystem->ProjectPointToNavigation(Destination, ProjectedDestination))
	{
		UE_LOG(LogUmbra, Warning, TEXT("Cannot project auto-move destination %s onto the navigation mesh."),
			*Destination.ToCompactString());
		return false;
	}

	UNavigationPath* NavigationPath = NavigationSystem->FindPathToLocationSynchronously(
		GetWorld(),
		ControlledPawn->GetActorLocation(),
		ProjectedDestination.Location,
		ControlledPawn);
	if (!NavigationPath || !NavigationPath->IsValid() || NavigationPath->IsPartial() || NavigationPath->PathPoints.Num() < 2)
	{
		UE_LOG(LogUmbra, Warning, TEXT("Cannot find a complete navigation path from %s to %s."),
			*ControlledPawn->GetActorLocation().ToCompactString(),
			*Destination.ToCompactString());
		return false;
	}

	AutoMovePathPoints = NavigationPath->PathPoints;
	AutoMoveTargetLocation = ProjectedDestination.Location;
	CurrentPathPointIndex = 1;
	bAutoMoving = true;
	return true;
}

void AUmbraPlayerController::BeginAttackTarget(AActor* TargetActor)
{
	if (!IsValid(TargetActor)
		|| !TargetActor->Implements<UUmbraAttackable>()
		|| !IUmbraAttackable::Execute_CanBeAttacked(TargetActor))
	{
		return;
	}
	AUmbraPlayerCharacter* UmbraCharacter = Cast<AUmbraPlayerCharacter>(GetPawn());
	if (!UmbraCharacter)
	{
		return;
	}
	if (IsPrimaryAttackActive())
	{
		if (bEnableAutoAttack && PendingAttackTarget.Get() == TargetActor)
		{
			ClearQueuedCommand();
			UmbraCharacter->TryActivatePrimaryAttack(TargetActor, true);
		}
		else if (!bEnableAutoAttack && UmbraCharacter->GetPrimaryAttackTarget() == TargetActor)
		{
			// Preserve the original click-to-buffer combo behavior when automatic attacks are disabled.
			UmbraCharacter->TryActivatePrimaryAttack(TargetActor, false);
		}
		else
		{
			QueueAttackCommand(TargetActor);
		}
		return;
	}

	CancelAutoMove();
	ResetPrimaryActionState();
	ClearQueuedCommand();
	PendingAttackTarget = TargetActor;
	if (APawn* ControlledPawn = GetPawn())
	{
		if (UPawnMovementComponent* MovementComponent = ControlledPawn->GetMovementComponent())
		{
			MovementComponent->StopMovementImmediately();
		}
	}
	UpdatePendingAttack();
}

void AUmbraPlayerController::UpdatePendingAttack()
{
	AActor* TargetActor = PendingAttackTarget.Get();
	AUmbraPlayerCharacter* UmbraCharacter = Cast<AUmbraPlayerCharacter>(GetPawn());
	if (!IsValid(TargetActor)
		|| !UmbraCharacter
		|| !UmbraCharacter->IsMovementEnabled()
		|| !IUmbraAttackable::Execute_CanBeAttacked(TargetActor))
	{
		if (IsPrimaryAttackActive())
		{
			FGameplayTagContainer BasicAttackTags;
			BasicAttackTags.AddTag(UmbraGameplayTags::Ability_Attack_Basic);
			UmbraCharacter->GetAbilitySystemComponent()->CancelAbilities(&BasicAttackTags);
		}
		CancelPendingAttack();
		return;
	}
	if (IsPrimaryAttackActive())
	{
		return;
	}

	const FVector TargetLocation = TargetActor->GetActorLocation();
	if (UmbraCharacter->IsTargetInPrimaryAttackRange(TargetActor))
	{
		CancelAutoMove();
		if (UPawnMovementComponent* MovementComponent = UmbraCharacter->GetMovementComponent())
		{
			MovementComponent->StopMovementImmediately();
		}
		if (UmbraCharacter->TryActivatePrimaryAttack(TargetActor, bEnableAutoAttack))
		{
			if (!bEnableAutoAttack)
			{
				PendingAttackTarget.Reset();
			}
		}
		return;
	}
	if (!bChaseAttackTarget)
	{
		CancelPendingAttack();
		return;
	}

	if (bAutoMoving)
	{
		if (FVector::DistSquared2D(AutoMoveTargetLocation, TargetLocation) > FMath::Square(AutoMoveAcceptanceRadius))
		{
			CancelAutoMove();
		}
		else
		{
			UpdateAutoMove();
		}
	}
	if (!bAutoMoving)
	{
		if (!StartAutoMoveToLocation(TargetLocation))
		{
			CancelPendingAttack();
		}
	}
}

void AUmbraPlayerController::UpdateAttackHover()
{
	AActor* NewHoveredActor = nullptr;
	bAttackHighlightDebugHasPawnHit = false;
	AttackHighlightDebugHit = FHitResult();
	GetAttackableUnderCursor(NewHoveredActor, &AttackHighlightDebugHit);
	bAttackHighlightDebugHasPawnHit = AttackHighlightDebugHit.bBlockingHit;
	AActor* CurrentHoveredActor = HoveredAttackTarget.Get();
	if (CurrentHoveredActor == NewHoveredActor)
	{
		return;
	}

	UE_LOG(LogUmbra, Log, TEXT("Attack highlight debug: hover changed from %s to %s."),
		*GetNameSafe(CurrentHoveredActor),
		*GetNameSafe(NewHoveredActor));
	if (bShowAttackHighlightDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			UmbraAttackHighlightDebug::HoverEventMessageKey,
			2.5f,
			FColor::Cyan,
			FString::Printf(TEXT("Hover changed: %s -> %s"), *GetNameSafe(CurrentHoveredActor), *GetNameSafe(NewHoveredActor)));
	}

	if (IsValid(CurrentHoveredActor))
	{
		IUmbraAttackable::Execute_SetAttackHighlighted(CurrentHoveredActor, false);
		if (bShowAttackHighlightDebug && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(UmbraAttackHighlightDebug::RequestEventMessageKey, 2.5f, FColor::White,
				TEXT("Highlight requested: False"));
		}
	}
	HoveredAttackTarget = NewHoveredActor;
	if (IsValid(NewHoveredActor))
	{
		IUmbraAttackable::Execute_SetAttackHighlighted(NewHoveredActor, true);
		if (bShowAttackHighlightDebug && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(UmbraAttackHighlightDebug::RequestEventMessageKey, 2.5f, FColor::Green,
				TEXT("Highlight requested: True"));
		}
	}
}

void AUmbraPlayerController::UpdateAttackHighlightDebug()
{
	if (!GEngine)
	{
		return;
	}

	if (!bShowAttackHighlightDebug)
	{
		ClearAttackHighlightDebugMessages();
		return;
	}

	AActor* HitActor = bAttackHighlightDebugHasPawnHit ? AttackHighlightDebugHit.GetActor() : nullptr;
	UPrimitiveComponent* HitComponent = bAttackHighlightDebugHasPawnHit ? AttackHighlightDebugHit.GetComponent() : nullptr;
	const bool bImplementsAttackable = IsValid(HitActor) && HitActor->Implements<UUmbraAttackable>();
	AActor* HoveredActor = HoveredAttackTarget.Get();
	USkeletalMeshComponent* EnemyMesh = IsValid(HoveredActor)
		? HoveredActor->FindComponentByClass<USkeletalMeshComponent>()
		: (IsValid(HitActor) ? HitActor->FindComponentByClass<USkeletalMeshComponent>() : nullptr);

	if (bForceHighlightDebug && EnemyMesh)
	{
		EnemyMesh->SetCustomDepthStencilValue(1);
		EnemyMesh->SetRenderCustomDepth(true);
	}

	const bool bMeshValid = IsValid(EnemyMesh);
	const bool bRenderCustomDepth = bMeshValid && EnemyMesh->bRenderCustomDepth;
	const int32 StencilValue = bMeshValid ? EnemyMesh->CustomDepthStencilValue : 0;
	FColor StatusColor = FColor::White;
	if (IsValid(HitActor) && !bImplementsAttackable)
	{
		StatusColor = FColor::Yellow;
	}
	else if (bImplementsAttackable && (!bMeshValid || (IsValid(HoveredActor) && !bRenderCustomDepth)))
	{
		StatusColor = FColor::Red;
	}
	else if (bImplementsAttackable && bMeshValid && bRenderCustomDepth)
	{
		StatusColor = FColor::Green;
	}

	const FString DebugText = FString::Printf(
		TEXT("[Highlight Debug]\nController: %s\nLocal Controller: %s\nMouse Cursor: %s\nCursor Query: Pawn Objects\nHit Actor: %s\nHit Component: %s\nImplements Attackable: %s\nHovered Target: %s\nMesh Valid: %s\nRender CustomDepth: %s\nStencil Value: %d"),
		*GetNameSafe(this),
		IsLocalPlayerController() ? TEXT("True") : TEXT("False"),
		bShowMouseCursor ? TEXT("True") : TEXT("False"),
		*GetNameSafe(HitActor),
		*GetNameSafe(HitComponent),
		bImplementsAttackable ? TEXT("True") : TEXT("False"),
		*GetNameSafe(HoveredActor),
		bMeshValid ? TEXT("True") : TEXT("False"),
		bRenderCustomDepth ? TEXT("True") : TEXT("False"),
		StencilValue);
	GEngine->AddOnScreenDebugMessage(UmbraAttackHighlightDebug::StatusMessageKey, 0.0f, StatusColor, DebugText);

	if (bForceHighlightDebug)
	{
		GEngine->AddOnScreenDebugMessage(UmbraAttackHighlightDebug::ForceTestMessageKey, 0.0f, FColor::Yellow,
			TEXT("FORCE HIGHLIGHT TEST: ACTIVE"));
	}
	else
	{
		GEngine->RemoveOnScreenDebugMessage(UmbraAttackHighlightDebug::ForceTestMessageKey);
	}
}

void AUmbraPlayerController::ClearAttackHighlightDebugMessages() const
{
	if (!GEngine)
	{
		return;
	}

	GEngine->RemoveOnScreenDebugMessage(UmbraAttackHighlightDebug::StatusMessageKey);
	GEngine->RemoveOnScreenDebugMessage(UmbraAttackHighlightDebug::HoverEventMessageKey);
	GEngine->RemoveOnScreenDebugMessage(UmbraAttackHighlightDebug::RequestEventMessageKey);
	GEngine->RemoveOnScreenDebugMessage(UmbraAttackHighlightDebug::ForceTestMessageKey);
	GEngine->RemoveOnScreenDebugMessage(UmbraAttackHighlightDebug::FunctionEventMessageKey);
}

void AUmbraPlayerController::CancelPendingAttack()
{
	PendingAttackTarget.Reset();
	if (AUmbraPlayerCharacter* UmbraCharacter = Cast<AUmbraPlayerCharacter>(GetPawn()))
	{
		UmbraCharacter->StopPrimaryAttackContinuation();
	}
}

bool AUmbraPlayerController::IsPrimaryAttackActive() const
{
	const AUmbraPlayerCharacter* UmbraCharacter = Cast<AUmbraPlayerCharacter>(GetPawn());
	const UAbilitySystemComponent* ASC = UmbraCharacter ? UmbraCharacter->GetAbilitySystemComponent() : nullptr;
	return ASC && ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Attacking);
}

void AUmbraPlayerController::QueueMoveCommand(const FVector& Destination)
{
	if (Destination.ContainsNaN())
	{
		return;
	}
	QueuedCommand = EUmbraQueuedPlayerCommand::Move;
	QueuedMoveDestination = Destination;
	QueuedAttackTarget.Reset();
	PendingAttackTarget.Reset();
	if (AUmbraPlayerCharacter* UmbraCharacter = Cast<AUmbraPlayerCharacter>(GetPawn()))
	{
		UmbraCharacter->StopPrimaryAttackContinuation();
		if (UUmbraAbilitySystemComponent* ASC = Cast<UUmbraAbilitySystemComponent>(UmbraCharacter->GetAbilitySystemComponent()))
		{
			ASC->RequestPrimaryAttackMovementCancellation();
		}
	}
}

void AUmbraPlayerController::QueueDirectMoveCommand(const FVector& WorldDirection)
{
	if (WorldDirection.ContainsNaN() || WorldDirection.IsNearlyZero())
	{
		return;
	}
	QueuedCommand = EUmbraQueuedPlayerCommand::DirectMove;
	QueuedMoveDestination = WorldDirection.GetSafeNormal();
	QueuedAttackTarget.Reset();
	PendingAttackTarget.Reset();
	if (AUmbraPlayerCharacter* UmbraCharacter = Cast<AUmbraPlayerCharacter>(GetPawn()))
	{
		UmbraCharacter->StopPrimaryAttackContinuation();
		if (UUmbraAbilitySystemComponent* ASC = Cast<UUmbraAbilitySystemComponent>(UmbraCharacter->GetAbilitySystemComponent()))
		{
			ASC->RequestPrimaryAttackMovementCancellation();
		}
	}
}

void AUmbraPlayerController::QueueAttackCommand(AActor* TargetActor)
{
	if (!IsAttackableTarget(TargetActor))
	{
		return;
	}
	QueuedCommand = EUmbraQueuedPlayerCommand::Attack;
	QueuedAttackTarget = TargetActor;
	QueuedMoveDestination = FVector::ZeroVector;
	if (AUmbraPlayerCharacter* UmbraCharacter = Cast<AUmbraPlayerCharacter>(GetPawn()))
	{
		UmbraCharacter->StopPrimaryAttackContinuation();
	}
}

void AUmbraPlayerController::QueueStopCommand()
{
	QueuedCommand = EUmbraQueuedPlayerCommand::Stop;
	QueuedAttackTarget.Reset();
	QueuedMoveDestination = FVector::ZeroVector;
	if (AUmbraPlayerCharacter* UmbraCharacter = Cast<AUmbraPlayerCharacter>(GetPawn()))
	{
		UmbraCharacter->StopPrimaryAttackContinuation();
	}
}

void AUmbraPlayerController::ClearQueuedCommand()
{
	QueuedCommand = EUmbraQueuedPlayerCommand::None;
	QueuedAttackTarget.Reset();
	QueuedMoveDestination = FVector::ZeroVector;
}

bool AUmbraPlayerController::HandlePrimaryAttackTransition(AActor* CurrentTarget)
{
	AUmbraPlayerCharacter* UmbraCharacter = Cast<AUmbraPlayerCharacter>(GetPawn());
	if (!UmbraCharacter || QueuedCommand != EUmbraQueuedPlayerCommand::None || !bEnableAutoAttack
		|| !UmbraCharacter->ShouldContinuePrimaryAttack() || !IsAttackableTarget(CurrentTarget))
	{
		return false;
	}
	PendingAttackTarget = CurrentTarget;
	return UmbraCharacter->IsTargetInPrimaryAttackRange(CurrentTarget);
}

void AUmbraPlayerController::ExecuteQueuedCommand()
{
	const EUmbraQueuedPlayerCommand Command = QueuedCommand;
	const FVector MoveDestination = QueuedMoveDestination;
	AActor* AttackTarget = QueuedAttackTarget.Get();
	ClearQueuedCommand();

	switch (Command)
	{
	case EUmbraQueuedPlayerCommand::Move:
		CancelPendingAttack();
		CancelAutoMove();
		StartAutoMoveToLocation(MoveDestination);
		break;
	case EUmbraQueuedPlayerCommand::DirectMove:
		CancelPendingAttack();
		CancelAutoMove();
		if (AUmbraPlayerCharacter* UmbraCharacter = Cast<AUmbraPlayerCharacter>(GetPawn()))
		{
			UmbraCharacter->AddMovementInput(MoveDestination);
		}
		break;
	case EUmbraQueuedPlayerCommand::Attack:
		CancelPendingAttack();
		BeginAttackTarget(AttackTarget);
		break;
	case EUmbraQueuedPlayerCommand::Stop:
		CancelPendingAttack();
		CancelAutoMove();
		break;
	default:
		break;
	}
}

void AUmbraPlayerController::NotifyPrimaryAttackAbilityEnded(bool bWasCancelled, bool bExecuteQueuedCommand)
{
	if (bExecuteQueuedCommand)
	{
		ExecuteQueuedCommand();
		return;
	}
	if (bWasCancelled)
	{
		CancelCombatCommandState();
		return;
	}
	ExecuteQueuedCommand();
	if (!IsAttackableTarget(PendingAttackTarget.Get()))
	{
		CancelPendingAttack();
	}
}

void AUmbraPlayerController::CancelCombatCommandState()
{
	ClearQueuedCommand();
	CancelPendingAttack();
	CancelAutoMove();
}

bool AUmbraPlayerController::GetAttackableUnderCursor(AActor*& OutTargetActor, FHitResult* OutCursorHit) const
{
	OutTargetActor = nullptr;
	if (IsPointerOverAttributeDebugPanel())
	{
		return false;
	}
	FHitResult CursorHit;
	float MouseX, MouseY;
	FVector Origin, Direction;
	const bool bHasPawnHit = IsLocalPlayerController() && GetMousePosition(MouseX, MouseY)
		&& (!GetHUD() || !GetHUD()->GetHitBoxAtCoordinates(FVector2D(MouseX, MouseY), true))
		&& DeprojectScreenPositionToWorld(MouseX, MouseY, Origin, Direction)
		&& TraceAttackablePawn(Origin, Origin + Direction * HitResultTraceDistance, CursorHit);
	if (OutCursorHit)
	{
		*OutCursorHit = CursorHit;
	}
	if (!bHasPawnHit)
	{
		return false;
	}

	AActor* HitActor = CursorHit.GetActor();
	if (!IsAttackableTarget(HitActor))
	{
		return false;
	}

	OutTargetActor = HitActor;
	return true;
}

bool AUmbraPlayerController::TraceAttackablePawn(const FVector& Start, const FVector& End, FHitResult& Hit) const
{
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	// At melee distance the player's capsule can cover the enemy on screen.
	// Ignore only our own pawn, preserving nearest-other-pawn selection semantics.
	FCollisionQueryParams Query(SCENE_QUERY_STAT(UmbraAttackSelection), false, GetPawn());
	return GetWorld() && GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, Objects, Query);
}

void AUmbraPlayerController::UpdateAutoMove()
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !AutoMovePathPoints.IsValidIndex(CurrentPathPointIndex))
	{
		CancelAutoMove();
		return;
	}

	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	if (FVector::DistSquared2D(PawnLocation, AutoMoveTargetLocation) <= FMath::Square(AutoMoveAcceptanceRadius))
	{
		CancelAutoMove();
		return;
	}

	while (AutoMovePathPoints.IsValidIndex(CurrentPathPointIndex))
	{
		const bool bFinalPoint = CurrentPathPointIndex == AutoMovePathPoints.Num() - 1;
		const float AcceptanceRadius = bFinalPoint ? AutoMoveAcceptanceRadius : PathPointAcceptanceRadius;
		if (FVector::DistSquared2D(PawnLocation, AutoMovePathPoints[CurrentPathPointIndex]) > FMath::Square(AcceptanceRadius))
		{
			break;
		}

		++CurrentPathPointIndex;
	}

	if (!AutoMovePathPoints.IsValidIndex(CurrentPathPointIndex))
	{
		CancelAutoMove();
		return;
	}

	MovePawnToward(AutoMovePathPoints[CurrentPathPointIndex]);
}

bool AUmbraPlayerController::GetNavigableCursorLocation(FVector& OutLocation) const
{
	FHitResult CursorHit;
	if (!GetCursorGroundHit(CursorHit))
	{
		return false;
	}

	const UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	FNavLocation ProjectedLocation;
	if (!NavigationSystem || !NavigationSystem->ProjectPointToNavigation(CursorHit.ImpactPoint, ProjectedLocation))
	{
		return false;
	}

	OutLocation = ProjectedLocation.Location;
	return true;
}

void AUmbraPlayerController::MovePawnToward(const FVector& WorldLocation)
{
	if (AUmbraPlayerCharacter* UmbraCharacter = Cast<AUmbraPlayerCharacter>(GetPawn()))
	{
		UmbraCharacter->MoveTowardWorldLocation(WorldLocation);
	}
}

void AUmbraPlayerController::CancelAutoMove()
{
	bAutoMoving = false;
	AutoMoveTargetLocation = FVector::ZeroVector;
	CurrentPathPointIndex = INDEX_NONE;
	AutoMovePathPoints.Reset();
}

bool AUmbraPlayerController::TryBeginManualMovement(const FVector& WorldDirection)
{
	if (bPrimaryActionIsHold)
	{
		return false;
	}
	if (IsPrimaryAttackActive())
	{
		QueueDirectMoveCommand(WorldDirection);
		return false;
	}

	CancelAutoMove();
	CancelPendingAttack();
	ResetPrimaryActionState();
	return true;
}

void AUmbraPlayerController::ResetPrimaryActionState()
{
	bPrimaryActionHeld = false;
	bPrimaryActionIsHold = false;
	PrimaryActionHeldTime = 0.0f;
	ActivePrimaryActionContext = EUmbraPrimaryActionContext::Ground;
}
