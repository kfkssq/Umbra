// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/UmbraEnemyCharacter.h"

#include "AI/UmbraAIController.h"
#include "AbilitySystem/Abilities/UmbraHitReactAbility.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Components/MeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include "Umbra.h"
#include "UmbraPlayerController.h"
#include "UI/UmbraEnemyHealthBarComponent.h"

AUmbraEnemyCharacter::AUmbraEnemyCharacter()
{
	AIControllerClass = AUmbraAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AbilitySystemComponent = CreateDefaultSubobject<UUmbraAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	AttributeSet = CreateDefaultSubobject<UUmbraAttributeSet>(TEXT("AttributeSet"));
	HealthBarComponent = CreateDefaultSubobject<UUmbraEnemyHealthBarComponent>(TEXT("HealthBarComponent"));
	HealthBarComponent->SetupAttachment(GetRootComponent());
	HealthBarComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));

	GetMesh()->SetRenderCustomDepth(false);
	GetMesh()->SetCustomDepthStencilValue(AttackHighlightStencilValue);
}

void AUmbraEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = EnemyMoveSpeed;
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AbilitySystemComponent->InitializeAttributes(InitialAttributesEffect, bUseDebugInitialAttributes ? &DebugInitialAttributes : nullptr);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetHealthAttribute())
		.AddUObject(this, &AUmbraEnemyCharacter::HandleHealthChanged);
	if (HasAuthority())
	{
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UUmbraHitReactAbility::StaticClass(), 1));
		if (BasicAttackAbilityClass)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(BasicAttackAbilityClass, 1));
		}
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
	return !bIsDead;
}

void AUmbraEnemyCharacter::SetAttackHighlighted_Implementation(bool bHighlighted)
{
	bHighlighted = bHighlighted && !bIsDead;
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

bool AUmbraEnemyCharacter::TryActivateBasicAttack()
{
	if (!bAIBehaviorEnabled || bIsDead || !IsValid(CombatTarget.Get()) || !AbilitySystemComponent)
	{
		return false;
	}

	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(UmbraGameplayTags::Ability_Attack_EnemyBasic);
	return AbilitySystemComponent->TryActivateAbilitiesByTag(AbilityTags);
}

void AUmbraEnemyCharacter::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	if (ChangeData.NewValue <= 0.0f && ChangeData.OldValue > 0.0f)
	{
		Die();
	}
}

void AUmbraEnemyCharacter::Die()
{
	if (bIsDead || !HasAuthority())
	{
		return;
	}

	bIsDead = true;
	ApplyDeathState();
	ForceNetUpdate();
}

void AUmbraEnemyCharacter::ApplyDeathState()
{
	if (bDeathStateApplied)
	{
		return;
	}
	bDeathStateApplied = true;
	CombatTarget.Reset();
	SetAttackHighlighted_Implementation(false);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(UmbraGameplayTags::State_Dead);
		AbilitySystemComponent->CancelAllAbilities();
	}
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (DeathMontage)
	{
		const float DeathDuration = PlayAnimMontage(DeathMontage);
		if (DeathDuration > 0.0f)
		{
			const float FreezeDelay = FMath::Max(DeathDuration - DeathPoseFreezeLeadTime, 0.0f);
			GetWorldTimerManager().SetTimer(
				DeathPoseTimerHandle,
				this,
				&AUmbraEnemyCharacter::FreezeDeathPose,
				FreezeDelay,
				false);
		}
	}
	OnDeathStarted();

	if (HasAuthority() && CorpseLifetime > 0.0f)
	{
		SetLifeSpan(CorpseLifetime);
	}
}

void AUmbraEnemyCharacter::FreezeDeathPose()
{
	if (bIsDead && GetMesh())
	{
		GetMesh()->bPauseAnims = true;
	}
}

void AUmbraEnemyCharacter::OnRep_IsDead()
{
	if (bIsDead)
	{
		ApplyDeathState();
	}
}

void AUmbraEnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AUmbraEnemyCharacter, bIsDead);
}
