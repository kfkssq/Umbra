// Copyright Epic Games, Inc. All Rights Reserved.

#include "UmbraPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Characters/UmbraPlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Umbra.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AUmbraPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController())
	{
		bShowMouseCursor = true;
		DefaultMouseCursor = EMouseCursor::Default;

		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
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
}

void AUmbraPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsLocalPlayerController())
	{
		return;
	}

	if (bPrimaryActionHeld && ActivePrimaryActionContext == EUmbraPrimaryActionContext::Ground)
	{
		UpdateHeldMovement(DeltaSeconds);
	}
	else if (bAutoMoving)
	{
		UpdateAutoMove();
	}
}

bool AUmbraPlayerController::GetCursorGroundHit(FHitResult& OutHitResult) const
{
	return IsLocalPlayerController()
		&& GetHitResultUnderCursor(ECC_Visibility, false, OutHitResult)
		&& OutHitResult.bBlockingHit;
}

void AUmbraPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

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
	return EUmbraPrimaryActionContext::Ground;
}

void AUmbraPlayerController::HandlePrimaryAbilityPressed(const FHitResult& CursorHit)
{
}

void AUmbraPlayerController::HandlePrimaryAbilityReleased()
{
}

void AUmbraPlayerController::PrimaryActionStarted()
{
	CancelAutoMove();
	bPrimaryActionHeld = false;
	bPrimaryActionIsHold = false;
	PrimaryActionHeldTime = 0.0f;

	if (!GetCursorGroundHit(PrimaryActionInitialHit))
	{
		return;
	}

	ActivePrimaryActionContext = DeterminePrimaryActionContext(PrimaryActionInitialHit);
	if (ActivePrimaryActionContext == EUmbraPrimaryActionContext::Ability)
	{
		HandlePrimaryAbilityPressed(PrimaryActionInitialHit);
		return;
	}

	bPrimaryActionHeld = true;
	if (AUmbraPlayerCharacter* UmbraCharacter = Cast<AUmbraPlayerCharacter>(GetPawn()))
	{
		UmbraCharacter->SetFacingTargetLocation(PrimaryActionInitialHit.ImpactPoint);
	}
}

void AUmbraPlayerController::PrimaryActionCompleted()
{
	if (ActivePrimaryActionContext == EUmbraPrimaryActionContext::Ability)
	{
		HandlePrimaryAbilityReleased();
	}
	else if (bPrimaryActionHeld && !bPrimaryActionIsHold)
	{
		StartAutoMoveToCursor();
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
		MovePawnToward(CursorLocation);
	}
}

void AUmbraPlayerController::StartAutoMoveToCursor()
{
	FVector Destination;
	if (!GetNavigableCursorLocation(Destination))
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!ControlledPawn || !NavigationSystem)
	{
		return;
	}

	UNavigationPath* NavigationPath = NavigationSystem->FindPathToLocationSynchronously(
		GetWorld(),
		ControlledPawn->GetActorLocation(),
		Destination,
		ControlledPawn);
	if (!NavigationPath || !NavigationPath->IsValid() || NavigationPath->IsPartial() || NavigationPath->PathPoints.Num() < 2)
	{
		return;
	}

	AutoMovePathPoints = NavigationPath->PathPoints;
	AutoMoveTargetLocation = Destination;
	CurrentPathPointIndex = 1;
	bAutoMoving = true;
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

bool AUmbraPlayerController::TryBeginManualMovement()
{
	if (bPrimaryActionIsHold)
	{
		return false;
	}

	CancelAutoMove();
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
