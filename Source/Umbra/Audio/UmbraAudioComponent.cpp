#include "Audio/UmbraAudioComponent.h"
#include "Audio/UmbraAudioSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Player/UmbraPlayerState.h"
#include "Engine/World.h"

UUmbraAudioComponent::UUmbraAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

UUmbraAudioComponent* UUmbraAudioComponent::Find(const AActor* Actor)
{
	return IsValid(Actor) ? Actor->FindComponentByClass<UUmbraAudioComponent>() : nullptr;
}

FUmbraAudioContext UUmbraAudioComponent::GetContext() const
{
	FUmbraAudioContext Context;
	Context.CharacterType = CharacterType;
	Context.Weapon = DefaultWeapon;
	Context.Surface = HitSurface;
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const AUmbraPlayerState* State = Pawn ? Pawn->GetPlayerState<AUmbraPlayerState>() : nullptr;
	if (State && State->GetEquipmentComponent())
	{
		for (const FUmbraEquippedItem& Item : State->GetEquipmentComponent()->GetSnapshot().Items)
			if (Item.Slot == EUmbraEquipmentSlot::MainHand && IsValid(Item.Definition))
			{
				Context.Weapon = Item.Definition->WeaponType;
				break;
			}
	}
	return Context;
}

void UUmbraAudioComponent::PlayLocal(EUmbraAudioEvent Event, FUmbraAudioContext Context, FVector Location)
{
	if (!GetWorld() || !GetOwner() || GetWorld()->GetNetMode() == NM_DedicatedServer) return;
	if (auto* Audio = GetWorld()->GetSubsystem<UUmbraAudioSubsystem>())
		Audio->Play(GetOwner(), Event, Context, Location, LibraryOverride);
}

void UUmbraAudioComponent::PlayAuthority(EUmbraAudioEvent Event, FUmbraAudioContext Context, FVector Location)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !GetOwner()->HasActorBegunPlay()) return;
	if (Event == EUmbraAudioEvent::Death)
	{
		if (bDeathSent) return;
		bDeathSent = true;
	}
	MulticastAudio(Event, Context, Location);
}

void UUmbraAudioComponent::MulticastAudio_Implementation(EUmbraAudioEvent Event, FUmbraAudioContext Context, FVector_NetQuantize Location)
{
	PlayLocal(Event, Context, Location);
}
