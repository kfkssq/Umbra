#pragma once

#include "CoreMinimal.h"
#include "UmbraDropPhysicsSettings.generated.h"

/** BP_WorldItem defaults; geometry is authored separately on ItemDefinition, in cm. */
USTRUCT(BlueprintType)
struct FUmbraDropPhysicsSettings
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="2000", Units="cm/s")) float UpwardSpeed = 260.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="1000", Units="cm/s")) float HorizontalSpeed = 80.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="1440")) float AngularSpeedDegrees = 180.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="20")) float LinearDamping = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="20")) float AngularDamping = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="1")) float Friction = 0.7f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="1")) float Restitution = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="10", Units="s")) float MinimumSimulationTime = 0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.05", ClampMax="2", Units="s")) float StableTime = 0.3f;
	/** Serialized name retained for existing BPs. This is a soft settling delay, never a teleport deadline. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(DisplayName="Settling Start Time", ClampMin="0.1", ClampMax="15", Units="s")) float MaximumSimulationTime = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1", ClampMax="5", Units="s")) float SettlingBlendTime = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="20")) float SettlingLinearDamping = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="20")) float SettlingAngularDamping = 8.f;
	/** Emergency recovery only: continuously unsupported, not total simulation lifetime. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="10", ClampMax="120", Units="s")) float UnsupportedRecoveryTime = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="500", ClampMax="10000", Units="cm")) float RecoveryFallDistance = 2000.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="100", Units="cm/s")) float LinearSleepThreshold = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="360")) float AngularSleepThresholdDegrees = 10.f;
	/** WorldDynamic (including other loot) and Pawn always ignored. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bCollideWithPhysicsBodies = true;
	void Sanitize();
};

/** One replicated snapshot keeps the final pose and settled flag together, including late relevance. */
USTRUCT()
struct FUmbraDropMotion
{
	GENERATED_BODY()
	UPROPERTY() FVector Location = FVector::ZeroVector;
	UPROPERTY() FQuat Rotation = FQuat::Identity;
	UPROPERTY() bool bStarted = false;
	UPROPERTY() bool bSettled = false;
};
