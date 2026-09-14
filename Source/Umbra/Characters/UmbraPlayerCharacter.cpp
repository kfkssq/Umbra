// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/UmbraPlayerCharacter.h"

#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "Interfaces/UmbraAttackable.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Player/UmbraPlayerState.h"
#include "Umbra.h"
#include "UmbraPlayerController.h"

AUmbraPlayerCharacter::AUmbraPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 500.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;
	GetMesh()->SetRenderCustomDepth(false);
	GetMesh()->SetCustomDepthStencilValue(0);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 900.0f;
	CameraBoom->SetRelativeRotation(FRotator(-55.0f, 0.0f, 0.0f));
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 8.0f;
	CameraBoom->CameraLagMaxDistance = 150.0f;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;
}

void AUmbraPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Blueprint defaults must not restore movement- or controller-driven rotation.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// The player and all owned presentation meshes are never part of the enemy-hover stencil mask.
	TInlineComponentArray<UMeshComponent*> OwnedMeshComponents(this);
	for (UMeshComponent* MeshComponent : OwnedMeshComponents)
	{
		MeshComponent->SetRenderCustomDepth(false);
		MeshComponent->SetCustomDepthStencilValue(0);
	}

	bMovementEnabled = true;
	if (GetCharacterMovement()->MovementMode == MOVE_None)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void AUmbraPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled())
	{
		if (UUmbraAbilitySystemComponent* AbilitySystemComponent = Cast<UUmbraAbilitySystemComponent>(GetAbilitySystemComponent()))
		{
			AbilitySystemComponent->ProcessAbilityInput(DeltaSeconds);
		}
	}

	UpdateFacingRotation(DeltaSeconds);
}

void AUmbraPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilitySystem();
}

void AUmbraPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitializeAbilitySystem();
}

UAbilitySystemComponent* AUmbraPlayerCharacter::GetAbilitySystemComponent() const
{
	const AUmbraPlayerState* UmbraPlayerState = GetPlayerState<AUmbraPlayerState>();
	return UmbraPlayerState ? UmbraPlayerState->GetAbilitySystemComponent() : nullptr;
}

void AUmbraPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogUmbra, Error, TEXT("%s requires an Enhanced Input component."), *GetNameSafe(this));
		return;
	}

	if (!MoveAction)
	{
		UE_LOG(LogUmbra, Warning, TEXT("No MoveAction is configured for %s."), *GetNameSafe(this));
	}
	else
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AUmbraPlayerCharacter::Move);
	}

	for (const FUmbraTaggedInputAction& TaggedInputAction : AbilityInputActions)
	{
		if (!TaggedInputAction.InputAction || !TaggedInputAction.InputTag.IsValid())
		{
			continue;
		}
		if (TaggedInputAction.InputTag.MatchesTagExact(UmbraGameplayTags::Input_Attack_Primary))
		{
			continue;
		}

		EnhancedInputComponent->BindAction(
			TaggedInputAction.InputAction,
			ETriggerEvent::Started,
			this,
			&AUmbraPlayerCharacter::AbilityInputTagPressed,
			TaggedInputAction.InputTag);
		EnhancedInputComponent->BindAction(
			TaggedInputAction.InputAction,
			ETriggerEvent::Completed,
			this,
			&AUmbraPlayerCharacter::AbilityInputTagReleased,
			TaggedInputAction.InputTag);
		EnhancedInputComponent->BindAction(
			TaggedInputAction.InputAction,
			ETriggerEvent::Canceled,
			this,
			&AUmbraPlayerCharacter::AbilityInputTagReleased,
			TaggedInputAction.InputTag);
	}
}

bool AUmbraPlayerCharacter::TryActivatePrimaryAttack(AActor* TargetActor)
{
	UUmbraAbilitySystemComponent* AbilitySystemComponent = Cast<UUmbraAbilitySystemComponent>(GetAbilitySystemComponent());
	if (!IsValid(TargetActor) || !AbilitySystemComponent)
	{
		return false;
	}

	PrimaryAttackTarget = TargetActor;
	if (!HasAuthority())
	{
		AbilitySystemComponent->ServerReceivePrimaryAttackIntent(TargetActor,
			AbilitySystemComponent->HasMatchingGameplayTag(UmbraGameplayTags::State_Attacking));
	}
	if (AbilitySystemComponent->HasMatchingGameplayTag(UmbraGameplayTags::State_Attacking))
	{
		FGameplayEventData ComboInputEvent;
		ComboInputEvent.EventTag = UmbraGameplayTags::Event_Attack_ComboInput;
		ComboInputEvent.Instigator = this;
		ComboInputEvent.Target = TargetActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			this,
			UmbraGameplayTags::Event_Attack_ComboInput,
			ComboInputEvent);
		return true;
	}

	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(UmbraGameplayTags::Ability_Attack_Basic);
	if (AbilitySystemComponent->TryActivateAbilitiesByTag(AbilityTags))
	{
		return true;
	}

	PrimaryAttackTarget.Reset();
	return false;
}

