#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AbilitySystem/Damage/UmbraDamage.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Player/UmbraPlayerState.h"
#include "Engine/World.h"
#include "GameplayEffect.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraDamageEntryTest, "Umbra.Damage.EntryCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraDamageEntryTest::RunTest(const FString& Parameters)
{
	TSubclassOf<UGameplayEffect> Effect = LoadClass<UGameplayEffect>(nullptr,
		TEXT("/Game/Blueprints/Abilities/Effects/GE_Damage_PlayerBasic.GE_Damage_PlayerBasic_C"));
	if (!TestNotNull(TEXT("Existing damage asset loads without migration"), Effect.Get())) return false;
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	auto CreateASC = [World]()
	{
		auto* Owner = World->SpawnActor<AUmbraPlayerState>();
		auto* ASC = Owner->GetUmbraAbilitySystemComponent();
		ASC->InitializeComponent();
		ASC->InitAbilityActorInfo(Owner, Owner);
		ASC->InitializeAttributes(nullptr);
		return ASC;
	};
	auto* Source = CreateASC();
	auto* Target = CreateASC();
	Source->SetNumericAttributeBase(UUmbraAttributeSet::GetAttackPowerAttribute(), 20.f);
	Source->SetNumericAttributeBase(UUmbraAttributeSet::GetAbilityPowerAttribute(), 40.f);
	Source->SetNumericAttributeBase(UUmbraAttributeSet::GetCriticalDamageMultiplierAttribute(), 2.f);
	Target->SetNumericAttributeBase(UUmbraAttributeSet::GetArmorAttribute(), 100.f);
	Target->SetNumericAttributeBase(UUmbraAttributeSet::GetMagicResistanceAttribute(), 300.f);
	int32 Submissions = 0;
	int32 HealthChanges = 0;
	FGameplayTagContainer ObservedTags;
	float ObservedMarker = 0.f;
	float ObservedLevel = 0.f;
	const FDelegateHandle AppliedHandle = Target->OnGameplayEffectAppliedDelegateToSelf.AddLambda(
		[&](UAbilitySystemComponent*, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle)
		{
			++Submissions;
			ObservedTags = Spec.GetDynamicAssetTags();
			ObservedMarker = Spec.GetSetByCallerMagnitude(UmbraGameplayTags::Damage_SourcePrimaryAttack, false, 0.f);
			ObservedLevel = Spec.GetLevel();
		});
	const FDelegateHandle HealthHandle = Target->GetGameplayAttributeValueChangeDelegate(
		UUmbraAttributeSet::GetHealthAttribute()).AddLambda([&](const FOnAttributeChangeData&) { ++HealthChanges; });
	FUmbraDamageRequest Request(EUmbraDamageSource::BasicAttack);
	Request.EffectClass = Effect;
	Request.Config.AttackPowerCoefficient = 2.f;
	Request.Config.AbilityPowerCoefficient = 0.5f;
	Request.Level = 3.f;
	Request.bRecordPrimaryAttackDamage = true;
	auto Reset = [&]()
	{
		Target->SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(), 100.f);
		Submissions = HealthChanges = 0;
	};
	for (const EUmbraDamageType Type : { EUmbraDamageType::Physical, EUmbraDamageType::Magical })
	{
		Request.Config.DamageType = Type;
		for (const bool bCritical : { false, true })
		{
			Source->SetNumericAttributeBase(UUmbraAttributeSet::GetCriticalChanceAttribute(), bCritical ? 1.f : 0.f);
			Reset();
			TestTrue(TEXT("Legacy submits"), UmbraPhysicalDamage::Apply(Source, Target, Effect, Request.Config, Request.Level, true));
			const float LegacyHealth = Target->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute());
			const float ExpectedDamage = (Type == EUmbraDamageType::Physical ? 30.f : 15.f) * (bCritical ? 2.f : 1.f);
			TestEqual(TEXT("Legacy matches golden formula"), LegacyHealth, 100.f - ExpectedDamage);
			Reset();
			TestTrue(TEXT("Unified submits"), UmbraDamage::Apply(Source, Target, Request));
			TestEqual(TEXT("Unified preserves damage"), Target->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()), LegacyHealth);
			TestEqual(TEXT("One request submits one effect"), Submissions, 1);
			TestEqual(TEXT("One hit changes health once"), HealthChanges, 1);
			TestEqual(TEXT("IncomingDamage consumed"), Target->GetNumericAttribute(UUmbraAttributeSet::GetIncomingDamageAttribute()), 0.f);
			TestTrue(TEXT("Basic attack source tag"), ObservedTags.HasTagExact(UmbraGameplayTags::Damage_Source_BasicAttack));
			TestFalse(TEXT("Basic attack is not skill"), ObservedTags.HasTagExact(UmbraGameplayTags::Damage_Source_Skill));
			TestEqual(TEXT("Player measurement preserved"), ObservedMarker, 1.f);
			TestEqual(TEXT("Ability level preserved"), ObservedLevel, 3.f);
		}
	}
	Reset();
	Request.bRecordPrimaryAttackDamage = false;
	TestTrue(TEXT("Enemy-style basic attack submits"), UmbraDamage::Apply(Source, Target, Request));
	TestTrue(TEXT("Enemy is basic attack"), ObservedTags.HasTagExact(UmbraGameplayTags::Damage_Source_BasicAttack));
	TestEqual(TEXT("Enemy does not gain player measurement marker"), ObservedMarker, 0.f);
	Reset();
	Request.Source = EUmbraDamageSource::Skill;
	TestTrue(TEXT("Skill submits"), UmbraDamage::Apply(Source, Target, Request));
	TestTrue(TEXT("Skill source tag"), ObservedTags.HasTagExact(UmbraGameplayTags::Damage_Source_Skill));
	TestFalse(TEXT("Skill is not basic attack"), ObservedTags.HasTagExact(UmbraGameplayTags::Damage_Source_BasicAttack));
	TestEqual(TEXT("Skill has no player marker"), ObservedMarker, 0.f);
	Reset();
	TestTrue(TEXT("Old unspecified call submits"), UmbraPhysicalDamage::Apply(Source, Target, Effect, Request.Config, 1.f));
	TestTrue(TEXT("Old false does not invent a skill or basic source"), ObservedTags.IsEmpty());
	TestEqual(TEXT("Old false keeps measurement off"), ObservedMarker, 0.f);
	Reset();
	Request.Config.AttackPowerCoefficient = Request.Config.AbilityPowerCoefficient = 0.f;
	TestTrue(TEXT("Zero damage still submits"), UmbraDamage::Apply(Source, Target, Request));
	TestEqual(TEXT("Zero damage leaves health unchanged"), Target->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()), 100.f);
	TestEqual(TEXT("Zero damage clears meta attribute"), Target->GetNumericAttribute(UUmbraAttributeSet::GetIncomingDamageAttribute()), 0.f);
	Reset();
	TestFalse(TEXT("Missing source rejected"), UmbraDamage::Apply(nullptr, Target, Request));
	TestFalse(TEXT("Missing target rejected"), UmbraDamage::Apply(Source, nullptr, Request));
	Request.bRecordPrimaryAttackDamage = true;
	AddExpectedError(TEXT("invalid source classification"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Skill cannot claim primary marker"), UmbraDamage::Apply(Source, Target, Request));
	Request.bRecordPrimaryAttackDamage = false;
	Request.EffectClass = UGameplayEffect::StaticClass();
	AddExpectedError(TEXT("must be Instant, have no Modifiers"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Invalid GE rejected"), UmbraDamage::Apply(Source, Target, Request));
	TestEqual(TEXT("Rejected requests do not submit"), Submissions, 0);
	TestEqual(TEXT("Rejected requests do not change health"), HealthChanges, 0);
	Target->OnGameplayEffectAppliedDelegateToSelf.Remove(AppliedHandle);
	Target->GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetHealthAttribute()).Remove(HealthHandle);
	World->DestroyWorld(false);
	return true;
}
#endif
