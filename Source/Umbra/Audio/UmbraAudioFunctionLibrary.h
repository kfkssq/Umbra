#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Audio/UmbraAudioTypes.h"
#include "UmbraAudioFunctionLibrary.generated.h"

class APlayerController;

UCLASS()
class UMBRA_API UUmbraAudioFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Call once at the leaf interaction/result owner; ignored for remote controllers. */
	UFUNCTION(BlueprintCallable, Category="Audio", meta=(AutoCreateRefTerm="Context"))
	static void PlayUI(APlayerController* Player, EUmbraAudioEvent Event, const FUmbraAudioContext& Context);
};
