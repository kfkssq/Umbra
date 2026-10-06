#include "Equipment/UmbraEquipmentComponent.h"
#include "Equipment/UmbraEquipmentEffect.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "Stats/UmbraDerivedStatsComponent.h"
#include "Net/UnrealNetwork.h"
#include "Umbra.h"

namespace
{
	bool Nonnegative(float Value) { return FMath::IsFinite(Value) && Value >= 0.f; }
	bool Ratio(float Value) { return Nonnegative(Value) && Value <= 1.f; }
	bool ValidSlot(EUmbraEquipmentSlot Slot) { return uint8(Slot) <= uint8(EUmbraEquipmentSlot::OffHand); }
}

UUmbraEquipmentComponent::UUmbraEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UUmbraEquipmentComponent::Initialize(UUmbraAbilitySystemComponent* ASC)
{
	if (!bEnableEquipment || !ASC || ASC->GetOwnerActor() != GetOwner()
		|| !ASC->IsOwnerActorAuthoritative() || !ASC->IsActorInfoReady()) return;
	if (BoundASC.Get() == ASC) { Refresh(); return; }
	auto* Power = GetOwner()->FindComponentByClass<UUmbraDerivedStatsComponent>();
	if (!Power || !Power->IsDerivedPowerActive() || Power->IsPublishing() || Power->EquipmentOwner.IsValid()
		|| CharacterLevel < 1 || !Ratio(UnmetWeaponBaseMultiplier) || !Ratio(UnmetWeaponScalingMultiplier)
		|| !Ratio(UnmetDefenseMultiplier) || !Nonnegative(BaseCapacity) || !Nonnegative(CapacityPerStrength)
		|| !Ratio(LightThreshold) || !Ratio(MediumThreshold) || LightThreshold > MediumThreshold)
	{
		UE_LOG(LogUmbra, Error, TEXT("Equipment requires active derived stats and valid startup settings."));
		return;
	}
	BoundASC = ASC;
	Derived = Power;
	Power->EquipmentOwner = this;
	RemovedHandle = ASC->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &ThisClass::OnEffectRemoved);
	Refresh();
}

bool UUmbraEquipmentComponent::CanMutate() const
{
	return BoundASC.IsValid() && BoundASC->IsOwnerActorAuthoritative() && BoundASC->IsActorInfoReady()
		&& Derived.IsValid() && Derived->IsDerivedPowerActive() && !Derived->IsPublishing() && !bUpdating;
}

bool UUmbraEquipmentComponent::IsReadyForCombat() const
{
	return !bUpdating && BoundASC.IsValid() && Snapshot.bValid;
}

bool UUmbraEquipmentComponent::ValidateItem(const UUmbraItemDefinition* Item) const
{
	if (!IsValid(Item) || Item->RequiredLevel < 1 || Item->AllowedSlots.IsEmpty()
		|| !Nonnegative(Item->Armor) || !Nonnegative(Item->MagicResistance) || !Nonnegative(Item->Weight)) return false;
	for (const auto Slot : Item->AllowedSlots)
		if (!ValidSlot(Slot) || (Item->Weapon && Slot != EUmbraEquipmentSlot::MainHand)) return false;
	for (const float Value : Item->Requirements.ToArray()) if (!Nonnegative(Value)) return false;
	for (const float Value : Item->PrimaryBonuses.ToArray()) if (!Nonnegative(Value)) return false;
	if (!UUmbraEquipmentEffect::ValidateBonuses(Item)) return false;
	if (Item->Weapon)
	{
		FUmbraDerivedStatsSnapshot Check;
		if (!Derived->Calculate(Item->Weapon->Damage, Check)) return false;
	}
	return true;
}

