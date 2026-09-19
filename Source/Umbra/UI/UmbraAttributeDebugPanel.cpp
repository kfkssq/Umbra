#include "UI/UmbraAttributeDebugPanel.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Characters/UmbraEnemyCharacter.h"
#include "GameFramework/PlayerState.h"
#include "UmbraPlayerController.h"

namespace
{
	TArray<FGameplayAttribute> DisplayedAttributes()
	{
		return {
			UUmbraAttributeSet::GetHealthAttribute(), UUmbraAttributeSet::GetMaxHealthAttribute(),
			UUmbraAttributeSet::GetHealthRegenAttribute(), UUmbraAttributeSet::GetResourceAttribute(),
			UUmbraAttributeSet::GetMaxResourceAttribute(), UUmbraAttributeSet::GetResourceRegenAttribute(),
			UUmbraAttributeSet::GetAttackPowerAttribute(), UUmbraAttributeSet::GetAbilityPowerAttribute(),
			UUmbraAttributeSet::GetAttackSpeedBonusAttribute(), UUmbraAttributeSet::GetCriticalChanceAttribute(),
			UUmbraAttributeSet::GetCriticalDamageMultiplierAttribute(), UUmbraAttributeSet::GetArmorAttribute(),
			UUmbraAttributeSet::GetMagicResistanceAttribute(), UUmbraAttributeSet::GetAbilityHasteAttribute(),
			UUmbraAttributeSet::GetMoveSpeedAttribute()
		};
	}
}

void UUmbraAttributeDebugPanel::NativeConstruct()
{
	Super::NativeConstruct();
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	SetVisibility(ESlateVisibility::Collapsed);
	return;
#else
	bShuttingDown = false;
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &ThisClass::HandlePawnChanged);
	}
	UUmbraAbilitySystemComponent::OnLifecycleChanged.Remove(LifecycleListener);
	LifecycleListener = UUmbraAbilitySystemComponent::OnLifecycleChanged.AddUObject(this, &ThisClass::HandleASCLifecycle);
	ViewPlayer();
#endif
}

void UUmbraAttributeDebugPanel::ShutdownPanel()
{
	if (bShuttingDown)
	{
		return;
	}
	bShuttingDown = true;
	UnbindTarget();
	UUmbraAbilitySystemComponent::OnLifecycleChanged.Remove(LifecycleListener);
	LifecycleListener.Reset();
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePawnChanged);
	}
}

void UUmbraAttributeDebugPanel::NativeDestruct()
{
	ShutdownPanel();
	if (AUmbraPlayerController* PC = Cast<AUmbraPlayerController>(GetOwningPlayer()))
	{
		PC->AttributeDebugPanelRemoved(this);
	}
	Super::NativeDestruct();
}

void UUmbraAttributeDebugPanel::ViewPlayer()
{
	bViewingPlayer = true;
	RefreshBinding();
}

void UUmbraAttributeDebugPanel::NotifyPlayerContextChanged()
{
	if (bViewingPlayer)
	{
		RefreshBinding();
	}
}

void UUmbraAttributeDebugPanel::SetSelectionFeedback(EUmbraAttributeDebugFeedback Feedback)
{
	if (!bShuttingDown)
	{
		SelectionFeedback = Feedback;
		RefreshViewState();
	}
}

void UUmbraAttributeDebugPanel::ViewEnemy(AActor* Enemy)
{
	if (bShuttingDown || !IsValid(Enemy) || !Cast<AUmbraEnemyCharacter>(Enemy))
	{
		return;
	}
	bViewingPlayer = false;
	BindTarget(Enemy, Cast<UUmbraAbilitySystemComponent>(
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Enemy)));
}

void UUmbraAttributeDebugPanel::RefreshBinding()
{
	if (bShuttingDown)
	{
		return;
	}
	AActor* Actor = ViewedActor.Get();
	if (bViewingPlayer)
	{
		APlayerController* PC = GetOwningPlayer();
		Actor = PC ? PC->GetPawn() : nullptr;
		if (Actor && Actor->IsActorBeingDestroyed())
		{
			Actor = nullptr;
		}
		if (!Actor && PC)
		{
			Actor = PC->PlayerState;
		}
		if (Actor && Actor->IsActorBeingDestroyed())
		{
			Actor = nullptr;
		}
	}
	else if (!IsValid(Actor))
	{
		SelectionFeedback = EUmbraAttributeDebugFeedback::ViewingPlayer;
		ViewPlayer();
		return;
	}
	BindTarget(Actor, Cast<UUmbraAbilitySystemComponent>(
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor)));
}

void UUmbraAttributeDebugPanel::BindTarget(AActor* Actor, UUmbraAbilitySystemComponent* ASC)
{
	if (ASC && (!ASC->IsActorInfoReady() || !ASC->GetSet<UUmbraAttributeSet>()))
	{
		ASC = nullptr;
	}
	if (ViewedActor == Actor && ViewedASC == ASC)
	{
		RefreshViewState();
		return;
	}
	UnbindTarget();
	ViewedActor = Actor;
	ViewedASC = ASC;
	if (Actor)
	{
		Actor->OnEndPlay.AddUniqueDynamic(this, &ThisClass::HandleTargetEndPlay);
	}
	if (ASC)
	{
		for (const FGameplayAttribute& Attribute : DisplayedAttributes())
		{
			const FDelegateHandle Handle = ASC->GetGameplayAttributeValueChangeDelegate(Attribute)
				.AddUObject(this, &ThisClass::HandleAttributeChanged);
			AttributeListeners.Emplace(Attribute, Handle);
		}
	}
	RefreshViewState();
}

