#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/Damage/UmbraDamage.h"
#include "AbilitySystem/Damage/UmbraTypedDamageExecution.h"
#include "Equipment/UmbraEquipmentComponent.h"
#include "Stats/UmbraDerivedStatsComponent.h"
#include "Player/UmbraPlayerState.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Curves/CurveFloat.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraDamageChannelsIntegrationTest, "Umbra.Damage.TypedChannels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraDamageChannelsIntegrationTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	auto CreateOwner = [&]()
	{
		auto* Owner = World->SpawnActor<AUmbraPlayerState>();
		auto* ASC = Owner->GetUmbraAbilitySystemComponent();
		ASC->InitializeComponent();
		ASC->InitAbilityActorInfo(Owner, Owner);
		Owner->InitializeAttributes();
		return Owner;
	};
	auto* SourceOwner = CreateOwner();
	auto* TargetOwner = CreateOwner();
	auto* Source = SourceOwner->GetUmbraAbilitySystemComponent();
	auto* Target = TargetOwner->GetUmbraAbilitySystemComponent();
	Target->SetNumericAttributeBase(UUmbraAttributeSet::GetMaxHealthAttribute(), 10000.f);
	int32 Submissions = 0, HealthChanges = 0;
	float LastCritical = -1.f, LastDisplayType = -1.f, LastMarker = 0.f;
	FGameplayTagContainer LastTags;
	const auto AppliedHandle = Target->OnGameplayEffectAppliedDelegateToSelf.AddLambda(
		[&](UAbilitySystemComponent*, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle)
		{
			++Submissions;
			LastCritical = Spec.GetSetByCallerMagnitude(UmbraGameplayTags::Damage_ResultCritical, false, -1.f);
			LastDisplayType = Spec.GetSetByCallerMagnitude(UmbraGameplayTags::Damage_Type, false, -1.f);
			LastMarker = Spec.GetSetByCallerMagnitude(UmbraGameplayTags::Damage_SourcePrimaryAttack, false, 0.f);
			LastTags = Spec.GetDynamicAssetTags();
		});
	const auto HealthHandle = Target->GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetHealthAttribute())
		.AddLambda([&](const FOnAttributeChangeData&) { ++HealthChanges; });
	auto Reset = [&]()
	{
		Target->SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(), 10000.f);
		Submissions = HealthChanges = 0;
	};
	auto Lost = [&]() { return 10000.f - Target->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()); };
	FUmbraDamageRequest Hit(EUmbraDamageSource::BasicAttack);
	Hit.Config.Typed.Model = EUmbraDamageModel::WeaponChannels;
	Hit.bRecordPrimaryAttackDamage = true;
	Reset();
	AddExpectedError(TEXT("Typed damage: invalid configuration"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Weapon mode requires derived channels"), UmbraDamage::Apply(Source, Target, Hit));
	TestEqual(TEXT("Invalid request is not submitted"), Submissions, 0);
	Source->SetNumericAttributeBase(UUmbraAttributeSet::GetStrengthAttribute(), 10.f);
	auto* Derived = SourceOwner->FindComponentByClass<UUmbraDerivedStatsComponent>();
	Derived->bUseWeaponDerivedPower = true;
	Derived->Initialize(Source);
	auto* Weapon = NewObject<UUmbraWeaponProfile>(SourceOwner);
	FUmbraWeaponDamageChannel Slash;
	Slash.Type = EUmbraWeaponDamageType::Slashing; Slash.BaseDamage = 140.f;
	Slash.Scaling.AddDefaulted_GetRef().Coefficient = 2.f;
	FUmbraWeaponDamageChannel Fire;
	Fire.Type = EUmbraWeaponDamageType::Fire; Fire.BaseDamage = 70.f;
	Weapon->Damage.Channels = {Slash, Fire};
	TestTrue(TEXT("Mixed weapon configured"), Derived->SetWeaponProfile(Weapon));
	TestEqual(TEXT("Derived AD160"), Derived->GetSnapshot().AttackPower, 160.f);
	TestEqual(TEXT("Derived AP70"), Derived->GetSnapshot().AbilityPower, 70.f);
	Hit.Config.AbilityPowerCoefficient = 0.2f;
	TestTrue(TEXT("Native typed GE needs no BP effect"), UmbraDamage::Apply(Source, Target, Hit));
	TestEqual(TEXT("Full weapon inherits 160 + 70 exactly once"), Lost(), 230.f);
	TestEqual(TEXT("One mixed-hit spec"), Submissions, 1);
	TestEqual(TEXT("One mixed-hit health event"), HealthChanges, 1);
	TestEqual(TEXT("Meta damage cleared"), Target->GetNumericAttribute(UUmbraAttributeSet::GetIncomingDamageAttribute()), 0.f);
	TestEqual(TEXT("Player measurement marker preserved"), LastMarker, 1.f);
	TestTrue(TEXT("Basic source tag preserved"), LastTags.HasTagExact(UmbraGameplayTags::Damage_Source_BasicAttack));
	TestEqual(TEXT("Physical-dominant display"), LastDisplayType, 0.f);

	Target->SetNumericAttributeBase(UUmbraAttributeSet::GetArmorAttribute(), 100.f);
	Target->SetNumericAttributeBase(UUmbraAttributeSet::GetMagicResistanceAttribute(), 300.f);
	Target->SetNumericAttributeBase(UUmbraAttributeSet::GetSlashingResistanceAttribute(), 100.f);
	Target->SetNumericAttributeBase(UUmbraAttributeSet::GetFireResistanceAttribute(), 25.f);
	Reset();
	UmbraDamage::Apply(Source, Target, Hit);
	TestEqual(TEXT("160*0.5*0.7 + 70*0.25*0.8 = 70"), Lost(), 70.f);
	Source->SetNumericAttributeBase(UUmbraAttributeSet::GetCriticalChanceAttribute(), 1.f);
	Reset(); UmbraDamage::Apply(Source, Target, Hit);
	TestEqual(TEXT("One crit doubles all channels"), Lost(), 140.f);
	TestEqual(TEXT("Crit notification marker"), LastCritical, 1.f);
	Hit.Config.Typed.bCanCritical = false;
	Reset(); UmbraDamage::Apply(Source, Target, Hit);
	TestEqual(TEXT("Non-crittable attack ignores chance1"), Lost(), 70.f);
	TestEqual(TEXT("Noncrit notification marker"), LastCritical, 0.f);
	Hit.Config.Typed.WeaponMultiplier = 2.f;
	Reset(); UmbraDamage::Apply(Source, Target, Hit);
	TestEqual(TEXT("Weapon multiplier applied once"), Lost(), 140.f);

	FUmbraDamageRequest Skill(EUmbraDamageSource::Skill);
	Skill.Config.Typed.Model = EUmbraDamageModel::ExplicitChannels;
	Skill.Config.Typed.bCanCritical = false;
	FUmbraAttackDamageChannel SkillFire;
	SkillFire.Type = EUmbraWeaponDamageType::Fire;
	SkillFire.BaseDamage = 10.f; SkillFire.AttackPowerCoefficient = 0.5f; SkillFire.AbilityPowerCoefficient = 2.f;
	Skill.Config.Typed.Channels = {SkillFire};
	Reset(); TestTrue(TEXT("Explicit typed skill"), UmbraDamage::Apply(Source, Target, Skill));
	TestEqual(TEXT("(10 + 160*0.5 + 70*2) *0.25*0.8 =46, no implicit weapon"), Lost(), 46.f);
	TestTrue(TEXT("Skill classified independently"), LastTags.HasTagExact(UmbraGameplayTags::Damage_Source_Skill));
	TestFalse(TEXT("Skill has no basic tag"), LastTags.HasTagExact(UmbraGameplayTags::Damage_Source_BasicAttack));
	TestEqual(TEXT("Skill not player-primary measurement"), LastMarker, 0.f);
	TestEqual(TEXT("Magical display color"), LastDisplayType, 1.f);

	const TArray<FGameplayAttribute> ResistanceAttributes = {
		UUmbraAttributeSet::GetSlashingResistanceAttribute(), UUmbraAttributeSet::GetBluntResistanceAttribute(),
		UUmbraAttributeSet::GetPiercingResistanceAttribute(), UUmbraAttributeSet::GetFireResistanceAttribute(),
		UUmbraAttributeSet::GetLightningResistanceAttribute(), UUmbraAttributeSet::GetColdResistanceAttribute(),
		UUmbraAttributeSet::GetRadiantResistanceAttribute(), UUmbraAttributeSet::GetPoisonResistanceAttribute(),
		UUmbraAttributeSet::GetShadowResistanceAttribute()};
	for (const auto& Attribute : ResistanceAttributes) Target->SetNumericAttributeBase(Attribute, 0.f);
	for (int32 Index = 0; Index < 9; ++Index)
	{
		Skill.Config.Typed.Channels[0] = FUmbraAttackDamageChannel();
		Skill.Config.Typed.Channels[0].Type = EUmbraWeaponDamageType(Index);
		Skill.Config.Typed.Channels[0].BaseDamage = 100.f;
		Target->SetNumericAttributeBase(ResistanceAttributes[(Index + 1) % 9], 100000.f);
		Reset(); UmbraDamage::Apply(Source, Target, Skill);
		TestEqual(TEXT("Other type resistance never leaks"), Lost(), Index < 3 ? 50.f : 25.f);
		Target->SetNumericAttributeBase(ResistanceAttributes[Index], 100000.f);
		Reset(); UmbraDamage::Apply(Source, Target, Skill);
		TestEqual(TEXT("Own type correctly maps and caps30 percent"), Lost(), Index < 3 ? 35.f : 17.5f);
		Target->SetNumericAttributeBase(ResistanceAttributes[Index], -100.f);
		TestEqual(TEXT("Negative rating clamped"), Target->GetNumericAttribute(ResistanceAttributes[Index]), 0.f);
		Target->SetNumericAttributeBase(ResistanceAttributes[(Index + 1) % 9], 0.f);
	}
	auto* Rules = NewObject<UUmbraDamageRules>(SourceOwner);
	Rules->DefenseKByLevel = NewObject<UCurveFloat>(Rules);
	Rules->DefenseKByLevel->FloatCurve.AddKey(1.f, 100.f);
	Rules->DefenseKByLevel->FloatCurve.AddKey(2.f, 200.f);
	TargetOwner->FindComponentByClass<UUmbraEquipmentComponent>()->CharacterLevel = 2;
	Skill.Config.Typed.Rules = Rules;
	Skill.Config.Typed.Channels[0].Type = EUmbraWeaponDamageType::Blunt;
	Skill.Level = 99.f;
	Reset(); UmbraDamage::Apply(Source, Target, Skill);
	TestTrue(TEXT("Defense curve uses target level2, not skill99"), FMath::IsNearlyEqual(Lost(), 100.f * 200.f / 300.f, 0.001f));
	Rules->DefenseKByLevel = nullptr;
	Rules->DefenseK = 0.f;
	Reset();
	AddExpectedError(TEXT("Typed damage: invalid configuration"), EAutomationExpectedErrorFlags::Contains, 3);
	TestFalse(TEXT("Invalid K rejects entire request"), UmbraDamage::Apply(Source, Target, Skill));
	Rules->DefenseK = 100.f;
	const FUmbraAttackDamageChannel Duplicate = Skill.Config.Typed.Channels[0];
	Skill.Config.Typed.Channels.Add(Duplicate);
	TestFalse(TEXT("Duplicate type rejects"), UmbraDamage::Apply(Source, Target, Skill));
	Skill.Config.Typed.Channels.SetNum(1);
	Skill.Config.Typed.Channels[0].BaseDamage = -1.f;
	TestFalse(TEXT("Negative explicit damage rejects"), UmbraDamage::Apply(Source, Target, Skill));
	TestEqual(TEXT("No partial submissions for invalid input"), Submissions, 0);
	Skill.Config.Typed.Channels.Reset();
	Reset(); TestTrue(TEXT("Explicit zero hit valid"), UmbraDamage::Apply(Source, Target, Skill));
	TestEqual(TEXT("Zero hit loses no health"), Lost(), 0.f);
	TestEqual(TEXT("Zero hit still one spec"), Submissions, 1);

	// Migration preserves the exact previous 174 result until the ability opts into typed mode.
	Source->SetNumericAttributeBase(UUmbraAttributeSet::GetCriticalChanceAttribute(), 0.f);
	Target->SetNumericAttributeBase(UUmbraAttributeSet::GetArmorAttribute(), 0.f);
	FUmbraDamageRequest Legacy(EUmbraDamageSource::BasicAttack);
	Legacy.EffectClass = LoadClass<UGameplayEffect>(nullptr, TEXT("/Game/Blueprints/Abilities/Effects/GE_Damage_PlayerBasic.GE_Damage_PlayerBasic_C"));
	Legacy.Config.AbilityPowerCoefficient = 0.2f;
	Reset(); TestTrue(TEXT("Legacy path remains available"), UmbraDamage::Apply(Source, Target, Legacy));
	TestEqual(TEXT("Legacy 160+70*0.2 remains174"), Lost(), 174.f);
	Target->OnGameplayEffectAppliedDelegateToSelf.Remove(AppliedHandle);
	Target->GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetHealthAttribute()).Remove(HealthHandle);
	World->DestroyWorld(false);
	return true;
}
#endif
