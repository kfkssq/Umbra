#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/Damage/UmbraDamage.h"
#include "AbilitySystem/Damage/UmbraDamageBonusComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Player/UmbraPlayerState.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraDamageBucketsIntegrationTest, "Umbra.Damage.BucketsAndMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraDamageBucketsIntegrationTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	auto CreateASC = [&]()
	{
		auto* Owner = World->SpawnActor<AUmbraPlayerState>();
		auto* ASC = Owner->GetUmbraAbilitySystemComponent();
		ASC->InitializeComponent(); ASC->InitAbilityActorInfo(Owner, Owner); Owner->InitializeAttributes();
		return ASC;
	};
	auto* Source = CreateASC();
	auto* Target = CreateASC();
	Target->SetNumericAttributeBase(UUmbraAttributeSet::GetMaxHealthAttribute(), 10000.f);
	Source->SetNumericAttributeBase(UUmbraAttributeSet::GetCriticalChanceAttribute(), 1.f);
	Source->SetNumericAttributeBase(UUmbraAttributeSet::GetAttackPowerAttribute(), 100.f);
	Source->SetNumericAttributeBase(UUmbraAttributeSet::GetCriticalDamageMultiplierAttribute(), 9.f);
	FUmbraDamageRequest Hit(EUmbraDamageSource::BasicAttack);
	Hit.Config.Typed.Model = EUmbraDamageModel::ExplicitChannels;
	Hit.Config.Typed.bUseDamageBuckets = true;
	FUmbraAttackDamageChannel Slash, Fire;
	Slash.Type = EUmbraWeaponDamageType::Slashing; Slash.BaseDamage = 100.f;
	Fire.Type = EUmbraWeaponDamageType::Fire; Fire.BaseDamage = 50.f;
	Hit.Config.Typed.Channels = {Slash, Fire};
	int32 HealthChanges = 0;
	const auto HealthHandle = Target->GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetHealthAttribute())
		.AddLambda([&](const FOnAttributeChangeData&) { ++HealthChanges; });
	auto Check = [&](float Expected, const TCHAR* Label)
	{
		Target->SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(), 10000.f);
		HealthChanges = 0;
		TestTrue(Label, UmbraDamage::Apply(Source, Target, Hit));
		const float Actual = 10000.f - Target->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute());
		TestTrue(FString::Printf(TEXT("%s: got %g expected %g"), Label, Actual, Expected), FMath::IsNearlyEqual(Actual, Expected, 0.002f));
		TestEqual(TEXT("One health notification for mixed hit"), HealthChanges, Expected > 0.f ? 1 : 0);
		TestEqual(TEXT("Meta damage reset"), Target->GetNumericAttribute(UUmbraAttributeSet::GetIncomingDamageAttribute()), 0.f);
	};
	auto Bonus = [](float Value)
	{
		FUmbraDamageBonus Result; Result.Magnitude = FScalableFloat(Value); return Result;
	};
	auto MakeGE = [&](const TArray<FUmbraDamageBonus>& Entries)
	{
		auto* GE = NewObject<UGameplayEffect>(Source);
		GE->DurationPolicy = EGameplayEffectDurationType::Infinite;
		GE->AddComponent<UUmbraDamageBonusComponent>().Bonuses = Entries;
		return GE;
	};
	auto Apply = [&](UGameplayEffect* GE) { return Source->ApplyGameplayEffectToSelf(GE, 1.f, Source->MakeEffectContext()); };
	FUmbraDamageBonus All = Bonus(0.1f), Slashing = Bonus(0.2f), Crit = Bonus(0.3f), Vulnerable = Bonus(0.2f);
	Slashing.Types = {EUmbraWeaponDamageType::Slashing};
	Crit.bRequiresCritical = true; Vulnerable.bRequiresVulnerable = true;
	const auto AHandle = Apply(MakeGE({All, Slashing, Crit, Vulnerable}));
	TestTrue(TEXT("A GE accepted"), AHandle.IsValid());
	Check(345.f, TEXT("Crit only: slash100*1.6*1.5 + fire50*1.4*1.5; old crit9 ignored"));
	Source->SetNumericAttributeBase(UUmbraAttributeSet::GetCriticalChanceAttribute(), 0.f);
	Check(185.f, TEXT("Noncrit excludes critical A"));
	auto* VulnerableGE = NewObject<UGameplayEffect>(Target);
	VulnerableGE->DurationPolicy = EGameplayEffectDurationType::Infinite;
	FInheritedTagContainer Granted;
	Granted.Added.AddTag(UmbraGameplayTags::State_Vulnerable);
	Granted.UpdateInheritedTagProperties(nullptr);
	VulnerableGE->AddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(Granted);
	const auto VHandle = Target->ApplyGameplayEffectToSelf(VulnerableGE, 1.f, Target->MakeEffectContext());
	TestTrue(TEXT("Vulnerability granted by GE"), Target->HasMatchingGameplayTag(UmbraGameplayTags::State_Vulnerable));
	Check(258.f, TEXT("Vulnerable without crit: slash100*1.5*1.2 + fire50*1.3*1.2"));
	Source->SetNumericAttributeBase(UUmbraAttributeSet::GetCriticalChanceAttribute(), 1.f);
	Check(468.f, TEXT("Both conditions: slash324 + fire144"));
	FUmbraDamageBonus Basic = Bonus(0.4f), Skill = Bonus(0.7f);
	Basic.AttackSource = EUmbraBonusAttackSource::BasicAttack;
	Skill.AttackSource = EUmbraBonusAttackSource::Skill;
	const auto SourceHandle = Apply(MakeGE({Basic, Skill}));
	Check(576.f, TEXT("Basic bonus alone: never skill bonus"));
	Hit.Source = EUmbraDamageSource::Skill;
	Check(657.f, TEXT("Skill bonus alone: never basic bonus"));
	Source->RemoveActiveGameplayEffect(SourceHandle);
	Hit.Source = EUmbraDamageSource::BasicAttack;
	FUmbraDamageBonus X = Bonus(1.1f), ConditionalX = Bonus(1.25f);
	X.Bucket = ConditionalX.Bucket = EUmbraDamageBucket::Multiplicative;
	ConditionalX.Types = {EUmbraWeaponDamageType::Slashing};
	ConditionalX.bRequiresCritical = ConditionalX.bRequiresVulnerable = true;
	ConditionalX.SourceRequirements.RequireTags.AddTag(UmbraGameplayTags::State_SuperArmor);
	const auto XHandle = Apply(MakeGE({X, ConditionalX}));
	Check(514.8f, TEXT("X1.1, conditional X missing source tag"));
	Source->AddLooseGameplayTag(UmbraGameplayTags::State_SuperArmor);
	Check(603.9f, TEXT("X factors multiply only matching slash:324*1.1*1.25 +144*1.1"));
	Source->RemoveLooseGameplayTag(UmbraGameplayTags::State_SuperArmor);
	Source->RemoveActiveGameplayEffect(XHandle);
	Check(468.f, TEXT("X removal leaves no residual"));
	Target->RemoveActiveGameplayEffect(VHandle);
	Check(345.f, TEXT("Vulnerability removal disables baseV and vulnerable A together"));
	Source->RemoveActiveGameplayEffect(AHandle);
	Check(225.f, TEXT("A removal leaves only base crit1.5"));

	// Per-stack contract: A sums value*N, X multiplies value^N, never 1+(X-1)*N.
	FUmbraDamageBonus StackA = Bonus(0.1f), StackX = Bonus(1.1f);
	StackX.Bucket = EUmbraDamageBucket::Multiplicative;
	auto* StackedGE = MakeGE({StackA, StackX});
	// UE 5.8 declares SetStackingType without exporting it to game modules.
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackedGE->StackingType = EGameplayEffectStackingType::AggregateByTarget;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackedGE->StackLimitCount = 5;
	FGameplayEffectSpec StackedSpec(StackedGE, Source->MakeEffectContext(), 1.f);
	StackedSpec.SetStackCount(2);
	auto StackHandle = Source->ApplyGameplayEffectSpecToSelf(StackedSpec);
	Check(326.7f, TEXT("Two stacks:150*1.2*1.21*1.5"));
	StackHandle = Source->SetActiveGameplayEffectInhibit(MoveTemp(StackHandle), true, true);
	Check(225.f, TEXT("Inhibited bonuses ignored"));
	StackHandle = Source->SetActiveGameplayEffectInhibit(MoveTemp(StackHandle), false, true);
	Check(326.7f, TEXT("Uninhibit restores exactly two stacks"));
	Source->RemoveActiveGameplayEffect(StackHandle, 1);
	Check(272.25f, TEXT("Remove one stack:150*1.1*1.1*1.5"));
	Source->RemoveActiveGameplayEffect(StackHandle);
	Check(225.f, TEXT("All stacks removed"));
	FUmbraDamageBonus TargetCondition = Bonus(0.5f);
	TargetCondition.TargetRequirements.RequireTags.AddTag(UmbraGameplayTags::State_Stunned);
	const auto ConditionalHandle = Apply(MakeGE({TargetCondition}));
	Check(225.f, TEXT("Target condition absent"));
	Target->AddLooseGameplayTag(UmbraGameplayTags::State_Stunned);
	Check(337.5f, TEXT("Target condition present"));
	Target->RemoveLooseGameplayTag(UmbraGameplayTags::State_Stunned);
	Source->RemoveActiveGameplayEffect(ConditionalHandle);

	auto* Rules = NewObject<UUmbraDamageRules>(Source);
	Rules->BaseCriticalMultiplier = 2.f; Rules->BaseVulnerableMultiplier = 1.4f;
	Hit.Config.Typed.Rules = Rules;
	Target->AddLooseGameplayTag(UmbraGameplayTags::State_Vulnerable);
	Check(420.f, TEXT("Configured base factors2 and1.4"));
	Hit.Config.Typed.bCanCritical = false;
	Check(210.f, TEXT("CanCritical false disables baseC but retains baseV"));
	Hit.Config.Typed.bCanCritical = true;
	const auto CompatibilityBuff = Apply(MakeGE({All}));
	Hit.Config.Typed.bUseDamageBuckets = false;
	Check(1350.f, TEXT("Phase4 compatibility uses old total9 and ignores A/X/V"));
	Hit.Config.Typed.Model = EUmbraDamageModel::Legacy;
	Hit.EffectClass = LoadClass<UGameplayEffect>(nullptr, TEXT("/Game/Blueprints/Abilities/Effects/GE_Damage_PlayerBasic.GE_Damage_PlayerBasic_C"));
	Check(900.f, TEXT("Legacy AD100*crit9, unaffected by bonus and vulnerability"));
	Source->RemoveActiveGameplayEffect(CompatibilityBuff);
	Target->RemoveLooseGameplayTag(UmbraGameplayTags::State_Vulnerable);
	Hit.Config.Typed.Model = EUmbraDamageModel::ExplicitChannels;
	Hit.Config.Typed.bUseDamageBuckets = true;
	Hit.Config.Typed.Rules = nullptr;
	AddExpectedError(TEXT("Damage bonus GE rejected"), EAutomationExpectedErrorFlags::Contains, 2);
	TestFalse(TEXT("Negative A rejected"), Apply(MakeGE({Bonus(-0.1f)})).IsValid());
	auto* Instant = MakeGE({All}); Instant->DurationPolicy = EGameplayEffectDurationType::Instant;
	TestFalse(TEXT("Instant modifier lifetime rejected"), Apply(Instant).IsValid());
	Check(225.f, TEXT("Rejected effects leave no residue"));
	TestEqual(TEXT("Source AD never rewritten by A/X"), Source->GetNumericAttribute(UUmbraAttributeSet::GetAttackPowerAttribute()), 100.f);
	Target->GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetHealthAttribute()).Remove(HealthHandle);
	World->DestroyWorld(false);
	return true;
}
#endif