EUmbraEquipResult UUmbraEquipmentComponent::Equip(EUmbraEquipmentSlot Slot, UUmbraItemDefinition* Item, FGuid InstanceId)
{
	if (!CanMutate()) return EUmbraEquipResult::NotReady;
	if (!InstanceId.IsValid() || !ValidateItem(Item)) return EUmbraEquipResult::InvalidItem;
	if (!ValidSlot(Slot) || !Item->AllowedSlots.Contains(Slot)) return EUmbraEquipResult::WrongSlot;
	if (CharacterLevel < Item->RequiredLevel) return EUmbraEquipResult::LevelTooLow;
	for (const auto& Existing : Snapshot.Items)
		if (Existing.InstanceId == InstanceId) return EUmbraEquipResult::DuplicateInstance;

	// Keep the old handle until the incoming GE is accepted. No partial replacement on rejection.
	bUpdating = true;
	Derived->bEquipmentUpdating = true;
	auto Context = BoundASC->MakeEffectContext();
	Context.AddSourceObject(Item);
	const auto Spec = BoundASC->MakeOutgoingSpec(UUmbraEquipmentEffect::StaticClass(), 1.f, Context);
	const auto Tags = UUmbraEquipmentEffect::MagnitudeTags();
	const auto Bonuses = Item->PrimaryBonuses.ToArray();
	for (const auto Tag : Tags) Spec.Data->SetSetByCallerMagnitude(Tag, 0.f);
	for (int32 Index = 0; Index < 4; ++Index) Spec.Data->SetSetByCallerMagnitude(Tags[Index], Bonuses[Index]);
	// Intrinsic defense is filled only after excluding the new handle for requirements.
	for (const auto& Affix : Item->AttributeBonuses)
		Spec.Data->SetSetByCallerMagnitude(Tags[UUmbraEquipmentEffect::AffixIndex(Affix.Stat)], Affix.Magnitude);
	const auto NewHandle = BoundASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	if (!BoundASC->GetActiveGameplayEffect(NewHandle))
	{
		Derived->bEquipmentUpdating = false;
		bUpdating = false;
		Refresh();
		return EUmbraEquipResult::EffectRejected;
	}
	const auto OldHandle = Handles.FindRef(Slot);
	Handles.Add(Slot, NewHandle);
	Snapshot.Items.RemoveAll([Slot](const FUmbraEquippedItem& Entry) { return Entry.Slot == Slot; });
	FUmbraEquippedItem& Entry = Snapshot.Items.AddDefaulted_GetRef();
	Entry.Slot = Slot;
	Entry.InstanceId = InstanceId;
	Entry.Definition = Item;
	if (OldHandle.IsValid()) BoundASC->RemoveActiveGameplayEffect(OldHandle);
	Derived->bEquipmentUpdating = false;
	bUpdating = false;
	Refresh();
	return Snapshot.bValid ? EUmbraEquipResult::Success : EUmbraEquipResult::AppliedInvalid;
}

EUmbraUnequipResult UUmbraEquipmentComponent::TryUnequipInstance(EUmbraEquipmentSlot Slot, FGuid ExpectedInstanceId, FUmbraEquippedItem& RemovedItem)
{
	RemovedItem = FUmbraEquippedItem();
	if (!GetOwner() || !GetOwner()->HasAuthority()) return EUmbraUnequipResult::AuthorityRequired;
	if (!CanMutate()) return EUmbraUnequipResult::NotReady;
	if (!ValidSlot(Slot)) return EUmbraUnequipResult::InvalidSlot;
	const auto* Entry = Snapshot.Items.FindByPredicate([Slot](const FUmbraEquippedItem& Item) { return Item.Slot == Slot; });
	if (!Entry) return EUmbraUnequipResult::EmptySlot;
	if (!ExpectedInstanceId.IsValid() || Entry->InstanceId != ExpectedInstanceId) return EUmbraUnequipResult::StaleInstance;
	const FUmbraEquippedItem Previous = *Entry;
	if (!Unequip(Slot)) return EUmbraUnequipResult::Failed;
	RemovedItem = Previous;
	return Snapshot.bValid ? EUmbraUnequipResult::Success : EUmbraUnequipResult::AppliedInvalid;
}

bool UUmbraEquipmentComponent::Unequip(EUmbraEquipmentSlot Slot)
{
	if (!CanMutate() || !Handles.Contains(Slot)) return false;
	bUpdating = true;
	Derived->bEquipmentUpdating = true;
	const auto Handle = Handles.FindAndRemoveChecked(Slot);
	Snapshot.Items.RemoveAll([Slot](const FUmbraEquippedItem& Entry) { return Entry.Slot == Slot; });
	BoundASC->RemoveActiveGameplayEffect(Handle);
	Derived->bEquipmentUpdating = false;
	bUpdating = false;
	Refresh();
	return true;
}

