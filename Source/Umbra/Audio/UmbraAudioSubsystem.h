#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Audio/UmbraAudioTypes.h"
#include "UmbraAudioSubsystem.generated.h"

class UUmbraAudioLibrary;
class USoundConcurrency;
class USoundAttenuation;
class UAudioComponent;

/** Local playback policy only. No gameplay state, tick, or network ownership. */
UCLASS()
class UMBRA_API UUmbraAudioSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	bool Play(UObject* Emitter, EUmbraAudioEvent Event, const FUmbraAudioContext& Context,
		const FVector& Location, UUmbraAudioLibrary* Override = nullptr, bool bForce2D = false);
	UFUNCTION(BlueprintCallable, Category="Audio") void SetCategoryVolume(EUmbraAudioCategory Category, float Volume);
	/** Owned, spatial loop. Caller starts/stops it; source asset must be authored as looping. */
	UAudioComponent* CreateLootLoop(AActor* Owner, const FUmbraAudioContext& Context, UUmbraAudioLibrary* Override = nullptr);
private:
	UPROPERTY(Transient) TObjectPtr<UUmbraAudioLibrary> DefaultLibrary;
	UPROPERTY(Transient) TObjectPtr<USoundConcurrency> DefaultConcurrency;
	UPROPERTY(Transient) TObjectPtr<USoundAttenuation> DefaultAttenuation;
	UPROPERTY(Transient) TObjectPtr<USoundConcurrency> LootLoopConcurrency;
	TMap<TWeakObjectPtr<UObject>, TMap<EUmbraAudioEvent, double>> NextEmitterTimes;
	TMap<EUmbraAudioEvent, double> NextGlobalTimes;
	TMap<EUmbraAudioCategory, float> CategoryVolumes;
	/** Cosmetic selection must not consume gameplay's global random sequence on a listen server. */
	FRandomStream AudioRandom;
};
