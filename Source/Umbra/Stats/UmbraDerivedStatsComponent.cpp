#include "Stats/UmbraDerivedStatsComponent.h"

#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Curves/CurveFloat.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "Umbra.h"
#include "Equipment/UmbraEquipmentComponent.h"

namespace
{
	TArray<FGameplayAttribute> Primaries()
	{
		return { UUmbraAttributeSet::GetStrengthAttribute(), UUmbraAttributeSet::GetDexterityAttribute(),
			UUmbraAttributeSet::GetIntelligenceAttribute(), UUmbraAttributeSet::GetFaithAttribute() };
	}
	bool WritesPower(const FGameplayEffectSpec& Spec)
	{
		for (const FGameplayModifierInfo& Modifier : Spec.Def->Modifiers)
			if (Modifier.Attribute == UUmbraAttributeSet::GetAttackPowerAttribute()
				|| Modifier.Attribute == UUmbraAttributeSet::GetAbilityPowerAttribute()) return true;
		return false;
	}
}

UUmbraDerivedStatsComponent::UUmbraDerivedStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	UnarmedProfile.Channels.AddDefaulted_GetRef().BaseDamage = 10.f;
}

void UUmbraDerivedStatsComponent::Initialize(UUmbraAbilitySystemComponent* ASC)
{
	if (!bUseWeaponDerivedPower || !ASC || ASC->GetOwnerActor() != GetOwner() || !ASC->IsOwnerActorAuthoritative()
		|| !ASC->IsActorInfoReady() || !ASC->GetSet<UUmbraAttributeSet>()) return;
	if (BoundASC.Get() == ASC) { Refresh(); return; }
	// Do not bake an existing duration AD/AP modifier into the derived output.
	for (const FActiveGameplayEffectHandle Handle : ASC->GetActiveEffects(FGameplayEffectQuery()))
	{
		const FActiveGameplayEffect* Effect = ASC->GetActiveGameplayEffect(Handle);
		if (Effect && WritesPower(Effect->Spec))
		{
			UE_LOG(LogUmbra, Error, TEXT("Derived power cannot start with active AD/AP modifiers."));
			return;
		}
	}
	Shutdown();
	BoundASC = ASC;
	Weapon = InitialWeapon;
	FUmbraDerivedStatsSnapshot Check;
	if (!Calculate(Weapon ? Weapon->Damage : UnarmedProfile, Check))
	{
		UE_LOG(LogUmbra, Error, TEXT("Derived power profile is invalid; legacy attributes retained."));
		BoundASC.Reset();
		return;
	}
	for (const FGameplayAttribute& Attribute : Primaries())
		PrimaryHandles.Add(Attribute, ASC->GetGameplayAttributeValueChangeDelegate(Attribute)
			.AddUObject(this, &ThisClass::OnPrimaryChanged));
	FGameplayEffectApplicationQuery Query;
	Query.BindUObject(this, &ThisClass::CanApplyEffect);
	ApplicationQueryHandle = Query.GetHandle();
	ASC->GameplayEffectApplicationQueries.Add(Query);
	Refresh();
}

