#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/UmbraCombatInfoTestTypes.h"
#include "UI/Combat/UmbraCombatStatData.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/Damage/UmbraDamageBonusComponent.h"
#include "Player/UmbraPlayerState.h"
#include "UmbraPlayerController.h"
#include "Equipment/UmbraEquipmentComponent.h"
#include "Stats/UmbraDerivedStatsComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameplayEffect.h"
#include "Tests/UmbraStatWidgetTestTypes.h"
#include "Components/TextBlock.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraCombatBindingTest, "Umbra.UI.CombatInfo.LiveBinding",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraCombatBindingTest::RunTest(const FString& Parameters)
{
 auto* PCClass = LoadClass<AUmbraPlayerController>(nullptr, TEXT("/Game/Blueprints/Player/BP_UmbraPlayerController.BP_UmbraPlayerController_C"));
 if (!TestNotNull(TEXT("Controller class"), PCClass)) return false;
 UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 World->InitializeActorsForPlay(FURL());
 auto* PC = World->SpawnActor<AUmbraPlayerController>(PCClass);
 PC->SetPlayer(NewObject<ULocalPlayer>(GEngine));
 auto MakeState = [&]()
 {
  auto* PS = World->SpawnActor<AUmbraPlayerState>();
  auto* ASC = PS->GetUmbraAbilitySystemComponent();
  if (!ASC->HasBeenInitialized()) ASC->InitializeComponent();
  ASC->InitAbilityActorInfo(PS, PS); PS->InitializeAttributes();
  return PS;
 };
 auto* PS = MakeState(); PC->PlayerState = PS;
 auto* ASC = PS->GetUmbraAbilitySystemComponent();
 auto* Page = NewObject<UUmbraCombatInfoTestWidget>(PC);
 Page->SetOwningPlayer(PC);
 Page->ConstructForTest();
 auto* Stats = NewObject<UUmbraStatsPanelTestWidget>(PC);
 Stats->SetOwningPlayer(PC); Stats->ConstructForTest();
 TArray<UTextBlock*> StatTexts;
 const TArray<EUmbraCombatStat> SharedStats = { EUmbraCombatStat::AttackPower, EUmbraCombatStat::AbilityPower,
  EUmbraCombatStat::Armor, EUmbraCombatStat::MagicResist, EUmbraCombatStat::AttackSpeed,
  EUmbraCombatStat::AbilityHaste, EUmbraCombatStat::CriticalChance, EUmbraCombatStat::MoveSpeed };
 for (int32 Index = 0; Index < SharedStats.Num(); ++Index)
 {
  auto* Row = NewObject<UUmbraStatEntryTestWidget>(Stats);
  auto* Text = NewObject<UTextBlock>(Row); StatTexts.Add(Text);
  Row->Configure(EUmbraCharacterStat(Index), nullptr, nullptr, Text); Stats->RegisterEntry(Row);
 }
 auto CheckShared = [&]()
 {
  for (int32 Index = 0; Index < SharedStats.Num(); ++Index)
   TestEqual(TEXT("StatsPanel shares secondary source and exact formatting"), StatTexts[Index]->GetText().ToString(),
    UmbraCombatStats::Read(Cast<AUmbraPlayerState>(PC->PlayerState), SharedStats[Index]).Text.ToString());
 };
 CheckShared();
 auto AddRow = [&](EUmbraCombatStat Key)
 {
  auto* Row = NewObject<UUmbraCombatStatEntryTestWidget>(Page);
  Row->CombatStat = Key;
  Page->RegisterCombatEntry(Row);
  return Row;
 };
 auto* AD = AddRow(EUmbraCombatStat::AttackPower);
 auto* Health = AddRow(EUmbraCombatStat::MaxHealth);
 auto* All = AddRow(EUmbraCombatStat::AllDamage);
 auto* Crit = AddRow(EUmbraCombatStat::CriticalDamage);
 auto* Type = AddRow(EUmbraCombatStat::SlashingDamage);
 auto* Vuln = AddRow(EUmbraCombatStat::VulnerableDamage);
 auto* Load = AddRow(EUmbraCombatStat::EquipLoad);
 auto* MaxLoad = AddRow(EUmbraCombatStat::MaxEquipLoad);
 auto* Base = AddRow(EUmbraCombatStat::BaseWeaponDamage);
 auto* NamedRow = NewObject<UUmbraCombatStatEntryTestWidget>(Page, TEXT("MagicResist"));
 TestTrue(TEXT("Exact instance name resolves identifier"), NamedRow->ResolveCombatStat() == EUmbraCombatStat::MagicResist);
 NamedRow->CombatStat = EUmbraCombatStat::Armor;
 TestTrue(TEXT("Explicit identifier overrides name"), NamedRow->ResolveCombatStat() == EUmbraCombatStat::Armor);
 auto* Resistance = AddRow(EUmbraCombatStat::FireResistance);
 ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetFireResistanceAttribute(), 25.f);
 TestEqual(TEXT("Resistance is raw rating"), Resistance->CombatValue.Value, 25.);
 TestFalse(TEXT("Resistance not presented as percentage"), Resistance->CombatValue.Text.ToString().Contains(TEXT("%")));
 TestTrue(TEXT("Missing weapon data unavailable"), Base->CombatValue.Status == EUmbraCombatStatStatus::Unavailable);
 TestTrue(TEXT("Unimplemented is distinct"), UmbraCombatStats::Read(PS, EUmbraCombatStat::DodgeChance).Status == EUmbraCombatStatStatus::Placeholder);
 TestTrue(TEXT("Stored only is distinct"), UmbraCombatStats::Read(PS, EUmbraCombatStat::HealthRegeneration).Status == EUmbraCombatStatStatus::StoredOnly);
 ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetMaxHealthAttribute(), 321.f);
 TestEqual(TEXT("GAS event updates immediately"), Health->CombatValue.Value, 321.);
 ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetAttackPowerAttribute(), 160.f);
 TestEqual(TEXT("Legacy AD reads current GAS"), AD->CombatValue.Value, 160.);
 ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetAbilityPowerAttribute(), 70.f);
 ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetCriticalChanceAttribute(), .123f);
 ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetAbilityHasteAttribute(), 15.f);
 CheckShared();
 ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetCriticalDamageMultiplierAttribute(), 9.f);
 TestEqual(TEXT("Old crit multiplier excluded"), Crit->CombatValue.Value, 0.);
 auto* GE = NewObject<UGameplayEffect>(ASC);
 GE->DurationPolicy = EGameplayEffectDurationType::Infinite;
 // UE5.8's setter is not exported; use the supported legacy field in this transient fixture.
 PRAGMA_DISABLE_DEPRECATION_WARNINGS
 GE->StackingType = EGameplayEffectStackingType::AggregateByTarget;
 PRAGMA_ENABLE_DEPRECATION_WARNINGS
 GE->StackLimitCount = 3;
 auto& Bonuses = GE->AddComponent<UUmbraDamageBonusComponent>().Bonuses;
 FUmbraDamageBonus A; A.Magnitude = FScalableFloat(0.1f); Bonuses.Add(A);
 A.Magnitude = FScalableFloat(0.2f); A.Types = {EUmbraWeaponDamageType::Slashing}; Bonuses.Add(A);
 A.Types.Reset(); A.bRequiresCritical = true; A.Magnitude = FScalableFloat(0.3f); Bonuses.Add(A);
 A.bRequiresCritical = false; A.bRequiresVulnerable = true; A.Magnitude = FScalableFloat(0.2f); Bonuses.Add(A);
 A.bRequiresVulnerable = false; A.AttackSource = EUmbraBonusAttackSource::Skill; A.Magnitude = FScalableFloat(2.f); Bonuses.Add(A);
 A.AttackSource = EUmbraBonusAttackSource::Any; A.Bucket = EUmbraDamageBucket::Multiplicative; A.Magnitude = FScalableFloat(1.1f); Bonuses.Add(A);
 auto Handle = ASC->ApplyGameplayEffectToSelf(GE, 1.f, ASC->MakeEffectContext());
 TestTrue(TEXT("GE added refreshes A"), FMath::IsNearlyEqual(All->CombatValue.Value, .1, .0001));
 TestTrue(TEXT("Type excludes global A"), FMath::IsNearlyEqual(Type->CombatValue.Value, .2, .0001));
 TestTrue(TEXT("Crit is A fraction"), FMath::IsNearlyEqual(Crit->CombatValue.Value, .3, .0001));
 TestTrue(TEXT("Vulnerable is A fraction"), FMath::IsNearlyEqual(Vuln->CombatValue.Value, .2, .0001));
 TestTrue(TEXT("Conditional and X exclusions exposed"), All->CombatValue.bHasAdditionalConditionalBonuses && All->CombatValue.bHasMultiplicativeBonuses);
 ASC->ApplyGameplayEffectToSelf(GE, 1.f, ASC->MakeEffectContext());
 TestTrue(TEXT("Stack event refreshes"), FMath::IsNearlyEqual(All->CombatValue.Value, .2, .0001));
 Handle = ASC->SetActiveGameplayEffectInhibit(MoveTemp(Handle), true, true);
 TestEqual(TEXT("Inhibition removes contribution"), All->CombatValue.Value, 0.);
 Handle = ASC->SetActiveGameplayEffectInhibit(MoveTemp(Handle), false, true);
 TestTrue(TEXT("Uninhibit restores contribution"), FMath::IsNearlyEqual(All->CombatValue.Value, .2, .0001));
 ASC->RemoveActiveGameplayEffect(Handle, 1);
 TestTrue(TEXT("Single stack removal refreshes"), FMath::IsNearlyEqual(All->CombatValue.Value, .1, .0001));
 ASC->RemoveActiveGameplayEffect(Handle);
 TestEqual(TEXT("Full removal refreshes"), All->CombatValue.Value, 0.);
 auto* Derived = PS->FindComponentByClass<UUmbraDerivedStatsComponent>();
 Derived->bUseWeaponDerivedPower = true;
 auto& Channel = Derived->UnarmedProfile.Channels[0];
 Channel.BaseDamage = 20.f;
 FUmbraWeaponScaling Scaling; Scaling.Coefficient = 2.f; Channel.Scaling.Add(Scaling);
 Derived->Initialize(ASC);
 ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetStrengthAttribute(), 10.f);
 TestEqual(TEXT("Snapshot AD refreshes on primary change"), AD->CombatValue.Value, 40.);
 TestEqual(TEXT("Base row excludes scaling"), Base->CombatValue.Value, 20.);
 CheckShared();
 auto* Equipment = PS->FindComponentByClass<UUmbraEquipmentComponent>();
 Equipment->bEnableEquipment = true; Equipment->Initialize(ASC);
 TestEqual(TEXT("Empty equipment has real zero load"), Load->CombatValue.Value, 0.);
 TestTrue(TEXT("Load has real data"), Load->CombatValue.Status == EUmbraCombatStatStatus::Live);
 TestEqual(TEXT("Capacity reads component"), MaxLoad->CombatValue.Value, 45.);
 ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetStrengthAttribute(), 20.f);
 TestEqual(TEXT("Capacity event updates"), MaxLoad->CombatValue.Value, 50.);
 auto* Other = MakeState(); PC->PlayerState = Other; Page->NotifyPlayerContextChanged();
 Stats->NotifyPlayerContextChanged(); CheckShared();
 TestEqual(TEXT("PlayerState rebind reads new AD"), AD->CombatValue.Value, 10.);
 ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetMaxHealthAttribute(), 444.f);
 TestTrue(TEXT("Old ASC detached"), Health->CombatValue.Value != 444.);
 auto* NextASC = Other->GetUmbraAbilitySystemComponent();
 NextASC->ClearActorInfo();
 CheckShared();
 TestTrue(TEXT("Lifecycle clears stale rows"), Health->CombatValue.Status == EUmbraCombatStatStatus::Unavailable);
 NextASC->InitAbilityActorInfo(Other, Other);
 CheckShared();
 TestTrue(TEXT("Lifecycle reconnects"), Health->CombatValue.Status == EUmbraCombatStatStatus::Live);
 Page->DestructForTest();
 Stats->DestructForTest();
 NextASC->SetNumericAttributeBase(UUmbraAttributeSet::GetMaxHealthAttribute(), 555.f);
 TestTrue(TEXT("Closed view stays detached"), Health->CombatValue.Status == EUmbraCombatStatStatus::Unavailable);
 Page->ConstructForTest(); Page->RegisterCombatEntry(Health);
 TestEqual(TEXT("Reopen reads current value"), Health->CombatValue.Value, 555.);
 Page->DestructForTest();
 GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
 return true;
}
#endif

