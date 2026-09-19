// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Abilities/UmbraBasicAttackAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/UmbraAnimNotify_AttackChainPoint.h"
#include "Animation/UmbraAnimNotifyState_AttackHitWindow.h"
#include "Characters/UmbraPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Interfaces/UmbraAttackable.h"
#include "Umbra.h"
#include "UmbraPlayerController.h"
#include "TimerManager.h"

static TAutoConsoleVariable<int32> CVarUmbraAttackLog(
	TEXT("umbra.Attack.Log"), 0, TEXT("1: log authoritative logical basic attack timing, presentation and hit results."));

UUmbraBasicAttackAbility::UUmbraBasicAttackAbility()
{
	InputTag = UmbraGameplayTags::Input_Attack_Primary;
	ActivationPolicy = EUmbraAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(UmbraGameplayTags::Ability_Attack_Basic);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(UmbraGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(UmbraGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(UmbraGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(UmbraGameplayTags::State_Stunned);
}

void UUmbraBasicAttackAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	UAnimInstance* AnimInstance = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!HasAnyConfiguredAttackMontage()
		|| !AnimInstance || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogUmbra, Error, TEXT("Basic attack cannot start: montageConfigured=%d animInstance=%s. Check GA_BasicAttack and the mesh AnimBP."),
			HasAnyConfiguredAttackMontage(), *GetNameSafe(AnimInstance));
		FinishAbility(true);
		return;
	}

	CurrentComboIndex = 0;
	bComboInputQueued = false;
	ComboInputTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		UmbraGameplayTags::Event_Attack_ComboInput,
		nullptr,
		false,
		true);
	if (!ComboInputTask)
	{
		FinishAbility(true);
		return;
	}
	ComboInputTask->EventReceived.AddDynamic(this, &UUmbraBasicAttackAbility::HandleComboInput);
	ComboInputTask->ReadyForActivation();

	// Legacy notify events remain available to other abilities; player basic attacks use timers.
	MovementCommandTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, UmbraGameplayTags::Event_Attack_MovementCommand, nullptr, false, true);
	if (!MovementCommandTask)
	{
		FinishAbility(true);
		return;
	}
	MovementCommandTask->EventReceived.AddDynamic(this, &ThisClass::HandleMovementCommand);
	MovementCommandTask->ReadyForActivation();
	DeathInterruptTask = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(
		this, UmbraGameplayTags::State_Dead, nullptr, true);
	StunInterruptTask = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(
		this, UmbraGameplayTags::State_Stunned, nullptr, true);
	if (!DeathInterruptTask || !StunInterruptTask)
	{
		FinishAbility(true);
		return;
	}
	DeathInterruptTask->Added.AddDynamic(this, &ThisClass::HandleForcedInterrupt);
	StunInterruptTask->Added.AddDynamic(this, &ThisClass::HandleForcedInterrupt);
	DeathInterruptTask->ReadyForActivation();
	StunInterruptTask->ReadyForActivation();
	if (!IsActive())
	{
		return;
	}

	if (!PlayCurrentAttackMontage())
	{
		FinishAbility(true);
	}
}

