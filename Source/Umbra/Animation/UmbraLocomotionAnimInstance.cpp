#include "Animation/UmbraLocomotionAnimInstance.h"
#include "GameFramework/Pawn.h"

void UUmbraLocomotionAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    const APawn* Pawn = TryGetPawnOwner();
    GroundSpeed = Pawn ? Pawn->GetVelocity().Size2D() : 0.f;
    bMoving = GroundSpeed > 5.f;
}
