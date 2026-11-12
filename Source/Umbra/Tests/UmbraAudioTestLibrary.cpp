#include "Tests/UmbraAudioTestLibrary.h"
#include "Components/AudioComponent.h"
#include "AudioDevice.h"
#include "ActiveSound.h"
#include "Umbra.h"

void UUmbraAudioTestLibrary::TraceLoopFader(UAudioComponent* Component, const FString& Phase)
{
#if WITH_EDITOR
	if (!IsValid(Component)) return;
	if (auto* Device = Component->GetAudioDevice())
	{
		Device->SendCommandToActiveSounds(Component->GetAudioComponentID(), [Phase](FActiveSound& Sound)
		{
			UE_LOG(LogUmbra, Display, TEXT("LootLoopFader phase=%s gain=%.6f target=%.6f seconds=%.3f"),
				*Phase, Sound.ComponentVolumeFader.GetVolume(), Sound.ComponentVolumeFader.GetTargetVolume(), Sound.PlaybackTime);
		});
	}
#endif
}
