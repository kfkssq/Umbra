// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/UmbraAbilitySystemComponent.h"

#include "AbilitySystem/UmbraGameplayAbility.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "AbilitySystem/UmbraDebugInitialAttributes.h"
#include "Characters/UmbraPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"
#include "Umbra.h"
#include "UmbraPlayerController.h"

static TAutoConsoleVariable<float> CVarUmbraAttackMeasureSeconds(
	TEXT("umbra.Attack.MeasureSeconds"), 0.f,
	TEXT("Server: set to N>0 to measure settled basic-attack GE damage for N seconds from the next attack start; one shot."));

void UUmbraAbilitySystemComponent::ServerReceivePrimaryAttackIntent_Implementation(
	AActor* Target, bool bCombo, bool bContinueAttacking)
{
	if (AUmbraPlayerCharacter* Character = Cast<AUmbraPlayerCharacter>(GetAvatarActor()))
	{
		Character->ReceivePrimaryAttackIntent(Target, bCombo, bContinueAttacking);
	}
}

void UUmbraAbilitySystemComponent::ServerCancelPrimaryAttackContinuation_Implementation()
{
	if (AUmbraPlayerCharacter* Character = Cast<AUmbraPlayerCharacter>(GetAvatarActor()))
	{
		Character->StopPrimaryAttackContinuation();
	}
}

void UUmbraAbilitySystemComponent::RequestPrimaryAttackMovementCancellation()
{
	if (AActor* Avatar = GetAvatarActor())
	{
		FGameplayEventData Event;
		Event.EventTag = UmbraGameplayTags::Event_Attack_MovementCommand;
		Event.Instigator = Avatar;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Avatar, Event.EventTag, Event);
	}
	if (!IsOwnerActorAuthoritative())
	{
		ServerRequestPrimaryAttackMovementCancellation();
	}
}

void UUmbraAbilitySystemComponent::ServerRequestPrimaryAttackMovementCancellation_Implementation()
{
	if (AUmbraPlayerCharacter* Character = Cast<AUmbraPlayerCharacter>(GetAvatarActor()))
	{
		Character->StopPrimaryAttackContinuation();
	}
	if (AActor* Avatar = GetAvatarActor())
	{
		FGameplayEventData Event;
		Event.EventTag = UmbraGameplayTags::Event_Attack_MovementCommand;
		Event.Instigator = Avatar;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Avatar, Event.EventTag, Event);
	}
}

uint32 UUmbraAbilitySystemComponent::BeginPrimaryAttackInstance(float EffectivePeriodSeconds)
{
	++PrimaryAttackInstanceSerial;
	if (PrimaryAttackInstanceSerial == 0)
	{
		++PrimaryAttackInstanceSerial;
	}
	ActivePrimaryAttackInstanceId = PrimaryAttackInstanceSerial;
	ActivePrimaryAttackStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	ActivePrimaryAttackPeriod = FMath::IsFinite(EffectivePeriodSeconds)
		? FMath::Max(0.f, EffectivePeriodSeconds) : 0.f;
	if (IsOwnerActorAuthoritative())
	{
		if (bPrimaryAttackDamageMeasurementActive
			&& ActivePrimaryAttackStartTime >= PrimaryAttackDamageMeasurementEnd)
		{
			FinishPrimaryAttackDamageMeasurement();
		}
		if (!bPrimaryAttackDamageMeasurementActive)
		{
			const float RequestedSeconds = CVarUmbraAttackMeasureSeconds.GetValueOnGameThread();
			if (FMath::IsFinite(RequestedSeconds) && RequestedSeconds > 0.f)
			{
				StartPrimaryAttackDamageMeasurement(FMath::Min(RequestedSeconds, 3600.f));
				CVarUmbraAttackMeasureSeconds->Set(0.f, ECVF_SetByConsole);
			}
		}
		if (bPrimaryAttackDamageMeasurementActive
			&& ActivePrimaryAttackStartTime < PrimaryAttackDamageMeasurementEnd)
		{
			++PrimaryAttackDamageMeasurementStarts;
		}
	}
	return ActivePrimaryAttackInstanceId;
}