bool UUmbraBasicAttackAbility::PlayCurrentAttackMontage()
{
	// Each combo step is one attack: changes during this montage are deferred
	// until the next step (or the next ability activation).
	CapturedAttackSpeed = ReadAttackSpeed();
	const bool bHighSpeedMode = bEnableHighSpeedAttackMode
		&& CapturedAttackSpeed >= GetSafeHighSpeedThreshold();
	if (bPreviousStepHighSpeed && !bHighSpeedMode)
	{
		// Leaving the high-speed loop always restarts the authored normal combo.
		CurrentComboIndex = 0;
	}
	if (bHighSpeedMode)
	{
		CurrentComboIndex = 0;
		CurrentHighSpeedMontageIndex = bPreviousStepHighSpeed
			? FindNextValidHighSpeedMontageIndex(CurrentHighSpeedMontageIndex)
			: FindNextValidHighSpeedMontageIndex(INDEX_NONE);
	}
	else
	{
		CurrentHighSpeedMontageIndex = INDEX_NONE;
	}
	CurrentAttackMontage = SelectAttackMontage(bHighSpeedMode);
	if (!CurrentAttackMontage)
	{
		UE_LOG(LogUmbra, Error, TEXT("Basic attack has no valid montage for the selected normal/high-speed step."));
		return false;
	}
	const AUmbraPlayerCharacter* AttackCharacter = Cast<AUmbraPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!AttackCharacter || !AttackCharacter->IsTargetInPrimaryAttackRange(AttackCharacter->GetPrimaryAttackTarget())) return false;
	bCurrentStepHighSpeed = bHighSpeedMode;
	bPreviousStepHighSpeed = bHighSpeedMode;
	const float VisualStrikeTime = ResolveVisualStrikeTime(CurrentAttackMontage);
	if (VisualStrikeTime <= 0.f)
	{
		UE_LOG(LogUmbra, Error, TEXT("Basic attack montage %s needs VisualStrikeTimeOverrides or a valid first Attack Hit Window Begin."),
			*GetNameSafe(CurrentAttackMontage));
		return false;
	}
	const float StableBasePeriod = ResolveBaseAttackInterval();
	if (StableBasePeriod <= 0.f) return false;
	CurrentEffectiveAttackPeriod = StableBasePeriod / CapturedAttackSpeed;
	const float SafeWindupRatio = FMath::Clamp(FMath::IsFinite(AttackWindupRatio) ? AttackWindupRatio : 0.3f, 0.01f, 0.99f);
	if (!FMath::IsNearlyEqual(SafeWindupRatio, AttackWindupRatio))
	{
		UE_LOG(LogUmbra, Warning, TEXT("AttackWindupRatio %.3f invalid; using %.3f."), AttackWindupRatio, SafeWindupRatio);
	}
	CurrentWindupSeconds = CurrentEffectiveAttackPeriod * SafeWindupRatio;
	const float AssetRateScale = FMath::Max(0.01f, CurrentAttackMontage->RateScale);
	const float RequestedRate = VisualStrikeTime / CurrentWindupSeconds;
	const float MaxActualRate = GetSafeMaxMontagePlayRate();
	const float ActualRate = FMath::Min(RequestedRate, MaxActualRate);
	CurrentMontagePlayRate = ActualRate / AssetRateScale;
	if (RequestedRate > MaxActualRate + KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogUmbra, Warning, TEXT("Basic attack %s cannot reach visual strike by logical windup %.3fs: requested rate %.3f, capped at %.3f. Shorten or replace the high-speed montage."),
			*GetNameSafe(CurrentAttackMontage), CurrentWindupSeconds, RequestedRate, ActualRate);
	}
	EndedHitWindowCount = 0;
	OpenHitWindowCount = 0;
	AttackPhase = EUmbraBasicAttackPhase::Windup;
	ReleaseActiveMontageTask();
	UUmbraAbilitySystemComponent* UmbraASC = GetUmbraAbilitySystemComponent();
	if (!UmbraASC)
	{
		return false;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackIntervalTimerHandle);
		World->GetTimerManager().ClearTimer(AttackWindupTimerHandle);
	}
	UmbraASC->EndPrimaryAttackInstance(AttackInstanceId);
	AttackInstanceId = UmbraASC->BeginPrimaryAttackInstance(CurrentEffectiveAttackPeriod);
	if (AUmbraPlayerCharacter* Player = Cast<AUmbraPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		StrikeTarget = Player->GetPrimaryAttackTarget();
	}
	bStrikeResolved = false;
	if (GetWorld())
	{
		const FTimerDelegate StrikeDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::ResolveLogicalStrike, AttackInstanceId);
		GetWorld()->GetTimerManager().SetTimer(AttackWindupTimerHandle, StrikeDelegate, CurrentWindupSeconds, false);
	}
	ScheduleIntervalTransition(AttackInstanceId);
	if (GetAvatarActorFromActorInfo()->HasAuthority() && CVarUmbraAttackLog.GetValueOnGameThread())
	{
		UE_LOG(LogUmbra, Log, TEXT("Attack start id=%u target=%s speed=%.3f period=%.4f windup=%.4f server=%.4f next=%.4f montage=%s requestedRate=%.3f actualRate=%.3f capped=%d"),
			AttackInstanceId, *GetNameSafe(StrikeTarget.Get()), CapturedAttackSpeed, CurrentEffectiveAttackPeriod,
			CurrentWindupSeconds, GetWorld()->GetTimeSeconds(), GetWorld()->GetTimeSeconds() + CurrentEffectiveAttackPeriod,
			*GetNameSafe(CurrentAttackMontage), RequestedRate, ActualRate, RequestedRate > MaxActualRate);
	}
	FacePrimaryAttackTarget();
	HitActorsThisComboStep.Reset();
	bHasPreviousHitSocketLocation = false;
	bHitWindowActive = false;
	bAttackIntervalCommitted = false;
	bMovementCancelRequested = false;
	bExecuteQueuedCommandOnEnd = false;
	bPendingTransitionFromMontageEnd = false;
	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		CurrentAttackMontage,
		CurrentMontagePlayRate,
		NAME_None,
		false);
	if (!ActiveMontageTask)
	{
		UmbraASC->EndPrimaryAttackInstance(AttackInstanceId);
		AttackInstanceId = 0;
		return false;
	}

	// Montage lifetime is visual only. It cannot end this strike or trigger the next one.
	ActiveMontageTask->ReadyForActivation();
	const ACharacter* AvatarCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	const UAnimInstance* ActiveAnim = AvatarCharacter && AvatarCharacter->GetMesh()
		? AvatarCharacter->GetMesh()->GetAnimInstance() : nullptr;
	if (!ActiveAnim || !ActiveAnim->Montage_IsPlaying(CurrentAttackMontage))
	{
		UE_LOG(LogUmbra, Error, TEXT("Basic attack montage %s did not start on %s. Check AnimBP, slot and montage asset."),
			*GetNameSafe(CurrentAttackMontage), *GetNameSafe(AvatarCharacter));
		FinishAbility(true);
		return false;
	}
	return IsActive() && AttackInstanceId != 0;
}

bool UUmbraBasicAttackAbility::CanActivateAbility(FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	const UUmbraAbilitySystemComponent* ASC = ActorInfo
		? Cast<UUmbraAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()) : nullptr;
	return !ASC || ASC->IsPrimaryAttackIntervalReady();
}

