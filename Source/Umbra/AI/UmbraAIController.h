// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "CoreMinimal.h"
#include "UmbraAIController.generated.h"

class AUmbraEnemyCharacter;

/** Base controller for C++-driven Umbra enemy AI. */
UCLASS()
class UMBRA_API AUmbraAIController : public AAIController
{
	GENERATED_BODY()

public:
	AUmbraAIController();
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(EditDefaultsOnly, Category = "Combat AI", meta = (ClampMin = "0.0"))
	float DetectionRange = 900.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat AI", meta = (ClampMin = "0.0"))
	float AttackRange = 260.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat AI", meta = (ClampMin = "0.0"))
	float LoseTargetRange = 1300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat AI", meta = (ClampMin = "0.0"))
	float AttackCooldown = 1.5f;

	/** Maximum stationary turn speed before an attack, in degrees per second. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat AI", meta = (ClampMin = "0.0"))
	float TurnRateDegreesPerSecond = 240.0f;

	/** Enemy must face within this yaw angle before an attack can begin. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat AI", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float AttackFacingTolerance = 8.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat AI")
	bool bReturnToSpawn = true;

	/** Returns the controlled Umbra enemy, or nullptr when no compatible pawn is possessed. */
	AUmbraEnemyCharacter* GetUmbraEnemy() const;

private:
	APawn* FindPlayerPawn() const;
	void ClearCombatTarget(bool bMoveHome);
	bool TurnTowardTarget(AUmbraEnemyCharacter* Enemy, const AActor* Target, float DeltaSeconds) const;

	TWeakObjectPtr<APawn> CombatTarget;
	FVector SpawnLocation = FVector::ZeroVector;
	double NextAttackTime = 0.0;
};