void UUmbraAbilitySystemComponent::StartPrimaryAttackDamageMeasurement(float DurationSeconds)
{
	UWorld* World = GetWorld();
	if (!World || !IsOwnerActorAuthoritative() || !FMath::IsFinite(DurationSeconds) || DurationSeconds <= 0.f)
	{
		return;
	}
	PrimaryAttackDamageMeasurementStart = World->GetTimeSeconds();
	PrimaryAttackDamageMeasurementDuration = DurationSeconds;
	PrimaryAttackDamageMeasurementEnd = PrimaryAttackDamageMeasurementStart + DurationSeconds;
	PrimaryAttackDamageMeasurementAvatar = GetNameSafe(GetAvatarActor());
	PrimaryAttackDamageMeasurementTotal = 0.0;
	PrimaryAttackDamageMeasurementStarts = 0;
	PrimaryAttackDamageMeasurementHits = 0;
	bPrimaryAttackDamageMeasurementActive = true;
	World->GetTimerManager().SetTimer(PrimaryAttackDamageMeasurementTimer,
		FTimerDelegate::CreateUObject(this, &ThisClass::FinishPrimaryAttackDamageMeasurement, false),
		DurationSeconds, false);
	UE_LOG(LogUmbra, Log, TEXT("[AttackMeasure] Started avatar=%s serverStart=%.4f duration=%.3f end=%.4f"),
		*PrimaryAttackDamageMeasurementAvatar, PrimaryAttackDamageMeasurementStart,
		DurationSeconds, PrimaryAttackDamageMeasurementEnd);
}

void UUmbraAbilitySystemComponent::RecordPrimaryAttackSettledDamage(float SettledDamage, const AActor* Target)
{
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (!IsOwnerActorAuthoritative() || !bPrimaryAttackDamageMeasurementActive
		|| Now < PrimaryAttackDamageMeasurementStart || Now >= PrimaryAttackDamageMeasurementEnd
		|| !FMath::IsFinite(SettledDamage))
	{
		return;
	}
	UE_LOG(LogUmbra, Log, TEXT("[AttackMeasure] Settled source=%s target=%s server=%.4f settledDamage=%.3f"),
		*PrimaryAttackDamageMeasurementAvatar, *GetNameSafe(Target), Now, SettledDamage);
	if (SettledDamage <= 0.f) return;
	PrimaryAttackDamageMeasurementTotal += SettledDamage;
	++PrimaryAttackDamageMeasurementHits;
}

void UUmbraAbilitySystemComponent::FinishPrimaryAttackDamageMeasurement(bool bAborted)
{
	if (!bPrimaryAttackDamageMeasurementActive) return;
	UWorld* World = GetWorld();
	const double ReportTime = World ? World->GetTimeSeconds() : PrimaryAttackDamageMeasurementEnd;
	if (World) World->GetTimerManager().ClearTimer(PrimaryAttackDamageMeasurementTimer);
	const double CountedSeconds = bAborted
		? FMath::Clamp(ReportTime - PrimaryAttackDamageMeasurementStart, 0.0, double(PrimaryAttackDamageMeasurementDuration))
		: double(PrimaryAttackDamageMeasurementDuration);
	UE_LOG(LogUmbra, Log,
		TEXT("[AttackMeasure] %s avatar=%s start=%.4f windowEnd=%.4f reported=%.4f seconds=%.3f starts=%d damagingHits=%d settledDamage=%.3f DPS=%.3f"),
		bAborted ? TEXT("Aborted") : TEXT("Complete"), *PrimaryAttackDamageMeasurementAvatar,
		PrimaryAttackDamageMeasurementStart, PrimaryAttackDamageMeasurementEnd, ReportTime, CountedSeconds,
		PrimaryAttackDamageMeasurementStarts, PrimaryAttackDamageMeasurementHits,
		PrimaryAttackDamageMeasurementTotal,
		CountedSeconds > 0.0 ? PrimaryAttackDamageMeasurementTotal / CountedSeconds : 0.0);
	AUmbraPlayerController* Recipient = nullptr;
	if (const APawn* AvatarPawn = Cast<APawn>(GetAvatarActor()))
	{
		Recipient = Cast<AUmbraPlayerController>(AvatarPawn->GetController());
	}
	if (!Recipient)
	{
		if (const APlayerState* OwnerPlayerState = Cast<APlayerState>(GetOwnerActor()))
		{
			Recipient = Cast<AUmbraPlayerController>(OwnerPlayerState->GetPlayerController());
		}
	}
	if (Recipient)
	{
		Recipient->ClientShowAttackDamageMeasurement(float(CountedSeconds),
			PrimaryAttackDamageMeasurementStarts, PrimaryAttackDamageMeasurementHits,
			float(PrimaryAttackDamageMeasurementTotal),
			CountedSeconds > 0.0 ? float(PrimaryAttackDamageMeasurementTotal / CountedSeconds) : 0.f,
			bAborted);
	}
	bPrimaryAttackDamageMeasurementActive = false;
	PrimaryAttackDamageMeasurementAvatar.Reset();
	PrimaryAttackDamageMeasurementTotal = 0.0;
	PrimaryAttackDamageMeasurementStarts = 0;
	PrimaryAttackDamageMeasurementHits = 0;
}