bool UUmbraBasicAttackAbility::StartNextComboStep(bool bAllowWrap)
{
	const float NextAttackSpeed = ReadAttackSpeed();
	const bool bNextHighSpeed = bEnableHighSpeedAttackMode
		&& NextAttackSpeed >= GetSafeHighSpeedThreshold();
	if (bNextHighSpeed || bCurrentStepHighSpeed)
	{
		CurrentComboIndex = 0;
	}
	else if (AttackMontages.IsValidIndex(CurrentComboIndex + 1))
	{
		++CurrentComboIndex;
	}
	else if (bAllowWrap && !AttackMontages.IsEmpty())
	{
		CurrentComboIndex = 0;
	}
	else
	{
		return false;
	}

	if (ComboWindowTask)
	{
		ComboWindowTask->EndTask();
		ComboWindowTask = nullptr;
	}

	bComboInputQueued = false;
	return PlayCurrentAttackMontage();
}

bool UUmbraBasicAttackAbility::StartComboGraceWindow()
{
	if (ComboWindowDuration <= 0.0f
		|| (!bCurrentStepHighSpeed && !AttackMontages.IsValidIndex(CurrentComboIndex + 1)))
	{
		return false;
	}

	// Montage notifies/completion already scale with play rate; this post-montage
	// grace period is a world-time delay and must be scaled explicitly.
	ComboWindowTask = UAbilityTask_WaitDelay::WaitDelay(this, ScaleAttackDuration(ComboWindowDuration));
	if (!ComboWindowTask)
	{
		return false;
	}

	ComboWindowTask->OnFinish.AddDynamic(this, &UUmbraBasicAttackAbility::HandleComboWindowExpired);
	ComboWindowTask->ReadyForActivation();
	return true;
}

float UUmbraBasicAttackAbility::ReadAttackSpeed() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const float Bonus = ASC
		? ASC->GetNumericAttribute(UUmbraAttributeSet::GetAttackSpeedBonusAttribute())
		: 0.f;
	const float Multiplier = 1.f + (FMath::IsFinite(Bonus) ? Bonus : 0.f);
	return FMath::Clamp(Multiplier, UUmbraAttributeSet::MinAttackSpeedMultiplier,
		UUmbraAttributeSet::MaxAttackSpeedMultiplier);
}

float UUmbraBasicAttackAbility::ScaleAttackDuration(float BaseDuration) const
{
	return FMath::Max(0.f, BaseDuration) / FMath::Clamp(CapturedAttackSpeed,
		UUmbraAttributeSet::MinAttackSpeedMultiplier, UUmbraAttributeSet::MaxAttackSpeedMultiplier);
}

float UUmbraBasicAttackAbility::GetSafeHighSpeedThreshold() const
{
	return FMath::Clamp(FMath::IsFinite(HighSpeedAttackThreshold) ? HighSpeedAttackThreshold : 3.f,
		UUmbraAttributeSet::MinAttackSpeedMultiplier, UUmbraAttributeSet::MaxAttackSpeedMultiplier);
}

float UUmbraBasicAttackAbility::ResolveBaseAttackInterval() const
{
	if (FMath::IsFinite(BaseAttackInterval) && BaseAttackInterval >= 0.01f)
	{
		return BaseAttackInterval;
	}

	// Auto mode preserves the authored playback speed of the first normal strike at 1x attack speed.
	const UAnimMontage* ReferenceMontage = AttackMontages.IsValidIndex(0) ? AttackMontages[0].Get() : nullptr;
	if (!ReferenceMontage)
	{
		UE_LOG(LogUmbra, Error, TEXT("BaseAttackInterval=0 requires normal AttackMontages[0]; set an explicit base period or configure the normal montage."));
		return 0.f;
	}
	return FMath::Max(0.01f, ReferenceMontage->GetPlayLength()
		/ FMath::Max(0.01f, ReferenceMontage->RateScale));
}

float UUmbraBasicAttackAbility::GetSafeMaxMontagePlayRate() const
{
	return FMath::Clamp(FMath::IsFinite(MaxAttackMontagePlayRate) ? MaxAttackMontagePlayRate : 3.f,
		0.1f, 10.f);
}

float UUmbraBasicAttackAbility::CalculateMontagePlayRate(float EffectiveAuthoredDuration,
	float InBaseAttackInterval, float AttackSpeedMultiplier, float MaxPlayRate)
{
	const float Duration = FMath::IsFinite(EffectiveAuthoredDuration)
		? FMath::Max(0.001f, EffectiveAuthoredDuration) : 1.f;
	const float Interval = FMath::IsFinite(InBaseAttackInterval)
		? FMath::Max(0.01f, InBaseAttackInterval) : 1.f;
	const float Speed = FMath::Clamp(FMath::IsFinite(AttackSpeedMultiplier) ? AttackSpeedMultiplier : 1.f,
		UUmbraAttributeSet::MinAttackSpeedMultiplier, UUmbraAttributeSet::MaxAttackSpeedMultiplier);
	const float SafeMaxPlayRate = FMath::Clamp(FMath::IsFinite(MaxPlayRate) ? MaxPlayRate : 3.f, 0.1f, 10.f);
	// EffectiveDuration / PlayRate == BaseInterval / Speed. Do not multiply speed again elsewhere.
	// The ceiling is a last-resort presentation/hit-window guard; use a shorter high-speed montage
	// when the requested interval must remain reachable without crushing the authored motion.
	return FMath::Clamp(Duration * Speed / Interval, 0.01f, SafeMaxPlayRate);
}