bool UUmbraDerivedStatsComponent::Calculate(const FUmbraWeaponDamageProfile& Profile, FUmbraDerivedStatsSnapshot& Out, float BaseFactor, float ScalingFactor) const
{
	const UUmbraAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC || Profile.Channels.Num() > 9) return false;
	const TArray<FGameplayAttribute> Attributes = Primaries();
	TSet<EUmbraWeaponDamageType> Types;
	double AD = 0.0, AP = 0.0;
	for (const FUmbraWeaponDamageChannel& Input : Profile.Channels)
	{
		if (uint8(Input.Type) > uint8(EUmbraWeaponDamageType::Shadow) || Types.Contains(Input.Type)
			|| !FMath::IsFinite(Input.BaseDamage) || Input.BaseDamage < 0.f || Input.Scaling.Num() > 4) return false;
		Types.Add(Input.Type);
		TSet<EUmbraPrimaryAttribute> Used;
		double Scaling = 0.0;
		for (const FUmbraWeaponScaling& Term : Input.Scaling)
		{
			if (!Attributes.IsValidIndex(uint8(Term.Attribute)) || Used.Contains(Term.Attribute)
				|| !FMath::IsFinite(Term.Coefficient) || Term.Coefficient < 0.f) return false;
			Used.Add(Term.Attribute);
			const float Points = ASC->GetNumericAttribute(Attributes[uint8(Term.Attribute)]);
			const float Units = Term.PointCurve ? Term.PointCurve->GetFloatValue(Points) : Points;
			if (!FMath::IsFinite(Units) || Units < 0.f) return false;
			Scaling += double(Units) * Term.Coefficient;
		}
		Scaling *= ScalingFactor;
		const double Base = double(Input.BaseDamage) * BaseFactor;
		const double Total = Base + Scaling;
		if (!FMath::IsFinite(Total) || Total > MAX_flt) return false;
		FUmbraDerivedDamageChannel& Channel = Out.Channels.AddDefaulted_GetRef();
		Channel.Type = Input.Type;
		Channel.BaseDamage = float(Base);
		Channel.ScalingDamage = float(Scaling);
		Channel.Damage = float(Total);
		if (uint8(Input.Type) <= uint8(EUmbraWeaponDamageType::Piercing)) AD += Channel.Damage;
		else AP += Channel.Damage;
	}
	if (AD > MAX_flt || AP > MAX_flt) return false;
	Out.AttackPower = float(AD);
	Out.AbilityPower = float(AP);
	Out.bValid = true;
	return true;
}

bool UUmbraDerivedStatsComponent::SetWeaponProfile(UUmbraWeaponProfile* Profile)
{
	if (!BoundASC.IsValid() || !BoundASC->IsOwnerActorAuthoritative() || IsPublishing() || EquipmentOwner.IsValid()) return false;
	FUmbraDerivedStatsSnapshot Check;
	if (!Calculate(Profile ? Profile->Damage : UnarmedProfile, Check)) return false;
	Weapon = Profile;
	Refresh();
	return Snapshot.bValid;
}

void UUmbraDerivedStatsComponent::Refresh()
{
	if (bEquipmentUpdating) { bDirty = true; return; }
	if (bRefreshing) { bDirty = true; return; }
	UUmbraAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC || !ASC->IsOwnerActorAuthoritative() || !ASC->IsActorInfoReady()) return;
	TGuardValue<bool> RefreshGuard(bRefreshing, true);
	// Only primary changes can dirty this calculation; AD/AP are outputs, never inputs.
	int32 Pass = 0;
	do
	{
		bDirty = false;
		FUmbraDerivedStatsSnapshot Next;
		if (!Calculate(Weapon ? Weapon->Damage : UnarmedProfile, Next, WeaponBaseMultiplier, WeaponScalingMultiplier))
		{
			Snapshot.bValid = false;
			OnDerivedStatsChanged.Broadcast(Snapshot);
			UE_LOG(LogUmbra, Error, TEXT("Derived power evaluation failed; snapshot invalid, last GAS values retained."));
			return;
		}
		Next.bUnarmed = Weapon == nullptr;
		Next.Revision = Snapshot.Revision == MAX_int32 ? 1 : Snapshot.Revision + 1;
		{
			TGuardValue<bool> Guard(bPublishing, true);
			PublishingEffect = NewObject<UGameplayEffect>(this);
			PublishingEffect->DurationPolicy = EGameplayEffectDurationType::Instant;
			const auto Add = [this](FGameplayAttribute Attribute, float Value)
			{
				FGameplayModifierInfo& Modifier = PublishingEffect->Modifiers.AddDefaulted_GetRef();
				Modifier.Attribute = Attribute;
				Modifier.ModifierOp = EGameplayModOp::Override;
				Modifier.ModifierMagnitude = FScalableFloat(Value);
			};
			Add(UUmbraAttributeSet::GetAttackPowerAttribute(), Next.AttackPower);
			Add(UUmbraAttributeSet::GetAbilityPowerAttribute(), Next.AbilityPower);
			ASC->ApplyGameplayEffectToSelf(PublishingEffect, 1.f, ASC->MakeEffectContext());
			PublishingEffect = nullptr;
			Next.bValid = ASC->GetNumericAttribute(UUmbraAttributeSet::GetAttackPowerAttribute()) == Next.AttackPower
				&& ASC->GetNumericAttribute(UUmbraAttributeSet::GetAbilityPowerAttribute()) == Next.AbilityPower;
			Snapshot = MoveTemp(Next);
		}
		// A reentrant consumer may change a primary; bound the drain to avoid an infinite feedback loop.
		OnDerivedStatsChanged.Broadcast(Snapshot);
		++Pass;
	} while (bDirty && Pass < 8);
	if (bDirty)
	{
		Snapshot.bValid = false;
		UE_LOG(LogUmbra, Error, TEXT("Derived power feedback loop detected; snapshot invalid."));
		OnDerivedStatsChanged.Broadcast(Snapshot);
	}
}

