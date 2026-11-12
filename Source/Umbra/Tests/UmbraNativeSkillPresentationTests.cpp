#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Skills/UmbraSweepDefinition.h"
#include "NiagaraSystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraNativeSkillPresentationTest, "Umbra.Skills.NativePresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUmbraNativeSkillPresentationTest::RunTest(const FString& Parameters)
{
    const auto* Existing = LoadObject<UUmbraSweepDefinition>(nullptr, TEXT("/Game/Skills/GlintstonePebble/DA_GlintstonePebble"));
    if (!TestNotNull(TEXT("Legacy skill fixture"), Existing)) return false;
    auto* Copy = DuplicateObject<UUmbraSweepDefinition>(Existing, GetTransientPackage());
    FString Reason;
    TestTrue(TEXT("Existing Cascade definition remains valid"), Copy->Validate(Reason));
    auto* Niagara = NewObject<UNiagaraSystem>();
    Copy->ProjectileNiagara = Niagara;
    TestFalse(TEXT("Reject double-rendered flight"), Copy->Validate(Reason));
    Copy->ProjectileEffect = nullptr;
    Copy->ImpactEffect = nullptr;
    Copy->ImpactNiagara = Niagara;
    TestTrue(TEXT("Niagara-only projectile is accepted"), Copy->Validate(Reason));
    Copy->ImpactNiagara = nullptr;
    TestFalse(TEXT("Missing impact is still rejected"), Copy->Validate(Reason));
    Copy->ImpactNiagara = Niagara;
    Copy->ActiveNiagara = Niagara;
    TestFalse(TEXT("Reject double-rendered cast"), Copy->Validate(Reason));
    Copy->ActiveEffect = nullptr;
    Copy->StrikeEffect = Existing->ImpactEffect;
    Copy->StrikeNiagara = Niagara;
    TestFalse(TEXT("Reject double-rendered release"), Copy->Validate(Reason));
    Copy->StrikeEffect = nullptr;
    TestTrue(TEXT("All Niagara phases accepted"), Copy->Validate(Reason));
    return true;
}
#endif