bool UUmbraAbilitySystemComponent::CommitPrimaryAttackInterval(uint32 AttackInstanceId)
{
	if (AttackInstanceId == 0 || AttackInstanceId != ActivePrimaryAttackInstanceId)
	{
		return false;
	}
	NextPrimaryAttackAllowedTime = FMath::Max(NextPrimaryAttackAllowedTime,
		ActivePrimaryAttackStartTime + ActivePrimaryAttackPeriod);
	return true;
}

void UUmbraAbilitySystemComponent::EndPrimaryAttackInstance(uint32 AttackInstanceId)
{
	if (AttackInstanceId != 0 && AttackInstanceId == ActivePrimaryAttackInstanceId)
	{
		ActivePrimaryAttackInstanceId = 0;
		ActivePrimaryAttackStartTime = 0.0;
		ActivePrimaryAttackPeriod = 0.f;
	}
}

bool UUmbraAbilitySystemComponent::IsPrimaryAttackIntervalReady() const
{
	return GetPrimaryAttackIntervalRemaining() <= KINDA_SMALL_NUMBER;
}

float UUmbraAbilitySystemComponent::GetPrimaryAttackIntervalRemaining() const
{
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	return static_cast<float>(FMath::Max(0.0, NextPrimaryAttackAllowedTime - Now));
}

FUmbraAbilitySystemLifecycle UUmbraAbilitySystemComponent::OnLifecycleChanged;

void UUmbraAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	if (AActor* PreviousAvatar = GetAvatarActor(); PreviousAvatar && PreviousAvatar != InAvatarActor)
	{
		FinishPrimaryAttackDamageMeasurement(true);
		// Instanced tasks and cached attack targets are scoped to the old avatar.
		CancelAllAbilities();
		ClearAbilityInput();
		ActivePrimaryAttackInstanceId = 0;
		ActivePrimaryAttackStartTime = 0.0;
		ActivePrimaryAttackPeriod = 0.f;
		NextPrimaryAttackAllowedTime = 0.0;
	}
	UnbindMoveSpeed();
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);
	bActorInfoReady = IsValid(InOwnerActor) && IsValid(InAvatarActor);
	if (bActorInfoReady && GetSet<UUmbraAttributeSet>())
	{
		// One-time migration fallback: preserve existing BP movement defaults unless
		// an initial GE or debug override supplies MoveSpeed. Rebinding never reseeds it.
		if (!bAttributesInitialized && IsOwnerActorAuthoritative())
		{
			if (const ACharacter* Character = Cast<ACharacter>(InAvatarActor))
			{
				SetNumericAttributeBase(UUmbraAttributeSet::GetMoveSpeedAttribute(), Character->GetCharacterMovement()->MaxWalkSpeed);
			}
		}
		MoveSpeedHandle = GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetMoveSpeedAttribute())
			.AddUObject(this, &ThisClass::HandleMoveSpeedChanged);
		ApplyMoveSpeed();
	}
	OnLifecycleChanged.Broadcast(this, bActorInfoReady);
}

