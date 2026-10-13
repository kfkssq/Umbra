#include "Loot/UmbraWorldItem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
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
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
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
	RefreshPresentation();
	ForceNetUpdate();
	return true;
}

void AUmbraWorldItem::BeginPlay()
{
	Super::BeginPlay();
	RefreshPresentation();
}

void AUmbraWorldItem::RefreshPresentation()
{
	SetActorHiddenInGame(bPickedUp);
	SetActorEnableCollision(!bPickedUp);
	if (!HasActorBegunPlay() || GetNetMode() == NM_DedicatedServer) return;
	Label->InitWidget();
	if (auto* Widget = Cast<UUmbraWorldItemLabel>(Label->GetUserWidgetObject()))
		Widget->SetItem(ItemInstance, bHighlighted);
}

void AUmbraWorldItem::SetHighlighted(bool bValue)
{
	if (bHighlighted == bValue) return;
	bHighlighted = bValue;
	Mesh->SetRenderCustomDepth(bValue);
	RefreshPresentation();
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
}
