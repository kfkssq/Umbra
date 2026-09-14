// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
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
	void ServerReceivePrimaryAttackIntent(AActor* Target, bool bCombo);

private:
	bool bActorInfoReady = false;
	bool bAttributesInitialized = false;

	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
};
