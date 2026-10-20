#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/UmbraItemInstance.h"
#include "UmbraWorldItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UUmbraWorldItemVisualComponent;
class AUmbraPlayerController;
struct FStreamableHandle;

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Item") TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Item") TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Item") TObjectPtr<UUmbraWorldItemVisualComponent> VisualComponent;
	/** Relative location is the label offset above mesh bounds, or fixed fallback without a mesh (cm). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Item") TObjectPtr<UWidgetComponent> Label;
private:
	UFUNCTION() void RefreshPresentation();
	UFUNCTION() void RefreshVisual();
	void CloseDropAnimationWindow();
	void RefreshLabel();
	void UpdateLabelLocation();
	void ClearMeshLoad();
	TSharedPtr<FStreamableHandle> MeshLoadHandle;
	uint64 MeshLoadRevision = 0;
	FSoftObjectPath RequestedMeshPath;
	FTimerHandle DropAnimationWindowTimer;
	/** Server world seconds at immutable payload initialization; -1 means not ready. */
	UPROPERTY(ReplicatedUsing = RefreshVisual) double DropSpawnServerTime = -1;
	/** Also protects late relevance from an initially unsynchronized GameState clock. */
	UPROPERTY(ReplicatedUsing = RefreshVisual) bool bDropAnimationWindowClosed = false;
	FVector LabelOffset = FVector(0, 0, 35);
	UPROPERTY(ReplicatedUsing = RefreshPresentation) FUmbraItemInstance ItemInstance;
	UPROPERTY(ReplicatedUsing = RefreshPresentation) bool bPickedUp = false;
	bool bTransferInProgress = false;
	bool bHighlighted = false;
};