float UUmbraBasicAttackAbility::CalculateEffectiveAttackPeriod(float EffectiveAuthoredDuration,
	float MontagePlayRate, float InBaseAttackInterval, float AttackSpeedMultiplier)
{
	const float Duration = FMath::IsFinite(EffectiveAuthoredDuration)
		? FMath::Max(0.001f, EffectiveAuthoredDuration) : 1.f;
	const float PlayRate = FMath::IsFinite(MontagePlayRate) ? FMath::Max(0.01f, MontagePlayRate) : 1.f;
	const float Interval = FMath::IsFinite(InBaseAttackInterval)
		? FMath::Max(0.01f, InBaseAttackInterval) : 1.f;
	const float Speed = FMath::Clamp(FMath::IsFinite(AttackSpeedMultiplier) ? AttackSpeedMultiplier : 1.f,
		UUmbraAttributeSet::MinAttackSpeedMultiplier, UUmbraAttributeSet::MaxAttackSpeedMultiplier);
	return FMath::Max(Interval / Speed, Duration / PlayRate);
}

bool UUmbraBasicAttackAbility::FindValidChainPointTime(
	const UAnimMontage* Montage, float& OutChainPointTime) const
{
	OutChainPointTime = 0.f;
	if (!Montage)
	{
		return false;
	}
	float LastHitWindowEnd = 0.f;
	for (const FAnimNotifyEvent& Notify : Montage->Notifies)
	{
		if (Notify.NotifyStateClass && Notify.NotifyStateClass->IsA<UUmbraAnimNotifyState_AttackHitWindow>())
		{
			LastHitWindowEnd = FMath::Max(LastHitWindowEnd, Notify.GetEndTriggerTime());
		}
	}
	for (const FAnimNotifyEvent& Notify : Montage->Notifies)
	{
		if (Notify.Notify && Notify.Notify->IsA<UUmbraAnimNotify_AttackChainPoint>())
		{
			const float Candidate = Notify.GetTriggerTime();
			if (Candidate > LastHitWindowEnd + KINDA_SMALL_NUMBER
				&& (!OutChainPointTime || Candidate < OutChainPointTime))
			{
				OutChainPointTime = Candidate;
			}
		}
	}
	return OutChainPointTime > 0.f;
}

bool UUmbraBasicAttackAbility::AnalyzeHitWindows(const UAnimMontage* Montage,
	float& OutFirstBegin, float& OutLastEnd, int32& OutWindowCount) const
{
	OutFirstBegin = 0.f;
	OutLastEnd = 0.f;
	OutWindowCount = 0;
	if (!Montage)
	{
		UE_LOG(LogUmbra, Error, TEXT("Basic attack has no montage while validating hit windows."));
		return false;
	}

	const float MontageLength = Montage->GetPlayLength();
	bool bValid = FMath::IsFinite(MontageLength) && MontageLength > KINDA_SMALL_NUMBER;
	OutFirstBegin = TNumericLimits<float>::Max();
	for (const FAnimNotifyEvent& Notify : Montage->Notifies)
	{
		if (!Notify.NotifyStateClass || !Notify.NotifyStateClass->IsA<UUmbraAnimNotifyState_AttackHitWindow>())
		{
			continue;
		}
		++OutWindowCount;
		const float Begin = Notify.GetTriggerTime();
		const float End = Notify.GetEndTriggerTime();
		const bool bWindowValid = FMath::IsFinite(Begin) && FMath::IsFinite(End)
			&& Begin >= 0.f && End > Begin + KINDA_SMALL_NUMBER
			&& End <= MontageLength + KINDA_SMALL_NUMBER;
		if (!bWindowValid)
		{
			UE_LOG(LogUmbra, Error,
				TEXT("Basic attack montage %s has an invalid Umbra Attack Hit Window [%.3f, %.3f] for montage length %.3f; the ability will end safely without damage."),
				*GetNameSafe(Montage), Begin, End, MontageLength);
			bValid = false;
			continue;
		}
		OutFirstBegin = FMath::Min(OutFirstBegin, Begin);
		OutLastEnd = FMath::Max(OutLastEnd, End);
	}

	if (OutWindowCount == 0)
	{
		UE_LOG(LogUmbra, Error,
			TEXT("Basic attack montage %s has no Umbra Attack Hit Window; the ability will end safely without damage."),
			*GetNameSafe(Montage));
		return false;
	}
	if (!bValid || OutFirstBegin == TNumericLimits<float>::Max() || OutLastEnd <= OutFirstBegin)
	{
		UE_LOG(LogUmbra, Error,
			TEXT("Basic attack montage %s has unusable hit-window bounds; the ability will end safely without damage."),
			*GetNameSafe(Montage));
		return false;
	}
	return true;
}

UUmbraAbilitySystemComponent* UUmbraBasicAttackAbility::GetUmbraAbilitySystemComponent() const
{
	return Cast<UUmbraAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}

