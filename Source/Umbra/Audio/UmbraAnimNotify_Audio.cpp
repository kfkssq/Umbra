#include "Audio/UmbraAnimNotify_Audio.h"
#include "Audio/UmbraAudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Engine/World.h"

void UUmbraAnimNotify_Audio::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	auto* Audio = UUmbraAudioComponent::Find(Owner);
	if (!Audio || !Owner->GetWorld() || !Owner->GetWorld()->IsGameWorld()
		|| Owner->GetNetMode() == NM_DedicatedServer) return;
	// Cosmetic meshes and menu preview meshes must not duplicate the gameplay mesh's notifies.
	if (const ACharacter* Character = Cast<ACharacter>(Owner))
		if (Character->GetMesh() != MeshComp) return;
	// Outcome events can never be authored as animation-hit approximations.
	if (Event != EUmbraAudioEvent::AttackSwing && Event != EUmbraAudioEvent::AttackVoice
		&& Event != EUmbraAudioEvent::Footstep && Event != EUmbraAudioEvent::Clothing
		&& Event != EUmbraAudioEvent::SkillCast) return;
	FUmbraAudioContext Context = Audio->GetContext();
	FVector Location = SocketName.IsNone() ? Owner->GetActorLocation() : MeshComp->GetSocketLocation(SocketName);
	if (Event == EUmbraAudioEvent::Footstep)
	{
		const ACharacter* Character = Cast<ACharacter>(Owner);
		if (!Character || !Character->GetCharacterMovement()->IsMovingOnGround()
			|| Character->GetVelocity().SizeSquared2D() < 1.f) return;
		FHitResult Hit;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(UmbraAudioFootstep), false, Owner);
		Query.bReturnPhysicalMaterial = true;
		const float Distance = FMath::IsFinite(GroundTraceDistance) ? FMath::Clamp(GroundTraceDistance, 1.f, 500.f) : 100.f;
		if (!Owner->GetWorld()->LineTraceSingleByChannel(Hit, Location + FVector(0,0,20),
			Location - FVector(0,0,Distance), ECC_Visibility, Query)) return;
		Context.Surface = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
		Location = Hit.ImpactPoint;
	}
	Audio->PlayLocal(Event, Context, Location);
}
