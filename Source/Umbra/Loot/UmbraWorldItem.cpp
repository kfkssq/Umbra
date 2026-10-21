#include "Loot/UmbraWorldItem.h"
#include "Loot/UmbraWorldItemVisualComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Components/WidgetComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/StreamableManager.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Player/UmbraPlayerState.h"
#include "UI/Items/UmbraWorldItemLabel.h"
#include "UmbraPlayerController.h"

AUmbraWorldItem::AUmbraWorldItem()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	PhysicsBody = CreateDefaultSubobject<UBoxComponent>(TEXT("PhysicsBody"));
	SetRootComponent(PhysicsBody);
	PhysicsBody->InitBoxExtent(FVector(20, 12, 8));
	PhysicsBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PhysicsBody->SetGenerateOverlapEvents(false);
	PhysicsBody->SetCanEverAffectNavigation(false);
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->SetupAttachment(PhysicsBody);
	Collision->InitSphereRadius(24.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Collision->SetGenerateOverlapEvents(false);
	Collision->SetCanEverAffectNavigation(false);
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetSimulatePhysics(false);
	Mesh->SetVisibility(false);
	Mesh->SetCanEverAffectNavigation(false);
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(Collision);
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMesh->SetGenerateOverlapEvents(false);
	SkeletalMesh->SetSimulatePhysics(false);
	SkeletalMesh->SetCanEverAffectNavigation(false);
	SkeletalMesh->SetVisibility(false);
	SkeletalMesh->SetComponentTickEnabled(false);
	VisualComponent = CreateDefaultSubobject<UUmbraWorldItemVisualComponent>(TEXT("VisualComponent"));
	Label = CreateDefaultSubobject<UWidgetComponent>(TEXT("Label"));
	Label->SetupAttachment(Collision);
	Label->SetAbsolute(false, true, true);
	Label->SetRelativeLocation(FVector(0, 0, 35));
	Label->SetWidgetSpace(EWidgetSpace::Screen);
	Label->SetDrawAtDesiredSize(true);
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Label->SetCanEverAffectNavigation(false);
}

bool AUmbraWorldItem::InitializeInstance(const FUmbraItemInstance& InInstance)
{
	if (!HasAuthority() || ItemInstance.InstanceId.IsValid() || !InInstance.IsValid()) return false;
	ItemInstance = InInstance;
	DropSpawnServerTime = GetWorld()->GetTimeSeconds();
	const float Duration = DropPhysics.bEnabled ? 0.f : VisualComponent->GetAnimationDuration();
	bDropAnimationWindowClosed = Duration <= 0.f;
	if (!bDropAnimationWindowClosed)
		GetWorldTimerManager().SetTimer(DropAnimationWindowTimer, this, &ThisClass::CloseDropAnimationWindow, Duration, false);
	if (HasActorBegunPlay()) StartDropPhysics();
	RefreshPresentation();
	ForceNetUpdate();
	return true;
}

void AUmbraWorldItem::BeginPlay()
{
	Super::BeginPlay();
	// The atomic DropMotion snapshot owns movement; ignore any legacy BP replication override.
	SetReplicateMovement(false);
	LabelOffset = Label->GetRelativeLocation();
	StartDropPhysics();
	RefreshPresentation();
}

void AUmbraWorldItem::ClearMeshLoad()
{
	VisualComponent->ClearVisual();
	RequestedMeshPath.Reset();
	++MeshLoadRevision;
	if (MeshLoadHandle) MeshLoadHandle->CancelHandle();
	MeshLoadHandle.Reset();
	Mesh->SetVisibility(false);
	Mesh->SetStaticMesh(nullptr);
	SkeletalMesh->SetVisibility(false);
	SkeletalMesh->SetSkeletalMesh(nullptr);
}

void AUmbraWorldItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(DropAnimationWindowTimer);
	GetWorldTimerManager().ClearTimer(DropPhysicsTimer);
	ClearMeshLoad();
	Label->SetVisibility(false);
	if (auto* Widget = Cast<UUmbraWorldItemLabel>(Label->GetUserWidgetObject()))
		Widget->SetItem(FUmbraItemInstance(), false);
	Super::EndPlay(EndPlayReason);
}