void AUmbraPlayerCharacter::ReceivePrimaryAttackIntent(AActor* TargetActor, bool bCombo)
{
	if (!HasAuthority())
	{
		return;
	}
	if (!IsValid(TargetActor) || TargetActor == this || TargetActor->GetWorld() != GetWorld()
		|| !TargetActor->Implements<UUmbraAttackable>() || !IUmbraAttackable::Execute_CanBeAttacked(TargetActor)
		|| FVector::DistSquared2D(GetActorLocation(), TargetActor->GetActorLocation()) > FMath::Square(PrimaryAttackRange + 100.f))
	{
		PrimaryAttackTarget.Reset();
		return;
	}
	PrimaryAttackTarget = TargetActor;
	// The authoritative montage/sweep still decides whether a hit actually occurs.
	if (bCombo && GetAbilitySystemComponent()
		&& GetAbilitySystemComponent()->HasMatchingGameplayTag(UmbraGameplayTags::State_Attacking))
	{
		FGameplayEventData Event;
		Event.EventTag = UmbraGameplayTags::Event_Attack_ComboInput;
		Event.Instigator = this;
		Event.Target = TargetActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, Event.EventTag, Event);
	}
	else if (!bCombo && GetAbilitySystemComponent()
		&& !GetAbilitySystemComponent()->HasMatchingGameplayTag(UmbraGameplayTags::State_Attacking))
	{
		// The client-predicted activation does not replicate this transient target pointer.
		// Start the authoritative ability once the owned ASC RPC arrives.
		TryActivatePrimaryAttack(TargetActor);
	}
}

void AUmbraPlayerCharacter::Move(const FInputActionValue& Value)
{
	if (!bMovementEnabled)
	{
		return;
	}

	const FVector2D MovementInput = Value.Get<FVector2D>();
	if (MovementInput.IsNearlyZero())
	{
		return;
	}

	if (AUmbraPlayerController* PlayerController = Cast<AUmbraPlayerController>(GetController()))
	{
		if (!PlayerController->TryBeginManualMovement())
		{
			return;
		}
	}

	const FRotator MovementRotation(0.0f, CameraBoom->GetComponentRotation().Yaw, 0.0f);
	const FVector ForwardDirection = MovementRotation.RotateVector(FVector::ForwardVector);
	const FVector RightDirection = MovementRotation.RotateVector(FVector::RightVector);
	const FVector MovementDirection =
		(ForwardDirection * MovementInput.Y + RightDirection * MovementInput.X).GetSafeNormal();

	if (!MovementDirection.IsNearlyZero())
	{
		DesiredFacingYaw = MovementDirection.Rotation().Yaw;
		bHasDesiredFacing = true;
	}

	AddMovementInput(ForwardDirection, MovementInput.Y);
	AddMovementInput(RightDirection, MovementInput.X);
}

void AUmbraPlayerCharacter::SetFacingTargetLocation(const FVector& WorldLocation)
{
	FVector FacingDirection = WorldLocation - GetActorLocation();
	FacingDirection.Z = 0.0f;
	if (!FacingDirection.IsNearlyZero())
	{
		DesiredFacingYaw = FacingDirection.Rotation().Yaw;
		bHasDesiredFacing = true;
	}
}

void AUmbraPlayerCharacter::MoveTowardWorldLocation(const FVector& WorldLocation)
{
	if (!bMovementEnabled)
	{
		return;
	}

	FVector MovementDirection = WorldLocation - GetActorLocation();
	MovementDirection.Z = 0.0f;
	MovementDirection = MovementDirection.GetSafeNormal();
	if (MovementDirection.IsNearlyZero())
	{
		return;
	}

	DesiredFacingYaw = MovementDirection.Rotation().Yaw;
	bHasDesiredFacing = true;
	AddMovementInput(MovementDirection);
}

void AUmbraPlayerCharacter::FinishSpawnAnimation()
{
	bMovementEnabled = true;
	if (GetCharacterMovement()->MovementMode == MOVE_None)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void AUmbraPlayerCharacter::AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (UUmbraAbilitySystemComponent* AbilitySystemComponent = Cast<UUmbraAbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		AbilitySystemComponent->AbilityInputTagPressed(InputTag);
	}
}

void AUmbraPlayerCharacter::AbilityInputTagReleased(FGameplayTag InputTag)
{
	if (UUmbraAbilitySystemComponent* AbilitySystemComponent = Cast<UUmbraAbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		AbilitySystemComponent->AbilityInputTagReleased(InputTag);
	}
}

void AUmbraPlayerCharacter::UpdateFacingRotation(float DeltaSeconds)
{
	if (!bHasDesiredFacing)
	{
		return;
	}

	const FRotator CurrentRotation(0.0f, GetActorRotation().Yaw, 0.0f);
	const FRotator TargetRotation(0.0f, DesiredFacingYaw, 0.0f);
	const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, MouseFacingInterpSpeed);
	SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
}

void AUmbraPlayerCharacter::InitializeAbilitySystem()
{
	AUmbraPlayerState* UmbraPlayerState = GetPlayerState<AUmbraPlayerState>();
	if (!UmbraPlayerState)
	{
		return;
	}

	UUmbraAbilitySystemComponent* AbilitySystemComponent = UmbraPlayerState->GetUmbraAbilitySystemComponent();
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->ClearAbilityInput();
	AbilitySystemComponent->InitAbilityActorInfo(UmbraPlayerState, this);
	UmbraPlayerState->InitializeAttributes();
	UmbraPlayerState->GrantInitialAbilities();
}
