#include "Loot/UmbraWorldItem.h"
#include "Audio/UmbraAudioComponent.h"
#include "Components/BoxComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Umbra.h"

namespace
{
	constexpr float SampleInterval = 0.05f;
	float FiniteClamp(float Value, float Default, float Min, float Max)
	{
		return FMath::Clamp(FMath::IsFinite(Value) ? Value : Default, Min, Max);
	}
}

void FUmbraDropPhysicsSettings::Sanitize()
{
	UpwardSpeed = FiniteClamp(UpwardSpeed, 260, 0, 2000);
	HorizontalSpeed = FiniteClamp(HorizontalSpeed, 80, 0, 1000);
	AngularSpeedDegrees = FiniteClamp(AngularSpeedDegrees, 180, 0, 1440);
	LinearDamping = FiniteClamp(LinearDamping, 0.5f, 0, 20);
	AngularDamping = FiniteClamp(AngularDamping, 3, 0, 20);
	Friction = FiniteClamp(Friction, 0.7f, 0, 1);
	Restitution = FiniteClamp(Restitution, 0.15f, 0, 1);
	MinimumSimulationTime = FiniteClamp(MinimumSimulationTime, 0.4f, 0, 10);
	StableTime = FiniteClamp(StableTime, 0.3f, 0.05f, 2);
	MaximumSimulationTime = FiniteClamp(MaximumSimulationTime, 3, MinimumSimulationTime + StableTime, 15);
	SettlingBlendTime = FiniteClamp(SettlingBlendTime, 0.8f, 0.1f, 5);
	SettlingLinearDamping = FiniteClamp(SettlingLinearDamping, 2, LinearDamping, 20);
	SettlingAngularDamping = FiniteClamp(SettlingAngularDamping, 8, AngularDamping, 20);
	UnsupportedRecoveryTime = FiniteClamp(UnsupportedRecoveryTime, 30, 10, 120);
	RecoveryFallDistance = FiniteClamp(RecoveryFallDistance, 2000, 500, 10000);
	LinearSleepThreshold = FiniteClamp(LinearSleepThreshold, 5, 0, 100);
	AngularSleepThresholdDegrees = FiniteClamp(AngularSleepThresholdDegrees, 10, 0, 360);
}

bool AUmbraWorldItem::FindDropGround(const FVector& Origin, const FQuat& Rotation,
	float Above, float Below, FTransform& OutPose) const
{
	// Static support is deliberate: a frozen item must not float when a supporting physics body moves.
	FCollisionQueryParams Query(SCENE_QUERY_STAT(UmbraDropSupport), false, this);
	const auto Objects = FCollisionObjectQueryParams(ECC_WorldStatic);
	const FCollisionShape Shape = FCollisionShape::MakeBox(PhysicsBody->GetScaledBoxExtent());
	FHitResult Hit;
	if (!GetWorld()->SweepSingleByObjectType(Hit, Origin + FVector(0, 0, Above),
		Origin - FVector(0, 0, Below), Rotation, Objects, Shape, Query)
		|| Hit.bStartPenetrating || Hit.ImpactNormal.Z < 0.65f) return false;
	const FVector Position = Hit.Location + FVector(0, 0, 0.5f);
	if (GetWorld()->OverlapAnyTestByObjectType(Position, Rotation, Objects, Shape, Query)) return false;
	OutPose = FTransform(Rotation, Position, GetActorScale3D());
	return true;
}

