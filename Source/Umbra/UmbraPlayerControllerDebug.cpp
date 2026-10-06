#include "UmbraPlayerController.h"
#include "UI/Inventory/UmbraInventoryMenu.h"
#include "Stats/UmbraDerivedStatsComponent.h"

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
#include "UI/Combat/UmbraCombatInfo.h"
#include "UI/Equipment/UmbraEquipmentMenu.h"
#include "Blueprint/WidgetTree.h"
#include "Umbra.h"
#include "GameplayEffect.h"
#include "Components/InputComponent.h"

void AUmbraPlayerController::SetupQuickGameplayEffectInput()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (!bEnableQuickGameplayEffectKeys || !IsLocalController() || !InputComponent || QuickGameplayEffectInput.Get() == InputComponent) return;
	InputComponent->BindKey(EKeys::Eight, IE_Pressed, this, &ThisClass::QuickGameplayEffectPressed);
	InputComponent->BindKey(EKeys::Nine, IE_Pressed, this, &ThisClass::RemoveQuickTestGameplayEffect);
	InputComponent->BindKey(EKeys::Seven, IE_Pressed, this, &ThisClass::QuickVulnerableGameplayEffectPressed);
	InputComponent->BindKey(EKeys::Six, IE_Pressed, this, &ThisClass::RemoveQuickVulnerableGameplayEffect);
	QuickGameplayEffectInput = InputComponent;
#endif
}

void AUmbraPlayerController::QuickGameplayEffectPressed() { ApplyQuickTestGameplayEffect(); }

void AUmbraPlayerController::QuickVulnerableGameplayEffectPressed()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (!HasAuthority() || bEndingPlay || IsPointerOverAttributeDebugPanel()) return;
	AActor* Candidate = nullptr;
	FHitResult PawnHit, VisibilityHit;
	const bool bHasPawnHit = GetAttackableUnderCursor(Candidate, &PawnHit);
	const bool bHasVisibilityHit = GetCursorGroundHit(VisibilityHit);
	// Match F2 selection: enemy capsules ignore Visibility, so trace Pawns and check occlusion separately.
	const bool bOccluded = bHasVisibilityHit && VisibilityHit.GetActor() != Candidate
		&& VisibilityHit.Distance + 1.f < PawnHit.Distance;
	if (!bHasPawnHit || !Cast<AUmbraEnemyCharacter>(Candidate) || bOccluded)
	{
		UE_LOG(LogUmbra, Log, TEXT("Quick Vulnerable GE: point at an unobstructed living enemy; previous effect kept."));
		return;
	}
	ApplyQuickVulnerableGameplayEffect(Candidate);
#endif
}

