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

protected:
	virtual void OnPossess(APawn* InPawn) override;

	/** Returns the controlled Umbra enemy, or nullptr when no compatible pawn is possessed. */
	AUmbraEnemyCharacter* GetUmbraEnemy() const;
};