void AUmbraWorldItem::StartDropPhysics()
{
	if (!HasAuthority() || bPhysicsInitialized || !ItemInstance.IsValid() || !DropPhysics.bEnabled) return;
	bPhysicsInitialized = true;
	DropPhysics.Sanitize();
	const auto* Definition = ItemInstance.Definition.Get();
	FVector Extent = Definition->WorldDropPhysicsExtent;
	for (int32 Axis = 0; Axis < 3; ++Axis) Extent[Axis] = FiniteClamp(Extent[Axis], 12, 1, 200);
	PhysicsCenter = Definition->WorldDropPhysicsCenter;
	PhysicsCenter = PhysicsCenter.ContainsNaN() ? FVector(0, 0, 8) : PhysicsCenter.BoundToCube(500.f);
	PhysicsBody->SetBoxExtent(Extent);
	PhysicsBody->SetCollisionObjectType(ECC_WorldDynamic);
	PhysicsBody->SetCollisionResponseToAllChannels(ECR_Ignore);
	PhysicsBody->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	if (DropPhysics.bCollideWithPhysicsBodies) PhysicsBody->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	PhysicsBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PhysicsBody->SetEnableGravity(true);
	PhysicsBody->SetLinearDamping(DropPhysics.LinearDamping);
	PhysicsBody->SetAngularDamping(DropPhysics.AngularDamping);
	PhysicsBody->SetUseCCD(true);
	DropPhysicalMaterial = NewObject<UPhysicalMaterial>(this);
	DropPhysicalMaterial->Friction = DropPhysics.Friction;
	DropPhysicalMaterial->Restitution = DropPhysics.Restitution;
	DropPhysicalMaterial->bOverrideFrictionCombineMode = true;
	DropPhysicalMaterial->FrictionCombineMode = EFrictionCombineMode::Max;
	DropPhysicalMaterial->bOverrideRestitutionCombineMode = true;
	DropPhysicalMaterial->RestitutionCombineMode = EFrictionCombineMode::Min;
	PhysicsBody->SetPhysMaterialOverride(DropPhysicalMaterial);
	const FVector CenterLocation = GetActorTransform().TransformPosition(PhysicsCenter);
	const FRotator ProxyRotation = Definition->WorldDropPhysicsRotation.ContainsNaN()
		? FRotator::ZeroRotator : Definition->WorldDropPhysicsRotation;
	const FQuat BodyRotation = GetActorQuat() * ProxyRotation.Quaternion();
	// Reserve a verified recovery pose before launch. No model loading is needed on dedicated servers.
	if (!FindDropGround(CenterLocation, BodyRotation, 200, 1000, SafeDropPose))
	{
		PhysicsBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DropMotion.bSettled = true;
		UE_LOG(LogUmbra, Warning, TEXT("Drop physics skipped for %s: no safe static ground within recovery range."), *GetName());
		return;
	}
	SetActorLocationAndRotation(CenterLocation, BodyRotation, false, nullptr, ETeleportType::TeleportPhysics);
	// Near-ground spawners may put the authored proxy inside the floor. Start at the reserved pose.
	if (GetWorld()->OverlapAnyTestByObjectType(CenterLocation, GetActorQuat(),
		FCollisionObjectQueryParams(ECC_WorldStatic), FCollisionShape::MakeBox(PhysicsBody->GetScaledBoxExtent()),
		FCollisionQueryParams(SCENE_QUERY_STAT(UmbraDropLaunch), false, this)))
		SetActorTransform(SafeDropPose, false, nullptr, ETeleportType::TeleportPhysics);
	PhysicsBody->SetSimulatePhysics(true);
	PhysicsBody->SetNotifyRigidBodyCollision(true);
	const float Angle = FMath::FRandRange(0.f, 2.f * PI);
	PhysicsBody->SetPhysicsLinearVelocity(FVector(FMath::Cos(Angle) * DropPhysics.HorizontalSpeed,
		FMath::Sin(Angle) * DropPhysics.HorizontalSpeed, DropPhysics.UpwardSpeed));
	PhysicsBody->SetPhysicsAngularVelocityInDegrees(FMath::VRand() * DropPhysics.AngularSpeedDegrees);
	PhysicsStartTime = GetWorld()->GetTimeSeconds();
	LastPhysicsSampleTime = PhysicsStartTime;
	PublishDropMotion(false);
	SetActorTickEnabled(GetNetMode() != NM_DedicatedServer);
	GetWorldTimerManager().SetTimer(DropPhysicsTimer, this, &ThisClass::UpdateDropPhysics, SampleInterval, true);
}

void AUmbraWorldItem::PublishDropMotion(bool bSettled)
{
	DropMotion.Location = GetActorLocation();
	DropMotion.Rotation = GetActorQuat();
	DropMotion.bStarted = true;
	DropMotion.bSettled = bSettled;
	ForceNetUpdate();
}

void AUmbraWorldItem::HandleAudioGroundHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	// First actual static-ground contact only; rolling, settling and recovery never retrigger it.
	if (!HasAuthority() || bLandAudioSent || bPickedUp || !ItemInstance.IsValid()
		|| HitComponent != PhysicsBody || !PhysicsBody->IsSimulatingPhysics() || !OtherComponent
		|| OtherComponent->GetCollisionObjectType() != ECC_WorldStatic || Hit.ImpactNormal.Z < 0.5f) return;
	bLandAudioSent = true;
	auto Context = FUmbraAudioContext::FromItem(ItemInstance.Definition);
	Context.Surface = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
	AudioEvents->PlayAuthority(EUmbraAudioEvent::LootLand, Context, Hit.ImpactPoint);
}

