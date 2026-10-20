#include "Loot/UmbraWorldItem.h"
#include "Loot/UmbraWorldItemVisualComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
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
	PrimaryActorTick.bCanEverTick = false;
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
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
	VisualComponent = CreateDefaultSubobject<UUmbraWorldItemVisualComponent>(TEXT("VisualComponent"));
	Label = CreateDefaultSubobject<UWidgetComponent>(TEXT("Label"));
	Label->SetupAttachment(Collision);
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
	const float Duration = VisualComponent->GetAnimationDuration();
	bDropAnimationWindowClosed = Duration <= 0.f;
	if (!bDropAnimationWindowClosed)
		GetWorldTimerManager().SetTimer(DropAnimationWindowTimer, this, &ThisClass::CloseDropAnimationWindow, Duration, false);
	RefreshPresentation();
	ForceNetUpdate();
	return true;
}

void AUmbraWorldItem::BeginPlay()
{
	Super::BeginPlay();
	LabelOffset = Label->GetRelativeLocation();
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
}

void AUmbraWorldItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(DropAnimationWindowTimer);
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
	if (Definition->WorldDropMesh.IsNull())
	{
		ClearMeshLoad();
		UpdateLabelLocation();
		return;
	}
	const FSoftObjectPath MeshPath = Definition->WorldDropMesh.ToSoftObjectPath();
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
	VisualComponent->InitializeVisual(Mesh, ItemInstance.InstanceId,
		FTransform(Definition->WorldDropRotation, Definition->WorldDropOffset, Definition->WorldDropScale),
		Definition->RarityColor, DropSpawnServerTime, bDropAnimationWindowClosed);
	VisualComponent->SetHighlighted(bHighlighted);
}

void AUmbraWorldItem::UpdateLabelLocation()
{
	FVector Location = LabelOffset;
	if (Mesh->GetStaticMesh())
	{
		// Actor-local bounds include the authored mesh offset, rotation and scale.
		const UUmbraItemDefinition* Definition = ItemInstance.Definition;
		const FTransform RestTransform = IsValid(Definition)
			? FTransform(Definition->WorldDropRotation, Definition->WorldDropOffset, Definition->WorldDropScale)
			: Mesh->GetRelativeTransform();
		const FBox Bounds = Mesh->GetStaticMesh()->GetBoundingBox().TransformBy(RestTransform);
		Location += FVector(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Max.Z);
	}
	Label->SetRelativeLocation(Location);
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
}