bool UUmbraBasicAttackAbility::IsCurrentAttackEvent(const FGameplayEventData& Payload) const
{
	if (const UAnimMontage* EventMontage = Cast<UAnimMontage>(Payload.OptionalObject))
	{
		if (EventMontage != CurrentAttackMontage)
		{
			return false;
		}
	}
	if (Payload.EventMagnitude > 0.f)
	{
		return static_cast<uint32>(Payload.EventMagnitude) == AttackInstanceId;
	}
	return AttackInstanceId != 0;
}

UAnimMontage* UUmbraBasicAttackAbility::SelectAttackMontage(bool bHighSpeedMode)
{
	if (bHighSpeedMode)
	{
		if (HighSpeedAttackMontages.IsValidIndex(CurrentHighSpeedMontageIndex)
			&& HighSpeedAttackMontages[CurrentHighSpeedMontageIndex])
		{
			return HighSpeedAttackMontages[CurrentHighSpeedMontageIndex];
		}
		return AttackMontages.IsValidIndex(0) ? AttackMontages[0] : nullptr;
	}
	return AttackMontages.IsValidIndex(CurrentComboIndex) ? AttackMontages[CurrentComboIndex] : nullptr;
}

UAnimMontage* UUmbraBasicAttackAbility::GetFirstValidHighSpeedAttackMontage() const
{
	const int32 Index = FindNextValidHighSpeedMontageIndex(INDEX_NONE);
	return HighSpeedAttackMontages.IsValidIndex(Index) ? HighSpeedAttackMontages[Index].Get() : nullptr;
}

int32 UUmbraBasicAttackAbility::FindNextValidHighSpeedMontageIndex(int32 PreviousIndex) const
{
	const int32 Count = HighSpeedAttackMontages.Num();
	if (Count <= 0)
	{
		return INDEX_NONE;
	}
	for (int32 Offset = 1; Offset <= Count; ++Offset)
	{
		const int32 Candidate = (PreviousIndex + Offset + Count) % Count;
		if (HighSpeedAttackMontages[Candidate])
		{
			return Candidate;
		}
	}
	return INDEX_NONE;
}

bool UUmbraBasicAttackAbility::HasAnyConfiguredAttackMontage() const
{
	return AttackMontages.ContainsByPredicate([](const TObjectPtr<UAnimMontage>& Montage)
	{
		return Montage != nullptr;
	}) || GetFirstValidHighSpeedAttackMontage() != nullptr;
}

bool UUmbraBasicAttackAbility::ShouldContinueAutoAttack() const
{
	AUmbraPlayerCharacter* Character = Cast<AUmbraPlayerCharacter>(GetAvatarActorFromActorInfo());
	AActor* Target = Character ? Character->GetPrimaryAttackTarget() : nullptr;
	if (!Character || !IsValid(Target) || !Character->ShouldContinuePrimaryAttack())
	{
		return false;
	}
	if (AUmbraPlayerController* Controller = Cast<AUmbraPlayerController>(Character->GetController()))
	{
		return Controller->HandlePrimaryAttackTransition(Target);
	}
	return false;
}

bool UUmbraBasicAttackAbility::TryAdvanceAtTransition()
{
	if (bMovementCancelRequested)
	{
		return false;
	}
	const AUmbraPlayerCharacter* Character = Cast<AUmbraPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->IsTargetInPrimaryAttackRange(StrikeTarget.Get())) return false;
	if (const UUmbraAbilitySystemComponent* ASC = GetUmbraAbilitySystemComponent();
		ASC && !ASC->IsPrimaryAttackIntervalReady())
	{
		return false;
	}
	const bool bAutoContinue = ShouldContinueAutoAttack();
	if (bAutoContinue)
	{
		return StartNextComboStep(true);
	}
	if (bComboInputQueued)
	{
		return StartNextComboStep(false);
	}
	return false;
}

void UUmbraBasicAttackAbility::QueueTransitionEvaluation(bool bFromMontageEnd)
{
	if (bEndingAttack || !IsActive() || AttackInstanceId == 0)
	{
		return;
	}
	bPendingTransitionFromMontageEnd |= bFromMontageEnd;
	if (UWorld* World = GetWorld(); World && !World->GetTimerManager().IsTimerActive(AttackIntervalTimerHandle))
	{
		const FTimerDelegate Delegate = FTimerDelegate::CreateUObject(
			this, &ThisClass::EvaluatePendingTransition, AttackInstanceId);
		AttackIntervalTimerHandle = World->GetTimerManager().SetTimerForNextTick(Delegate);
	}
}

void UUmbraBasicAttackAbility::EvaluatePendingTransition(uint32 ExpectedAttackInstanceId)
{
	if (bEndingAttack || !IsActive() || ExpectedAttackInstanceId == 0
		|| ExpectedAttackInstanceId != AttackInstanceId)
	{
		return;
	}
	if (!bStrikeResolved)
	{
		ScheduleIntervalTransition(ExpectedAttackInstanceId);
		return;
	}
	if (const UUmbraAbilitySystemComponent* ASC = GetUmbraAbilitySystemComponent();
		ASC && !ASC->IsPrimaryAttackIntervalReady())
	{
		ScheduleIntervalTransition(ExpectedAttackInstanceId);
		return;
	}

	bPendingTransitionFromMontageEnd = false;
	if (TryAdvanceAtTransition())
	{
		return;
	}

	AUmbraPlayerCharacter* Character = Cast<AUmbraPlayerCharacter>(GetAvatarActorFromActorInfo());
	AUmbraPlayerController* Controller = Character
		? Cast<AUmbraPlayerController>(Character->GetController()) : nullptr;
	if ((Controller && Controller->HasQueuedPrimaryCommand())
		|| (Character && Character->ShouldContinuePrimaryAttack()))
	{
		FinishAbility(false);
		return;
	}
	FinishAbility(false);
}

