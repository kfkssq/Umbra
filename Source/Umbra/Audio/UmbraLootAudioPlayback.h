#pragma once
#include "CoreMinimal.h"

namespace UmbraLootAudioPlayback
{
	// Seconds within a looping Wave. Pitch is fixed once when the component is created.
	inline float ResumeOffset(double ElapsedSeconds, float Pitch, float Duration)
	{
		if (!FMath::IsFinite(ElapsedSeconds) || !FMath::IsFinite(Pitch) || !FMath::IsFinite(Duration)
			|| ElapsedSeconds <= 0 || Pitch <= 0 || Duration <= 0) return 0.f;
		return float(FMath::Fmod(ElapsedSeconds * double(Pitch), double(Duration)));
	}
}