bool UUmbraDerivedStatsComponent::SetEquipmentWeapon(UUmbraWeaponProfile* Profile, float BaseMultiplier, float ScalingMultiplier)
{
	if (!EquipmentOwner.IsValid() || !BoundASC.IsValid() || bRefreshing) return false;
	const float PreviousBase = WeaponBaseMultiplier, PreviousScaling = WeaponScalingMultiplier;
	WeaponBaseMultiplier = BaseMultiplier;
	WeaponScalingMultiplier = ScalingMultiplier;
	FUmbraDerivedStatsSnapshot Check;
	if (!Calculate(Profile ? Profile->Damage : UnarmedProfile, Check, BaseMultiplier, ScalingMultiplier))
	{
		WeaponBaseMultiplier = PreviousBase;
		WeaponScalingMultiplier = PreviousScaling;
		return false;
	}
	Weapon = Profile;
	return true;
}

void UUmbraDerivedStatsComponent::OnPrimaryChanged(const FOnAttributeChangeData& Data)
{
	if (auto* Equipment = EquipmentOwner.Get()) Equipment->Refresh();
	else Refresh();
}

bool UUmbraDerivedStatsComponent::CanApplyEffect(const FActiveGameplayEffectsContainer& Container, const FGameplayEffectSpec& Spec) const
{
	if (Spec.Def == PublishingEffect || !WritesPower(Spec)) return true;
	UE_LOG(LogUmbra, Warning, TEXT("Derived power rejects direct AD/AP GE modifiers: %s"), *GetNameSafe(Spec.Def));
	return false;
}

void UUmbraDerivedStatsComponent::Shutdown()
{
	if (UUmbraAbilitySystemComponent* ASC = BoundASC.Get())
	{
		for (const auto& Pair : PrimaryHandles) ASC->GetGameplayAttributeValueChangeDelegate(Pair.Key).Remove(Pair.Value);
		ASC->GameplayEffectApplicationQueries.RemoveAll([this](const FGameplayEffectApplicationQuery& Query)
			{ return Query.GetHandle() == ApplicationQueryHandle; });
	}
	PrimaryHandles.Reset();
	ApplicationQueryHandle.Reset();
	BoundASC.Reset();
	Snapshot.bValid = false;
}

void UUmbraDerivedStatsComponent::EndPlay(const EEndPlayReason::Type Reason) { Shutdown(); Super::EndPlay(Reason); }
void UUmbraDerivedStatsComponent::OnUnregister() { Shutdown(); Super::OnUnregister(); }
void UUmbraDerivedStatsComponent::OnRep_Snapshot() { OnDerivedStatsChanged.Broadcast(Snapshot); }
void UUmbraDerivedStatsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UUmbraDerivedStatsComponent, Snapshot);
}
