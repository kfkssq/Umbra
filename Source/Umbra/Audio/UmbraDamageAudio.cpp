#include "Audio/UmbraDamageAudio.h"
#include "Audio/UmbraAudioComponent.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

FUmbraDamageAudio FUmbraDamageAudio::Capture(UAbilitySystemComponent* Target, const FGameplayEffectSpec& Spec, float Damage, float HealthBefore)
{
	FUmbraDamageAudio Result;
	if (!Target || !Target->IsOwnerActorAuthoritative() || !FMath::IsFinite(Damage) || Damage <= 0.f
		|| !FMath::IsFinite(HealthBefore) || HealthBefore <= 0.f) return Result;
	Result.Victim = UUmbraAudioComponent::Find(Target->GetAvatarActor());
	if (!Result.Victim.IsValid()) return Result;
	Result.Position = Target->GetAvatarActor()->GetActorLocation();
	Result.VictimContext = Result.Victim->GetContext();
	Result.HitContext = Result.VictimContext;
	if (const UAbilitySystemComponent* Source = Spec.GetContext().GetOriginalInstigatorAbilitySystemComponent())
	{
		Result.Attacker = UUmbraAudioComponent::Find(Source->GetAvatarActor());
		if (Result.Attacker.IsValid())
		{
			Result.AttackerContext = Result.Attacker->GetContext();
			Result.AttackerPosition = Source->GetAvatarActor()->GetActorLocation();
			Result.HitContext.Weapon = Result.AttackerContext.Weapon;
		}
	}
	if (const FHitResult* Hit = Spec.GetContext().GetHitResult())
		if (Hit->PhysMaterial.IsValid()) Result.HitContext.Surface = UPhysicalMaterial::DetermineSurfaceType(Hit->PhysMaterial.Get());
	Result.bCritical = Spec.GetSetByCallerMagnitude(UmbraGameplayTags::Damage_ResultCritical, false, 0.f) == 1.f;
	FGameplayTagContainer Tags;
	Spec.GetAllAssetTags(Tags);
	Result.bSkill = Tags.HasTagExact(UmbraGameplayTags::Damage_Source_Skill);
	Result.bValid = true;
	return Result;
}

void FUmbraDamageAudio::Dispatch(float HealthAfter) const
{
	if (!bValid) return;
	if (Victim.IsValid())
	{
		Victim->PlayAuthority(bSkill ? EUmbraAudioEvent::SkillHit : EUmbraAudioEvent::Hit, HitContext, Position);
		if (bCritical) Victim->PlayAuthority(EUmbraAudioEvent::Critical, HitContext, Position);
		Victim->PlayAuthority(HealthAfter <= 0.f ? EUmbraAudioEvent::Death : EUmbraAudioEvent::Hurt, VictimContext, Position);
	}
	if (bCritical && Attacker.IsValid() && Attacker != Victim)
		Attacker->PlayAuthority(EUmbraAudioEvent::CriticalVoice, AttackerContext, AttackerPosition);
	if (HealthAfter <= 0.f && Attacker.IsValid() && Attacker != Victim)
		Attacker->PlayAuthority(EUmbraAudioEvent::Kill, AttackerContext, AttackerPosition);
}
