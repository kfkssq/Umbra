#include "UmbraPlayerController.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Characters/UmbraPlayerCharacter.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "Umbra.h"

bool AUmbraPlayerController::CanPickupItems() const
{
	const auto* UmbraCharacter = Cast<AUmbraPlayerCharacter>(GetPawn());
	const auto* ASC = UmbraCharacter ? UmbraCharacter->GetAbilitySystemComponent() : nullptr;
	return UmbraCharacter && UmbraCharacter->IsMovementEnabled() && ASC && ASC->GetAvatarActor() == UmbraCharacter
		&& UmbraCharacter->GetCharacterMovement()->MovementMode != MOVE_None
		&& ASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()) > 0.f
		&& !ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Dead)
		&& !ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Stunned)
		&& !ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Attacking);
}

bool AUmbraPlayerController::IsWithinPickupRange(const AUmbraWorldItem* Item) const
{
	const float Range = FMath::IsFinite(PickupRange) ? FMath::Clamp(PickupRange, 100.f, 500.f) : 150.f;
	return IsValid(Item) && GetPawn() && FVector::DistSquared(GetPawn()->GetActorLocation(), Item->GetActorLocation()) <= FMath::Square(Range);
}

AUmbraWorldItem* AUmbraPlayerController::GetWorldItemUnderCursor() const
{
	if (!IsLocalPlayerController() || IsPointerOverAttributeDebugPanel()) return nullptr;
	FHitResult Hit;
	// Complex visibility queries cover the displayed weapon, including parts outside
	// the root pickup sphere. Keep the original simple query as the no-mesh fallback.
	if (GetHitResultUnderCursor(ECC_Visibility, true, Hit) && Hit.bBlockingHit)
	{
		if (auto* Item = Cast<AUmbraWorldItem>(Hit.GetActor())) return Item;
	}
	return GetCursorGroundHit(Hit) ? Cast<AUmbraWorldItem>(Hit.GetActor()) : nullptr;
}

void AUmbraPlayerController::UpdateWorldItemHover()
{
	AActor* Enemy = nullptr;
	auto* Item = GetAttackableUnderCursor(Enemy) ? nullptr : GetWorldItemUnderCursor();
	if (HoveredWorldItem.Get() == Item) return;
	if (auto* Old = HoveredWorldItem.Get()) Old->SetHighlighted(false);
	HoveredWorldItem = Item;
	if (Item) Item->SetHighlighted(true);
}

void AUmbraPlayerController::BeginPickup(AUmbraWorldItem* Item)
{
	CancelAutoMove();
	CancelPendingAttack();
	ResetPrimaryActionState();
	ClearQueuedCommand();
	if (!IsValid(Item)) { ReportPickupResult(EUmbraPickupResult::Unavailable); return; }
	if (IsPrimaryAttackActive())
	{
		QueuedCommand = EUmbraQueuedPlayerCommand::Pickup;
		QueuedPickup = Item;
		if (auto* UmbraCharacter = Cast<AUmbraPlayerCharacter>(GetPawn()))
		{
			UmbraCharacter->StopPrimaryAttackContinuation();
			if (auto* ASC = Cast<UUmbraAbilitySystemComponent>(UmbraCharacter->GetAbilitySystemComponent()))
				ASC->RequestPrimaryAttackMovementCancellation();
		}
		return;
	}
	if (!CanPickupItems()) { ReportPickupResult(EUmbraPickupResult::NotAllowed); return; }
	if (IsWithinPickupRange(Item)) { ServerPickupWorldItem(Item); return; }
	if (!StartAutoMoveToLocation(Item->GetActorLocation())) { ReportPickupResult(EUmbraPickupResult::NoPath); return; }
	PendingPickup = Item;
	const float Timeout = FMath::IsFinite(PickupApproachTimeout) ? FMath::Clamp(PickupApproachTimeout, 1.f, 60.f) : 15.f;
	PickupDeadline = GetWorld()->GetTimeSeconds() + Timeout;
	GetWorldTimerManager().SetTimer(PickupApproachTimer, this, &AUmbraPlayerController::CheckPickupApproach, 0.05f, true);
}

void AUmbraPlayerController::CheckPickupApproach()
{
	EUmbraPickupResult Failure = EUmbraPickupResult::Success;
	if (!PendingPickup.IsValid()) Failure = EUmbraPickupResult::Unavailable;
	else if (!CanPickupItems()) Failure = EUmbraPickupResult::NotAllowed;
	else if (GetWorld()->GetTimeSeconds() >= PickupDeadline) Failure = EUmbraPickupResult::TimedOut;
	else if (!bAutoMoving) Failure = EUmbraPickupResult::Cancelled;
	if (Failure != EUmbraPickupResult::Success)
	{
		CancelAutoMove();
		ReportPickupResult(Failure);
		return;
	}
	if (IsWithinPickupRange(PendingPickup.Get())) CompletePickupApproach();
}

void AUmbraPlayerController::CompletePickupApproach()
{
	auto* Item = PendingPickup.Get();
	CancelAutoMove();
	if (APawn* ControlledPawn = GetPawn())
		if (auto* Movement = ControlledPawn->GetMovementComponent()) Movement->StopMovementImmediately();
	ServerPickupWorldItem(Item);
}

void AUmbraPlayerController::CancelPickupApproach()
{
	if (GetWorld() && GetWorldTimerManager().IsTimerActive(PickupApproachTimer))
	{
		bAutoMoving = false;
		AutoMovePathPoints.Reset();
		CurrentPathPointIndex = INDEX_NONE;
	}
	PendingPickup.Reset();
	if (GetWorld()) GetWorldTimerManager().ClearTimer(PickupApproachTimer);
	if (QueuedCommand == EUmbraQueuedPlayerCommand::Pickup) ClearQueuedCommand();
}

void AUmbraPlayerController::ServerPickupWorldItem_Implementation(AUmbraWorldItem* Item)
{
	ClientPickupResult(IsValid(Item) ? Item->TryPickup(this) : EUmbraPickupResult::Unavailable);
}

void AUmbraPlayerController::ClientPickupResult_Implementation(EUmbraPickupResult Result)
{
	ReportPickupResult(Result);
}

void AUmbraPlayerController::ReportPickupResult(EUmbraPickupResult Result)
{
	UE_LOG(LogUmbra, Log, TEXT("Pickup: %s"), *UEnum::GetValueAsString(Result));
	BP_PickupResult(Result);
}