void UUmbraAttributeDebugPanel::UnbindTarget()
{
	if (UUmbraAbilitySystemComponent* ASC = ViewedASC.Get())
	{
		for (const auto& Listener : AttributeListeners)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Listener.Key).Remove(Listener.Value);
		}
	}
	AttributeListeners.Reset();
	if (AActor* Actor = ViewedActor.Get())
	{
		Actor->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleTargetEndPlay);
	}
	ViewedASC.Reset();
	ViewedActor.Reset();
}

void UUmbraAttributeDebugPanel::HandleASCLifecycle(UUmbraAbilitySystemComponent* ASC, bool bReady)
{
	if (bShuttingDown || !ASC || ASC->GetWorld() != GetWorld())
	{
		return;
	}
	if (!bReady && ViewedASC == ASC)
	{
		SelectionFeedback = EUmbraAttributeDebugFeedback::ViewingPlayer;
		ViewPlayer();
		return;
	}
	if (bReady && (bViewingPlayer || ASC->GetOwnerActor() == ViewedActor.Get() || ASC->GetAvatarActor() == ViewedActor.Get()))
	{
		RefreshBinding();
	}
}

void UUmbraAttributeDebugPanel::HandlePawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	if (bViewingPlayer)
	{
		RefreshBinding();
	}
}

void UUmbraAttributeDebugPanel::HandleTargetEndPlay(AActor* Actor, EEndPlayReason::Type Reason)
{
	UnbindTarget();
	SelectionFeedback = EUmbraAttributeDebugFeedback::ViewingPlayer;
	ViewPlayer();
}

void UUmbraAttributeDebugPanel::HandleAttributeChanged(const FOnAttributeChangeData& Data)
{
	RefreshViewState();
}

FUmbraAttributeDebugViewState UUmbraAttributeDebugPanel::MakeViewState(const UUmbraAttributeSet* Attributes,
	AActor* TargetActor, bool bReady, bool bInViewingPlayer, EUmbraAttributeDebugFeedback Feedback)
{
	FUmbraAttributeDebugViewState State;
	State.TargetActor = TargetActor;
	State.TargetName = IsValid(TargetActor) ? TargetActor->GetActorNameOrLabel() : TEXT("Local Player");
	State.bViewingPlayer = bInViewingPlayer;
	State.bReady = bReady && Attributes;
	State.Feedback = Feedback;
	if (!Attributes)
	{
		return State;
	}

	State.Health = Attributes->GetHealth();
	State.MaxHealth = Attributes->GetMaxHealth();
	State.HealthRegen = Attributes->GetHealthRegen();
	State.Resource = Attributes->GetResource();
	State.MaxResource = Attributes->GetMaxResource();
	State.ResourceRegen = Attributes->GetResourceRegen();
	State.AttackPower = Attributes->GetAttackPower();
	State.AbilityPower = Attributes->GetAbilityPower();
	State.AttackSpeedBonus = Attributes->GetAttackSpeedBonus() * 100.f;
	State.CriticalChance = Attributes->GetCriticalChance() * 100.f;
	State.CriticalDamageMultiplier = Attributes->GetCriticalDamageMultiplier() * 100.f;
	State.Armor = Attributes->GetArmor();
	State.MagicResistance = Attributes->GetMagicResistance();
	State.AbilityHaste = Attributes->GetAbilityHaste();
	State.MoveSpeed = Attributes->GetMoveSpeed();
	return State;
}

void UUmbraAttributeDebugPanel::RefreshViewState()
{
	if (bShuttingDown)
	{
		return;
	}
	const UUmbraAbilitySystemComponent* ASC = ViewedASC.Get();
	const UUmbraAttributeSet* Attributes = ASC ? ASC->GetSet<UUmbraAttributeSet>() : nullptr;
	const bool bReady = Attributes && ASC->IsActorInfoReady();
	ViewState = MakeViewState(Attributes, ViewedActor.Get(), bReady, bViewingPlayer, SelectionFeedback);
	BP_ApplyViewState(ViewState);
}

void UUmbraAttributeDebugPanel::RequestOperation(EUmbraAttributeDebugOperation Operation)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (!bShuttingDown && ViewedActor.IsValid() && ViewedASC.IsValid() && ViewedASC->IsActorInfoReady())
	{
		if (AUmbraPlayerController* PC = Cast<AUmbraPlayerController>(GetOwningPlayer()))
		{
			PC->RequestAttributeDebugOperation(ViewedActor.Get(), Operation);
		}
	}
	// Also recovers keyboard routing if a designer accidentally left a button focusable.
	UWidgetBlueprintLibrary::SetFocusToGameViewport();
#endif
}

void UUmbraAttributeDebugPanel::NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event)
{
	Super::NativeOnMouseEnter(Geometry, Event);
	if (AUmbraPlayerController* PC = Cast<AUmbraPlayerController>(GetOwningPlayer()))
	{
		// Prevent a world-space hold/release from leaking through the panel.
		PC->StopPointerActionsForDebugUI();
	}
}

FReply UUmbraAttributeDebugPanel::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	return FReply::Handled();
}

FReply UUmbraAttributeDebugPanel::NativeOnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event)
{
	return FReply::Handled();
}
