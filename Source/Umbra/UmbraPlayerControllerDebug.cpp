#include "UmbraPlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/Effects/UmbraDebugEffects.h"
#include "Characters/UmbraEnemyCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/UmbraAttackable.h"
#include "UI/UmbraAttributeDebugPanel.h"
#include "UI/UmbraCharacterStatsPanel.h"
#include "Blueprint/WidgetTree.h"
#include "Umbra.h"

void AUmbraPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	if (AttributeDebugPanel)
	{
		AttributeDebugPanel->NotifyPlayerContextChanged();
	}
	if (IsValid(CombatHUD) && CombatHUD->WidgetTree)
	{
		TArray<UWidget*> Widgets;
		CombatHUD->WidgetTree->GetAllWidgets(Widgets);
		for (UWidget* Widget : Widgets)
		{
			if (UUmbraCharacterStatsPanel* Panel = Cast<UUmbraCharacterStatsPanel>(Widget))
			{
				Panel->NotifyPlayerContextChanged();
			}
		}
	}
}

bool AUmbraPlayerController::IsAttackableTarget(AActor* Target)
{
	return IsValid(Target) && Target->Implements<UUmbraAttackable>()
		&& IUmbraAttackable::Execute_CanBeAttacked(Target);
}

void AUmbraPlayerController::CreateAttributeDebugPanel()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (!bEnableAttributeDebugPanel || !IsLocalController() || IsValid(AttributeDebugPanel))
	{
		return;
	}
	if (!AttributeDebugPanelClass)
	{
		UE_LOG(LogUmbra, Warning, TEXT("Attribute debug: AttributeDebugPanelClass is not configured on %s."), *GetName());
		return;
	}
	AttributeDebugPanel = CreateWidget<UUmbraAttributeDebugPanel>(this, AttributeDebugPanelClass);
	if (AttributeDebugPanel)
	{
		AttributeDebugPanel->AddToPlayerScreen(20);
	}
	// The existing controller already uses GameAndUI and a visible cursor.
	// We do not replace its input mode or capture settings.
#endif
}

void AUmbraPlayerController::SetupAttributeDebugInput()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (!bEnableAttributeDebugPanel || !IsLocalController())
	{
		return;
	}
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent);
	if (!Input || !AttributeDebugInputBindingHandles.IsEmpty())
	{
		return;
	}
	if (ViewPlayerAttributesAction)
	{
		AttributeDebugInputBindingHandles.Add(Input->BindAction(ViewPlayerAttributesAction,
			ETriggerEvent::Started, this, &AUmbraPlayerController::ViewPlayerAttributes).GetHandle());
	}
	if (LockHoveredAttributesAction)
	{
		AttributeDebugInputBindingHandles.Add(Input->BindAction(LockHoveredAttributesAction,
			ETriggerEvent::Started, this, &AUmbraPlayerController::LockHoveredAttributes).GetHandle());
	}
	if (!ViewPlayerAttributesAction || !LockHoveredAttributesAction || !AttributeDebugMappingContext)
	{
		UE_LOG(LogUmbra, Warning, TEXT("Attribute debug: configure both actions and AttributeDebugMappingContext on %s."), *GetName());
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (AttributeDebugMappingContext && !Subsystem->HasMappingContext(AttributeDebugMappingContext))
		{
			Subsystem->AddMappingContext(AttributeDebugMappingContext, 10);
			bAddedAttributeDebugMapping = true;
		}
	}
#endif
}

void AUmbraPlayerController::CleanupAttributeDebugInput()
{
	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent))
	{
		for (uint32 Handle : AttributeDebugInputBindingHandles)
		{
			Input->RemoveBindingByHandle(Handle);
		}
	}
	AttributeDebugInputBindingHandles.Reset();
	if (bAddedAttributeDebugMapping && GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(AttributeDebugMappingContext);
		}
	}
	bAddedAttributeDebugMapping = false;
}

void AUmbraPlayerController::RemoveAttributeDebugPanel()
{
	if (UUmbraAttributeDebugPanel* Panel = AttributeDebugPanel)
	{
		AttributeDebugPanel = nullptr;
		Panel->ShutdownPanel();
		Panel->RemoveFromParent();
	}
	CleanupAttributeDebugInput();
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (HasAuthority())
	{
		ClearAttributeDebugEffects();
	}
	else if (!bEndingPlay && IsLocalController())
	{
		ServerClearAttributeDebugEffects();
	}
#endif
}

void AUmbraPlayerController::AttributeDebugPanelRemoved(UUmbraAttributeDebugPanel* RemovedPanel)
{
	if (AttributeDebugPanel == RemovedPanel)
	{
		AttributeDebugPanel = nullptr;
		CleanupAttributeDebugInput();
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
		if (!bEndingPlay)
		{
			ServerClearAttributeDebugEffects();
		}
#endif
	}
}

bool AUmbraPlayerController::IsPointerOverAttributeDebugPanel() const
{
	return IsValid(AttributeDebugPanel) && AttributeDebugPanel->IsInViewport() && AttributeDebugPanel->IsHovered();
}

void AUmbraPlayerController::StopPointerActionsForDebugUI()
{
	ResetPrimaryActionState();
	CancelPendingAttack();
	CancelAutoMove();
}

void AUmbraPlayerController::ViewPlayerAttributes()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (AttributeDebugPanel)
	{
		AttributeDebugPanel->ViewPlayer();
		AttributeDebugPanel->SetSelectionFeedback(EUmbraAttributeDebugFeedback::ViewingPlayer);
		UE_LOG(LogUmbra, Log, TEXT("Attribute debug: F1 selected local player."));
	}
#endif
}

