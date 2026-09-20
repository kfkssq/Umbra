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
	TestEqual(TEXT("Three layers raise health by 60"), PlayerAttributes->GetHealth(), 150.f);
	TestEqual(TEXT("Three layers raise max health by 60"), PlayerAttributes->GetMaxHealth(), 160.f);
	TestEqual(TEXT("Three layers raise health regen by 60"), PlayerAttributes->GetHealthRegen(), 60.f);
	TestEqual(TEXT("Three layers raise resource by 60"), PlayerAttributes->GetResource(), 160.f);
	TestEqual(TEXT("Three layers raise max resource by 60"), PlayerAttributes->GetMaxResource(), 160.f);
	TestEqual(TEXT("Three layers raise resource regen by 60"), PlayerAttributes->GetResourceRegen(), 60.f);
	TestEqual(TEXT("Three layers raise strength by 60"), PlayerAttributes->GetAttackPower(), 70.f);
	TestEqual(TEXT("Three layers raise intelligence by 60"), PlayerAttributes->GetAbilityPower(), 60.f);
	TestEqual(TEXT("Three layers add 0.6x attack speed"), PlayerAttributes->GetAttackSpeed(), 1.6f);
	TestEqual(TEXT("Three layers add 60 percentage points of critical chance"), PlayerAttributes->GetCriticalChance(), 0.6f);
	TestEqual(TEXT("Three layers add 60 percentage points to critical damage multiplier"),
		PlayerAttributes->GetCriticalDamageMultiplier(), 2.6f);
	TestEqual(TEXT("Three layers raise armor by 60"), PlayerAttributes->GetArmor(), 60.f);
	TestEqual(TEXT("Three layers raise magic resistance by 60"), PlayerAttributes->GetMagicResistance(), 60.f);
	TestEqual(TEXT("Three layers raise ability haste by 60"), PlayerAttributes->GetAbilityHaste(), 60.f);
	TestEqual(TEXT("Three layers raise move speed by 60"), PlayerAttributes->GetMoveSpeed(), 660.f);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Operate(PlayerProxy, EUmbraAttributeDebugOperation::AddEffect);
	}
	TestEqual(TEXT("Add Effect continues stacking beyond the first layers"), PlayerAttributes->GetAttackPower(), 130.f);
	TestEqual(TEXT("Critical chance remains capped at 100 percent"), PlayerAttributes->GetCriticalChance(), 1.f);

	Operate(Enemy, EUmbraAttributeDebugOperation::AddEffect);
	TestEqual(TEXT("Independent enemy handle (10 base + 20 buff)"), EnemyAttributes->GetAttackPower(), 30.f);
	Operate(PlayerProxy, EUmbraAttributeDebugOperation::RemoveEffect);
	TestEqual(TEXT("Remove clears every player layer"), PlayerAttributes->GetAttackPower(), 10.f);
	TestEqual(TEXT("Remove restores capped critical chance"), PlayerAttributes->GetCriticalChance(), 0.f);
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
	TestEqual(TEXT("Healing capped at buffed max"), PlayerAttributes->GetHealth(), 120.f);
	Operate(PlayerProxy, EUmbraAttributeDebugOperation::RemoveEffect);
	TestEqual(TEXT("Buff removal clips current health"), PlayerAttributes->GetHealth(), 100.f);
	Operate(PlayerProxy, EUmbraAttributeDebugOperation::Heal);
	TestEqual(TEXT("Healing capped at normal max"), PlayerAttributes->GetHealth(), 100.f);
	Operate(PlayerProxy, EUmbraAttributeDebugOperation::Damage);
	Operate(PlayerProxy, EUmbraAttributeDebugOperation::RemoveEffect);
	TestEqual(TEXT("Unrelated operation does not repeat damage"), PlayerAttributes->GetHealth(), 90.f);
	PlayerASC->SetNumericAttributeBase(UUmbraAttributeSet::GetAttackSpeedAttribute(), 1.25f);
	PlayerASC->SetNumericAttributeBase(UUmbraAttributeSet::GetCriticalChanceAttribute(), 0.4f);
	PlayerASC->SetNumericAttributeBase(UUmbraAttributeSet::GetCriticalDamageMultiplierAttribute(), 1.75f);
	const FUmbraAttributeDebugViewState ViewState = UUmbraAttributeDebugPanel::MakeViewState(
		PlayerAttributes, PlayerProxy, true, true, EUmbraAttributeDebugFeedback::ViewingPlayer);
	TestEqual(TEXT("View state supplies direct attack speed"), ViewState.AttackSpeed, 1.25f);
	TestEqual(TEXT("View state formats attack speed with two decimals"), ViewState.AttackSpeedDisplay.ToString(), FString(TEXT("1.25")));
	PlayerASC->SetNumericAttributeBase(UUmbraAttributeSet::GetAttackSpeedAttribute(), 2.3f);
	const FUmbraAttributeDebugViewState FasterViewState = UUmbraAttributeDebugPanel::MakeViewState(
		PlayerAttributes, PlayerProxy, true, true, EUmbraAttributeDebugFeedback::ViewingPlayer);
	TestEqual(TEXT("View state retains trailing attack speed zero"), FasterViewState.AttackSpeedDisplay.ToString(), FString(TEXT("2.30")));
	TestEqual(TEXT("View state supplies critical chance as percent"), ViewState.CriticalChance, 40.f);
	TestEqual(TEXT("View state supplies critical damage as percent"), ViewState.CriticalDamageMultiplier, 175.f);
	TestEqual(TEXT("View state passes current health"), ViewState.Health, 90.f);
	TestEqual(TEXT("View state passes max health"), ViewState.MaxHealth, PlayerAttributes->GetMaxHealth());
	TestEqual(TEXT("View state passes health regen"), ViewState.HealthRegen, PlayerAttributes->GetHealthRegen());
	TestEqual(TEXT("View state passes resource"), ViewState.Resource, PlayerAttributes->GetResource());
	TestEqual(TEXT("View state passes max resource"), ViewState.MaxResource, PlayerAttributes->GetMaxResource());
	TestEqual(TEXT("View state passes resource regen"), ViewState.ResourceRegen, PlayerAttributes->GetResourceRegen());
	TestEqual(TEXT("View state passes strength"), ViewState.AttackPower, PlayerAttributes->GetAttackPower());
	TestEqual(TEXT("View state passes intelligence"), ViewState.AbilityPower, PlayerAttributes->GetAbilityPower());
	TestEqual(TEXT("View state passes armor"), ViewState.Armor, PlayerAttributes->GetArmor());
	TestEqual(TEXT("View state passes magic resistance"), ViewState.MagicResistance, PlayerAttributes->GetMagicResistance());
	TestEqual(TEXT("View state passes ability haste"), ViewState.AbilityHaste, PlayerAttributes->GetAbilityHaste());
	TestEqual(TEXT("View state passes move speed"), ViewState.MoveSpeed, PlayerAttributes->GetMoveSpeed());
	TestEqual(TEXT("View state passes target"), ViewState.TargetActor.Get(), static_cast<AActor*>(PlayerProxy));
	TestTrue(TEXT("View state passes player-view context"), ViewState.bViewingPlayer);
	TestEqual(TEXT("View state passes feedback enum"), ViewState.Feedback, EUmbraAttributeDebugFeedback::ViewingPlayer);
	TestTrue(TEXT("View state is ready when GAS attributes are available"), ViewState.bReady);

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
