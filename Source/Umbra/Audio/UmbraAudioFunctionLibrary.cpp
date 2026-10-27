#include "Audio/UmbraAudioFunctionLibrary.h"
#include "Audio/UmbraAudioComponent.h"
#include "Audio/UmbraAudioSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

void UUmbraAudioFunctionLibrary::PlayUI(APlayerController* Player, EUmbraAudioEvent Event, const FUmbraAudioContext& Context)
{
	if (!IsValid(Player) || !Player->IsLocalPlayerController() || !Player->GetWorld()) return;
	if (auto* Audio = Player->GetWorld()->GetSubsystem<UUmbraAudioSubsystem>())
	{
		const auto* Component = UUmbraAudioComponent::Find(Player);
		Audio->Play(Player, Event, Context, FVector::ZeroVector, Component ? Component->LibraryOverride.Get() : nullptr, true);
	}
}