void UUmbraAbilitySystemComponent::UnbindMoveSpeed()
{
	GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetMoveSpeedAttribute()).Remove(MoveSpeedHandle);
	MoveSpeedHandle.Reset();
}

void UUmbraAbilitySystemComponent::ApplyMoveSpeed()
{
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActor()))
	{
		const float Speed = GetNumericAttribute(UUmbraAttributeSet::GetMoveSpeedAttribute());
		const float SafeSpeed = FMath::IsFinite(Speed) ? FMath::Max(0.f, Speed) : 0.f;
		Character->GetCharacterMovement()->MaxWalkSpeed = SafeSpeed;
		if (AUmbraPlayerCharacter* PlayerCharacter = Cast<AUmbraPlayerCharacter>(Character))
		{
			// Keep movement-facing responsiveness proportional to GAS MoveSpeed without Tick polling.
			PlayerCharacter->ApplyMoveSpeedDrivenYawRate(SafeSpeed);
		}
	}
}

void UUmbraAbilitySystemComponent::HandleMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	ApplyMoveSpeed();
}

void UUmbraAbilitySystemComponent::ClearActorInfo()
{
	FinishPrimaryAttackDamageMeasurement(true);
	CancelAllAbilities();
	ClearAbilityInput();
	UnbindMoveSpeed();
	bActorInfoReady = false;
	ActivePrimaryAttackInstanceId = 0;
	ActivePrimaryAttackStartTime = 0.0;
	ActivePrimaryAttackPeriod = 0.f;
	NextPrimaryAttackAllowedTime = 0.0;
	Super::ClearActorInfo();
	OnLifecycleChanged.Broadcast(this, false);
}

void UUmbraAbilitySystemComponent::OnUnregister()
{
	FinishPrimaryAttackDamageMeasurement(true);
	UnbindMoveSpeed();
	bActorInfoReady = false;
	OnLifecycleChanged.Broadcast(this, false);
	Super::OnUnregister();
}