void AUmbraWorldItem::UpdateDropPhysics()
{
	if (bPickedUp || IsActorBeingDestroyed()) { GetWorldTimerManager().ClearTimer(DropPhysicsTimer); return; }
	const float Age = GetWorld()->GetTimeSeconds() - PhysicsStartTime;
	// A hitch can dispatch a looping timer several times in one frame; those are not independent observations.
	const float SampleSeconds = FMath::Clamp(static_cast<float>(GetWorld()->GetTimeSeconds() - LastPhysicsSampleTime), 0.f, SampleInterval * 2);
	LastPhysicsSampleTime = GetWorld()->GetTimeSeconds();
	FTransform SupportedPose;
	const bool bSupported = FindDropGround(GetActorLocation(), GetActorQuat(), 1.f, 3.f, SupportedPose);
	const bool bSlow = PhysicsBody->GetPhysicsLinearVelocity().Size() <= DropPhysics.LinearSleepThreshold
		&& PhysicsBody->GetPhysicsAngularVelocityInDegrees().Size() <= DropPhysics.AngularSleepThresholdDegrees;
	StableSeconds = Age >= DropPhysics.MinimumSimulationTime && bSupported && bSlow ? StableSeconds + SampleSeconds : 0.f;
	if (StableSeconds >= DropPhysics.StableTime)
	{
		FreezeDrop(GetActorTransform());
		return;
	}
	// A time limit must never place a normally falling/rolling item at a synthetic rest pose.
	// Only supported bodies receive extra damping; leaving an edge restores normal gravity-driven motion.
	GroundedSettlingSeconds = Age >= DropPhysics.MaximumSimulationTime && bSupported
		? GroundedSettlingSeconds + SampleSeconds : 0.f;
	const float Blend = FMath::Clamp(GroundedSettlingSeconds / DropPhysics.SettlingBlendTime, 0.f, 1.f);
	PhysicsBody->SetLinearDamping(FMath::Lerp(DropPhysics.LinearDamping, DropPhysics.SettlingLinearDamping, Blend));
	PhysicsBody->SetAngularDamping(FMath::Lerp(DropPhysics.AngularDamping, DropPhysics.SettlingAngularDamping, Blend));
	if (bSupported) UnsupportedSince = -1;
	else if (UnsupportedSince < 0) UnsupportedSince = GetWorld()->GetTimeSeconds();
	const bool bLostSupport = UnsupportedSince >= 0
		&& GetWorld()->GetTimeSeconds() - UnsupportedSince >= DropPhysics.UnsupportedRecoveryTime;
	const bool bFellOut = GetActorLocation().Z < SafeDropPose.GetLocation().Z - DropPhysics.RecoveryFallDistance;
	if (bLostSupport || bFellOut)
	{
		// Exceptional void/stuck recovery only. Revalidate the original floor before using it.
		FTransform Recovery;
		if (FindDropGround(GetActorLocation(), GetActorQuat(), 50, 1000, Recovery)
			|| FindDropGround(SafeDropPose.GetLocation(), SafeDropPose.GetRotation(), 1, 5, Recovery))
		{
			FreezeDrop(Recovery);
			return;
		}
	}
	PublishDropMotion(false);
}

void AUmbraWorldItem::FreezeDrop(const FTransform& Pose)
{
	PhysicsBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PhysicsBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	PhysicsBody->SetSimulatePhysics(false);
	PhysicsBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorTransform(Pose, false, nullptr, ETeleportType::TeleportPhysics);
	GetWorldTimerManager().ClearTimer(DropPhysicsTimer);
	SetActorTickEnabled(false);
	SyncSkeletalQueryTransform();
	UpdateLabelLocation();
	PublishDropMotion(true);
	RefreshLootAmbience();
}

void AUmbraWorldItem::OnRep_DropMotion()
{
	if (HasAuthority() || !DropMotion.bStarted) return;
	// Clients display server poses only. No competing local Chaos simulation or attachment detachment.
	PhysicsBody->SetSimulatePhysics(false);
	PhysicsBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (!bReceivedMotion || DropMotion.bSettled)
		SetActorLocationAndRotation(DropMotion.Location, DropMotion.Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	bReceivedMotion = true;
	SetActorTickEnabled(!DropMotion.bSettled);
	RefreshVisual();
	UpdateLabelLocation();
}

void AUmbraWorldItem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() && DropMotion.bStarted && !DropMotion.bSettled)
	{
		const float Alpha = FMath::Clamp(DeltaSeconds / SampleInterval, 0.f, 1.f);
		SetActorLocationAndRotation(FMath::Lerp(GetActorLocation(), DropMotion.Location, Alpha),
			FQuat::Slerp(GetActorQuat(), DropMotion.Rotation, Alpha), false, nullptr, ETeleportType::TeleportPhysics);
	}
	SyncSkeletalQueryTransform();
	UpdateLabelLocation();
}
