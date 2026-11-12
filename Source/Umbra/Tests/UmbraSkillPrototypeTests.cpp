#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Skills/UmbraSweepDefinition.h"
#include "Skills/UmbraSweepAbility.h"
#include "UI/UmbraSkillBar.h"
#include "Player/UmbraPlayerState.h"
#include "UmbraPlayerController.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "AbilitySystem/Damage/UmbraDamage.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraSweepSectorTest,"Umbra.Skills.SweepSector",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraSweepSectorTest::RunTest(const FString& Parameters)
{
    auto* Narrow=LoadObject<UUmbraSweepDefinition>(nullptr,TEXT("/Game/Skills/Prototype/DA_Sweep"));
    auto* Wide=LoadObject<UUmbraSweepDefinition>(nullptr,TEXT("/Game/Skills/Prototype/DA_WideSweep"));
    if (!TestNotNull(TEXT("Saved sweep definition"),Narrow) || !TestNotNull(TEXT("Saved variant definition"),Wide)) return false;
    FString Reason;
    TestTrue(TEXT("Base validates"),Narrow->Validate(Reason)); TestTrue(TEXT("Variant validates"),Wide->Validate(Reason));
    const FVector O=FVector::ZeroVector, F=FVector::ForwardVector;
    TestTrue(TEXT("Forward target"),Narrow->ContainsTarget(O,F,FVector(180,0,0)));
    TestFalse(TEXT("Behind is excluded"),Narrow->ContainsTarget(O,F,FVector(-100,0,0)));
    TestFalse(TEXT("Vertical floor separation"),Narrow->ContainsTarget(O,F,FVector(100,0,200)));
    TestFalse(TEXT("Base excludes outer target"),Narrow->ContainsTarget(O,F,FVector(300,0,0)));
    TestTrue(TEXT("Variant includes outer target"),Wide->ContainsTarget(O,F,FVector(300,0,0)));
    TestFalse(TEXT("Base excludes side target"),Narrow->ContainsTarget(O,F,FVector(60,190,0)));
    TestTrue(TEXT("Variant includes side target"),Wide->ContainsTarget(O,F,FVector(60,190,0)));
    TestTrue(TEXT("Variants share a montage"),Narrow->Montage==Wide->Montage);
    TestTrue(TEXT("Variant tradeoff lowers weapon multiplier"),Wide->Damage.Typed.WeaponMultiplier<Narrow->Damage.Typed.WeaponMultiplier);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraSkillCooldownViewTest,"Umbra.Skills.CooldownView",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraSkillCooldownViewTest::RunTest(const FString& Parameters)
{
    auto* Ignore = IConsoleManager::Get().FindConsoleVariable(TEXT("umbra.Skills.IgnoreCooldowns"));
    const int32 Previous = Ignore->GetInt();
    ON_SCOPE_EXIT { Ignore->Set(Previous, ECVF_SetByCode); };
    Ignore->Set(0, ECVF_SetByCode);
    auto* Definition=LoadObject<UUmbraSweepDefinition>(nullptr,TEXT("/Game/Skills/Prototype/DA_Sweep"));
    if (!TestNotNull(TEXT("Definition"),Definition)) return false;
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    auto* PC=World->SpawnActor<APlayerController>(); PC->Player=NewObject<ULocalPlayer>(GEngine);
    auto* PS=World->SpawnActor<AUmbraPlayerState>(); PC->PlayerState=PS;
    FUmbraSkillSlot SkillSlot; SkillSlot.Definition=Definition; SkillSlot.InputTag=UmbraGameplayTags::Input_Skill_One; SkillSlot.KeyLabel=FText::FromString(TEXT("Q"));
    PS->InitialSkillSlots.Add(SkillSlot);
    auto* ASC=PS->GetUmbraAbilitySystemComponent(); ASC->InitAbilityActorInfo(PS,PS); PS->InitializeAttributes(); PS->GrantInitialAbilities();
    auto* Widget=NewObject<UUmbraSkillSlotWidget>(PC); Widget->SetOwningPlayer(PC);
    TestTrue(TEXT("UI observes granted source object"),Widget->ReadViewState().bGranted);
    ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetResourceAttribute(),100.f);
    auto Cost=ASC->MakeOutgoingSpec(UUmbraSkillCostEffect::StaticClass(),1,ASC->MakeEffectContext());
    Cost.Data->SetSetByCallerMagnitude(FName(TEXT("SkillCost")),-15.f); ASC->ApplyGameplayEffectSpecToSelf(*Cost.Data.Get());
    TestEqual(TEXT("Cost uses GAS resource"),ASC->GetNumericAttribute(UUmbraAttributeSet::GetResourceAttribute()),85.f);
    auto Cooldown=ASC->MakeOutgoingSpec(UUmbraSkillCooldownEffect::StaticClass(),1,ASC->MakeEffectContext());
    Cooldown.Data->SetDuration(4.f,true); Cooldown.Data->DynamicGrantedTags.AddTag(Definition->CooldownTag);
    const auto CooldownHandle=ASC->ApplyGameplayEffectSpecToSelf(*Cooldown.Data.Get());
    TestTrue(TEXT("Cooldown granted tag is present"),ASC->HasMatchingGameplayTag(Definition->CooldownTag));
    auto View=Widget->ReadViewState();
    TestTrue(TEXT("UI observes actual active effect"),View.CooldownRemaining>3.9f && View.CooldownFraction>0.97f);
    Ignore->Set(1, ECVF_SetByCode);
    TestEqual(TEXT("Debug bypass hides existing cooldown"),Widget->ReadViewState().CooldownRemaining,0.f);
    TestTrue(TEXT("Debug bypass preserves authored duration"),Definition->CooldownSeconds>0.f);
    Ignore->Set(0, ECVF_SetByCode);
    TestTrue(TEXT("Restoring cooldown observes the same GE"),Widget->ReadViewState().CooldownRemaining>0.f);
    ASC->RemoveActiveGameplayEffect(CooldownHandle);
    TestEqual(TEXT("Removing effect resets UI without a local timer"),Widget->ReadViewState().CooldownRemaining,0.f);
    ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetResourceAttribute(),0.f);
    TestFalse(TEXT("Unaffordable state uses GAS"),Widget->ReadViewState().bAffordable);
    ASC->AddLooseGameplayTag(UmbraGameplayTags::State_Stunned);
    TestTrue(TEXT("Stunned state blocks the slot"),Widget->ReadViewState().bBlocked);
    PS->InitialSkillSlots.Reset(); TestFalse(TEXT("Missing slot clears view"),Widget->ReadViewState().bGranted);
    GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraGreystoneExpansionTest,"Umbra.Skills.GreystoneExpansion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraGreystoneExpansionTest::RunTest(const FString& Parameters)
{
    TSet<FGameplayTag> Cooldowns;
    for (const TCHAR* Name : {TEXT("SwordStorm"), TEXT("LeapSlam"), TEXT("Guard"), TEXT("StoneRecovery")})
    {
        auto* Definition = LoadObject<UUmbraSweepDefinition>(nullptr, *FString::Printf(TEXT("/Game/Skills/Greystone/DA_%s"), Name));
        if (!TestNotNull(Name, Definition)) return false;
        FString Reason;
        TestTrue(Name, Definition->Validate(Reason));
        TestNotNull(TEXT("Configured icon"), Definition->Icon.Get());
        if (Definition->Execution == EUmbraSkillExecution::SwordStorm)
            TestTrue(TEXT("Storm animation spans the six pulses"), Definition->Montage && Definition->Montage->GetPlayLength()/Definition->PlayRate >= 3.f);
        TestTrue(TEXT("Montage uses the character's output slot"), Definition->Montage && Definition->Montage->SlotAnimTracks.Num() == 1
            && Definition->Montage->SlotAnimTracks[0].SlotName == TEXT("UpperBody"));
        if (Definition->Execution == EUmbraSkillExecution::StoneRecovery && Definition->Montage)
        {
            const auto& Segments = Definition->Montage->SlotAnimTracks[0].AnimTrack.AnimSegments;
            const auto* Animation = Segments.Num() ? Cast<UAnimSequence>(Segments[0].GetAnimReference()) : nullptr;
            TestTrue(TEXT("Recovery uses bounded visual slam, not global root locking"), Animation && !Animation->bForceRootLock
                && !Animation->bEnableRootMotion && Animation->GetName() == TEXT("A_StoneRecovery_Slam"));
        }
        Cooldowns.Add(Definition->CooldownTag);
        if (Definition->Execution != EUmbraSkillExecution::Guard)
        {
            TestTrue(TEXT("Circular skill includes rear targets"), Definition->ContainsTarget(FVector::ZeroVector,FVector::ForwardVector,FVector(-180,0,0)));
            TestFalse(TEXT("Circular skill excludes another floor"), Definition->ContainsTarget(FVector::ZeroVector,FVector::ForwardVector,FVector(0,0,500)));
        }
        auto* Invalid = DuplicateObject<UUmbraSweepDefinition>(Definition, GetTransientPackage());
        switch (Invalid->Execution)
        {
        case EUmbraSkillExecution::SwordStorm: Invalid->PulseInterval = 0.f; break;
        case EUmbraSkillExecution::LeapSlam: Invalid->LeapDuration = 0.f; break;
        case EUmbraSkillExecution::Guard: Invalid->GuardArmor = -1.f; break;
        case EUmbraSkillExecution::StoneRecovery: Invalid->HealMaxHealthFraction = 2.f; break;
        default: break;
        }
        TestFalse(TEXT("Unsafe tuning rejected"), Invalid->Validate(Reason));
    }
    TestEqual(TEXT("Independent cooldown tags"), Cooldowns.Num(), 4);

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    auto* PS = World->SpawnActor<AUmbraPlayerState>();
    auto* ASC = PS->GetUmbraAbilitySystemComponent();
    ASC->InitAbilityActorInfo(PS, PS); PS->InitializeAttributes();
    ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetArmorAttribute(), 0.f);
    ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetSlashingResistanceAttribute(), 0.f);
    ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(), 100.f);
    FUmbraDamageRequest Damage(EUmbraDamageSource::Skill);
    Damage.Config.Typed.Model = EUmbraDamageModel::ExplicitChannels;
    Damage.Config.Typed.bCanCritical = false;
    auto& Channel = Damage.Config.Typed.Channels.AddDefaulted_GetRef();
    Channel.Type = EUmbraWeaponDamageType::Slashing;
    Channel.BaseDamage = 30.f;
    TestTrue(TEXT("Baseline typed damage"), UmbraDamage::Apply(ASC, ASC, Damage));
    const float Baseline = 100.f - ASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute());
    ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(), 100.f);
    auto Guard = ASC->MakeOutgoingSpec(UUmbraSkillGuardEffect::StaticClass(),1.f,ASC->MakeEffectContext());
    Guard.Data->SetDuration(2.5f,true);
    Guard.Data->SetSetByCallerMagnitude(FName(TEXT("GuardArmor")),100.f);
    Guard.Data->SetSetByCallerMagnitude(FName(TEXT("GuardResistance")),50.f);
    auto GuardHandle = ASC->ApplyGameplayEffectSpecToSelf(*Guard.Data.Get());
    TestTrue(TEXT("Guarded typed damage"), UmbraDamage::Apply(ASC, ASC, Damage));
    const float Guarded = 100.f - ASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute());
    TestTrue(TEXT("Guard mitigates through the real damage execution"), Baseline > 0.f && Guarded > 0.f && Guarded < Baseline * .6f);
    ASC->RemoveActiveGameplayEffect(GuardHandle);
    TestEqual(TEXT("Guard removal restores armor"),ASC->GetNumericAttribute(UUmbraAttributeSet::GetArmorAttribute()),0.f);
    TestEqual(TEXT("Guard removal restores resistance"),ASC->GetNumericAttribute(UUmbraAttributeSet::GetSlashingResistanceAttribute()),0.f);
    auto Heal = ASC->MakeOutgoingSpec(UUmbraSkillHealEffect::StaticClass(),1.f,ASC->MakeEffectContext());
    Heal.Data->SetSetByCallerMagnitude(FName(TEXT("SkillHeal")),1000.f);
    ASC->ApplyGameplayEffectSpecToSelf(*Heal.Data.Get());
    TestEqual(TEXT("Healing clamps to MaxHealth"),ASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()),ASC->GetNumericAttribute(UUmbraAttributeSet::GetMaxHealthAttribute()));
    GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
    return true;
}
#endif