void UUmbraAbilitySystemComponent::InitializeAttributes(TSubclassOf<UGameplayEffect> InitialEffect, const FUmbraDebugInitialAttributes* DebugAttributes)
{
	if (bAttributesInitialized || !IsOwnerActorAuthoritative() || !GetSet<UUmbraAttributeSet>())
	{
		return;
	}
	if (InitialEffect && InitialEffect.GetDefaultObject()->DurationPolicy != EGameplayEffectDurationType::Instant)
	{
		UE_LOG(LogTemp, Error, TEXT("Umbra initial attributes must use an Instant Gameplay Effect: %s"), *GetNameSafe(InitialEffect));
		return;
	}
	bAttributesInitialized = true;
	if (InitialEffect)
	{
		const FGameplayEffectSpecHandle Spec = MakeOutgoingSpec(InitialEffect, 1.f, MakeEffectContext());
		if (Spec.IsValid())
		{
			ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
#if !UE_BUILD_SHIPPING
	if (DebugAttributes)
	{
		UGameplayEffect* DebugEffect = NewObject<UGameplayEffect>(GetTransientPackage());
		DebugEffect->DurationPolicy = EGameplayEffectDurationType::Instant;
		const auto AddOverride = [DebugEffect](const FGameplayAttribute& Attribute, float Value)
		{
			FGameplayModifierInfo& Modifier = DebugEffect->Modifiers.AddDefaulted_GetRef();
			Modifier.Attribute = Attribute;
			Modifier.ModifierOp = EGameplayModOp::Override;
			Modifier.ModifierMagnitude = FScalableFloat(Value);
		};
		AddOverride(UUmbraAttributeSet::GetMaxHealthAttribute(), DebugAttributes->MaxHealth);
		AddOverride(UUmbraAttributeSet::GetMaxResourceAttribute(), DebugAttributes->MaxResource);
		AddOverride(UUmbraAttributeSet::GetHealthRegenAttribute(), DebugAttributes->HealthRegen);
		AddOverride(UUmbraAttributeSet::GetResourceRegenAttribute(), DebugAttributes->ResourceRegen);
		AddOverride(UUmbraAttributeSet::GetAttackPowerAttribute(), DebugAttributes->AttackPower);
		AddOverride(UUmbraAttributeSet::GetAbilityPowerAttribute(), DebugAttributes->AbilityPower);
		AddOverride(UUmbraAttributeSet::GetAttackSpeedAttribute(), DebugAttributes->AttackSpeed);
		AddOverride(UUmbraAttributeSet::GetCriticalChanceAttribute(), DebugAttributes->CriticalChance);
		AddOverride(UUmbraAttributeSet::GetCriticalDamageMultiplierAttribute(), DebugAttributes->CriticalDamageMultiplier);
		AddOverride(UUmbraAttributeSet::GetArmorAttribute(), DebugAttributes->Armor);
		AddOverride(UUmbraAttributeSet::GetMagicResistanceAttribute(), DebugAttributes->MagicResistance);
		AddOverride(UUmbraAttributeSet::GetAbilityHasteAttribute(), DebugAttributes->AbilityHaste);
		AddOverride(UUmbraAttributeSet::GetMoveSpeedAttribute(), DebugAttributes->MoveSpeed);
		ApplyGameplayEffectToSelf(DebugEffect, 1.f, MakeEffectContext());
	}
#endif
	// Fill separately, after all initial maxima have been evaluated.
	const UUmbraAttributeSet* Attributes = GetSet<UUmbraAttributeSet>();
	SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(), Attributes->GetMaxHealth());
	SetNumericAttributeBase(UUmbraAttributeSet::GetResourceAttribute(), Attributes->GetMaxResource());
}

UUmbraAbilitySystemComponent::UUmbraAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
	SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

void UUmbraAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		const UUmbraGameplayAbility* UmbraAbility = Cast<UUmbraGameplayAbility>(AbilitySpec.Ability);
		if (UmbraAbility && UmbraAbility->GetInputTag().MatchesTagExact(InputTag))
		{
			InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
			InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
		}
	}
}

void UUmbraAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		const UUmbraGameplayAbility* UmbraAbility = Cast<UUmbraGameplayAbility>(AbilitySpec.Ability);
		if (UmbraAbility && UmbraAbility->GetInputTag().MatchesTagExact(InputTag))
		{
			InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
			InputHeldSpecHandles.Remove(AbilitySpec.Handle);
		}
	}
}

void UUmbraAbilitySystemComponent::ProcessAbilityInput(float DeltaTime)
{
	TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;

	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);
		const UUmbraGameplayAbility* UmbraAbility = AbilitySpec ? Cast<UUmbraGameplayAbility>(AbilitySpec->Ability) : nullptr;
		if (UmbraAbility
			&& !AbilitySpec->IsActive()
			&& UmbraAbility->GetActivationPolicy() == EUmbraAbilityActivationPolicy::WhileInputActive)
		{
			AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
		}
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);
		if (!AbilitySpec)
		{
			continue;
		}

		AbilitySpec->InputPressed = true;
		if (AbilitySpec->IsActive())
		{
			AbilitySpecInputPressed(*AbilitySpec);
		}
		else if (const UUmbraGameplayAbility* UmbraAbility = Cast<UUmbraGameplayAbility>(AbilitySpec->Ability))
		{
			if (UmbraAbility->GetActivationPolicy() == EUmbraAbilityActivationPolicy::OnInputTriggered)
			{
				AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
			}
		}
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(SpecHandle);
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);
		if (!AbilitySpec)
		{
			continue;
		}

		AbilitySpec->InputPressed = false;
		if (AbilitySpec->IsActive())
		{
			AbilitySpecInputReleased(*AbilitySpec);
		}
	}

	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UUmbraAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}
