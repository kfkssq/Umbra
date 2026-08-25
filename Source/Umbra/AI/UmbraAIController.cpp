// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/UmbraAIController.h"

#include "Characters/UmbraEnemyCharacter.h"
#include "Umbra.h"

void AUmbraAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!Cast<AUmbraEnemyCharacter>(InPawn))
	{
		UE_LOG(LogUmbra, Warning, TEXT("%s can only control AUmbraEnemyCharacter pawns."), *GetNameSafe(this));
	}
}

AUmbraEnemyCharacter* AUmbraAIController::GetUmbraEnemy() const
{
	return Cast<AUmbraEnemyCharacter>(GetPawn());
}
