#pragma once

#include "CoreMinimal.h"

class UAbilitySystemComponent;
class AUmbraPlayerController;
struct FGameplayEffectSpec;

/** Per-hit presentation snapshot. Capture before Health callbacks can destroy the victim. */
struct FUmbraDamageNotification
{

	static FUmbraDamageNotification Capture(UAbilitySystemComponent* Target, const FGameplayEffectSpec& Spec, float Damage);
	void Dispatch() const;
	bool HasRecipient() const { return Recipient.IsValid(); }

private:
	TWeakObjectPtr<AUmbraPlayerController> Recipient;
	FVector Position = FVector::ZeroVector;
	float Amount = 0.f;
	uint8 Type = 0;
	bool bCritical = false;
};
