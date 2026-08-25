// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/UmbraEnemyCharacter.h"

#include "AI/UmbraAIController.h"
#include "AbilitySystem/Abilities/UmbraHitReactAbility.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Umbra.h"
#include "UmbraPlayerController.h"

AUmbraEnemyCharacter::AUmbraEnemyCharacter()
{
	AIControllerClass = AUmbraAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AbilitySystemComponent = CreateDefaultSubobject<UUmbraAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	AttributeSet = CreateDefaultSubobject<UUmbraAttributeSet>(TEXT("AttributeSet"));

	GetMesh()->SetRenderCustomDepth(false);
	GetMesh()->SetCustomDepthStencilValue(AttackHighlightStencilValue);
}

void AUmbraEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	if (HasAuthority())
	{
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UUmbraHitReactAbility::StaticClass(), 1));
	}

	// Blueprint and imported-asset defaults must not highlight the body, weapons, or ShadowCylinder at startup.
	TInlineComponentArray<UMeshComponent*> OwnedMeshComponents(this);
	for (UMeshComponent* MeshComponent : OwnedMeshComponents)
	{
		MeshComponent->SetRenderCustomDepth(false);
	}
	GetMesh()->SetCustomDepthStencilValue(AttackHighlightStencilValue);
}

UAbilitySystemComponent* AUmbraEnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

bool AUmbraEnemyCharacter::CanBeAttacked_Implementation() const
{
	return true;
}

void AUmbraEnemyCharacter::SetAttackHighlighted_Implementation(bool bHighlighted)
{
	USkeletalMeshComponent* EnemyMesh = GetMesh();
	if (!EnemyMesh)
	{
		return;
	}

	EnemyMesh->SetCustomDepthStencilValue(AttackHighlightStencilValue);
	EnemyMesh->SetRenderCustomDepth(bHighlighted);

	UE_LOG(LogUmbra, Log,
		TEXT("Attack highlight debug: %s highlighted=%s, mesh=%s, render custom depth=%s, stencil=%d."),
		*GetNameSafe(this),
		bHighlighted ? TEXT("true") : TEXT("false"),
		*GetNameSafe(EnemyMesh),
		EnemyMesh->bRenderCustomDepth ? TEXT("true") : TEXT("false"),
		EnemyMesh->CustomDepthStencilValue);

	const AUmbraPlayerController* UmbraController = GetWorld()
		? Cast<AUmbraPlayerController>(GetWorld()->GetFirstPlayerController())
		: nullptr;
	if (GEngine && UmbraController && UmbraController->ShouldShowAttackHighlightDebug())
	{
		const bool bActualRenderCustomDepth = EnemyMesh->bRenderCustomDepth;
		const FColor MessageColor = bActualRenderCustomDepth == bHighlighted ? FColor::Green : FColor::Red;
		const FString DebugText = FString::Printf(
			TEXT("Highlight function reached: True\nRequested Highlight: %s\nRender CustomDepth: %s\nStencil: %d"),
			bHighlighted ? TEXT("True") : TEXT("False"),
			bActualRenderCustomDepth ? TEXT("True") : TEXT("False"),
			EnemyMesh->CustomDepthStencilValue);
		GEngine->AddOnScreenDebugMessage(1010, 2.5f, MessageColor, DebugText);
	}
}
