#include "AbilitySystem/UmbraAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/Damage/UmbraPhysicalDamage.h"
#include "Umbra.h"
#include "Characters/UmbraEnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerState.h"
#include "UmbraPlayerController.h"
#include "GameplayTags/UmbraGameplayTags.h"

UUmbraAttributeSet::UUmbraAttributeSet()
{
	InitMaxHealth(100.f);
	InitHealth(100.f);
	InitMaxResource(100.f);
	InitResource(100.f);
	InitAttackPower(10.f);
	InitCriticalDamageMultiplier(2.f);
	InitMoveSpeed(600.f);
}

void UUmbraAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& Value) const
{
	if (Attribute == GetIncomingDamageAttribute()) return;
	if (!FMath::IsFinite(Value)) Value = 0.f;
	if (Attribute == GetMaxHealthAttribute() || Attribute == GetCriticalDamageMultiplierAttribute())
		Value = FMath::Max(Value, 1.f);
	else if (Attribute == GetHealthAttribute())
		Value = FMath::Clamp(Value, 0.f, FMath::Max(1.f, GetMaxHealth()));
	else if (Attribute == GetResourceAttribute())
		Value = FMath::Clamp(Value, 0.f, FMath::Max(0.f, GetMaxResource()));
	else if (Attribute == GetCriticalChanceAttribute())
		Value = FMath::Clamp(Value, 0.f, 1.f);
	else
		Value = FMath::Max(Value, 0.f);
}
void UUmbraAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}
void UUmbraAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	// Also runs when duration/infinite modifiers are added, removed or recalculated.
	ClampAttribute(Attribute, NewValue);
}
void UUmbraAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	// Only clip on cap reduction. Never bake all evaluated buffs into BaseValue.
	if (Attribute == GetMaxHealthAttribute() && NewValue < OldValue && GetHealth() > NewValue)
		SetHealth(FMath::Min(Health.GetBaseValue(), NewValue));
	else if (Attribute == GetMaxResourceAttribute() && NewValue < OldValue && GetResource() > NewValue)
		SetResource(FMath::Min(Resource.GetBaseValue(), NewValue));
}
void UUmbraAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float Damage = GetIncomingDamage();
		const float HealthBefore = GetHealth();
		// Capture before Health delegates can destroy the victim.
		TWeakObjectPtr<AUmbraPlayerController> Recipient;
		FVector DisplayPosition = FVector::ZeroVector;
		const float Type = Data.EffectSpec.GetSetByCallerMagnitude(UmbraGameplayTags::Damage_Type, false, -1.f);
		const float Critical = Data.EffectSpec.GetSetByCallerMagnitude(UmbraGameplayTags::Damage_ResultCritical, false, -1.f);
		auto* TargetASC = GetOwningAbilitySystemComponent();
		if (TargetASC && TargetASC->IsOwnerActorAuthoritative() && Damage > 0.f && FMath::IsFinite(Damage)
			&& (Type == 0.f || Type == 1.f) && (Critical == 0.f || Critical == 1.f))
		{
			if (auto* Enemy = Cast<AUmbraEnemyCharacter>(TargetASC->GetAvatarActor()))
			{
				DisplayPosition = Enemy->GetMesh() ? Enemy->GetMesh()->Bounds.Origin : Enemy->GetActorLocation();
				if (auto* SourceASC = Data.EffectSpec.GetContext().GetOriginalInstigatorAbilitySystemComponent())
				{
					if (auto* Pawn = Cast<APawn>(SourceASC->GetAvatarActor()))
						Recipient = Cast<AUmbraPlayerController>(Pawn->GetController());
					if (!Recipient.IsValid())
						if (auto* PS = Cast<APlayerState>(SourceASC->GetOwnerActor()))
							Recipient = Cast<AUmbraPlayerController>(PS->GetPlayerController());
				}
			}
		}
		// Clear first: Health delegates can synchronously cause another damage execution.
		SetIncomingDamage(0.f);
		if (FMath::IsFinite(Damage) && Damage > 0.f)
			SetHealth(Health.GetBaseValue() - Damage);
		if (Recipient.IsValid())
			Recipient->ClientShowDamageNumber(DisplayPosition, Damage, uint8(Type), Critical == 1.f);
		if (UmbraPhysicalDamage::IsLoggingEnabled())
		{
			UE_LOG(LogUmbra, Log, TEXT("[DamageHealth] Target=%s GE=%s Incoming=%.3f Health=%.3f->%.3f Lost=%.3f"),
				*GetNameSafe(GetOwningAbilitySystemComponent()->GetAvatarActor()), *GetNameSafe(Data.EffectSpec.Def),
				Damage, HealthBefore, GetHealth(), HealthBefore - GetHealth());
		}
	}
}
void UUmbraAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UUmbraAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUmbraAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUmbraAttributeSet, HealthRegen, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUmbraAttributeSet, Resource, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUmbraAttributeSet, MaxResource, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUmbraAttributeSet, ResourceRegen, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUmbraAttributeSet, AttackPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUmbraAttributeSet, AbilityPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUmbraAttributeSet, AttackSpeedBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUmbraAttributeSet, CriticalChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUmbraAttributeSet, CriticalDamageMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUmbraAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUmbraAttributeSet, MagicResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUmbraAttributeSet, AbilityHaste, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUmbraAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
}
void UUmbraAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUmbraAttributeSet, Health, OldValue);
}
void UUmbraAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUmbraAttributeSet, MaxHealth, OldValue);
}
void UUmbraAttributeSet::OnRep_HealthRegen(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUmbraAttributeSet, HealthRegen, OldValue);
}
void UUmbraAttributeSet::OnRep_Resource(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUmbraAttributeSet, Resource, OldValue);
}
void UUmbraAttributeSet::OnRep_MaxResource(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUmbraAttributeSet, MaxResource, OldValue);
}
void UUmbraAttributeSet::OnRep_ResourceRegen(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUmbraAttributeSet, ResourceRegen, OldValue);
}
void UUmbraAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUmbraAttributeSet, AttackPower, OldValue);
}
void UUmbraAttributeSet::OnRep_AbilityPower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUmbraAttributeSet, AbilityPower, OldValue);
}
void UUmbraAttributeSet::OnRep_AttackSpeedBonus(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUmbraAttributeSet, AttackSpeedBonus, OldValue);
}
void UUmbraAttributeSet::OnRep_CriticalChance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUmbraAttributeSet, CriticalChance, OldValue);
}
void UUmbraAttributeSet::OnRep_CriticalDamageMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUmbraAttributeSet, CriticalDamageMultiplier, OldValue);
}
void UUmbraAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUmbraAttributeSet, Armor, OldValue);
}
void UUmbraAttributeSet::OnRep_MagicResistance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUmbraAttributeSet, MagicResistance, OldValue);
}
void UUmbraAttributeSet::OnRep_AbilityHaste(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUmbraAttributeSet, AbilityHaste, OldValue);
}
void UUmbraAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUmbraAttributeSet, MoveSpeed, OldValue);
}
