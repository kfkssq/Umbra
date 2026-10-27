#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Audio/UmbraAudioTypes.h"
#include "UmbraAudioComponent.generated.h"

class UUmbraAudioLibrary;

UCLASS(ClassGroup=(Umbra), meta=(BlueprintSpawnableComponent))
class UMBRA_API UUmbraAudioComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UUmbraAudioComponent();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio") TObjectPtr<UUmbraAudioLibrary> LibraryOverride;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio") FName CharacterType;
	/** Fallback for enemies and characters without an equipped main-hand definition. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio") EUmbraWeaponType DefaultWeapon = EUmbraWeaponType::Unknown;
	/** Used only when authoritative damage has no physical-material HitResult. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio") TEnumAsByte<EPhysicalSurface> HitSurface = SurfaceType_Default;
	UFUNCTION(BlueprintPure, Category="Audio") FUmbraAudioContext GetContext() const;
	/** Local animation presentation. Do not also multicast this event. */
	UFUNCTION(BlueprintCallable, Category="Audio") void PlayLocal(EUmbraAudioEvent Event, FUmbraAudioContext Context, FVector Location);
	/** Authority-only presentation; no server RPC or gameplay mutation. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Audio") void PlayAuthority(EUmbraAudioEvent Event, FUmbraAudioContext Context, FVector Location);
	static UUmbraAudioComponent* Find(const AActor* Actor);
private:
	UFUNCTION(NetMulticast, Unreliable) void MulticastAudio(EUmbraAudioEvent Event, FUmbraAudioContext Context, FVector_NetQuantize Location);
	bool bDeathSent = false;
};