void UUmbraEquipmentComponent::Refresh()
{
	if (bUpdating) { bDirty = true; return; }
	if (!CanMutate()) return;
	TGuardValue<bool> Guard(bUpdating, true);
	const auto Attributes = UUmbraEquipmentEffect::Attributes();
	const auto Tags = UUmbraEquipmentEffect::MagnitudeTags();
	int32 Pass = 0;
	do
	{
		bDirty = false;
		Derived->bEquipmentUpdating = true;
		Snapshot.bValid = true;
		double Weight = 0.;
		UUmbraWeaponProfile* Weapon = nullptr;
		float BaseMultiplier = 1.f, ScalingMultiplier = 1.f;
		for (auto& Entry : Snapshot.Items)
		{
			const auto Handle = Handles.FindRef(Entry.Slot);
			const auto* Effect = BoundASC->GetActiveGameplayEffect(Handle);
			if (!Effect || !ValidateItem(Entry.Definition)) { Snapshot.bValid = false; continue; }
			const auto* Item = Entry.Definition.Get();
			FUmbraEquipmentRequirementsPreview Requirements;
			Entry.bRequirementsMet = ReadRequirements(Item, Handle, Requirements) && Requirements.bPrimariesMet;
			const float DefenseMultiplier = Entry.bRequirementsMet ? 1.f : UnmetDefenseMultiplier;
			TMap<FGameplayTag, float> Defense;
			Defense.Add(Tags[4], Item->Armor * DefenseMultiplier);
			Defense.Add(Tags[5], Item->MagicResistance * DefenseMultiplier);
			// Requirement penalties affect intrinsic defenses, never fixed affixes.
			for (const auto& Affix : Item->AttributeBonuses)
			{
				if (Affix.Stat == EUmbraEquipmentAffixStat::Armor) Defense[Tags[4]] += Affix.Magnitude;
				if (Affix.Stat == EUmbraEquipmentAffixStat::MagicResistance) Defense[Tags[5]] += Affix.Magnitude;
			}
			// Avoid dirtying GAS for an unchanged requirement result.
			if (Effect->Spec.GetSetByCallerMagnitude(Tags[4], false) != Defense[Tags[4]]
				|| Effect->Spec.GetSetByCallerMagnitude(Tags[5], false) != Defense[Tags[5]])
				BoundASC->UpdateActiveGameplayEffectSetByCallerMagnitudes(Handle, Defense);
			Weight += Item->Weight;
			if (Entry.Slot == EUmbraEquipmentSlot::MainHand && Item->Weapon)
			{
				Weapon = Item->Weapon;
				BaseMultiplier = Entry.bRequirementsMet ? 1.f : UnmetWeaponBaseMultiplier;
				ScalingMultiplier = Entry.bRequirementsMet ? 1.f : UnmetWeaponScalingMultiplier;
			}
		}
		const double Capacity = double(BaseCapacity) + double(CapacityPerStrength) * BoundASC->GetNumericAttribute(Attributes[0]);
		Snapshot.bValid &= FMath::IsFinite(Capacity) && Capacity >= 0. && Capacity <= MAX_flt && Weight <= MAX_flt;
		if (Snapshot.bValid)
		{
			Snapshot.EquipLoad = float(Weight);
			Snapshot.MaxEquipLoad = float(Capacity);
			Snapshot.LoadState = Weight > Capacity ? EUmbraLoadState::Overweight
				: Weight <= Capacity * LightThreshold ? EUmbraLoadState::Light
				: Weight <= Capacity * MediumThreshold ? EUmbraLoadState::Medium : EUmbraLoadState::Heavy;
			Snapshot.bValid = Derived->SetEquipmentWeapon(Weapon, BaseMultiplier, ScalingMultiplier);
		}
		Derived->bEquipmentUpdating = false;
		if (Snapshot.bValid) Derived->Refresh();
		Snapshot.bValid &= Derived->GetSnapshot().bValid;
		Snapshot.Revision = Snapshot.Revision == MAX_int32 ? 1 : Snapshot.Revision + 1;
		// Mutations are rejected inside this notification; primary changes request another pass.
		OnEquipmentChanged.Broadcast(Snapshot);
		++Pass;
	} while (bDirty && Pass < 8);
	if (bDirty) Snapshot.bValid = false;
	if (!Snapshot.bValid) UE_LOG(LogUmbra, Error, TEXT("Equipment snapshot invalid; combat blocked until inputs recover."));
	if (bDirty) OnEquipmentChanged.Broadcast(Snapshot);
}

