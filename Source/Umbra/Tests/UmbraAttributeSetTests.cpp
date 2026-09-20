#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/UmbraDebugInitialAttributes.h"
#include "Player/UmbraPlayerState.h"
#include "Engine/World.h"
#include "GameplayEffect.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraAttributeBoundaryTest, "Umbra.Attributes.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraAttributeBoundaryTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	AUmbraPlayerState* Owner = World->SpawnActor<AUmbraPlayerState>();
	UUmbraAbilitySystemComponent* ASC = Owner->GetUmbraAbilitySystemComponent();
	// This isolated world does not run the map BeginPlay/component initialization sequence.
	ASC->InitializeComponent();
	ASC->InitAbilityActorInfo(Owner, Owner);
	ASC->InitializeAttributes(nullptr);
	const UUmbraAttributeSet* Attributes = ASC->GetSet<UUmbraAttributeSet>();
	if (!TestNotNull(TEXT("Shared set registered"), Attributes))
	{
		World->DestroyWorld(false);
		return false;
	}

	auto Apply = [ASC](const FGameplayAttribute& Attribute, float Magnitude,
		EGameplayEffectDurationType Duration = EGameplayEffectDurationType::Instant)
	{
		UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage());
		Effect->DurationPolicy = Duration;
		Effect->DurationMagnitude = FScalableFloat(30.f);
		FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FScalableFloat(Magnitude);
		return ASC->ApplyGameplayEffectSpecToSelf(FGameplayEffectSpec(Effect, ASC->MakeEffectContext(), 1.f));
	};
	TestEqual(TEXT("Initial health full"), Attributes->GetHealth(), 100.f);
	TestEqual(TEXT("Initial resource full"), Attributes->GetResource(), 100.f);
	TestEqual(TEXT("Default crit total multiplier"), Attributes->GetCriticalDamageMultiplier(), 2.f);
	TestEqual(TEXT("Default strength"), Attributes->GetAttackPower(), 10.f);
	TestEqual(TEXT("Default attack speed is 1x"), Attributes->GetAttackSpeed(), 1.f);
	TestEqual(TEXT("Default move speed"), Attributes->GetMoveSpeed(), 500.f);
	const FUmbraDebugInitialAttributes DebugDefaults;
	TestEqual(TEXT("Debug default move speed"), DebugDefaults.MoveSpeed, 500.f);
	TestEqual(TEXT("Debug default attack speed is 1x"), DebugDefaults.AttackSpeed, 1.f);
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetAttackSpeedAttribute(), -10.f);
	TestEqual(TEXT("Attack speed has safe minimum"), Attributes->GetAttackSpeed(), 0.2f);
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetAttackSpeedAttribute(), 10.f);
	TestEqual(TEXT("Attack speed has safe maximum"), Attributes->GetAttackSpeed(), 10.f);
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetAttackSpeedAttribute(), 1.f);
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetMoveSpeedAttribute(), -100.f);
	TestEqual(TEXT("Move speed cannot become negative"), Attributes->GetMoveSpeed(), 0.f);
	Apply(UUmbraAttributeSet::GetHealthAttribute(), -25.f);
	ASC->InitAbilityActorInfo(Owner, Owner);
	ASC->InitializeAttributes(nullptr);
	TestEqual(TEXT("Rebinding does not heal"), Attributes->GetHealth(), 75.f);
	Apply(UUmbraAttributeSet::GetIncomingDamageAttribute(), 20.f);
	TestEqual(TEXT("Meta damage consumed"), Attributes->GetHealth(), 55.f);
	TestEqual(TEXT("Meta reset"), Attributes->GetIncomingDamage(), 0.f);
	Apply(UUmbraAttributeSet::GetArmorAttribute(), 1.f);
	TestEqual(TEXT("Other GE does not repeat damage"), Attributes->GetHealth(), 55.f);

	const auto MaxBuff = Apply(UUmbraAttributeSet::GetMaxHealthAttribute(), 100.f, EGameplayEffectDurationType::Infinite);
	TestEqual(TEXT("Raised maximum does not fill"), Attributes->GetHealth(), 55.f);
	Apply(UUmbraAttributeSet::GetHealthAttribute(), 500.f);
	TestEqual(TEXT("Healing capped"), Attributes->GetHealth(), 200.f);
	ASC->RemoveActiveGameplayEffect(MaxBuff);
	TestEqual(TEXT("Max buff removal clips health"), Attributes->GetHealth(), 100.f);
	const auto MaxDebuff = Apply(UUmbraAttributeSet::GetMaxHealthAttribute(), -60.f, EGameplayEffectDurationType::Infinite);
	TestEqual(TEXT("Max debuff clips health"), Attributes->GetHealth(), 40.f);
	ASC->RemoveActiveGameplayEffect(MaxDebuff);
	TestEqual(TEXT("Max debuff removal does not heal"), Attributes->GetHealth(), 40.f);

	const auto CritBuff = Apply(UUmbraAttributeSet::GetCriticalChanceAttribute(), 2.f, EGameplayEffectDurationType::HasDuration);
	TestEqual(TEXT("Duration crit capped"), Attributes->GetCriticalChance(), 1.f);
	Apply(UUmbraAttributeSet::GetArmorAttribute(), 1.f);
	ASC->RemoveActiveGameplayEffect(CritBuff);
	TestEqual(TEXT("Buff not baked into base"), Attributes->GetCriticalChance(), 0.f);
	const auto ArmorDebuff = Apply(UUmbraAttributeSet::GetArmorAttribute(), -100.f, EGameplayEffectDurationType::Infinite);
	TestEqual(TEXT("Duration armor nonnegative"), Attributes->GetArmor(), 0.f);
	ASC->RemoveActiveGameplayEffect(ArmorDebuff);
	TestEqual(TEXT("Debuff removal restores armor"), Attributes->GetArmor(), 2.f);
	Apply(UUmbraAttributeSet::GetMaxResourceAttribute(), -1000.f);
	TestEqual(TEXT("Zero max resource"), Attributes->GetMaxResource(), 0.f);
	TestEqual(TEXT("Resource clipped to zero"), Attributes->GetResource(), 0.f);
	Apply(UUmbraAttributeSet::GetMaxResourceAttribute(), 50.f);
	TestEqual(TEXT("Resource increase does not fill"), Attributes->GetResource(), 0.f);
	Apply(UUmbraAttributeSet::GetMaxHealthAttribute(), -1000.f);
	TestEqual(TEXT("Minimum max health"), Attributes->GetMaxHealth(), 1.f);
	Apply(UUmbraAttributeSet::GetCriticalDamageMultiplierAttribute(), -100.f);
	TestEqual(TEXT("Minimum crit multiplier"), Attributes->GetCriticalDamageMultiplier(), 1.f);
	Apply(UUmbraAttributeSet::GetIncomingDamageAttribute(), 10000.f);
	TestEqual(TEXT("Lethal damage capped at zero"), Attributes->GetHealth(), 0.f);
	Apply(UUmbraAttributeSet::GetIncomingDamageAttribute(), -100.f);
	TestEqual(TEXT("Negative damage does not heal"), Attributes->GetHealth(), 0.f);
	TestEqual(TEXT("Negative meta also reset"), Attributes->GetIncomingDamage(), 0.f);
	World->DestroyWorld(false);
	return true;
}
#endif
