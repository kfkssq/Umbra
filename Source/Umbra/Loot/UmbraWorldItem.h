#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/UmbraItemInstance.h"
#include "UmbraWorldItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class AUmbraPlayerController;

UENUM(BlueprintType)
enum class EUmbraPickupResult : uint8
{
	Success, InventoryFull, DuplicateInstance, InvalidItem, NotReady, OutOfRange,
	Unavailable, NotAllowed, NoPath, Cancelled, TimedOut
};

/** Server-created immutable payload; never owned by or attached to the enemy corpse. */
UCLASS(Blueprintable)
class UMBRA_API AUmbraWorldItem : public AActor
{
	GENERATED_BODY()
public:
	AUmbraWorldItem();
	bool InitializeInstance(const FUmbraItemInstance& InInstance);
	UFUNCTION(BlueprintPure, Category = "World Item") FUmbraItemInstance GetItemInstance() const { return ItemInstance; }
	EUmbraPickupResult TryPickup(AUmbraPlayerController* Controller);
	void SetHighlighted(bool bValue);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Item") TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Item") TObjectPtr<UStaticMeshComponent> Mesh;
	/** Set Widget Class to a child of UmbraWorldItemLabel; optional, including on dedicated servers. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Item") TObjectPtr<UWidgetComponent> Label;
private:
	UFUNCTION() void RefreshPresentation();
	UPROPERTY(ReplicatedUsing = RefreshPresentation) FUmbraItemInstance ItemInstance;
	UPROPERTY(ReplicatedUsing = RefreshPresentation) bool bPickedUp = false;
	bool bTransferInProgress = false;
	bool bHighlighted = false;
};
