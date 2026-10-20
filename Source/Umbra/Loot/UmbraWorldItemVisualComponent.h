#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UmbraWorldItemVisualComponent.generated.h"

class UCurveFloat;
class UMeshComponent;


/** Local presentation only. Never moves the actor or its pickup collision. */
UCLASS(ClassGroup = (Umbra), meta = (BlueprintSpawnableComponent))
class UMBRA_API UUmbraWorldItemVisualComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UUmbraWorldItemVisualComponent();
	void InitializeVisual(UMeshComponent* InMesh, const FGuid& InstanceId,
		const FTransform& InBaseTransform, FLinearColor InRarityColor, double SpawnTime, bool bWindowClosed);
	void ClearVisual();
	void SetHighlighted(bool bValue);
	float GetAnimationDuration() const;
	/** Data hook for future art integration; does not transmit RGB through CustomStencil. */
	UFUNCTION(BlueprintPure, Category = "World Item|Highlight") FLinearColor GetRarityHighlightColor() const { return RarityHighlightColor; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Item|Drop Animation") bool bEnableDropAnimation = true;
	/** Phase durations in seconds. Zero skips that phase. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Item|Drop Animation", meta = (ClampMin = "0", Units = "s")) float RiseDuration = 0.18f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Item|Drop Animation", meta = (ClampMin = "0", Units = "s")) float FallDuration = 0.40f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Item|Drop Animation", meta = (ClampMin = "0", Units = "s")) float BounceDuration = 0.22f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Item|Drop Animation", meta = (ClampMin = "0", Units = "cm")) float RiseHeight = 70.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Item|Drop Animation", meta = (ClampMin = "0", Units = "cm")) float BounceHeight = 8.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Item|Drop Animation", meta = (ClampMin = "0")) float InitialScaleMultiplier = 0.65f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Item|Drop Animation", meta = (ClampMin = "0")) float PeakScaleMultiplier = 1.1f;
	/** Temporary Euler rotation at the apex, in degrees; returns to zero at touchdown. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Item|Drop Animation") FRotator AdditionalAirRotation = FRotator(0, 180, 45);
	/** Optional phase easing: X and Y in [0,1], endpoints 0 and 1. Applied to height and scale. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Item|Drop Animation") TObjectPtr<UCurveFloat> AnimationCurve;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Item|Highlight") bool bEnableHoverOutline = true;
	/** 0/1 reserved (background/enemy). Material must explicitly support the selected ID. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Item|Highlight", meta = (ClampMin = "2", ClampMax = "255")) int32 CustomStencilValue = 2;
protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	void ApplyAnimation(float Age);
	void FinishAnimation();
	float Ease(float Alpha) const;
	TWeakObjectPtr<UMeshComponent> DropMesh;
	TWeakObjectPtr<UObject> AnimatedMesh;
	FGuid ConsumedInstanceId;
	FTransform BaseTransform;
	FLinearColor RarityHighlightColor = FLinearColor::White;
	double AnimationSpawnTime = -1;
	float LastAge = 0;
	bool bHighlighted = false;
};
