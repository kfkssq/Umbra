#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Skills/UmbraSweepDefinition.h"
#include "Animation/AnimMontage.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundNodeRandom.h"
#include "Sound/SoundNodeWavePlayer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraPebbleDefinitionTest, "Umbra.Skills.PebbleDefinition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraPebbleDefinitionTest::RunTest(const FString& Parameters)
{
    const auto* Asset = LoadObject<UUmbraSweepDefinition>(nullptr, TEXT("/Game/Skills/GlintstonePebble/DA_GlintstonePebble"));
    if (!TestNotNull(TEXT("Playable definition exists"), Asset)) return false;
    FString Reason;
    TestTrue(TEXT("Authored projectile validates"), Asset->Validate(Reason));
    TestTrue(TEXT("Release precedes montage completion"), Asset->StrikeTime < Asset->Montage->GetPlayLength());
    TestEqual(TEXT("Seven resource per commit"), Asset->ResourceCost, 7.f);
    TestEqual(TEXT("Uses existing typed GAS damage"), Asset->Damage.Typed.Model, EUmbraDamageModel::ExplicitChannels);
    auto* Copy = DuplicateObject<UUmbraSweepDefinition>(Asset, GetTransientPackage());
    Copy->ProjectileMaxSpeed = Copy->ProjectileInitialSpeed - 1.f;
    TestFalse(TEXT("Reject inverted speed bounds"), Copy->Validate(Reason));
    Copy->ProjectileMaxSpeed = Asset->ProjectileMaxSpeed;
    Copy->ProjectileLifetime = 0.f;
    TestFalse(TEXT("Reject zero lifespan"), Copy->Validate(Reason));
    Copy->ProjectileLifetime = Asset->ProjectileLifetime;
    Copy->ProjectileEffect = nullptr;
    TestFalse(TEXT("Reject invisible flight"), Copy->Validate(Reason));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraPebbleOriginalAudioTest, "Umbra.Skills.PebbleOriginalAudio",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraPebbleOriginalAudioTest::RunTest(const FString& Parameters)
{
    const auto* Asset = LoadObject<UUmbraSweepDefinition>(nullptr, TEXT("/Game/Skills/GlintstonePebble/DA_GlintstonePebble"));
    if (!TestNotNull(TEXT("Definition"), Asset)) return false;
    TestNull(TEXT("Synthetic charge removed"), Asset->CastSound.Get());
    const auto* Release = Cast<USoundWave>(Asset->StrikeSound);
    const auto* Flight = Cast<USoundWave>(Asset->ProjectileSound);
    const auto* Impact = Cast<USoundCue>(Asset->ImpactSound);
    if (!TestNotNull(TEXT("Release"), Release) || !TestNotNull(TEXT("Flight"), Flight)
        || !TestNotNull(TEXT("Impact cue"), Impact)) return false;
    TestEqual(TEXT("Release media"), Release->GetName(), FString(TEXT("S_ER_Pebble_Release_524617251")));
    TestEqual(TEXT("Flight media"), Flight->GetName(), FString(TEXT("S_ER_Pebble_Flight_1040010023")));
    TestFalse(TEXT("Release one-shot"), bool(Release->bLooping));
    TestTrue(TEXT("Flight loops"), bool(Flight->bLooping));
    TestTrue(TEXT("Exact decoded loop duration"), FMath::IsNearlyEqual(Flight->Duration, 18687.f/44100.f, .0001f));
    TestTrue(TEXT("Default release gain"), FMath::IsNearlyEqual(Release->Volume, FMath::Pow(10.f,-3.f/20.f)));
    TestTrue(TEXT("Default flight gain"), FMath::IsNearlyEqual(Flight->Volume, FMath::Pow(10.f,-15.f/20.f)));
    TestNotNull(TEXT("Spatial flight attenuation"), Flight->AttenuationSettings.Get());
    const auto* Random = Cast<USoundNodeRandom>(Impact->FirstNode);
    if (!TestNotNull(TEXT("Random variants"), Random)) return false;
    TestEqual(TEXT("Three variants"), Random->ChildNodes.Num(), 3);
    TSet<FString> Expected{TEXT("S_ER_Pebble_Impact_82601330"), TEXT("S_ER_Pebble_Impact_164447618"), TEXT("S_ER_Pebble_Impact_123415567")};
    for (const auto& Child : Random->ChildNodes)
    {
        const auto* Player = Cast<USoundNodeWavePlayer>(Child);
        if (!TestNotNull(TEXT("Wave player"), Player) || !TestNotNull(TEXT("Original variant"), Player->GetSoundWave())) return false;
        TestTrue(TEXT("Expected unique original media"), Expected.Remove(Player->GetSoundWave()->GetName()) == 1);
        TestFalse(TEXT("Impact one-shot"), bool(Player->bLooping));
    }
    TestEqual(TEXT("All variants wired"), Expected.Num(), 0);
    return true;
}
#endif
