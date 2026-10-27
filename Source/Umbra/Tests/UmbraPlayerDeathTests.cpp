#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Characters/UmbraPlayerCharacter.h"
#include "Player/UmbraPlayerState.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/Effects/UmbraDebugEffects.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraPlayerDeathTest, "Umbra.Combat.PlayerDeath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraPlayerDeathTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    UClass* PlayerClass = LoadClass<AUmbraPlayerCharacter>(nullptr,
        TEXT("/Game/Blueprints/Player/BP_UmbraPlayerCharacter.BP_UmbraPlayerCharacter_C"));
    if (!TestNotNull(TEXT("Player blueprint"), PlayerClass))
    {
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
        return false;
    }
    AUmbraPlayerCharacter* Player = World->SpawnActor<AUmbraPlayerCharacter>(PlayerClass);
    AUmbraPlayerState* State = World->SpawnActor<AUmbraPlayerState>();
    Player->SetPlayerState(State);
    Player->OnRep_PlayerState();
    Player->DispatchBeginPlay();
    auto* ASC = State->GetUmbraAbilitySystemComponent();
    TestFalse(TEXT("Alive initially"), Player->IsDead());
    ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(), 10.f);
    auto Damage = ASC->MakeOutgoingSpec(UUmbraDebugDamageEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
    ASC->ApplyGameplayEffectSpecToSelf(*Damage.Data.Get());
    TestTrue(TEXT("Lethal GAS damage enters death"), Player->IsDead());
    TestTrue(TEXT("Death tag"), ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Dead));
    TestFalse(TEXT("Movement disabled"), Player->IsMovementEnabled());
    TestEqual(TEXT("Movement mode disabled"), Player->GetCharacterMovement()->MovementMode.GetValue(), MOVE_None);
    TestEqual(TEXT("Capsule disabled"), Player->GetCapsuleComponent()->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
    Player->FinishSpawnAnimation();
    TestFalse(TEXT("Late spawn callback cannot revive"), Player->IsMovementEnabled());
    TestFalse(TEXT("Attack rejected"), Player->TryActivatePrimaryAttack(State));
    ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(), 25.f);
    ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(), 0.f);
    TestEqual(TEXT("Repeated health crossings do not stack tag"), ASC->GetTagCount(UmbraGameplayTags::State_Dead), 1);
    Player->Destroy();
    TestFalse(TEXT("Persistent ASC removes dead pawn listener"), ASC->GetGameplayAttributeValueChangeDelegate(
        UUmbraAttributeSet::GetHealthAttribute()).IsBoundToObject(Player));
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
#endif
