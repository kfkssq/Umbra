#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UmbraAudioSettings.generated.h"

class UUmbraAudioLibrary;

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Umbra Audio"))
class UMBRA_API UUmbraAudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	/** Project Settings > Game > Umbra Audio. Empty is a supported silent configuration. */
	UPROPERTY(Config, EditAnywhere, Category="Audio") TSoftObjectPtr<UUmbraAudioLibrary> DefaultLibrary;
	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