void UUmbraBasicAttackAbility::ScheduleIntervalTransition(uint32 ExpectedAttackInstanceId)
{
	UUmbraAbilitySystemComponent* ASC = GetUmbraAbilitySystemComponent();
	UWorld* World = GetWorld();
	if (!ASC || !World || ExpectedAttackInstanceId != AttackInstanceId)
	{
		return;
	}
	const float Delay = FMath::Max(bAttackIntervalCommitted
		? ASC->GetPrimaryAttackIntervalRemaining() : CurrentEffectiveAttackPeriod, 0.001f);
	const FTimerDelegate Delegate = FTimerDelegate::CreateUObject(
		this, &ThisClass::EvaluatePendingTransition, ExpectedAttackInstanceId);
	World->GetTimerManager().SetTimer(AttackIntervalTimerHandle, Delegate, Delay, false);
}

void UUmbraBasicAttackAbility::ReleaseActiveMontageTask()
{
	if (!ActiveMontageTask)
	{
		return;
	}
	ActiveMontageTask->OnCompleted.RemoveDynamic(this, &ThisClass::HandleMontageCompleted);
	ActiveMontageTask->OnInterrupted.RemoveDynamic(this, &ThisClass::HandleMontageInterrupted);
	ActiveMontageTask->OnCancelled.RemoveDynamic(this, &ThisClass::HandleMontageCancelled);
	ActiveMontageTask->EndTask();
	ActiveMontageTask = nullptr;
}

float UUmbraBasicAttackAbility::ResolveVisualStrikeTime(const UAnimMontage* Montage) const
{
	if (!Montage) return 0.f;
	if (const float* Override = VisualStrikeTimeOverrides.Find(const_cast<UAnimMontage*>(Montage)))
	{
		return FMath::IsFinite(*Override) && *Override > 0.f && *Override <= Montage->GetPlayLength() ? *Override : 0.f;
	}
	float FirstBegin = TNumericLimits<float>::Max();
	for (const FAnimNotifyEvent& Notify : Montage->Notifies)
	{
		const float Begin = Notify.GetTriggerTime();
		const float End = Notify.GetEndTriggerTime();
		if (Notify.NotifyStateClass && Notify.NotifyStateClass->IsA<UUmbraAnimNotifyState_AttackHitWindow>()
			&& FMath::IsFinite(Begin) && FMath::IsFinite(End)
			&& Begin > 0.f && End > Begin && End <= Montage->GetPlayLength() + KINDA_SMALL_NUMBER)
		{
			FirstBegin = FMath::Min(FirstBegin, Begin);
		}
	}
	return FirstBegin == TNumericLimits<float>::Max() ? 0.f : FirstBegin;
}

void UUmbraBasicAttackAbility::ResolveLogicalStrike(uint32 ExpectedAttackInstanceId)
{
	UUmbraAbilitySystemComponent* ASC = GetUmbraAbilitySystemComponent();
	if (!IsActive() || bEndingAttack || bStrikeResolved || ExpectedAttackInstanceId == 0
		|| ExpectedAttackInstanceId != AttackInstanceId || !ASC
		|| ASC->GetActivePrimaryAttackInstanceId() != ExpectedAttackInstanceId) return;
	bStrikeResolved = true;
	AttackPhase = EUmbraBasicAttackPhase::Recovery;
	bAttackIntervalCommitted = ASC->CommitPrimaryAttackInterval(ExpectedAttackInstanceId);
	AUmbraPlayerCharacter* Character = Cast<AUmbraPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->HasAuthority()) return;
	AActor* Target = StrikeTarget.Get();
	const TCHAR* Result = TEXT("hit");
	if (ASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()) <= 0.f
		|| ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Dead)
		|| ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Stunned) || bMovementCancelRequested)
		Result = TEXT("attacker cancelled/dead/disabled");
	else if (!IsValid(Target) || Target->IsActorBeingDestroyed() || Target != Character->GetPrimaryAttackTarget()
		|| !Target->Implements<UUmbraAttackable>() || !IUmbraAttackable::Execute_CanBeAttacked(Target))
		Result = TEXT("original target invalid/dead");
	else if (!Character->IsTargetInPrimaryAttackRange(Target, FMath::Max(0.f, HitDistanceTolerance)))
		Result = TEXT("out of range");
	else if (bCheckAttackOcclusion)
	{
		FHitResult Blocker;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(UmbraLogicalAttackOcclusion), false, Character);
		const FVector Start = Character->GetActorLocation() + FVector(0.f, 0.f, 50.f);
		const FVector End = Target->GetActorLocation() + FVector(0.f, 0.f, 50.f);
		if (GetWorld()->LineTraceSingleByChannel(Blocker, Start, End, ECC_Visibility, Query)
			&& Blocker.GetActor() != Target)
			Result = TEXT("occluded");
	}
	if (FCString::Strcmp(Result, TEXT("hit")) == 0)
	{
		if (!UmbraPhysicalDamage::Apply(ASC, UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target),
			DamageEffectClass, DamageConfig, GetAbilityLevel(), this))
			Result = TEXT("damage effect rejected");
		else
		{
			FGameplayEventData HitReactEvent;
			HitReactEvent.EventTag = UmbraGameplayTags::Event_Combat_HitReceived;
			HitReactEvent.Instigator = Character;
			HitReactEvent.Target = Target;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, HitReactEvent.EventTag, HitReactEvent);
		}
	}
	if (CVarUmbraAttackLog.GetValueOnGameThread())
		UE_LOG(LogUmbra, Log, TEXT("Attack strike id=%u target=%s server=%.4f result=%s next=%.4f"),
			ExpectedAttackInstanceId, *GetNameSafe(Target), GetWorld()->GetTimeSeconds(), Result,
			GetWorld()->GetTimeSeconds() + ASC->GetPrimaryAttackIntervalRemaining());
}

