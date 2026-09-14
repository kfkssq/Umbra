#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/Effects/UmbraDebugEffects.h"
#include "Characters/UmbraEnemyCharacter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "UI/UmbraAttributeDebugPanel.h"
#include "UmbraPlayerController.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraAttributeDebugTest, "Umbra.Attributes.DebugOperations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraAttributeDebugTest::RunTest(const FString& Parameters)
{
	// Use the project's existing concrete controller BP, without modifying its asset defaults.
	UClass* ControllerClass = LoadClass<AUmbraPlayerController>(nullptr,
		TEXT("/Game/Blueprints/Player/BP_UmbraPlayerController.BP_UmbraPlayerController_C"));
	if (!TestNotNull(TEXT("Project controller class"), ControllerClass))
	{
		return false;
	}
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	// Actor::ProcessEvent intentionally skips calls before this initialization.
	World->InitializeActorsForPlay(FURL());
	AUmbraPlayerController* PC = World->SpawnActor<AUmbraPlayerController>(ControllerClass);
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AUmbraEnemyCharacter* PlayerProxy = World->SpawnActor<AUmbraEnemyCharacter>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	AUmbraEnemyCharacter* Enemy = World->SpawnActor<AUmbraEnemyCharacter>(FVector(200.f, 0.f, 0.f), FRotator::ZeroRotator, SpawnParams);
	// Only the ownership/authority path is tested here, not player character presentation.
	PC->Possess(PlayerProxy);
	auto ReadyASC = [](AUmbraEnemyCharacter* Actor)
	{
		UUmbraAbilitySystemComponent* ASC = CastChecked<UUmbraAbilitySystemComponent>(Actor->GetAbilitySystemComponent());
		if (!ASC->HasBeenInitialized())
		{
			ASC->InitializeComponent();
		}
		ASC->InitAbilityActorInfo(Actor, Actor);
		ASC->InitializeAttributes(nullptr);
		return ASC;
	};
	UUmbraAbilitySystemComponent* PlayerASC = ReadyASC(PlayerProxy);
	UUmbraAbilitySystemComponent* EnemyASC = ReadyASC(Enemy);
	const UUmbraAttributeSet* PlayerAttributes = PlayerASC->GetSet<UUmbraAttributeSet>();
	const UUmbraAttributeSet* EnemyAttributes = EnemyASC->GetSet<UUmbraAttributeSet>();

	FBoolProperty* EnableProperty = FindFProperty<FBoolProperty>(AUmbraPlayerController::StaticClass(), TEXT("bEnableAttributeDebugPanel"));
	UFunction* OperationFunction = PC->FindFunction(TEXT("ServerAttributeDebugOperation"));
	auto Operate = [PC, OperationFunction](AActor* Target, EUmbraAttributeDebugOperation Operation)
	{
		struct FParameters
		{
			AActor* Target;
			EUmbraAttributeDebugOperation Operation;
		} Params{ Target, Operation };
		PC->ProcessEvent(OperationFunction, &Params);
	};
	EnableProperty->SetPropertyValue_InContainer(PC, false);
	Operate(PlayerProxy, EUmbraAttributeDebugOperation::AddEffect);
	TestEqual(TEXT("Disabled feature rejects mutation"), PlayerAttributes->GetAttackPower(), 10.f);
	EnableProperty->SetPropertyValue_InContainer(PC, true);
	Operate(PlayerProxy, EUmbraAttributeDebugOperation::Damage);
	TestEqual(TEXT("Native damage uses IncomingDamage"), PlayerAttributes->GetHealth(), 90.f);
	TestEqual(TEXT("IncomingDamage cleared"), PlayerAttributes->GetIncomingDamage(), 0.f);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Operate(PlayerProxy, EUmbraAttributeDebugOperation::AddEffect);
	}
	TestEqual(TEXT("Repeated add does not stack"), PlayerAttributes->GetAttackPower(), 20.f);
	TestEqual(TEXT("Test max health buff"), PlayerAttributes->GetMaxHealth(), 200.f);
	TestEqual(TEXT("Max health increase does not heal"), PlayerAttributes->GetHealth(), 90.f);

	Operate(Enemy, EUmbraAttributeDebugOperation::AddEffect);
	TestEqual(TEXT("Independent enemy handle"), EnemyAttributes->GetAttackPower(), 20.f);
	Operate(PlayerProxy, EUmbraAttributeDebugOperation::RemoveEffect);
	TestEqual(TEXT("Remove player buff"), PlayerAttributes->GetAttackPower(), 10.f);
	TestEqual(TEXT("Enemy buff survives player removal"), EnemyAttributes->GetAttackPower(), 30.f);
	Operate(Enemy, EUmbraAttributeDebugOperation::RemoveEffect);
	TestEqual(TEXT("Remove enemy after switching back"), EnemyAttributes->GetAttackPower(), 10.f);

	// An identical effect created by somebody else must survive this controller's removal.
	const FGameplayEffectSpecHandle OtherSpec = PlayerASC->MakeOutgoingSpec(UUmbraDebugAttributeEffect::StaticClass(), 1.f, PlayerASC->MakeEffectContext());
	const FActiveGameplayEffectHandle OtherHandle = PlayerASC->ApplyGameplayEffectSpecToSelf(*OtherSpec.Data.Get());
	Operate(PlayerProxy, EUmbraAttributeDebugOperation::AddEffect);
	Operate(PlayerProxy, EUmbraAttributeDebugOperation::RemoveEffect);
	TestTrue(TEXT("Unrelated same-class effect retained"), PlayerASC->GetActiveGameplayEffect(OtherHandle) != nullptr);
	TestEqual(TEXT("Only unrelated attack buff remains"), PlayerAttributes->GetAttackPower(), 30.f);
	PlayerASC->RemoveActiveGameplayEffect(OtherHandle);

	Operate(PlayerProxy, EUmbraAttributeDebugOperation::AddEffect);
	for (int32 Index = 0; Index < 20; ++Index)
	{
		Operate(PlayerProxy, EUmbraAttributeDebugOperation::Heal);
	}
	TestEqual(TEXT("Healing capped at buffed max"), PlayerAttributes->GetHealth(), 200.f);
	Operate(PlayerProxy, EUmbraAttributeDebugOperation::RemoveEffect);
	TestEqual(TEXT("Buff removal clips current health"), PlayerAttributes->GetHealth(), 100.f);
	Operate(PlayerProxy, EUmbraAttributeDebugOperation::Heal);
	TestEqual(TEXT("Healing capped at normal max"), PlayerAttributes->GetHealth(), 100.f);
	Operate(PlayerProxy, EUmbraAttributeDebugOperation::Damage);
	Operate(PlayerProxy, EUmbraAttributeDebugOperation::RemoveEffect);
	TestEqual(TEXT("Unrelated operation does not repeat damage"), PlayerAttributes->GetHealth(), 90.f);
	TestTrue(TEXT("Snapshot formats total crit multiplier as percent"),
		UUmbraAttributeDebugPanel::FormatAttributeSnapshot(*PlayerAttributes).ToString().Contains(TEXT("200.0%")));
	TestFalse(TEXT("Snapshot excludes damage meta"),
		UUmbraAttributeDebugPanel::FormatAttributeSnapshot(*PlayerAttributes).ToString().Contains(TEXT("IncomingDamage")));

	Operate(Enemy, EUmbraAttributeDebugOperation::AddEffect);
	Enemy->Destroy();
	Operate(PlayerProxy, EUmbraAttributeDebugOperation::AddEffect);
	PC->ProcessEvent(PC->FindFunction(TEXT("ServerClearAttributeDebugEffects")), nullptr);
	TestEqual(TEXT("Cleanup removes owned effects safely after target destruction"), PlayerAttributes->GetAttackPower(), 10.f);
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