void AUmbraWorldItem::RefreshPresentation()
{
	if (bPickedUp || !ItemInstance.IsValid())
	{
		bHighlighted = false;
		ClearMeshLoad();
	}
	SetActorHiddenInGame(bPickedUp);
	SetActorEnableCollision(!bPickedUp);
	if (!HasActorBegunPlay() || GetNetMode() == NM_DedicatedServer) return;
	// Allow cursor queries on the visible model, without physics, overlaps or navigation.
	// Enforce these responses even when an existing BP has serialized component overrides.
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCanEverAffectNavigation(false);
	Label->SetWidgetSpace(EWidgetSpace::Screen);
	Label->SetDrawAtDesiredSize(true);
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Label->SetCanEverAffectNavigation(false);
	Label->SetVisibility(!bPickedUp);
	UpdateLabelLocation();
	RefreshLabel();
	const UUmbraItemDefinition* Definition = ItemInstance.Definition;
	if (bPickedUp || !ItemInstance.IsValid()) return;
	if (Definition->WorldDropMesh.IsNull() && Definition->WorldDropSkeletalMesh.IsNull())
	{
		ClearMeshLoad();
		UpdateLabelLocation();
		return;
	}
	const FSoftObjectPath MeshPath = !Definition->WorldDropSkeletalMesh.IsNull()
		? Definition->WorldDropSkeletalMesh.ToSoftObjectPath() : Definition->WorldDropMesh.ToSoftObjectPath();
	if (RequestedMeshPath == MeshPath)
	{
		RefreshVisual();
		return;
	}
	ClearMeshLoad();
	RequestedMeshPath = MeshPath;
	Mesh->SetRelativeTransform(FTransform(Definition->WorldDropRotation, Definition->WorldDropOffset, Definition->WorldDropScale));
	const uint64 Revision = MeshLoadRevision;
	// Weak delegate protects destruction; revision also rejects stale completions for the same asset.
	MeshLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(MeshPath,
		FStreamableDelegate::CreateWeakLambda(this, [this, MeshPath, Revision]()
		{
			if (Revision != MeshLoadRevision || bPickedUp || IsActorBeingDestroyed()) return;
			USkeletalMesh* LoadedSkeletalMesh = Cast<USkeletalMesh>(MeshPath.ResolveObject());
			SkeletalMesh->SetSkeletalMesh(LoadedSkeletalMesh);
			SkeletalMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			SkeletalMesh->SetSimulatePhysics(false);
			// Reference-pose triangle queries cover armor outside the pickup sphere, without ragdoll physics.
			SkeletalMesh->SetEnablePerPolyCollision(true);
			SkeletalMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
			SkeletalMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			SkeletalMesh->SetCollisionEnabled(LoadedSkeletalMesh ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
			SkeletalMesh->RefreshBoneTransforms();
			SkeletalMesh->SetComponentTickEnabled(false);
			SkeletalMesh->SetVisibility(LoadedSkeletalMesh != nullptr);
			UStaticMesh* LoadedMesh = Cast<UStaticMesh>(MeshPath.ResolveObject());
			Mesh->SetStaticMesh(LoadedMesh);
			Mesh->SetVisibility(LoadedMesh != nullptr);
			UpdateLabelLocation();
			RefreshLabel();
			RefreshVisual();
		}));
}

void AUmbraWorldItem::CloseDropAnimationWindow()
{
	bDropAnimationWindowClosed = true;
	RefreshVisual();
	ForceNetUpdate();
}

void AUmbraWorldItem::RefreshVisual()
{
	if (!HasActorBegunPlay() || GetNetMode() == NM_DedicatedServer) return;
	if (bPickedUp || !ItemInstance.IsValid())
	{
		VisualComponent->ClearVisual();
		return;
	}
	const UUmbraItemDefinition* Definition = ItemInstance.Definition;
	VisualComponent->InitializeVisual(GetDisplayMesh(), ItemInstance.InstanceId,
		GetRestTransform(),
		Definition->RarityColor, DropSpawnServerTime, bDropAnimationWindowClosed || DropPhysics.bEnabled);
	VisualComponent->SetHighlighted(bHighlighted);
	SyncSkeletalQueryTransform();
	UpdateLabelLocation();
}

void AUmbraWorldItem::SyncSkeletalQueryTransform()
{
	// These meshes remain in reference pose. UE's per-poly transform update depends on MeshObject;
	// explicitly keep the query body aligned even before render-state creation or without an RHI.
	if (SkeletalMesh->GetSkeletalMeshAsset() && SkeletalMesh->bEnablePerPolyCollision && SkeletalMesh->IsPhysicsStateCreated())
	{
		SkeletalMesh->BodyInstance.UpdateBodyScale(SkeletalMesh->GetComponentScale());
		SkeletalMesh->BodyInstance.SetBodyTransform(SkeletalMesh->GetComponentTransform(), ETeleportType::TeleportPhysics);
	}
}

UMeshComponent* AUmbraWorldItem::GetDisplayMesh() const
{
	return SkeletalMesh->GetSkeletalMeshAsset() ? static_cast<UMeshComponent*>(SkeletalMesh.Get()) : Mesh.Get();
}

FTransform AUmbraWorldItem::GetRestTransform() const
{
	const auto* Definition = ItemInstance.Definition.Get();
	if (!IsValid(Definition)) return FTransform::Identity;
	FTransform Rest(Definition->WorldDropRotation, Definition->WorldDropOffset, Definition->WorldDropScale);
	if (const auto* Asset = SkeletalMesh->GetSkeletalMeshAsset())
	{
		// Armor pivots often remain at the character's feet. Offset.Z supplies extra clearance in cm.
		const FTransform Shape(Definition->WorldDropRotation, FVector::ZeroVector, Definition->WorldDropScale);
		const FBox Bounds = Asset->GetBounds().GetBox().TransformBy(Shape);
		Rest.AddToTranslation(FVector(0, 0, -Bounds.Min.Z));
	}
	if (DropMotion.bStarted)
	{
		const FVector Center = Definition->WorldDropPhysicsCenter;
		const FRotator Rotation = Definition->WorldDropPhysicsRotation.ContainsNaN() ? FRotator::ZeroRotator : Definition->WorldDropPhysicsRotation;
		Rest = Rest.GetRelativeTransform(FTransform(Rotation,
			Center.ContainsNaN() ? FVector(0, 0, 8) : Center.BoundToCube(500.f)));
	}
	return Rest;
}

void AUmbraWorldItem::UpdateLabelLocation()
{
	FVector Location = LabelOffset;
	FBox Bounds(ForceInit);
	if (const auto* Asset = SkeletalMesh->GetSkeletalMeshAsset())
		Bounds = Asset->GetBounds().GetBox().TransformBy(GetRestTransform());
	else if (Mesh->GetStaticMesh())
		Bounds = Mesh->GetStaticMesh()->GetBoundingBox().TransformBy(GetRestTransform());
	if (Bounds.IsValid)
	{
		const FBox WorldBounds = Bounds.TransformBy(GetActorTransform());
		Label->SetWorldLocation(FVector(WorldBounds.GetCenter().X, WorldBounds.GetCenter().Y, WorldBounds.Max.Z) + Location);
	}
	else Label->SetWorldLocation(GetActorLocation() + Location);
}

void AUmbraWorldItem::RefreshLabel()
{
	Label->InitWidget();
	if (auto* Widget = Cast<UUmbraWorldItemLabel>(Label->GetUserWidgetObject()))
		Widget->SetItem(bPickedUp ? FUmbraItemInstance() : ItemInstance, bHighlighted);
}

void AUmbraWorldItem::SetHighlighted(bool bValue)
{
	bValue = bValue && !bPickedUp && ItemInstance.IsValid();
	if (bHighlighted == bValue) return;
	bHighlighted = bValue;
	VisualComponent->SetHighlighted(bValue);
	if (HasActorBegunPlay() && GetNetMode() != NM_DedicatedServer) RefreshLabel();
}

EUmbraPickupResult AUmbraWorldItem::TryPickup(AUmbraPlayerController* Controller)
{
	if (!HasAuthority() || !IsValid(Controller) || !Controller->HasAuthority()) return EUmbraPickupResult::NotReady;
	if (bPickedUp || IsActorBeingDestroyed()) return EUmbraPickupResult::Unavailable;
	if (bTransferInProgress) return EUmbraPickupResult::NotReady;
	if (!ItemInstance.IsValid()) return EUmbraPickupResult::InvalidItem;
	if (!Controller->CanPickupItems()) return EUmbraPickupResult::NotAllowed;
	if (!Controller->IsWithinPickupRange(this)) return EUmbraPickupResult::OutOfRange;
	auto* State = Controller->GetPlayerState<AUmbraPlayerState>();
	if (!IsValid(State)) return EUmbraPickupResult::NotReady;
	// Inventory publishes synchronously. Guard before calling it to reject reentrant/second-player pickup.
	TGuardValue<bool> Guard(bTransferInProgress, true);
	const auto Result = State->AcceptLootInstance(ItemInstance);
	if (Result != EUmbraInventoryResult::Success)
	{
		switch (Result)
		{
		case EUmbraInventoryResult::Full: return EUmbraPickupResult::InventoryFull;
		case EUmbraInventoryResult::DuplicateInstance:
		case EUmbraInventoryResult::InvalidIdentity: return EUmbraPickupResult::DuplicateInstance;
		case EUmbraInventoryResult::InvalidItem: return EUmbraPickupResult::InvalidItem;
		default: return EUmbraPickupResult::NotReady;
		}
	}
	bPickedUp = true;
	RefreshPresentation();
	ForceNetUpdate();
	Destroy();
	return EUmbraPickupResult::Success;
}

void AUmbraWorldItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AUmbraWorldItem, ItemInstance);
	DOREPLIFETIME(AUmbraWorldItem, bPickedUp);
	DOREPLIFETIME(AUmbraWorldItem, DropSpawnServerTime);
	DOREPLIFETIME(AUmbraWorldItem, bDropAnimationWindowClosed);
	DOREPLIFETIME(AUmbraWorldItem, DropMotion);
}
