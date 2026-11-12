#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UmbraAudioTestLibrary.generated.h"

class UAudioComponent;

UCLASS()
class UUmbraAudioTestLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Editor smoke evidence from the audio thread's actual fader, not a game-thread estimate. */
	UFUNCTION(BlueprintCallable, meta=(DevelopmentOnly))
	static void TraceLoopFader(UAudioComponent* Component, const FString& Phase);
};