void UUmbraBasicAttackAbility::FacePrimaryAttackTarget()
{
	AUmbraPlayerCharacter* Character = Cast<AUmbraPlayerCharacter>(GetAvatarActorFromActorInfo());
	const AActor* TargetActor = Character ? Character->GetPrimaryAttackTarget() : nullptr;
	if (!Character || !TargetActor)
	{
		return;
	}

	FVector FacingDirection = TargetActor->GetActorLocation() - Character->GetActorLocation();
	FacingDirection.Z = 0.0f;
	if (!FacingDirection.IsNearlyZero())
	{
		// This strike temporarily owns stationary facing. Locomotion gets ownership back
		// on every EndAbility path without writing another rotation.
		Character->BeginPrimaryAttackFacing(AttackInstanceId, TargetActor->GetActorLocation());
	}
}

void UUmbraBasicAttackAbility::FinishAbility(bool bWasCancelled)
{
	if (IsActive() && !bEndingAttack)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
	}
}

void UUmbraBasicAttackAbility::HandleMovementCommand(FGameplayEventData Payload)
{
	(void)Payload;
	if (bEndingAttack || !IsActive())
	{
		return;
	}
	CancelMovementAtCurrentPhase();
}

void UUmbraBasicAttackAbility::CancelMovementAtCurrentPhase()
{
	if (AUmbraPlayerCharacter* Character = Cast<AUmbraPlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		Character->StopPrimaryAttackContinuation();
	}
	bComboInputQueued = false;
	bMovementCancelRequested = true;
	bExecuteQueuedCommandOnEnd = true;

	// Both windup and recovery release movement immediately. A resolved strike keeps its interval in the ASC.
	StopMontageForMovementCancellation();
}

void UUmbraBasicAttackAbility::StopMontageForMovementCancellation()
{
	if (bEndingAttack || !IsActive())
	{
		return;
	}
	float BlendOutTime = FMath::Clamp(FMath::IsFinite(MovementCancelBlendOutTime)
		? MovementCancelBlendOutTime : 0.10f, 0.f, 1.f);
	if (CurrentAttackMontage && CurrentAttackMontage->HasRootMotion())
	{
		// Root-motion extraction during a visual blend can continue overriding locomotion.
		// Stop immediately for root-motion attacks so the queued movement is authoritative now.
		UE_LOG(LogUmbra, Warning,
			TEXT("Movement-cancelling root-motion attack montage %s with zero blend to prevent root motion from blocking movement."),
			*GetNameSafe(CurrentAttackMontage));
		BlendOutTime = 0.f;
	}
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInstance->Montage_Stop(BlendOutTime, CurrentAttackMontage);
		}
	}
	FinishAbility(true);
}

