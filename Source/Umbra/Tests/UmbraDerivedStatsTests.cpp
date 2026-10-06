#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/Effects/UmbraDebugEffects.h"
#include "AbilitySystem/Damage/UmbraDamage.h"
#include "Stats/UmbraDerivedStatsComponent.h"
#include "Player/UmbraPlayerState.h"
#include "Curves/CurveFloat.h"
#include "Engine/World.h"
#include "GameplayEffect.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraDerivedStatsTest, "Umbra.Attributes.WeaponDerivedPower",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraDerivedStatsTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	auto* Owner = World->SpawnActor<AUmbraPlayerState>();
	auto* ASC = Owner->GetUmbraAbilitySystemComponent();
	ASC->InitializeComponent();
	ASC->InitAbilityActorInfo(Owner, Owner);
	Owner->InitializeAttributes();
	auto* Derived = Owner->FindComponentByClass<UUmbraDerivedStatsComponent>();
	if (!TestNotNull(TEXT("Player owns derived component"), Derived)) { World->DestroyWorld(false); return false; }
	TestFalse(TEXT("Legacy mode default"), Derived->IsDerivedPowerActive());
	TestFalse(TEXT("Legacy has no misleading snapshot"), Derived->GetSnapshot().bValid);
	TestEqual(TEXT("Legacy AD retained"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetAttackPowerAttribute()), 10.f);
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetStrengthAttribute(), 10.f);
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetDexterityAttribute(), 20.f);
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetIntelligenceAttribute(), 30.f);
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetFaithAttribute(), 40.f);
	// Test opt-in before binding. Production sets this on the owning BP component before spawn.
	Derived->bUseWeaponDerivedPower = true;
	Derived->Initialize(ASC);
	TestTrue(TEXT("Unarmed valid"), Derived->GetSnapshot().bValid);
	TestTrue(TEXT("Unarmed identity"), Derived->GetSnapshot().bUnarmed);
	TestEqual(TEXT("Unarmed fallback AD"), Derived->GetSnapshot().AttackPower, 10.f);
	TestEqual(TEXT("Unarmed fallback AP"), Derived->GetSnapshot().AbilityPower, 0.f);
	auto* Weapon = NewObject<UUmbraWeaponProfile>(Owner);
	FUmbraWeaponDamageChannel Physical;
	Physical.Type = EUmbraWeaponDamageType::Slashing;
	Physical.BaseDamage = 80.f;
	FUmbraWeaponScaling Str; Str.Attribute = EUmbraPrimaryAttribute::Strength; Str.Coefficient = 2.f;
	FUmbraWeaponScaling Dex; Dex.Attribute = EUmbraPrimaryAttribute::Dexterity; Dex.Coefficient = 3.f;
	Physical.Scaling = { Str, Dex };
	FUmbraWeaponDamageChannel Magic;
	Magic.Type = EUmbraWeaponDamageType::Fire;
	Magic.BaseDamage = 20.f;
	FUmbraWeaponScaling Int; Int.Attribute = EUmbraPrimaryAttribute::Intelligence; Int.Coefficient = 1.f;
	FUmbraWeaponScaling Faith; Faith.Attribute = EUmbraPrimaryAttribute::Faith; Faith.Coefficient = 0.5f;
	Magic.Scaling = { Int, Faith };
	Weapon->Damage.Channels = { Physical, Magic };
	TestTrue(TEXT("Select mixed weapon"), Derived->SetWeaponProfile(Weapon));
	TestEqual(TEXT("AD = 80 + 10*2 + 20*3"), Derived->GetSnapshot().AttackPower, 160.f);
	TestEqual(TEXT("AP = 20 + 30 + 40*0.5"), Derived->GetSnapshot().AbilityPower, 70.f);
	TestEqual(TEXT("Physical scaling breakdown"), Derived->GetSnapshot().Channels[0].ScalingDamage, 80.f);
	TestEqual(TEXT("GAS is published AD"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetAttackPowerAttribute()), 160.f);
	TestEqual(TEXT("GAS is published AP"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetAbilityPowerAttribute()), 70.f);
	for (int32 Index = 0; Index < 20; ++Index) Derived->Initialize(ASC);
	TestEqual(TEXT("Repeated initialize does not accumulate"), Derived->GetSnapshot().AttackPower, 160.f);
	const FGameplayEffectSpecHandle Buff = ASC->MakeOutgoingSpec(UUmbraDebugDerivedAttributeEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
	const FActiveGameplayEffectHandle BuffHandle = ASC->ApplyGameplayEffectSpecToSelf(*Buff.Data.Get());
	TestTrue(TEXT("Primary debug buff accepted"), BuffHandle.IsValid());
	TestEqual(TEXT("Buff derives AD once"), Derived->GetSnapshot().AttackPower, 260.f);
	TestEqual(TEXT("Buff derives AP once"), Derived->GetSnapshot().AbilityPower, 100.f);
	ASC->RemoveActiveGameplayEffect(BuffHandle);
	TestEqual(TEXT("Removing buff restores AD"), Derived->GetSnapshot().AttackPower, 160.f);
	TestEqual(TEXT("Removing buff restores AP"), Derived->GetSnapshot().AbilityPower, 70.f);
	AddExpectedError(TEXT("Derived power rejects direct AD/AP"), EAutomationExpectedErrorFlags::Contains, 1);
	const auto OldBuff = ASC->MakeOutgoingSpec(UUmbraDebugAttributeEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
	TestFalse(TEXT("Old direct-power debug GE rejected"), ASC->ApplyGameplayEffectSpecToSelf(*OldBuff.Data.Get()).IsValid());
	TestEqual(TEXT("Rejected GE does not add primaries"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetStrengthAttribute()), 10.f);
	TestTrue(TEXT("Unequip returns unarmed"), Derived->SetWeaponProfile(nullptr));
	TestEqual(TEXT("Unequip has no residual AP"), Derived->GetSnapshot().AbilityPower, 0.f);
	TestTrue(TEXT("Reequip restores weapon"), Derived->SetWeaponProfile(Weapon));
	auto* Invalid = NewObject<UUmbraWeaponProfile>(Owner);
	Invalid->Damage.Channels = { Physical, Physical };
	TestFalse(TEXT("Duplicate type rejected"), Derived->SetWeaponProfile(Invalid));
	TestEqual(TEXT("Invalid replacement leaves power intact"), Derived->GetSnapshot().AttackPower, 160.f);
	Invalid->Damage.Channels = { Physical };
	Invalid->Damage.Channels[0].BaseDamage = -1.f;
	TestFalse(TEXT("Negative base rejected"), Derived->SetWeaponProfile(Invalid));
	Invalid->Damage.Channels[0] = Physical;
	Invalid->Damage.Channels[0].Scaling.Add(Str);
	TestFalse(TEXT("Duplicate primary scaling rejected"), Derived->SetWeaponProfile(Invalid));
	auto* Curved = NewObject<UUmbraWeaponProfile>(Owner);
	Curved->Damage.Channels = { Magic };
	auto* Curve = NewObject<UCurveFloat>(Curved);
	Curve->FloatCurve.AddKey(0.f, 0.f);
	Curve->FloatCurve.AddKey(30.f, 60.f);
	Curved->Damage.Channels[0].Scaling[0].PointCurve = Curve;
	TestTrue(TEXT("Curve weapon accepted"), Derived->SetWeaponProfile(Curved));
	TestEqual(TEXT("Curve applies to its type"), Derived->GetSnapshot().AbilityPower, 100.f);
	TestEqual(TEXT("Pure magical weapon clears AD"), Derived->GetSnapshot().AttackPower, 0.f);
	Derived->SetWeaponProfile(Weapon);
	bool bChangedPrimary = false;
	bool bReentrantDamageAccepted = true;
	FUmbraDamageRequest Hit(EUmbraDamageSource::BasicAttack);
	Hit.EffectClass = LoadClass<UGameplayEffect>(nullptr,
		TEXT("/Game/Blueprints/Abilities/Effects/GE_Damage_PlayerBasic.GE_Damage_PlayerBasic_C"));
	const auto Reentrant = ASC->GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetAttackPowerAttribute())
		.AddLambda([&](const FOnAttributeChangeData&)
		{
			if (!bChangedPrimary)
			{
				bChangedPrimary = true;
				bReentrantDamageAccepted = UmbraDamage::Apply(ASC, ASC, Hit);
				ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetIntelligenceAttribute(), 50.f);
			}
		});
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetStrengthAttribute(), 20.f);
	TestFalse(TEXT("No damage during partially published pair"), bReentrantDamageAccepted);
	TestEqual(TEXT("Primary mutation drains without recursion"), Derived->GetSnapshot().AbilityPower, 90.f);
	TestEqual(TEXT("New strength produces AD"), Derived->GetSnapshot().AttackPower, 180.f);
	ASC->GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetAttackPowerAttribute()).Remove(Reentrant);
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(), 50.f);
	ASC->ClearActorInfo();
	ASC->InitAbilityActorInfo(Owner, Owner);
	Owner->InitializeAttributes();
	TestEqual(TEXT("Avatar rebind keeps selected weapon"), Derived->GetSnapshot().AttackPower, 180.f);
	TestEqual(TEXT("Avatar rebind never refills health"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()), 50.f);
	auto* TargetOwner = World->SpawnActor<AUmbraPlayerState>();
	auto* TargetASC = TargetOwner->GetUmbraAbilitySystemComponent();
	TargetASC->InitializeComponent();
	TargetASC->InitAbilityActorInfo(TargetOwner, TargetOwner);
	TargetOwner->InitializeAttributes();
	TargetASC->SetNumericAttributeBase(UUmbraAttributeSet::GetArmorAttribute(), 100.f);
	TestTrue(TEXT("Committed snapshot valid after all updates"), Derived->GetSnapshot().bValid);
	TestTrue(TEXT("Existing execution accepts derived source"), UmbraDamage::Apply(ASC, TargetASC, Hit));
	TestEqual(TEXT("Legacy execution reads derived AD180, armor100 => damage90"),
		TargetASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()), 10.f);
	Derived->DestroyComponent();
	TestFalse(TEXT("Destroy removes primary listener"), ASC->GetGameplayAttributeValueChangeDelegate(
		UUmbraAttributeSet::GetStrengthAttribute()).IsBoundToObject(Derived));
	TestTrue(TEXT("Destroy removes application query"), ASC->GameplayEffectApplicationQueries.IsEmpty());
	World->DestroyWorld(false);
	return true;
}
#endif
