#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "UmbraLocomotionAnimInstance.generated.h"

/** Minimal native locomotion inputs; animation assets and pose blending remain in the Anim Blueprint. */
UCLASS(Blueprintable)
class UMBRA_API UUmbraLocomotionAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    UPROPERTY(BlueprintReadOnly, Category="Locomotion") float GroundSpeed = 0.f;
    UPROPERTY(BlueprintReadOnly, Category="Locomotion") bool bMoving = false;
};
