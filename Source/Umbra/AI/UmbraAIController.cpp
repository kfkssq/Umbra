// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/UmbraAIController.h"

#include "AbilitySystemComponent.h"
#include "Characters/UmbraEnemyCharacter.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Umbra.h"

AUmbraAIController::AUmbraAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AUmbraAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!Cast<AUmbraEnemyCharacter>(InPawn))
	{
		UE_LOG(LogUmbra, Warning, TEXT("%s can only control AUmbraEnemyCharacter pawns."), *GetNameSafe(this));
	}
	else
	{
		SpawnLocation = InPawn->GetActorLocation();
	}
}

void AUmbraAIController::OnUnPossess()
{
	CombatTarget.Reset();
	Super::OnUnPossess();
}

void AUmbraAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AUmbraEnemyCharacter* Enemy = GetUmbraEnemy();
	if (!Enemy || Enemy->IsDead())
	{
		StopMovement();
		return;
	}
	if (Enemy->GetAbilitySystemComponent()->HasMatchingGameplayTag(UmbraGameplayTags::State_Attacking))
	{
		StopMovement();
		return;
	}

	APawn* Target = CombatTarget.Get();
	if (!IsValid(Target) || Target->IsActorBeingDestroyed())
	{
		Target = FindPlayerPawn();
		if (Target && FVector::DistSquared2D(Enemy->GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(DetectionRange))
		{
			CombatTarget = Target;
			Enemy->SetCombatTarget(Target);
		}
		else
		{
			return;
		}
	}

	const float DistanceSquared = FVector::DistSquared2D(Enemy->GetActorLocation(), Target->GetActorLocation());
	if (DistanceSquared > FMath::Square(LoseTargetRange))
	{
		ClearCombatTarget(true);
		return;
	}
	if (DistanceSquared > FMath::Square(AttackRange))
	{
		// Do not add both pawns' collision radii to the acceptance distance. Doing so can
		// stop the path outside AttackRange and leave the enemy waiting for the player.
		MoveToActor(Target, AttackRange * 0.85f, false, true, true, nullptr, true);
		return;
	}

	StopMovement();
	const bool bIsFacingTarget = TurnTowardTarget(Enemy, Target, DeltaSeconds);

	const double Now = GetWorld()->GetTimeSeconds();
	if (bIsFacingTarget && Now >= NextAttackTime && Enemy->TryActivateBasicAttack())
	{
		NextAttackTime = Now + AttackCooldown;
	}
}

bool AUmbraAIController::TurnTowardTarget(
	AUmbraEnemyCharacter* Enemy,
	const AActor* Target,
	float DeltaSeconds) const
{
	FVector Facing = Target->GetActorLocation() - Enemy->GetActorLocation();
	Facing.Z = 0.0f;
	if (Facing.IsNearlyZero())
	{
		return true;
	}

	const float DesiredYaw = Facing.Rotation().Yaw;
	const float CurrentYaw = Enemy->GetActorRotation().Yaw;
	const float YawError = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentYaw, DesiredYaw));
	const float NewYaw = FMath::FixedTurn(CurrentYaw, DesiredYaw, TurnRateDegreesPerSecond * DeltaSeconds);
	Enemy->SetActorRotation(FRotator(0.0f, NewYaw, 0.0f));
	return YawError <= AttackFacingTolerance;
}

APawn* AUmbraAIController::FindPlayerPawn() const
{
	return GetWorld() ? UGameplayStatics::GetPlayerPawn(GetWorld(), 0) : nullptr;
}

void AUmbraAIController::ClearCombatTarget(bool bMoveHome)
{
	CombatTarget.Reset();
	if (AUmbraEnemyCharacter* Enemy = GetUmbraEnemy())
	{
		Enemy->SetCombatTarget(nullptr);
	}
	StopMovement();
	if (bMoveHome && bReturnToSpawn)
	{
		MoveToLocation(SpawnLocation, 25.0f, true, true, true, false, nullptr, true);
	}
}

AUmbraEnemyCharacter* AUmbraAIController::GetUmbraEnemy() const
{
	return Cast<AUmbraEnemyCharacter>(GetPawn());
}
