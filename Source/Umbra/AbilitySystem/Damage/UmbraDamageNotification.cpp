#include "AbilitySystem/Damage/UmbraDamageNotification.h"

#include "AbilitySystemComponent.h"
#include "Characters/UmbraEnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffect.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "UmbraPlayerController.h"

FUmbraDamageNotification FUmbraDamageNotification::Capture(
	UAbilitySystemComponent* Target, const FGameplayEffectSpec& Spec, float Damage)
{
	FUmbraDamageNotification Notification;
	const float Type = Spec.GetSetByCallerMagnitude(UmbraGameplayTags::Damage_Type, false, -1.f);
	const float Critical = Spec.GetSetByCallerMagnitude(UmbraGameplayTags::Damage_ResultCritical, false, -1.f);
	if (!Target || !Target->IsOwnerActorAuthoritative() || !FMath::IsFinite(Damage) || Damage <= 0.f
		|| (Type != 0.f && Type != 1.f) || (Critical != 0.f && Critical != 1.f))
	{
		return Notification;
	}
	const AUmbraEnemyCharacter* Enemy = Cast<AUmbraEnemyCharacter>(Target->GetAvatarActor());
	if (!Enemy)
	{
		return Notification;
	}
	Notification.Position = Enemy->GetMesh() ? Enemy->GetMesh()->Bounds.Origin : Enemy->GetActorLocation();
	if (const UAbilitySystemComponent* Source = Spec.GetContext().GetOriginalInstigatorAbilitySystemComponent())
	{
		if (const APawn* Pawn = Cast<APawn>(Source->GetAvatarActor()))
		{
			Notification.Recipient = Cast<AUmbraPlayerController>(Pawn->GetController());
		}
		if (!Notification.Recipient.IsValid())
		{
			if (const APlayerState* PlayerState = Cast<APlayerState>(Source->GetOwnerActor()))
			{
				Notification.Recipient = Cast<AUmbraPlayerController>(PlayerState->GetPlayerController());
			}
		}
	}
	Notification.Amount = Damage;
	Notification.Type = uint8(Type);
	Notification.bCritical = Critical == 1.f;
	return Notification;
}

void FUmbraDamageNotification::Dispatch() const
{
	if (Recipient.IsValid())
	{
		Recipient->ClientShowDamageNumber(Position, Amount, Type, bCritical);
	}
}