FActiveGameplayEffectHandle AUmbraPlayerController::ApplyQuickVulnerableGameplayEffect(AActor* Target)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (!HasAuthority() || bEndingPlay) return FActiveGameplayEffectHandle();
	auto* Enemy = Cast<AUmbraEnemyCharacter>(Target);
	auto* ASC = IsValid(Enemy) && IsAttackableTarget(Enemy) ? Enemy->GetUmbraAbilitySystemComponent() : nullptr;
	auto* SourceASC = Cast<UUmbraAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerState));
	if (!ASC || !ASC->IsActorInfoReady() || !ASC->IsOwnerActorAuthoritative()
		|| !SourceASC || !SourceASC->IsActorInfoReady() || !SourceASC->IsOwnerActorAuthoritative())
	{
		UE_LOG(LogUmbra, Warning, TEXT("Quick Vulnerable GE: living enemy and ready player/enemy ASCs required."));
		return FActiveGameplayEffectHandle();
	}
	if (QuickVulnerableGameplayEffectASC.Get() == ASC && ASC->GetActiveGameplayEffect(QuickVulnerableGameplayEffectHandle))
	{
		UE_LOG(LogUmbra, Log, TEXT("Quick Vulnerable GE: already active on %s; press 6 to remove."), *GetNameSafe(Enemy));
		return QuickVulnerableGameplayEffectHandle;
	}
	const TSubclassOf<UGameplayEffect> EffectClass = QuickVulnerableGameplayEffect.LoadSynchronous();
	const UGameplayEffect* Effect = EffectClass ? EffectClass.GetDefaultObject() : nullptr;
	if (!Effect || Effect->DurationPolicy == EGameplayEffectDurationType::Instant)
	{
		UE_LOG(LogUmbra, Warning, TEXT("Quick Vulnerable GE: configure a Duration/Infinite QuickVulnerableGameplayEffect."));
		return FActiveGameplayEffectHandle();
	}
	for (const auto Handle : ASC->GetActiveEffects(FGameplayEffectQuery()))
	{
		const auto* Existing = ASC->GetActiveGameplayEffect(Handle);
		if (Existing && Existing->Spec.Def == Effect)
		{
			UE_LOG(LogUmbra, Warning, TEXT("Quick Vulnerable GE: already active from another source; previous effect kept."));
			return FActiveGameplayEffectHandle();
		}
	}
	const auto Spec = SourceASC->MakeOutgoingSpec(EffectClass, 1.f, SourceASC->MakeEffectContext());
	if (!Spec.IsValid()) return FActiveGameplayEffectHandle();
	RemoveQuickVulnerableGameplayEffect();
	const auto Handle = SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), ASC);
	if (!ASC->GetActiveGameplayEffect(Handle))
	{
		UE_LOG(LogUmbra, Warning, TEXT("Quick Vulnerable GE: application rejected for %s."), *GetNameSafe(Enemy));
		return FActiveGameplayEffectHandle();
	}
	QuickVulnerableGameplayEffectASC = ASC;
	QuickVulnerableGameplayEffectHandle = Handle;
	UE_LOG(LogUmbra, Log, TEXT("Quick Vulnerable GE: applied %s to %s at Level 1; press 6 to remove."),
		*GetNameSafe(EffectClass), *GetNameSafe(Enemy));
	return Handle;
#else
	return FActiveGameplayEffectHandle();
#endif
}

void AUmbraPlayerController::RemoveQuickVulnerableGameplayEffect()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (!HasAuthority()) return;
	const auto Handle = QuickVulnerableGameplayEffectHandle;
	auto* ASC = QuickVulnerableGameplayEffectASC.Get();
	QuickVulnerableGameplayEffectHandle.Invalidate();
	QuickVulnerableGameplayEffectASC.Reset();
	if (ASC && ASC->GetActiveGameplayEffect(Handle))
	{
		ASC->RemoveActiveGameplayEffect(Handle);
		UE_LOG(LogUmbra, Log, TEXT("Quick Vulnerable GE: removed the saved enemy test effect."));
	}
#endif
}

