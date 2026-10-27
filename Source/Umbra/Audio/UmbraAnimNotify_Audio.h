#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Audio/UmbraAudioTypes.h"
#include "UmbraAnimNotify_Audio.generated.h"

/** Presentation only: never submits damage or changes montage timing. */
UCLASS(const, DisplayName="Umbra Audio")
class UMBRA_API UUmbraAnimNotify_Audio : public UAnimNotify
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio") EUmbraAudioEvent Event = EUmbraAudioEvent::AttackSwing;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio") FName SocketName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio", meta=(ClampMin="1", ClampMax="500")) float GroundTraceDistance = 100.f;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