void AUmbraPlayerController::LockHoveredAttributes()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (!AttributeDebugPanel)
	{
		return;
	}
	AActor* Candidate = nullptr;
	FHitResult PawnHit;
	FHitResult VisibilityHit;
	const bool bHasPawnHit = GetAttackableUnderCursor(Candidate, &PawnHit);
	const bool bHasVisibilityHit = GetCursorGroundHit(VisibilityHit);
	// Existing enemy capsules ignore Visibility. Use the same Pawn query as attack
	// hover, while retaining the first Visibility hit as an occlusion check.
	const bool bOccluded = bHasVisibilityHit && VisibilityHit.GetActor() != Candidate
		&& VisibilityHit.Distance + 1.f < PawnHit.Distance;
	if (bHasPawnHit && Cast<AUmbraEnemyCharacter>(Candidate) && !bOccluded)
	{
		AttributeDebugPanel->ViewEnemy(Candidate);
		AttributeDebugPanel->SetSelectionFeedback(EUmbraAttributeDebugFeedback::EnemyLocked);
		UE_LOG(LogUmbra, Log, TEXT("Attribute debug: F2 locked enemy %s."), *GetNameSafe(Candidate));
	}
	else
	{
		AttributeDebugPanel->SetSelectionFeedback(EUmbraAttributeDebugFeedback::EnemyLockFailed);
		UE_LOG(LogUmbra, Log, TEXT("Attribute debug: F2 kept target. Pawn=%s, Visibility=%s, Occluded=%s."),
			*GetNameSafe(Candidate), *GetNameSafe(VisibilityHit.GetActor()), bOccluded ? TEXT("true") : TEXT("false"));
	}
#endif
}

void AUmbraPlayerController::RequestAttributeDebugOperation(AActor* Target, EUmbraAttributeDebugOperation Operation)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (bEnableAttributeDebugPanel && IsLocalController() && IsValid(AttributeDebugPanel) && IsValid(Target))
	{
		ServerAttributeDebugOperation(Target, Operation);
	}
#endif
}

void AUmbraPlayerController::ServerAttributeDebugOperation_Implementation(AActor* Target, EUmbraAttributeDebugOperation Operation)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (!bEnableAttributeDebugPanel || !HasAuthority() || !IsValid(Target) || Target->GetWorld() != GetWorld())
	{
		return;
	}
	const bool bOwnPlayer = Target == GetPawn() || Target == PlayerState;
	const bool bNearbyEnemy = Cast<AUmbraEnemyCharacter>(Target)
		&& Target->Implements<UUmbraAttackable>() && GetPawn()
		&& FVector::DistSquared(GetPawn()->GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(10000.f);
	if (!bOwnPlayer && !bNearbyEnemy)
	{
		UE_LOG(LogUmbra, Warning, TEXT("Attribute debug rejected target %s for %s."), *GetNameSafe(Target), *GetName());
		return;
	}
	UUmbraAbilitySystemComponent* ASC = Cast<UUmbraAbilitySystemComponent>(
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target));
	if (!IsValid(ASC) || !ASC->IsActorInfoReady() || !ASC->IsOwnerActorAuthoritative() || !ASC->GetSet<UUmbraAttributeSet>())
	{
		return;
	}
	for (auto It = AttributeDebugEffects.CreateIterator(); It; ++It)
	{
		UAbilitySystemComponent* TrackedASC = It.Key().Get();
		if (!IsValid(TrackedASC))
		{
			It.RemoveCurrent();
			continue;
		}
		It.Value().RemoveAll([TrackedASC](const FActiveGameplayEffectHandle& Handle)
		{
			return !TrackedASC->GetActiveGameplayEffect(Handle);
		});
		if (It.Value().IsEmpty())
		{
			It.RemoveCurrent();
		}
	}
	if (Operation == EUmbraAttributeDebugOperation::RemoveEffect)
	{
		if (TArray<FActiveGameplayEffectHandle>* Handles = AttributeDebugEffects.Find(ASC))
		{
			for (const FActiveGameplayEffectHandle& Handle : *Handles)
			{
				ASC->RemoveActiveGameplayEffect(Handle);
			}
			AttributeDebugEffects.Remove(ASC);
		}
		return;
	}
	TSubclassOf<UGameplayEffect> EffectClass;
	switch (Operation)
	{
	case EUmbraAttributeDebugOperation::AddEffect:
		EffectClass = UUmbraDebugAttributeEffect::StaticClass();
		break;
	case EUmbraAttributeDebugOperation::Damage:
		EffectClass = UUmbraDebugDamageEffect::StaticClass();
		break;
	case EUmbraAttributeDebugOperation::Heal:
		EffectClass = UUmbraDebugHealEffect::StaticClass();
		break;
	default:
		return;
	}
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
	if (Spec.IsValid())
	{
		const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		if (Operation == EUmbraAttributeDebugOperation::AddEffect && Handle.IsValid())
		{
			AttributeDebugEffects.FindOrAdd(ASC).Add(Handle);
		}
	}
#endif
}

void AUmbraPlayerController::ClearAttributeDebugEffects()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (HasAuthority())
	{
		for (const auto& Entry : AttributeDebugEffects)
		{
			if (UAbilitySystemComponent* ASC = Entry.Key.Get())
			{
				for (const FActiveGameplayEffectHandle& Handle : Entry.Value)
				{
					ASC->RemoveActiveGameplayEffect(Handle);
				}
			}
		}
		AttributeDebugEffects.Reset();
	}
#endif
}

void AUmbraPlayerController::ServerClearAttributeDebugEffects_Implementation()
{
	ClearAttributeDebugEffects();
}
