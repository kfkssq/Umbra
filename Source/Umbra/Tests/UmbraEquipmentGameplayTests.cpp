#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/Damage/UmbraDamage.h"
#include "Equipment/UmbraEquipmentComponent.h"
#include "Equipment/UmbraEquipmentEffect.h"
#include "Stats/UmbraDerivedStatsComponent.h"
#include "Player/UmbraPlayerState.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraEquipmentTest, "Umbra.Equipment.RequirementsAndLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraEquipmentTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	auto* Owner = World->SpawnActor<AUmbraPlayerState>();
	auto* ASC = Owner->GetUmbraAbilitySystemComponent();
	ASC->InitializeComponent();
	ASC->InitAbilityActorInfo(Owner, Owner);
	Owner->InitializeAttributes();
	auto* Derived = Owner->FindComponentByClass<UUmbraDerivedStatsComponent>();
	auto* Equipment = Owner->FindComponentByClass<UUmbraEquipmentComponent>();
	TestFalse(TEXT("Equipment opt-in default off"), Equipment->bEnableEquipment);
	TestFalse(TEXT("Disabled has no valid placeholder"), Equipment->GetSnapshot().bValid);
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetStrengthAttribute(), 10.f);
	Derived->bUseWeaponDerivedPower = true;
	Equipment->bEnableEquipment = true;
	Owner->InitializeAttributes();
	TestTrue(TEXT("Enabled empty equipment valid"), Equipment->IsReadyForCombat());
	TestEqual(TEXT("Capacity 40 + 10*0.5"), Equipment->GetSnapshot().MaxEquipLoad, 45.f);
	auto* Sword = NewObject<UUmbraItemDefinition>(Owner);
	Sword->AllowedSlots = {EUmbraEquipmentSlot::MainHand};
	Sword->Weapon = NewObject<UUmbraWeaponProfile>(Sword);
	auto& Channel = Sword->Weapon->Damage.Channels.AddDefaulted_GetRef();
	Channel.BaseDamage = 100.f;
	Channel.Scaling.AddDefaulted_GetRef().Coefficient = 2.f;
	Sword->Requirements.Strength = 15.f;
	Sword->PrimaryBonuses.Strength = 10.f;
	Sword->Weight = 20.f;
	const FGuid SwordId = FGuid::NewGuid();
	TestEqual(TEXT("Reject wrong slot"), Equipment->Equip(EUmbraEquipmentSlot::Head, Sword, SwordId), EUmbraEquipResult::WrongSlot);
	Sword->RequiredLevel = 2;
	TestEqual(TEXT("Reject insufficient level"), Equipment->Equip(EUmbraEquipmentSlot::MainHand, Sword, SwordId), EUmbraEquipResult::LevelTooLow);
	Sword->RequiredLevel = 1;
	TestEqual(TEXT("Allow insufficient primaries"), Equipment->Equip(EUmbraEquipmentSlot::MainHand, Sword, SwordId), EUmbraEquipResult::Success);
	TestEqual(TEXT("Ordinary primary remains active"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetStrengthAttribute()), 20.f);
	TestFalse(TEXT("Own +10 cannot satisfy own 15 requirement"), Equipment->GetSnapshot().Items[0].bRequirementsMet);
	TestEqual(TEXT("Penalized base50, scaling0"), Derived->GetSnapshot().AttackPower, 50.f);
	TestEqual(TEXT("Worn weight only"), Equipment->GetSnapshot().EquipLoad, 20.f);
	TestEqual(TEXT("Capacity includes ordinary bonus"), Equipment->GetSnapshot().MaxEquipLoad, 50.f);
	TestTrue(TEXT("Medium load"), Equipment->GetSnapshot().LoadState == EUmbraLoadState::Medium);
	TestFalse(TEXT("Old profile API cannot bypass equipment"), Derived->SetWeaponProfile(nullptr));
	TestEqual(TEXT("Duplicate identity rejected"), Equipment->Equip(EUmbraEquipmentSlot::MainHand, Sword, SwordId), EUmbraEquipResult::DuplicateInstance);

	// A multiplier exposes the error in subtracting the item's raw +10 from the final value:
	// (10 + 10) * 1.4 = 28; subtracting 10 gives 18, but excluding its GE correctly gives 14.
	auto* Multiplier = NewObject<UGameplayEffect>(Owner);
	Multiplier->DurationPolicy = EGameplayEffectDurationType::Infinite;
	auto& Modifier = Multiplier->Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UUmbraAttributeSet::GetStrengthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Multiplicitive;
	Modifier.ModifierMagnitude = FScalableFloat(1.4f);
	const auto MultiplierHandle = ASC->ApplyGameplayEffectToSelf(Multiplier, 1.f, ASC->MakeEffectContext());
	TestFalse(TEXT("Self-exclusion respects multiplicative aggregation"), Equipment->GetSnapshot().Items[0].bRequirementsMet);
	TestEqual(TEXT("Still penalized under insufficient external multiplier"), Derived->GetSnapshot().AttackPower, 50.f);
	ASC->RemoveActiveGameplayEffect(MultiplierHandle);
	auto* Ring = NewObject<UUmbraItemDefinition>(Owner);
	Ring->AllowedSlots = {EUmbraEquipmentSlot::Ring1, EUmbraEquipmentSlot::Ring2};
	Ring->PrimaryBonuses.Strength = 5.f;
	Ring->Weight = 5.f;
	TestEqual(TEXT("Other item contribution accepted"), Equipment->Equip(EUmbraEquipmentSlot::Ring1, Ring, FGuid::NewGuid()), EUmbraEquipResult::Success);
	TestTrue(TEXT("Other item's +5 satisfies weapon"), Equipment->GetSnapshot().Items[0].bRequirementsMet);
	TestEqual(TEXT("Full base + all current primary scaling"), Derived->GetSnapshot().AttackPower, 150.f);
	for (int32 Index = 0; Index < 10; ++Index) Owner->InitializeAttributes();
	TestEqual(TEXT("Initialize does not duplicate effects"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetStrengthAttribute()), 25.f);
	TestEqual(TEXT("Only two equipped effects"), ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), 2);
	TestTrue(TEXT("Remove assisting ring"), Equipment->Unequip(EUmbraEquipmentSlot::Ring1));
	TestEqual(TEXT("Requirement penalty returns"), Derived->GetSnapshot().AttackPower, 50.f);

	auto* Armor = NewObject<UUmbraItemDefinition>(Owner);
	Armor->AllowedSlots = {EUmbraEquipmentSlot::Chest};
	Armor->Requirements.Strength = 30.f;
	Armor->PrimaryBonuses.Strength = 20.f;
	Armor->Armor = 100.f;
	Armor->MagicResistance = 40.f;
	Armor->Weight = 60.f;
	Equipment->Equip(EUmbraEquipmentSlot::Chest, Armor, FGuid::NewGuid());
	TestEqual(TEXT("Armor cannot qualify itself"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetArmorAttribute()), 50.f);
	TestEqual(TEXT("Intrinsic magic defense penalized"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetMagicResistanceAttribute()), 20.f);
	TestEqual(TEXT("Armor affix restores weapon scaling"), Derived->GetSnapshot().AttackPower, 180.f);
	TestEqual(TEXT("Equipped sum80"), Equipment->GetSnapshot().EquipLoad, 80.f);
	TestTrue(TEXT("Overweight state only"), Equipment->GetSnapshot().LoadState == EUmbraLoadState::Overweight);
	TestEqual(TEXT("No movement punishment"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetMoveSpeedAttribute()), 500.f);
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetStrengthAttribute(), 20.f);
	TestEqual(TEXT("External stat satisfies armor without self"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetArmorAttribute()), 100.f);
	TestEqual(TEXT("External stat refreshes full defense"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetMagicResistanceAttribute()), 40.f);

	FUmbraDamageRequest Hit(EUmbraDamageSource::BasicAttack);
	Hit.EffectClass = LoadClass<UGameplayEffect>(nullptr,
		TEXT("/Game/Blueprints/Abilities/Effects/GE_Damage_PlayerBasic.GE_Damage_PlayerBasic_C"));
	auto* OtherOwner = World->SpawnActor<AUmbraPlayerState>();
	auto* OtherASC = OtherOwner->GetUmbraAbilitySystemComponent();
	OtherASC->InitializeComponent();
	OtherASC->InitAbilityActorInfo(OtherOwner, OtherOwner);
	OtherOwner->InitializeAttributes();
	bool bHitDuringUpdate = true;
	bool bTargetHitDuringUpdate = true;
	const auto DuringUpdate = ASC->GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetStrengthAttribute())
		.AddLambda([&](const FOnAttributeChangeData&)
		{
			bHitDuringUpdate = UmbraDamage::Apply(ASC, OtherASC, Hit);
			bTargetHitDuringUpdate = UmbraDamage::Apply(OtherASC, ASC, Hit);
		});
	TestTrue(TEXT("Remove armor"), Equipment->Unequip(EUmbraEquipmentSlot::Chest));
	TestFalse(TEXT("Damage rejected during gear mutation"), bHitDuringUpdate);
	TestFalse(TEXT("Incoming damage rejected during target gear mutation"), bTargetHitDuringUpdate);
	ASC->GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetStrengthAttribute()).Remove(DuringUpdate);
	TestEqual(TEXT("No armor residue"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetArmorAttribute()), 0.f);
	TestEqual(TEXT("No magic defense residue"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetMagicResistanceAttribute()), 0.f);

	FGameplayEffectApplicationQuery Reject;
	Reject.BindLambda([](const FActiveGameplayEffectsContainer&, const FGameplayEffectSpec& Spec)
		{ return !Spec.Def->IsA<UUmbraEquipmentEffect>(); });
	const auto RejectHandle = Reject.GetHandle();
	ASC->GameplayEffectApplicationQueries.Add(Reject);
	TestEqual(TEXT("Rejected replacement preserves old gear"), Equipment->Equip(EUmbraEquipmentSlot::MainHand, Sword, FGuid::NewGuid()), EUmbraEquipResult::EffectRejected);
	TestEqual(TEXT("Old identity retained"), Equipment->GetSnapshot().Items[0].InstanceId, SwordId);
	TestEqual(TEXT("No bonus residue after rejected replacement"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetStrengthAttribute()), 30.f);
	ASC->GameplayEffectApplicationQueries.RemoveAll([RejectHandle](const FGameplayEffectApplicationQuery& Query) { return Query.GetHandle() == RejectHandle; });
	for (int32 Index = 0; Index < 10; ++Index)
		TestEqual(TEXT("Repeat replacement"), Equipment->Equip(EUmbraEquipmentSlot::MainHand, Sword, FGuid::NewGuid()), EUmbraEquipResult::Success);
	TestEqual(TEXT("Repeated replacement has one effect"), ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), 1);
	TestEqual(TEXT("Repeated replacement has one bonus"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetStrengthAttribute()), 30.f);
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(), 37.f);
	ASC->ClearActorInfo();
	ASC->InitAbilityActorInfo(Owner, Owner);
	Owner->InitializeAttributes();
	TestEqual(TEXT("Rebind retains equipped gear"), Equipment->GetSnapshot().Items.Num(), 1);
	TestEqual(TEXT("Rebind never refills health"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()), 37.f);
	TestTrue(TEXT("Unequip weapon"), Equipment->Unequip(EUmbraEquipmentSlot::MainHand));
	TestEqual(TEXT("Return to unarmed10"), Derived->GetSnapshot().AttackPower, 10.f);
	TestEqual(TEXT("Empty load0"), Equipment->GetSnapshot().EquipLoad, 0.f);
	TestEqual(TEXT("All bonuses removed"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetStrengthAttribute()), 20.f);
	// An external dispel must not leave a valid snapshot claiming the missing effect still exists.
	Equipment->Equip(EUmbraEquipmentSlot::MainHand, Sword, FGuid::NewGuid());
	AddExpectedError(TEXT("Equipment snapshot invalid"), EAutomationExpectedErrorFlags::Contains, 0);
	const auto EquippedEffects = ASC->GetActiveEffects(FGameplayEffectQuery());
	ASC->RemoveActiveGameplayEffect(EquippedEffects[0]);
	TestFalse(TEXT("External removal invalidates snapshot"), Equipment->GetSnapshot().bValid);
	TestFalse(TEXT("External removal blocks combat"), UmbraDamage::Apply(OtherASC, ASC, Hit));
	TestTrue(TEXT("Unequip stale entry recovers"), Equipment->Unequip(EUmbraEquipmentSlot::MainHand));
	TestTrue(TEXT("Recovered snapshot valid"), Equipment->IsReadyForCombat());
	Equipment->Equip(EUmbraEquipmentSlot::MainHand, Sword, FGuid::NewGuid());
	Equipment->DestroyComponent();
	TestEqual(TEXT("Destroy removes own GE"), ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), 0);
	TestEqual(TEXT("Destroy restores primary"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetStrengthAttribute()), 20.f);
	TestTrue(TEXT("Profile API available after owner destroyed"), Derived->SetWeaponProfile(nullptr));
	World->DestroyWorld(false);
	return true;
}
#endif
