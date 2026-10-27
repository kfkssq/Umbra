#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Audio/UmbraAudioTypes.h"
#include "UmbraAudioLibrary.generated.h"

class USoundBase;
class USoundAttenuation;
class USoundConcurrency;

USTRUCT(BlueprintType)
struct UMBRA_API FUmbraAudioEntry
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EUmbraAudioEvent Event = EUmbraAudioEvent::UIHover;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EUmbraAudioCategory Category = EUmbraAudioCategory::UI;
	/** Unknown / Default / None are wildcards. Rarity filtering is explicitly enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FUmbraAudioContext Match;
	/** Hard references load with the library, never on the hit/notify path. Null entries are ignored. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<TObjectPtr<USoundBase>> Sounds;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bSpatial = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="4")) float Volume = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.25", ClampMax="4")) float PitchMin = 0.95f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.25", ClampMax="4")) float PitchMax = 1.05f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="1")) float Probability = 1.f;
	/** Seconds, shared by all variants of this event for one emitter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float Cooldown = 0.08f;
	/** Seconds between this event across all emitters in one world. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float GlobalCooldown = 0.015f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<USoundAttenuation> Attenuation;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<USoundConcurrency> Concurrency;
	int32 MatchScore(EUmbraAudioEvent InEvent, const FUmbraAudioContext& Context) const;
};

UCLASS(BlueprintType)
class UMBRA_API UUmbraAudioLibrary : public UDataAsset
{
	GENERATED_BODY()
public:
	/** Most specific match wins; equal specificity uses the first row. Empty rows deliberately mute. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FUmbraAudioEntry> Events;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<EUmbraAudioCategory, float> CategoryVolumes;
	const FUmbraAudioEntry* Find(EUmbraAudioEvent Event, const FUmbraAudioContext& Context) const;
};
