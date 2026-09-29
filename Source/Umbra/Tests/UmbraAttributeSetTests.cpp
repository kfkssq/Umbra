#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/UmbraDebugInitialAttributes.h"
#include "AbilitySystem/Effects/UmbraDebugEffects.h"
#include "Player/UmbraPlayerState.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "UObject/UnrealType.h"

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
	TestEqual(TEXT("Default attack power"), Attributes->GetAttackPower(), 10.f);
	TestEqual(TEXT("Default Strength"), Attributes->GetStrength(), 0.f);
	TestEqual(TEXT("Default Dexterity"), Attributes->GetDexterity(), 0.f);
	TestEqual(TEXT("Default Intelligence"), Attributes->GetIntelligence(), 0.f);
	TestEqual(TEXT("Default Faith"), Attributes->GetFaith(), 0.f);
	TestEqual(TEXT("Default attack speed is 1x"), Attributes->GetAttackSpeed(), 1.f);
	TestEqual(TEXT("Default move speed"), Attributes->GetMoveSpeed(), 500.f);
	const FUmbraDebugInitialAttributes DebugDefaults;
	TestEqual(TEXT("Debug default move speed"), DebugDefaults.MoveSpeed, 500.f);
	TestEqual(TEXT("Debug default attack speed is 1x"), DebugDefaults.AttackSpeed, 1.f);
	TestEqual(TEXT("Debug default Strength"), DebugDefaults.Strength, 0.f);
	TestEqual(TEXT("Debug default Dexterity"), DebugDefaults.Dexterity, 0.f);
	TestEqual(TEXT("Debug default Intelligence"), DebugDefaults.Intelligence, 0.f);
	TestEqual(TEXT("Debug default Faith"), DebugDefaults.Faith, 0.f);
	const FName PrimaryAttributeNames[] = {
		GET_MEMBER_NAME_CHECKED(UUmbraAttributeSet, Strength),
		GET_MEMBER_NAME_CHECKED(UUmbraAttributeSet, Dexterity),
		GET_MEMBER_NAME_CHECKED(UUmbraAttributeSet, Intelligence),
		GET_MEMBER_NAME_CHECKED(UUmbraAttributeSet, Faith)
	};
	for (const FName AttributeName : PrimaryAttributeNames)
	{
		const FProperty* Property = FindFProperty<FProperty>(UUmbraAttributeSet::StaticClass(), AttributeName);
		TestTrue(*FString::Printf(TEXT("%s is replicated"), *AttributeName.ToString()),
			Property && Property->HasAnyPropertyFlags(CPF_Net));
	}
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetStrengthAttribute(), -10.f);
	TestEqual(TEXT("Primary attributes cannot become negative"), Attributes->GetStrength(), 0.f);
	const FGameplayEffectSpecHandle PrimaryDebugSpec = ASC->MakeOutgoingSpec(
		UUmbraDebugAttributeEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
	const FActiveGameplayEffectHandle PrimaryDebugHandle = ASC->ApplyGameplayEffectSpecToSelf(*PrimaryDebugSpec.Data.Get());
	TestEqual(TEXT("Debug GE raises Strength"), Attributes->GetStrength(), 20.f);
	TestEqual(TEXT("Debug GE raises Dexterity"), Attributes->GetDexterity(), 20.f);
	TestEqual(TEXT("Debug GE raises Intelligence"), Attributes->GetIntelligence(), 20.f);
	TestEqual(TEXT("Debug GE raises Faith"), Attributes->GetFaith(), 20.f);
	ASC->RemoveActiveGameplayEffect(PrimaryDebugHandle);
	TestEqual(TEXT("Removing debug GE restores Strength"), Attributes->GetStrength(), 0.f);
	TestEqual(TEXT("Removing debug GE restores Dexterity"), Attributes->GetDexterity(), 0.f);
	TestEqual(TEXT("Removing debug GE restores Intelligence"), Attributes->GetIntelligence(), 0.f);
	TestEqual(TEXT("Removing debug GE restores Faith"), Attributes->GetFaith(), 0.f);
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

	AUmbraPlayerState* DebugOwner = World->SpawnActor<AUmbraPlayerState>();
	UUmbraAbilitySystemComponent* DebugASC = DebugOwner->GetUmbraAbilitySystemComponent();
	DebugASC->InitializeComponent();
	DebugASC->InitAbilityActorInfo(DebugOwner, DebugOwner);
	FUmbraDebugInitialAttributes DebugValues;
	DebugValues.Strength = 11.f;
	DebugValues.Dexterity = 12.f;
	DebugValues.Intelligence = 13.f;
	DebugValues.Faith = 14.f;
	DebugASC->InitializeAttributes(nullptr, &DebugValues);
	const UUmbraAttributeSet* DebugAttributes = DebugASC->GetSet<UUmbraAttributeSet>();
	TestEqual(TEXT("Debug initial Strength"), DebugAttributes->GetStrength(), 11.f);
	TestEqual(TEXT("Debug initial Dexterity"), DebugAttributes->GetDexterity(), 12.f);
	TestEqual(TEXT("Debug initial Intelligence"), DebugAttributes->GetIntelligence(), 13.f);
	TestEqual(TEXT("Debug initial Faith"), DebugAttributes->GetFaith(), 14.f);
	DebugValues.Strength = 99.f;
	DebugASC->InitializeAttributes(nullptr, &DebugValues);
	TestEqual(TEXT("Repeated initialization does not reset primary attributes"), DebugAttributes->GetStrength(), 11.f);
	World->DestroyWorld(false);
	return true;
}
#endif
