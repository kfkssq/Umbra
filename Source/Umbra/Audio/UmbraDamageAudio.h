#pragma once

#include "CoreMinimal.h"
#include "Audio/UmbraAudioTypes.h"

class UUmbraAudioComponent;
class UAbilitySystemComponent;
struct FGameplayEffectSpec;

/** Snapshot before Health delegates; dispatch after settlement without reading destroyed targets. */
struct FUmbraDamageAudio
{
	static FUmbraDamageAudio Capture(UAbilitySystemComponent* Target, const FGameplayEffectSpec& Spec, float Damage, float HealthBefore);
	void Dispatch(float HealthAfter) const;
private:
	TWeakObjectPtr<UUmbraAudioComponent> Victim;
	TWeakObjectPtr<UUmbraAudioComponent> Attacker;
	FUmbraAudioContext HitContext;
	FUmbraAudioContext VictimContext;
	FUmbraAudioContext AttackerContext;
	FVector Position = FVector::ZeroVector;
	FVector AttackerPosition = FVector::ZeroVector;
	bool bCritical = false;
	bool bSkill = false;
	bool bValid = false;
};
