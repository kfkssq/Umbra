// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "UmbraAbilitySystemComponent.generated.h"

class UGameplayEffect;
struct FUmbraDebugInitialAttributes;
class UUmbraAbilitySystemComponent;
DECLARE_MULTICAST_DELEGATE_TwoParams(FUmbraAbilitySystemLifecycle, UUmbraAbilitySystemComponent*, bool);

/** Ability system component shared by Umbra player ability owners. */
UCLASS()
class UMBRA_API UUmbraAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UUmbraAbilitySystemComponent();

	/** Native lifecycle notification for observers; never initializes attributes. */
	static FUmbraAbilitySystemLifecycle OnLifecycleChanged;
	bool IsActorInfoReady() const { return bActorInfoReady; }
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;
	virtual void ClearActorInfo() override;
	virtual void OnUnregister() override;

	/** Authority-only, once per ASC lifetime; call after InitAbilityActorInfo. */
	void InitializeAttributes(TSubclassOf<UGameplayEffect> InitialEffect, const FUmbraDebugInitialAttributes* DebugAttributes = nullptr);

	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);
	void ProcessAbilityInput(float DeltaTime);
	void ClearAbilityInput();

	/** Shares the owning player's target/queued combo before GAS activation on the same ASC channel. */
	UFUNCTION(Server, Reliable)
	void ServerReceivePrimaryAttackIntent(AActor* Target, bool bCombo, bool bContinueAttacking);

	/** Stops server-side automatic continuation without cancelling the current strike. */
	UFUNCTION(Server, Reliable)
	void ServerCancelPrimaryAttackContinuation();

	/** Sends the same phase-aware movement-cancel request to predicted and authoritative abilities. */
	void RequestPrimaryAttackMovementCancellation();

	/** Server validates the current attack phase before cancelling or deferring movement. */
	UFUNCTION(Server, Reliable)
	void ServerRequestPrimaryAttackMovementCancellation();

	/** Starts one strike's transient identity without consuming its interval until the first hit window. */
	uint32 BeginPrimaryAttackInstance(float EffectivePeriodSeconds);
	/** Commits the strike-start-based interval when the first hit window proves that the strike was issued. */
	bool CommitPrimaryAttackInterval(uint32 AttackInstanceId);
	/** Invalidates callbacks/notifies for one ended strike while preserving any committed interval. */
	void EndPrimaryAttackInstance(uint32 AttackInstanceId);
	uint32 GetActivePrimaryAttackInstanceId() const { return ActivePrimaryAttackInstanceId; }
	bool IsPrimaryAttackIntervalReady() const;
	float GetPrimaryAttackIntervalRemaining() const;
	/** Records this ASC's basic-attack GE damage at authoritative settlement, capped by pre-hit Health. */
	void RecordPrimaryAttackSettledDamage(float SettledDamage, const AActor* Target);
	bool IsPrimaryAttackDamageMeasurementActive() const { return bPrimaryAttackDamageMeasurementActive; }

private:
	friend class FUmbraCombatMaintenanceTest;
	void UnbindMoveSpeed();
	void ApplyMoveSpeed();
	void HandleMoveSpeedChanged(const FOnAttributeChangeData& Data);
	void StartPrimaryAttackDamageMeasurement(float DurationSeconds);
	void FinishPrimaryAttackDamageMeasurement(bool bAborted = false);
	FDelegateHandle MoveSpeedHandle;

	bool bActorInfoReady = false;
	bool bAttributesInitialized = false;
	uint32 PrimaryAttackInstanceSerial = 0;
	uint32 ActivePrimaryAttackInstanceId = 0;
	double ActivePrimaryAttackStartTime = 0.0;
	float ActivePrimaryAttackPeriod = 0.f;
	double NextPrimaryAttackAllowedTime = 0.0;
	FTimerHandle PrimaryAttackDamageMeasurementTimer;
	FString PrimaryAttackDamageMeasurementAvatar;
	double PrimaryAttackDamageMeasurementStart = 0.0;
	double PrimaryAttackDamageMeasurementEnd = 0.0;
	double PrimaryAttackDamageMeasurementTotal = 0.0;
	float PrimaryAttackDamageMeasurementDuration = 0.f;
	int32 PrimaryAttackDamageMeasurementStarts = 0;
	int32 PrimaryAttackDamageMeasurementHits = 0;
	bool bPrimaryAttackDamageMeasurementActive = false;

	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
};