void UUmbraEquipmentComponent::OnEffectRemoved(const FActiveGameplayEffect& Effect)
{
	for (const auto& Pair : Handles)
		if (Pair.Value == Effect.Handle) { Refresh(); return; }
}

bool UUmbraEquipmentComponent::ReadRequirements(const UUmbraItemDefinition* Item,
	FActiveGameplayEffectHandle Excluded, FUmbraEquipmentRequirementsPreview& Out) const
{
	Out = FUmbraEquipmentRequirementsPreview();
	if (!IsValid(Item) || !BoundASC.IsValid() || !BoundASC->IsActorInfoReady()) return false;
	FGameplayTagContainer OwnedTags;
	BoundASC->GetOwnedGameplayTags(OwnedTags);
	TArray<FActiveGameplayEffectHandle> Ignore;
	if (Excluded.IsValid()) Ignore.Add(Excluded);
	const auto Attributes = UUmbraEquipmentEffect::Attributes();
	const auto Required = Item->Requirements.ToArray();
	Out.CurrentLevel = CharacterLevel;
	Out.bLevelMet = CharacterLevel >= Item->RequiredLevel;
	Out.bPrimariesMet = true;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const float Value = BoundASC->GetFilteredAttributeValue(Attributes[Index], FGameplayTagRequirements(), OwnedTags, Ignore);
		if (!FMath::IsFinite(Value)) { Out = FUmbraEquipmentRequirementsPreview(); return false; }
		// Filtered aggregators bypass the AttributeSet's nonnegative clamp.
		Out.CurrentPrimaries.Add(FMath::Max(0.f, Value));
		Out.PrimaryMet.Add(Out.CurrentPrimaries.Last() >= Required[Index]);
		Out.bPrimariesMet &= Out.PrimaryMet.Last();
	}
	Out.bKnown = true;
	Out.WeaponBaseMultiplier = Out.bPrimariesMet ? 1.f : UnmetWeaponBaseMultiplier;
	Out.WeaponScalingMultiplier = Out.bPrimariesMet ? 1.f : UnmetWeaponScalingMultiplier;
	Out.DefenseMultiplier = Out.bPrimariesMet ? 1.f : UnmetDefenseMultiplier;
	return true;
}

bool UUmbraEquipmentComponent::QueryRequirements(const UUmbraItemDefinition* Item, bool bHasTargetSlot,
	EUmbraEquipmentSlot TargetSlot, FUmbraEquipmentRequirementsPreview& Out) const
{
	Out = FUmbraEquipmentRequirementsPreview();
	if (!IsReadyForCombat() || !ValidateItem(Item)
		|| (bHasTargetSlot && (!ValidSlot(TargetSlot) || !Item->AllowedSlots.Contains(TargetSlot)))) return false;
	return ReadRequirements(Item, bHasTargetSlot ? Handles.FindRef(TargetSlot) : FActiveGameplayEffectHandle(), Out);
}

void UUmbraEquipmentComponent::Shutdown()
{
	bUpdating = true;
	if (Derived.IsValid()) Derived->bEquipmentUpdating = true;
	if (auto* ASC = BoundASC.Get())
	{
		ASC->OnAnyGameplayEffectRemovedDelegate().Remove(RemovedHandle);
		for (const auto& Pair : Handles) ASC->RemoveActiveGameplayEffect(Pair.Value);
	}
	Handles.Reset();
	Snapshot = FUmbraEquipmentSnapshot();
	if (Derived.IsValid() && Derived->EquipmentOwner.Get() == this)
	{
		Derived->SetEquipmentWeapon(nullptr, 1.f, 1.f);
		Derived->EquipmentOwner.Reset();
		Derived->bEquipmentUpdating = false;
		Derived->Refresh();
	}
	Derived.Reset();
	BoundASC.Reset();
	bUpdating = false;
	// Views must discard stale equipment even if the component is removed without a pawn change.
	OnEquipmentChanged.Broadcast(Snapshot);
}
void UUmbraEquipmentComponent::EndPlay(const EEndPlayReason::Type Reason) { Shutdown(); Super::EndPlay(Reason); }
void UUmbraEquipmentComponent::OnUnregister() { Shutdown(); Super::OnUnregister(); }
void UUmbraEquipmentComponent::OnRep_Snapshot() { OnEquipmentChanged.Broadcast(Snapshot); }
void UUmbraEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UUmbraEquipmentComponent, Snapshot);
}
