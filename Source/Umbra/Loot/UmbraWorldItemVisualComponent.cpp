#include "Loot/UmbraWorldItemVisualComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Curves/CurveFloat.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

namespace
{
	UObject* VisualAsset(UMeshComponent* Mesh)
	{
		if (auto* Static = Cast<UStaticMeshComponent>(Mesh)) return Static->GetStaticMesh();
		if (auto* Skeletal = Cast<USkeletalMeshComponent>(Mesh)) return Skeletal->GetSkeletalMeshAsset();
		return nullptr;
	}
}

UUmbraWorldItemVisualComponent::UUmbraWorldItemVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

float UUmbraWorldItemVisualComponent::GetAnimationDuration() const
{
	return bEnableDropAnimation ? FMath::Max(0.f, RiseDuration) + FMath::Max(0.f, FallDuration) + FMath::Max(0.f, BounceDuration) : 0.f;
}

void UUmbraWorldItemVisualComponent::InitializeVisual(UMeshComponent* InMesh, const FGuid& InstanceId,
	const FTransform& InBaseTransform, FLinearColor InRarityColor, double SpawnTime, bool bWindowClosed)
{
	const bool bSameVisual = DropMesh == InMesh && InMesh && AnimatedMesh == VisualAsset(InMesh)
		&& ConsumedInstanceId == InstanceId && BaseTransform.Equals(InBaseTransform);
	if (!bSameVisual)
	{
		ClearVisual();
		DropMesh = InMesh;
		AnimatedMesh = InMesh ? VisualAsset(InMesh) : nullptr;
		BaseTransform = InBaseTransform;
		if (InMesh) InMesh->SetRelativeTransform(BaseTransform);
	}
	RarityHighlightColor = InRarityColor;
	SetHighlighted(bHighlighted);
	if (bWindowClosed) FinishAnimation();
	if (!InstanceId.IsValid() || !InMesh || !VisualAsset(InMesh) || SpawnTime < 0) return;
	if (ConsumedInstanceId == InstanceId) return;
	ConsumedInstanceId = InstanceId;
	const AGameStateBase* State = GetWorld()->GetGameState();
	// Without a synchronized clock prefer a stationary item to replaying an old drop.
	if (bWindowClosed || !bEnableDropAnimation || (!GetOwner()->HasAuthority() && !State)) return;
	const double Now = State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	AnimationSpawnTime = SpawnTime;
	LastAge = FMath::Max(0.f, static_cast<float>(Now - SpawnTime));
	if (LastAge >= GetAnimationDuration()) return;
	ApplyAnimation(LastAge);
	SetComponentTickEnabled(true);
}

void UUmbraWorldItemVisualComponent::SetHighlighted(bool bValue)
{
	bHighlighted = bValue;
	if (UMeshComponent* Mesh = DropMesh.Get())
	{
		Mesh->SetCustomDepthStencilValue(FMath::Clamp(CustomStencilValue, 2, 255));
		Mesh->SetRenderCustomDepth(bHighlighted && bEnableHoverOutline && VisualAsset(Mesh));
	}
}

void UUmbraWorldItemVisualComponent::FinishAnimation()
{
	SetComponentTickEnabled(false);
	if (UMeshComponent* Mesh = DropMesh.Get()) Mesh->SetRelativeTransform(BaseTransform);
}

void UUmbraWorldItemVisualComponent::ClearVisual()
{
	FinishAnimation();
	if (UMeshComponent* Mesh = DropMesh.Get()) Mesh->SetRenderCustomDepth(false);
	DropMesh.Reset();
	AnimatedMesh.Reset();
	bHighlighted = false;
	RarityHighlightColor = FLinearColor::White;
	// Keep consumed identity through reload/OnRep so a cleared mesh cannot replay the drop.
}

float UUmbraWorldItemVisualComponent::Ease(float Alpha) const
{
	Alpha = FMath::Clamp(Alpha, 0.f, 1.f);
	if (Alpha == 0.f || Alpha == 1.f) return Alpha;
	return AnimationCurve ? FMath::Clamp(AnimationCurve->GetFloatValue(Alpha), 0.f, 1.f) : Alpha;
}

void UUmbraWorldItemVisualComponent::ApplyAnimation(float Age)
{
	const float Rise = FMath::Max(0.f, RiseDuration);
	const float Fall = FMath::Max(0.f, FallDuration);
	const float Bounce = FMath::Max(0.f, BounceDuration);
	float Height = 0.f, Scale = 1.f, RotationWeight = 0.f;
	if (Age < Rise)
	{
		const float T = Ease(Age / Rise);
		const float Lift = 1.f - FMath::Square(1.f - T);
		Height = FMath::Max(0.f, RiseHeight) * Lift;
		Scale = FMath::Lerp(InitialScaleMultiplier, PeakScaleMultiplier, Lift);
		RotationWeight = T;
	}
	else if (Age < Rise + Fall)
	{
		const float T = Ease((Age - Rise) / Fall);
		Height = FMath::Max(0.f, RiseHeight) * (1.f - T * T);
		Scale = FMath::Lerp(PeakScaleMultiplier, 1.f, T);
		RotationWeight = 1.f - T;
	}
	else if (Bounce > 0.f)
	{
		const float T = Ease((Age - Rise - Fall) / Bounce);
		Height = FMath::Max(0.f, BounceHeight) * 4.f * T * (1.f - T);
	}
	FTransform Pose = BaseTransform;
	Pose.AddToTranslation(FVector(0, 0, Height));
	Pose.SetRotation(BaseTransform.GetRotation() * (AdditionalAirRotation * RotationWeight).Quaternion());
	Pose.SetScale3D(BaseTransform.GetScale3D() * FMath::Max(0.f, Scale));
	if (UMeshComponent* Mesh = DropMesh.Get()) Mesh->SetRelativeTransform(Pose);
}

void UUmbraWorldItemVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!DropMesh.IsValid() || VisualAsset(DropMesh.Get()) != AnimatedMesh.Get())
	{
		ClearVisual();
		return;
	}
	const AGameStateBase* State = GetWorld()->GetGameState();
	const double Now = State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	// Clock corrections may advance playback, but never rewind it or extend its lifetime.
	LastAge = FMath::Max(LastAge + DeltaTime, static_cast<float>(Now - AnimationSpawnTime));
	if (!bEnableDropAnimation || LastAge >= GetAnimationDuration()) FinishAnimation();
	else ApplyAnimation(LastAge);
}

void UUmbraWorldItemVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearVisual();
	Super::EndPlay(EndPlayReason);
}