FActiveGameplayEffectHandle AUmbraPlayerController::ApplyQuickTestGameplayEffect()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	// Match SwitchHasAuthority; this debug shortcut deliberately adds no client-to-server RPC.
	if (!HasAuthority() || bEndingPlay) return FActiveGameplayEffectHandle();
	auto* ASC = Cast<UUmbraAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerState));
	if (!ASC || !ASC->IsActorInfoReady() || !ASC->IsOwnerActorAuthoritative())
	{
		UE_LOG(LogUmbra, Warning, TEXT("Quick GE: player ASC is not ready."));
		return FActiveGameplayEffectHandle();
	}
	if (QuickGameplayEffectASC.IsValid() && QuickGameplayEffectASC.Get() != ASC) RemoveQuickTestGameplayEffect();
	if (ASC->GetActiveGameplayEffect(QuickTestGameplayEffectHandle))
	{
		UE_LOG(LogUmbra, Log, TEXT("Quick GE: already active; repeated key ignored. Press 9 to remove."));
		return QuickTestGameplayEffectHandle;
	}
	QuickTestGameplayEffectHandle.Invalidate();
	QuickGameplayEffectASC.Reset();
	const TSubclassOf<UGameplayEffect> EffectClass = QuickTestGameplayEffect.LoadSynchronous();
	const UGameplayEffect* Effect = EffectClass ? EffectClass.GetDefaultObject() : nullptr;
	if (!Effect || Effect->DurationPolicy == EGameplayEffectDurationType::Instant)
	{
		UE_LOG(LogUmbra, Warning, TEXT("Quick GE: configure a Duration/Infinite effect in QuickTestGameplayEffect (Instant cannot be undone)."));
		return FActiveGameplayEffectHandle();
	}
	// Do not create another instance if this same effect was already applied outside the shortcut.
	for (const auto Handle : ASC->GetActiveEffects(FGameplayEffectQuery()))
	{
		const auto* Existing = ASC->GetActiveGameplayEffect(Handle);
		if (Existing && Existing->Spec.Def == Effect)
		{
			UE_LOG(LogUmbra, Warning, TEXT("Quick GE: this class is already active from another source; no extra instance or ownership taken."));
			return FActiveGameplayEffectHandle();
		}
	}
	const auto Spec = ASC->MakeOutgoingSpec(EffectClass, 1.f, ASC->MakeEffectContext());
	if (!Spec.IsValid()) return FActiveGameplayEffectHandle();
	const auto Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	if (!ASC->GetActiveGameplayEffect(Handle))
	{
		UE_LOG(LogUmbra, Warning, TEXT("Quick GE: application rejected for %s."), *GetNameSafe(EffectClass));
		return FActiveGameplayEffectHandle();
	}
	QuickGameplayEffectASC = ASC;
	QuickTestGameplayEffectHandle = Handle;
	UE_LOG(LogUmbra, Log, TEXT("Quick GE: applied %s to player at Level 1; press 9 to remove."), *GetNameSafe(EffectClass));
	return Handle;
#else
	return FActiveGameplayEffectHandle();
#endif
}

void AUmbraPlayerController::RemoveQuickTestGameplayEffect()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (!HasAuthority()) return;
	// Clear first so synchronous callbacks cannot accidentally operate on the previous handle.
	const auto Handle = QuickTestGameplayEffectHandle;
	auto* ASC = QuickGameplayEffectASC.Get();
	QuickTestGameplayEffectHandle.Invalidate();
	QuickGameplayEffectASC.Reset();
	if (ASC && ASC->GetActiveGameplayEffect(Handle))
	{
		ASC->RemoveActiveGameplayEffect(Handle);
		UE_LOG(LogUmbra, Log, TEXT("Quick GE: removed the saved test effect."));
	}
#endif
}

void AUmbraPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	if (AttributeDebugPanel)
	{
		AttributeDebugPanel->NotifyPlayerContextChanged();
	}
	// CharacterMenu contains nested UserWidgets; each panel still owns its own ASC subscription.
	TFunction<void(UUserWidget*)> NotifyStatsPanels = [&NotifyStatsPanels](UUserWidget* Root)
	{
		if (!IsValid(Root)) return;
		if (UUmbraCharacterStatsPanel* Panel = Cast<UUmbraCharacterStatsPanel>(Root))
			Panel->NotifyPlayerContextChanged();
		if (UUmbraCombatInfo* Panel = Cast<UUmbraCombatInfo>(Root))
			Panel->NotifyPlayerContextChanged();
		if (UUmbraInventoryMenu* Panel = Cast<UUmbraInventoryMenu>(Root))
			Panel->NotifyPlayerContextChanged();
		if (UUmbraEquipmentMenu* Panel = Cast<UUmbraEquipmentMenu>(Root))
			Panel->NotifyPlayerContextChanged();
		if (!Root->WidgetTree) return;
		TArray<UWidget*> Widgets;
		Root->WidgetTree->GetAllWidgets(Widgets);
		for (UWidget* Widget : Widgets)
		{
			if (UUserWidget* Child = Cast<UUserWidget>(Widget)) NotifyStatsPanels(Child);
		}
	};
	NotifyStatsPanels(CombatHUD);
	NotifyStatsPanels(CharacterMenu);
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
		if (const auto* Derived = ASC->GetOwnerActor()->FindComponentByClass<UUmbraDerivedStatsComponent>();
			Derived && Derived->IsDerivedPowerActive()) EffectClass = UUmbraDebugDerivedAttributeEffect::StaticClass();
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
