#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UmbraPlayerController.h"
#include "Player/UmbraPlayerState.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/Effects/UmbraDebugEffects.h"
#include "Components/InputComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Characters/UmbraEnemyCharacter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraQuickEffectTest, "Umbra.Debug.QuickGameplayEffect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraQuickEffectTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	UClass* PCClass = LoadClass<AUmbraPlayerController>(nullptr,
		TEXT("/Game/Blueprints/Player/BP_UmbraPlayerController.BP_UmbraPlayerController_C"));
	if (!TestNotNull(TEXT("Player controller class"), PCClass)) { World->DestroyWorld(false); return false; }
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	// CanBeAttacked is a BlueprintNativeEvent; Actor::ProcessEvent needs actors initialized for play.
	World->InitializeActorsForPlay(FURL());
	auto* PC = World->SpawnActor<AUmbraPlayerController>(PCClass);
	PC->Player = NewObject<ULocalPlayer>(GEngine);
	PC->InputComponent = NewObject<UInputComponent>(PC);
	PC->SetupQuickGameplayEffectInput();
	PC->SetupQuickGameplayEffectInput();
	TestEqual(TEXT("Setup twice creates only four key bindings"), PC->InputComponent->KeyBindings.Num(), 4);
	TestNotNull(TEXT("Default test asset class resolves"), PC->QuickTestGameplayEffect.LoadSynchronous());
	TestNotNull(TEXT("Default vulnerable asset class resolves"), PC->QuickVulnerableGameplayEffect.LoadSynchronous());
	auto MakeState = [&]()
	{
		auto* State = World->SpawnActor<AUmbraPlayerState>();
		auto* ASC = State->GetUmbraAbilitySystemComponent();
		if (!ASC->HasBeenInitialized()) ASC->InitializeComponent();
		ASC->InitAbilityActorInfo(State, State); State->InitializeAttributes();
		return State;
	};
	auto* State = MakeState();
	PC->PlayerState = State;
	auto* ASC = State->GetUmbraAbilitySystemComponent();
	PC->QuickTestGameplayEffect = UUmbraDebugDerivedAttributeEffect::StaticClass();
	const auto First = PC->ApplyQuickTestGameplayEffect();
	TestTrue(TEXT("Apply produces saved active handle"), First.IsValid() && First == PC->QuickTestGameplayEffectHandle);
	for (int32 Index = 0; Index < 10; ++Index)
		TestTrue(TEXT("Repeated apply returns same handle"), PC->ApplyQuickTestGameplayEffect() == First);
	TestEqual(TEXT("Exactly one effect, no stacking"), ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), 1);
	TestEqual(TEXT("Level1"), ASC->GetActiveGameplayEffect(First)->Spec.GetLevel(), 1.f);
	TestEqual(TEXT("Bonus applied once"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetStrengthAttribute()), 20.f);
	PC->RemoveQuickTestGameplayEffect();
	PC->RemoveQuickTestGameplayEffect();
	TestEqual(TEXT("Remove restores stats"), ASC->GetNumericAttribute(UUmbraAttributeSet::GetStrengthAttribute()), 0.f);
	TestFalse(TEXT("Stored handle cleared"), PC->QuickTestGameplayEffectHandle.IsValid());
	const auto Second = PC->ApplyQuickTestGameplayEffect();
	ASC->RemoveActiveGameplayEffect(Second);
	TestTrue(TEXT("Externally removed effect can be applied again"), PC->ApplyQuickTestGameplayEffect().IsValid());
	PC->RemoveQuickTestGameplayEffect();
	const auto External = ASC->ApplyGameplayEffectToSelf(GetDefault<UUmbraDebugDerivedAttributeEffect>(), 1.f, ASC->MakeEffectContext());
	AddExpectedError(TEXT("already active from another source"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("External same class blocks duplication"), PC->ApplyQuickTestGameplayEffect().IsValid());
	PC->RemoveQuickTestGameplayEffect();
	TestNotNull(TEXT("Remove never touches external source"), ASC->GetActiveGameplayEffect(External));
	ASC->RemoveActiveGameplayEffect(External);
	PC->ApplyQuickTestGameplayEffect();
	auto* NextState = MakeState();
	PC->PlayerState = NextState;
	TestTrue(TEXT("New player state can receive buff"), PC->ApplyQuickTestGameplayEffect().IsValid());
	TestEqual(TEXT("Old ASC effect cleaned on switch"), ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), 0);
	PC->RemoveQuickTestGameplayEffect();
	TestEqual(TEXT("New ASC effect removable"), NextState->GetUmbraAbilitySystemComponent()->GetActiveEffects(FGameplayEffectQuery()).Num(), 0);
	// Use a known native effect to verify ownership independently of the user's GE asset tuning.
	PC->QuickVulnerableGameplayEffect = UUmbraDebugDerivedAttributeEffect::StaticClass();
	auto MakeEnemy = [&]()
	{
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto* Enemy = World->SpawnActor<AUmbraEnemyCharacter>(AUmbraEnemyCharacter::StaticClass(), FTransform::Identity, Spawn);
		auto* EnemyASC = Enemy->GetUmbraAbilitySystemComponent();
		if (!EnemyASC->HasBeenInitialized()) EnemyASC->InitializeComponent();
		EnemyASC->InitAbilityActorInfo(Enemy, Enemy);
		return Enemy;
	};
	auto* Enemy = MakeEnemy();
	auto* EnemyASC = Enemy->GetUmbraAbilitySystemComponent();
	const auto EnemyHandle = PC->ApplyQuickVulnerableGameplayEffect(Enemy);
	TestTrue(TEXT("Enemy effect handle saved"), EnemyHandle.IsValid() && EnemyHandle == PC->QuickVulnerableGameplayEffectHandle);
	for (int32 Index = 0; Index < 10; ++Index)
		TestTrue(TEXT("Enemy repeated application is idempotent"), PC->ApplyQuickVulnerableGameplayEffect(Enemy) == EnemyHandle);
	TestEqual(TEXT("Enemy receives one bonus"), EnemyASC->GetNumericAttribute(UUmbraAttributeSet::GetStrengthAttribute()), 20.f);
	if (const auto* Active = EnemyASC->GetActiveGameplayEffect(EnemyHandle))
	{
		TestEqual(TEXT("Enemy effect level1"), Active->Spec.GetLevel(), 1.f);
		TestTrue(TEXT("Enemy effect source is player"), Active->Spec.GetContext().GetInstigatorAbilitySystemComponent() == NextState->GetUmbraAbilitySystemComponent());
	}
	AddExpectedError(TEXT("living enemy and ready player/enemy ASCs required"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Non-enemy rejected"), PC->ApplyQuickVulnerableGameplayEffect(NextState).IsValid());
	TestNotNull(TEXT("Invalid target keeps previous effect"), EnemyASC->GetActiveGameplayEffect(EnemyHandle));
	PC->ApplyQuickTestGameplayEffect();
	auto* OtherEnemy = MakeEnemy();
	TestTrue(TEXT("Switch enemy applies"), PC->ApplyQuickVulnerableGameplayEffect(OtherEnemy).IsValid());
	TestNull(TEXT("Switch enemy cleans previous"), EnemyASC->GetActiveGameplayEffect(EnemyHandle));
	PC->RemoveQuickVulnerableGameplayEffect();
	PC->RemoveQuickVulnerableGameplayEffect();
	TestEqual(TEXT("Enemy remove restores bonus"), OtherEnemy->GetUmbraAbilitySystemComponent()->GetNumericAttribute(UUmbraAttributeSet::GetStrengthAttribute()), 0.f);
	TestNotNull(TEXT("Enemy remove preserves player buff"), NextState->GetUmbraAbilitySystemComponent()->GetActiveGameplayEffect(PC->QuickTestGameplayEffectHandle));
	PC->RemoveQuickTestGameplayEffect();
	const auto EnemyExternal = EnemyASC->ApplyGameplayEffectToSelf(GetDefault<UUmbraDebugDerivedAttributeEffect>(), 1.f, EnemyASC->MakeEffectContext());
	AddExpectedError(TEXT("already active from another source"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Enemy external duplicate rejected"), PC->ApplyQuickVulnerableGameplayEffect(Enemy).IsValid());
	PC->RemoveQuickVulnerableGameplayEffect();
	TestNotNull(TEXT("Enemy external source preserved"), EnemyASC->GetActiveGameplayEffect(EnemyExternal));
	EnemyASC->RemoveActiveGameplayEffect(EnemyExternal);
	const auto Reapply = PC->ApplyQuickVulnerableGameplayEffect(Enemy);
	EnemyASC->RemoveActiveGameplayEffect(Reapply);
	TestTrue(TEXT("Enemy external removal allows reapply"), PC->ApplyQuickVulnerableGameplayEffect(Enemy).IsValid());
	PC->RemoveQuickVulnerableGameplayEffect();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
#endif
