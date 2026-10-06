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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraEquipmentAffixTest, "Umbra.Equipment.FixedAffixesAndDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraEquipmentAffixTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	auto MakeOwner = [&]()
	{
		auto* Owner = World->SpawnActor<AUmbraPlayerState>();
		auto* ASC = Owner->GetUmbraAbilitySystemComponent();
		ASC->InitializeComponent(); ASC->InitAbilityActorInfo(Owner, Owner); Owner->InitializeAttributes();
		return Owner;
	};
	auto* Owner = MakeOwner();
	auto* ASC = Owner->GetUmbraAbilitySystemComponent();
	auto* Target = MakeOwner()->GetUmbraAbilitySystemComponent();
	auto* Derived = Owner->FindComponentByClass<UUmbraDerivedStatsComponent>();
	auto* Equipment = Owner->FindComponentByClass<UUmbraEquipmentComponent>();
	Derived->bUseWeaponDerivedPower = true;
	Equipment->bEnableEquipment = true;
	Owner->InitializeAttributes();
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetCriticalChanceAttribute(), 0.f);
	Target->SetNumericAttributeBase(UUmbraAttributeSet::GetMaxHealthAttribute(), 10000.f);
	const float InitialMaxHealth = ASC->GetNumericAttribute(UUmbraAttributeSet::GetMaxHealthAttribute());
	const float InitialHealth = ASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute());
	const float InitialSpeed = ASC->GetNumericAttribute(UUmbraAttributeSet::GetAttackSpeedAttribute());
	auto* Ring = NewObject<UUmbraItemDefinition>(Owner);
	Ring->AllowedSlots = {EUmbraEquipmentSlot::Ring1, EUmbraEquipmentSlot::Ring2};
	auto AddAffix = [](UUmbraItemDefinition* Item, EUmbraEquipmentAffixStat Stat, float Value)
	{
		auto& Entry = Item->AttributeBonuses.AddDefaulted_GetRef(); Entry.Stat = Stat; Entry.Magnitude = Value;
	};
	AddAffix(Ring, EUmbraEquipmentAffixStat::MaxHealth, 100.f);
	AddAffix(Ring, EUmbraEquipmentAffixStat::AttackSpeed, 0.2f);
	AddAffix(Ring, EUmbraEquipmentAffixStat::Armor, 20.f);
	Ring->Armor = 100.f;
	Ring->Requirements.Strength = 1000.f;
	FUmbraDamageBonus All, X;
	All.Magnitude = FScalableFloat(0.1f);
	X.Bucket = EUmbraDamageBucket::Multiplicative; X.Magnitude = FScalableFloat(1.2f);
	Ring->DamageBonuses = {All, X};
	FUmbraDamageRequest Hit(EUmbraDamageSource::BasicAttack);
	Hit.Config.Typed.Model = EUmbraDamageModel::ExplicitChannels;
	Hit.Config.Typed.bUseDamageBuckets = true;
	auto& Channel = Hit.Config.Typed.Channels.AddDefaulted_GetRef();
	Channel.Type = EUmbraWeaponDamageType::Slashing; Channel.BaseDamage = 100.f;
	auto CheckDamage = [&](float Expected)
	{
		Target->SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(), 10000.f);
		TestTrue(TEXT("Equipment hit accepted"), UmbraDamage::Apply(ASC, Target, Hit));
		TestTrue(TEXT("Equipment A/X changes actual health loss"), FMath::IsNearlyEqual(
			10000.f - Target->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()), Expected, 0.002f));
	};
	TestEqual(TEXT("Fixed affixes equip"), Equipment->Equip(EUmbraEquipmentSlot::Ring1, Ring, FGuid::NewGuid()), EUmbraEquipResult::Success);
	TestEqual(TEXT("One GE owns attributes and A/X"), ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), 1);
	TestEqual(TEXT("MaxHealth additive"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetMaxHealthAttribute()), InitialMaxHealth + 100.f);
	TestEqual(TEXT("Equipping does not heal"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()), InitialHealth);
	TestTrue(TEXT("AttackSpeed uses multiplier units"), FMath::IsNearlyEqual(ASC->GetNumericAttribute(UUmbraAttributeSet::GetAttackSpeedAttribute()), InitialSpeed + 0.2f));
	TestEqual(TEXT("Intrinsic defense penalized, fixed armor not penalized"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetArmorAttribute()), 70.f);
	CheckDamage(132.f);
	UmbraDamageBonuses::FPanelSummary Panel;
	TestTrue(TEXT("Panel reads equipment source asset"), UmbraDamageBonuses::ReadPanelSummary(ASC, Panel));
	TestTrue(TEXT("Panel A10%"), FMath::IsNearlyEqual(Panel.All, 0.1, 0.0001));
	TestTrue(TEXT("Panel flags separate X"), Panel.bHasMultiplicative);

	auto* Other = NewObject<UUmbraItemDefinition>(Owner);
	Other->AllowedSlots = Ring->AllowedSlots;
	FUmbraDamageBonus Slash; Slash.Magnitude = FScalableFloat(0.2f); Slash.Types = {EUmbraWeaponDamageType::Slashing};
	Other->DamageBonuses = {Slash};
	TestEqual(TEXT("Second definition uses same native GE independently"), Equipment->Equip(EUmbraEquipmentSlot::Ring2, Other, FGuid::NewGuid()), EUmbraEquipResult::Success);
	CheckDamage(156.f);
	for (int32 Index = 0; Index < 5; ++Index)
		TestEqual(TEXT("Repeat replacement succeeds"), Equipment->Equip(EUmbraEquipmentSlot::Ring1, Ring, FGuid::NewGuid()), EUmbraEquipResult::Success);
	TestEqual(TEXT("Replacement leaves exactly two effects"), ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), 2);
	CheckDamage(156.f);

	auto* Bad = NewObject<UUmbraItemDefinition>(Owner);
	Bad->AllowedSlots = Ring->AllowedSlots;
	AddAffix(Bad, EUmbraEquipmentAffixStat::Armor, 2.f);
	AddAffix(Bad, EUmbraEquipmentAffixStat::Armor, 3.f);
	TestEqual(TEXT("Duplicate affix rejected before replacing old equipment"), Equipment->Equip(EUmbraEquipmentSlot::Ring1, Bad, FGuid::NewGuid()), EUmbraEquipResult::InvalidItem);
	Bad->AttributeBonuses.Reset(); X.Magnitude = FScalableFloat(-1.f); Bad->DamageBonuses = {X};
	TestEqual(TEXT("Invalid X rejected before mutation"), Equipment->Equip(EUmbraEquipmentSlot::Ring1, Bad, FGuid::NewGuid()), EUmbraEquipResult::InvalidItem);
	CheckDamage(156.f);
	TestTrue(TEXT("Unequip first"), Equipment->Unequip(EUmbraEquipmentSlot::Ring1));
	CheckDamage(120.f);
	TestEqual(TEXT("Health capacity restored"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetMaxHealthAttribute()), InitialMaxHealth);
	TestEqual(TEXT("Armor fully removed"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetArmorAttribute()), 0.f);
	TestEqual(TEXT("Attack speed restored"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetAttackSpeedAttribute()), InitialSpeed);
	Equipment->DestroyComponent();
	TestEqual(TEXT("Shutdown removes all effects"), ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), 0);
	TestTrue(TEXT("Panel after shutdown valid"), UmbraDamageBonuses::ReadPanelSummary(ASC, Panel));
	TestEqual(TEXT("No additive residue"), Panel.All, 0.);
	TestEqual(TEXT("No typed residue"), Panel.Types[0], 0.);
	TestFalse(TEXT("No multiplicative residue"), Panel.bHasMultiplicative);
	World->DestroyWorld(false);
	return true;
}
#endif
