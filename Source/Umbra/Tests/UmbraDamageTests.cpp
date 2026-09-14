#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/Damage/UmbraPhysicalDamageExecution.h"
#include "AbilitySystem/Damage/UmbraPhysicalDamage.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Player/UmbraPlayerState.h"
#include "Engine/World.h"
#include "GameplayEffect.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraTypedDamageTest, "Umbra.Damage.Types",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraTypedDamageTest::RunTest(const FString& Parameters)
{
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
	Source->SetNumericAttributeBase(UUmbraAttributeSet::GetCriticalChanceAttribute(), 1.f);
	Target->SetNumericAttributeBase(UUmbraAttributeSet::GetArmorAttribute(), 100.f);
	Target->SetNumericAttributeBase(UUmbraAttributeSet::GetMagicResistanceAttribute(), 300.f);
	auto Hit = [Source, Target](float Type, bool bCrit, float ADCoefficient = 2.f)
	{
		Target->SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(), 100.f);
		auto* GE = NewObject<UGameplayEffect>();
		GE->DurationPolicy = EGameplayEffectDurationType::Instant;
		GE->Executions.AddDefaulted_GetRef().CalculationClass = UUmbraPhysicalDamageExecution::StaticClass();
		FGameplayEffectSpec Spec(GE, Source->MakeEffectContext(), 1.f);
		if (Type != -1.f) Spec.SetSetByCallerMagnitude(UmbraGameplayTags::Damage_Type, Type);
		Spec.SetSetByCallerMagnitude(UmbraGameplayTags::Damage_AttackPowerCoefficient, ADCoefficient);
		Spec.SetSetByCallerMagnitude(UmbraGameplayTags::Damage_AbilityPowerCoefficient, 0.5f);
		Source->SetNumericAttributeBase(UUmbraAttributeSet::GetCriticalChanceAttribute(), bCrit ? 1.f : 0.f);
		Source->ApplyGameplayEffectSpecToTarget(Spec, Target);
		return Target->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute());
	};
	TestEqual(TEXT("Physical mixed scaling: 60 raw / 2"), Hit(0.f, false), 70.f);
	TestEqual(TEXT("Magical mixed scaling: 60 raw / 4"), Hit(1.f, false), 85.f);
	TestEqual(TEXT("Magical guaranteed crit doubles total"), Hit(1.f, true), 70.f);
	TestEqual(TEXT("Clamp the complete raw expression"), Hit(1.f, false, -100.f), 100.f);
	TestEqual(TEXT("IncomingDamage consumed once"), Target->GetNumericAttribute(UUmbraAttributeSet::GetIncomingDamageAttribute()), 0.f);
	AddExpectedError(TEXT("missing or invalid Damage.Type"), EAutomationExpectedErrorFlags::Contains, 2);
	TestEqual(TEXT("Missing type rejects hit"), Hit(-1.f, false), 100.f);
	TestEqual(TEXT("Unknown type rejects hit"), Hit(2.f, false), 100.f);
	FUmbraPhysicalDamageConfig Defaults;
	TestTrue(TEXT("Legacy default physical"), Defaults.DamageType == EUmbraDamageType::Physical);
	TestEqual(TEXT("Legacy default no AP scaling"), Defaults.AbilityPowerCoefficient, 0.f);
	World->DestroyWorld(false);
	return true;
}
#endif


