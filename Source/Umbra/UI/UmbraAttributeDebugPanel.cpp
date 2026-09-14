#include "UI/UmbraAttributeDebugPanel.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Characters/UmbraEnemyCharacter.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"
#include "UmbraPlayerController.h"
#include "Umbra.h"

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
	SetIsFocusable(false);
	SetVisibility(ESlateVisibility::Visible);
	if (!TargetNameText || !AttributesText || !HintText || !AddEffectButton
		|| !RemoveEffectButton || !DamageButton || !HealButton)
	{
		UE_LOG(LogUmbra, Error, TEXT("Attribute debug: missing BindWidget controls in %s. See Docs/AttributeDebugPanel.md."), *GetName());
	}
	if (HintText)
	{
		HintText->SetText(FText::FromString(TEXT("F1 查看玩家\n悬停敌人后按 F2 锁定查看")));
	}
	if (AddEffectButton) AddEffectButton->OnClicked.AddUniqueDynamic(this, &ThisClass::AddEffectClicked);
	if (RemoveEffectButton) RemoveEffectButton->OnClicked.AddUniqueDynamic(this, &ThisClass::RemoveEffectClicked);
	if (DamageButton) DamageButton->OnClicked.AddUniqueDynamic(this, &ThisClass::DamageClicked);
	if (HealButton) HealButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HealClicked);
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
	if (AddEffectButton) AddEffectButton->OnClicked.RemoveDynamic(this, &ThisClass::AddEffectClicked);
	if (RemoveEffectButton) RemoveEffectButton->OnClicked.RemoveDynamic(this, &ThisClass::RemoveEffectClicked);
	if (DamageButton) DamageButton->OnClicked.RemoveDynamic(this, &ThisClass::DamageClicked);
	if (HealButton) HealButton->OnClicked.RemoveDynamic(this, &ThisClass::HealClicked);
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

void UUmbraAttributeDebugPanel::SetSelectionFeedback(const FText& Message)
{
	if (HintText && !bShuttingDown)
	{
		HintText->SetText(FText::Format(
			NSLOCTEXT("UmbraAttributeDebug", "SelectionHint", "F1 查看玩家 | 悬停敌人后 F2 锁定\n{0}"), Message));
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
		RefreshSnapshot();
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
	RefreshSnapshot();
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
	ViewPlayer();
}

void UUmbraAttributeDebugPanel::HandleAttributeChanged(const FOnAttributeChangeData& Data)
{
	RefreshSnapshot();
}

FText UUmbraAttributeDebugPanel::FormatAttributeSnapshot(const UUmbraAttributeSet& A)
{
	return FText::FromString(FString::Printf(
		TEXT("生命：%.1f / %.1f\n生命恢复：%.1f 点/秒\n资源：%.1f / %.1f\n资源恢复：%.1f 点/秒\n")
		TEXT("攻击力：%.1f\n技能强度：%.1f\n攻速加成：%.1f%%\n暴击率：%.1f%%\n暴击总倍率：%.1f%%\n")
		TEXT("护甲：%.1f\n魔法抗性：%.1f\n技能急速：%.1f\n移速：%.1f 厘米/秒"),
		A.GetHealth(), A.GetMaxHealth(), A.GetHealthRegen(), A.GetResource(), A.GetMaxResource(), A.GetResourceRegen(),
		A.GetAttackPower(), A.GetAbilityPower(), A.GetAttackSpeedBonus() * 100.f, A.GetCriticalChance() * 100.f,
		A.GetCriticalDamageMultiplier() * 100.f, A.GetArmor(), A.GetMagicResistance(), A.GetAbilityHaste(), A.GetMoveSpeed()));
}

void UUmbraAttributeDebugPanel::RefreshSnapshot()
{
	if (bShuttingDown)
	{
		return;
	}
	const UUmbraAbilitySystemComponent* ASC = ViewedASC.Get();
	const UUmbraAttributeSet* Attributes = ASC ? ASC->GetSet<UUmbraAttributeSet>() : nullptr;
	const bool bReady = Attributes && ASC->IsActorInfoReady();
	if (TargetNameText)
	{
		TargetNameText->SetText(FText::FromString(ViewedActor.IsValid()
			? ViewedActor->GetActorNameOrLabel() : TEXT("本地玩家")));
	}
	if (AttributesText)
	{
		AttributesText->SetText(bReady ? FormatAttributeSnapshot(*Attributes)
			: FText::FromString(TEXT("等待玩家 / Ability System 就绪…")));
	}
	for (UButton* Button : { AddEffectButton.Get(), RemoveEffectButton.Get(), DamageButton.Get(), HealButton.Get() })
	{
		if (Button) Button->SetIsEnabled(bReady);
	}
}

void UUmbraAttributeDebugPanel::SendOperation(EUmbraAttributeDebugOperation Operation)
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

void UUmbraAttributeDebugPanel::AddEffectClicked() { SendOperation(EUmbraAttributeDebugOperation::AddEffect); }
void UUmbraAttributeDebugPanel::RemoveEffectClicked() { SendOperation(EUmbraAttributeDebugOperation::RemoveEffect); }
void UUmbraAttributeDebugPanel::DamageClicked() { SendOperation(EUmbraAttributeDebugOperation::Damage); }
void UUmbraAttributeDebugPanel::HealClicked() { SendOperation(EUmbraAttributeDebugOperation::Heal); }

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