void UUmbraBasicAttackAbility::EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bEndingAttack || !IsEndAbilityValid(Handle, ActorInfo))
	{
		return;
	}
	if (ScopeLockCount > 0)
	{
		WaitingToExecute.Add(FPostLockDelegate::CreateUObject(this, &ThisClass::EndAbility,
			Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled));
		return;
	}
	TGuardValue<bool> EndingGuard(bEndingAttack, true);
	const bool bExecuteQueuedCommand = bExecuteQueuedCommandOnEnd;
	const uint32 EndingAttackInstanceId = AttackInstanceId;
	AUmbraPlayerCharacter* Character = Cast<AUmbraPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (Character && Character->HasAuthority() && CVarUmbraAttackLog.GetValueOnGameThread())
	{
		UE_LOG(LogUmbra, Log, TEXT("Attack end id=%u server=%.4f cancelled=%d resolved=%d intervalCommitted=%d"),
			EndingAttackInstanceId, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0,
			bWasCancelled, bStrikeResolved, bAttackIntervalCommitted);
	}
	if (bWasCancelled && !bMovementCancelRequested && CurrentAttackMontage)
	{
		if (const ACharacter* AvatarCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (UAnimInstance* Anim = AvatarCharacter->GetMesh() ? AvatarCharacter->GetMesh()->GetAnimInstance() : nullptr)
			{
				Anim->Montage_Stop(0.f, CurrentAttackMontage);
			}
		}
	}
	AUmbraPlayerController* Controller = nullptr;
	if (Character)
	{
		Controller = Cast<AUmbraPlayerController>(Character->GetController());
	}
	CurrentComboIndex = INDEX_NONE;
	CurrentHighSpeedMontageIndex = INDEX_NONE;
	CapturedAttackSpeed = 1.f;
	bComboInputQueued = false;
	HitActorsThisComboStep.Reset();
	bHasPreviousHitSocketLocation = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackIntervalTimerHandle);
		World->GetTimerManager().ClearTimer(AttackWindupTimerHandle);
	}
	if (UUmbraAbilitySystemComponent* ASC = GetUmbraAbilitySystemComponent())
	{
		ASC->EndPrimaryAttackInstance(AttackInstanceId);
	}
	AttackInstanceId = 0;
	if (ActiveMontageTask)
	{
		// Normal completion leaves visual recovery playing; a new strike can replace it.
		ActiveMontageTask->OnCompleted.RemoveDynamic(this, &ThisClass::HandleMontageCompleted);
		ActiveMontageTask->OnInterrupted.RemoveDynamic(this, &ThisClass::HandleMontageInterrupted);
		ActiveMontageTask->OnCancelled.RemoveDynamic(this, &ThisClass::HandleMontageCancelled);
		ActiveMontageTask = nullptr;
	}
	ComboInputTask = nullptr;
	AttackHitWindowTask = nullptr;
	AttackChainPointTask = nullptr;
	MovementCommandTask = nullptr;
	ComboWindowTask = nullptr;
	DeathInterruptTask = nullptr;
	StunInterruptTask = nullptr;
	CurrentAttackMontage = nullptr;
	StrikeTarget.Reset();
	bStrikeResolved = false;
	CurrentWindupSeconds = 0.f;
	CurrentMontagePlayRate = 1.f;
	CurrentEffectiveAttackDuration = 0.f;
	CurrentEffectiveAttackPeriod = 0.f;
	CurrentFirstHitWindowTime = 0.f;
	CurrentLastHitWindowTime = 0.f;
	bCurrentStepHighSpeed = false;
	bPreviousStepHighSpeed = false;
	bCurrentMontageHasChainPoint = false;
	bHitWindowActive = false;
	bHitWindowConfigurationValid = false;
	bAttackIntervalCommitted = false;
	bMovementCancelRequested = false;
	bExecuteQueuedCommandOnEnd = false;
	bPendingTransitionFromMontageEnd = false;
	ExpectedHitWindowCount = 0;
	EndedHitWindowCount = 0;
	OpenHitWindowCount = 0;
	AttackPhase = EUmbraBasicAttackPhase::Invalid;

	if (Character)
	{
		Character->EndPrimaryAttackFacing(EndingAttackInstanceId);
		Character->ClearPrimaryAttackTarget();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	if (Controller)
	{
		Controller->NotifyPrimaryAttackAbilityEnded(bWasCancelled, bExecuteQueuedCommand);
	}
}

void UUmbraBasicAttackAbility::HandleMontageCompleted()
{
	if (bEndingAttack || !IsActive()) return;
	ActiveMontageTask = nullptr;
	if (bMovementCancelRequested)
	{
		// Missing Notify End must not strand an active-window movement command.
		StopMontageForMovementCancellation();
		return;
	}
	AttackPhase = bHitWindowConfigurationValid
		? EUmbraBasicAttackPhase::Recovery : EUmbraBasicAttackPhase::Invalid;
	QueueTransitionEvaluation(true);
}

void UUmbraBasicAttackAbility::HandleMontageInterrupted()
{
	FinishAbility(true);
}

void UUmbraBasicAttackAbility::HandleMontageCancelled()
{
	FinishAbility(true);
}

void UUmbraBasicAttackAbility::HandleComboInput(FGameplayEventData Payload)
{
	(void)Payload;
	if (!bEndingAttack && IsActive())
	{
		// A newer attack command supersedes a movement command that was still waiting
		// for the final active window to close.
		bMovementCancelRequested = false;
		bExecuteQueuedCommandOnEnd = false;
	}
	const bool bHasManualNextStep = bCurrentStepHighSpeed
		|| AttackMontages.IsValidIndex(CurrentComboIndex + 1);
	if (!bEndingAttack && IsActive() && bHasManualNextStep)
	{
		if (!ActiveMontageTask && ComboWindowTask)
		{
			if (!StartNextComboStep(false))
			{
				FinishAbility(true);
			}
			return;
		}

		bComboInputQueued = true;
	}
}

void UUmbraBasicAttackAbility::HandleComboWindowExpired()
{
	ComboWindowTask = nullptr;
	FinishAbility(false);
}

void UUmbraBasicAttackAbility::HandleAttackChainPoint(FGameplayEventData Payload)
{
	if (bEndingAttack || !IsActive() || !IsCurrentAttackEvent(Payload)
		|| !bCurrentMontageHasChainPoint || AttackPhase == EUmbraBasicAttackPhase::Active)
	{
		return;
	}
	QueueTransitionEvaluation(false);
}

void UUmbraBasicAttackAbility::HandleForcedInterrupt()
{
	// Death and hard crowd control bypass authored recovery/transition points.
	// They also discard a movement command that may have been buffered during Active.
	bMovementCancelRequested = false;
	bExecuteQueuedCommandOnEnd = false;
	FinishAbility(true);
}

void UUmbraBasicAttackAbility::HandleAttackHitWindow(FGameplayEventData Payload)
{
	// Player basic attacks never consume legacy sweep events. Other abilities still can.
	(void)Payload;
}
