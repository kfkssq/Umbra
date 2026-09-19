// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/UmbraPlayerCharacter.h"

#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "Interfaces/UmbraAttackable.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
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

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
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

	// CharacterMovement is the sole owner of locomotion facing. Blueprint component
	// defaults may tune RotationRate, but may not restore controller/custom-Tick rotation.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	PrimaryAttackFacingInstanceId = 0;

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

float AUmbraPlayerCharacter::GetLocomotionAnimationPlayRate() const
{
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	const float ReferenceSpeed = FMath::IsFinite(LocomotionAnimationReferenceSpeed)
		? FMath::Max(1.f, LocomotionAnimationReferenceSpeed) : 500.f;
	const float MinRate = FMath::Clamp(FMath::IsFinite(MinLocomotionAnimationPlayRate)
		? MinLocomotionAnimationPlayRate : 0.25f, 0.05f, 10.f);
	const float MaxRate = FMath::Max(MinRate, FMath::Clamp(FMath::IsFinite(MaxLocomotionAnimationPlayRate)
		? MaxLocomotionAnimationPlayRate : 3.f, 0.05f, 10.f));
	const float TargetSpeed = Movement && FMath::IsFinite(Movement->MaxWalkSpeed)
		? FMath::Max(0.f, Movement->MaxWalkSpeed) : 0.f;
	return FMath::Clamp(TargetSpeed / ReferenceSpeed, MinRate, MaxRate);
}

float AUmbraPlayerCharacter::CalculateMovementYawRate(float MoveSpeed) const
{
	const float ReferenceSpeed = FMath::IsFinite(LocomotionAnimationReferenceSpeed)
		? FMath::Max(1.f, LocomotionAnimationReferenceSpeed) : 500.f;
	const float BaseYawRate = FMath::IsFinite(BaseMovementYawRate)
		? FMath::Max(0.f, BaseMovementYawRate) : 600.f;
	const float MaxYawRate = FMath::Max(BaseYawRate, FMath::IsFinite(MaxMovementYawRate)
		? FMath::Max(0.f, MaxMovementYawRate) : 1800.f);
	const float SafeMoveSpeed = FMath::IsFinite(MoveSpeed) ? FMath::Max(0.f, MoveSpeed) : 0.f;
	return FMath::Clamp(BaseYawRate * SafeMoveSpeed / ReferenceSpeed, 0.f, MaxYawRate);
}

void AUmbraPlayerCharacter::ApplyMoveSpeedDrivenYawRate(float MoveSpeed)
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->RotationRate.Yaw = CalculateMovementYawRate(MoveSpeed);
	}
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

bool AUmbraPlayerCharacter::TryActivatePrimaryAttack(AActor* TargetActor, bool bContinueAttacking)
{
	UUmbraAbilitySystemComponent* AbilitySystemComponent = Cast<UUmbraAbilitySystemComponent>(GetAbilitySystemComponent());
	if (!IsValid(TargetActor) || !AbilitySystemComponent)
	{
		return false;
	}

	PrimaryAttackTarget = TargetActor;
	bContinuePrimaryAttack = bContinueAttacking;
	if (!HasAuthority())
	{
		AbilitySystemComponent->ServerReceivePrimaryAttackIntent(TargetActor,
			AbilitySystemComponent->HasMatchingGameplayTag(UmbraGameplayTags::State_Attacking),
			bContinueAttacking);
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
	bContinuePrimaryAttack = false;
	return false;
}

bool AUmbraPlayerCharacter::IsTargetInPrimaryAttackRange(const AActor* TargetActor, float ExtraTolerance) const
{
	if (!IsValid(TargetActor)) return false;
	const ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
	const float TargetRadius = TargetCharacter && TargetCharacter->GetCapsuleComponent()
		? TargetCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius() : 0.f;
	return FVector::DistSquared2D(GetActorLocation(), TargetActor->GetActorLocation())
		<= FMath::Square(FMath::Max(0.f, PrimaryAttackRange + TargetRadius + FMath::Max(0.f, ExtraTolerance)));
}

void AUmbraPlayerCharacter::ReceivePrimaryAttackIntent(
	AActor* TargetActor, bool bCombo, bool bContinueAttacking)
{
	if (!HasAuthority())
	{
		return;
	}
	if (!IsValid(TargetActor) || TargetActor == this || TargetActor->GetWorld() != GetWorld()
		|| !TargetActor->Implements<UUmbraAttackable>() || !IUmbraAttackable::Execute_CanBeAttacked(TargetActor)
		|| !IsTargetInPrimaryAttackRange(TargetActor, 100.f))
	{
		PrimaryAttackTarget.Reset();
		bContinuePrimaryAttack = false;
		return;
	}
	PrimaryAttackTarget = TargetActor;
	bContinuePrimaryAttack = bContinueAttacking;
	// The authoritative windup timer still decides whether this intent resolves a hit.
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
		TryActivatePrimaryAttack(TargetActor, bContinueAttacking);
	}
}

void AUmbraPlayerCharacter::StopPrimaryAttackContinuation()
{
	const bool bWasContinuing = bContinuePrimaryAttack;
	bContinuePrimaryAttack = false;
	if (bWasContinuing && !HasAuthority())
	{
		if (UUmbraAbilitySystemComponent* ASC = Cast<UUmbraAbilitySystemComponent>(GetAbilitySystemComponent()))
		{
			ASC->ServerCancelPrimaryAttackContinuation();
		}
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

	const FRotator MovementRotation(0.0f, CameraBoom->GetComponentRotation().Yaw, 0.0f);
	const FVector ForwardDirection = MovementRotation.RotateVector(FVector::ForwardVector);
	const FVector RightDirection = MovementRotation.RotateVector(FVector::RightVector);
	const FVector MovementDirection =
		(ForwardDirection * MovementInput.Y + RightDirection * MovementInput.X).GetSafeNormal();
	if (AUmbraPlayerController* PlayerController = Cast<AUmbraPlayerController>(GetController()))
	{
		if (!PlayerController->TryBeginManualMovement(MovementDirection))
		{
			return;
		}
	}

	AddMovementInput(ForwardDirection, MovementInput.Y);
	AddMovementInput(RightDirection, MovementInput.X);
}

void AUmbraPlayerCharacter::BeginPrimaryAttackFacing(uint32 AttackInstanceId, const FVector& WorldLocation)
{
	if (AttackInstanceId == 0)
	{
		return;
	}

	PrimaryAttackFacingInstanceId = AttackInstanceId;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->StopMovementImmediately();
	ConsumeMovementInputVector();
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	FVector FacingDirection = WorldLocation - GetActorLocation();
	FacingDirection.Z = 0.0f;
	if (!FacingDirection.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.0f, FacingDirection.Rotation().Yaw, 0.0f));
	}
}

void AUmbraPlayerCharacter::EndPrimaryAttackFacing(uint32 AttackInstanceId)
{
	if (AttackInstanceId == 0 || AttackInstanceId != PrimaryAttackFacingInstanceId)
	{
		return;
	}

	PrimaryAttackFacingInstanceId = 0;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;

	const UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponent();
	if (!AbilitySystemComponent
		|| !AbilitySystemComponent->HasMatchingGameplayTag(UmbraGameplayTags::State_Dead))
	{
		// Do not write ActorRotation here. The first real movement input/path segment
		// becomes CharacterMovement's desired direction and RotationRate limits the turn.
		GetCharacterMovement()->bOrientRotationToMovement = true;
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
