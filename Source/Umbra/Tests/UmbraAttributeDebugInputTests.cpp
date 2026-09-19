#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/Effects/UmbraDebugEffects.h"
#include "Characters/UmbraEnemyCharacter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EnhancedPlayerInput.h"
#include "InputMappingContext.h"
#include "UI/UmbraAttributeDebugPanel.h"
#include "Widgets/SWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraAttributeDebugInputTest, "Umbra.Attributes.DebugInputAndWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraAttributeDebugInputTest::RunTest(const FString& Parameters)
{
	// Validate the effective config, not merely the text in DefaultInput.ini.
	for (const FKeyBind& Binding : GetDefault<UEnhancedPlayerInput>()->DebugExecBindings)
	{
		TestFalse(TEXT("F1/F2 must not execute legacy viewmode commands"),
			(Binding.Key == EKeys::F1 || Binding.Key == EKeys::F2)
			&& !Binding.bDisabled && Binding.Command.Contains(TEXT("viewmode")));
	}
	const UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_AttributeDebug.IMC_AttributeDebug"));
	if (!TestNotNull(TEXT("Configured debug IMC exists"), Context))
	{
		return false;
	}
	int32 PlayerMappings = 0;
	int32 EnemyMappings = 0;
	for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
	{
		if (Mapping.Key == EKeys::F1 && GetNameSafe(Mapping.Action) == TEXT("IA_Debug_ViewPlayer")) ++PlayerMappings;
		if (Mapping.Key == EKeys::F2 && GetNameSafe(Mapping.Action) == TEXT("IA_Debug_LockHovered")) ++EnemyMappings;
	}
	TestEqual(TEXT("F1 maps exactly once to player view"), PlayerMappings, 1);
	TestEqual(TEXT("F2 maps exactly once to enemy lock"), EnemyMappings, 1);

	UClass* WidgetClass = LoadClass<UUmbraAttributeDebugPanel>(nullptr,
		TEXT("/Game/UI/Debug/WBP_AttributeDebugPanel.WBP_AttributeDebugPanel_C"));
	if (!TestNotNull(TEXT("Actual WBP inherits attribute debug panel"), WidgetClass))
	{
		return false;
	}
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	AUmbraEnemyCharacter* Enemy = World->SpawnActor<AUmbraEnemyCharacter>();
	Enemy->DispatchBeginPlay();
	UUmbraAbilitySystemComponent* ASC = CastChecked<UUmbraAbilitySystemComponent>(Enemy->GetAbilitySystemComponent());
	UUmbraAttributeDebugPanel* Panel = CreateWidget<UUmbraAttributeDebugPanel>(World, WidgetClass);
	if (!TestNotNull(TEXT("Actual WBP can be instantiated"), Panel))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		return false;
	}
	// Build the actual designer tree, invoke NativeConstruct, and use the same view
	// method called by F2. No viewport rendering or hardware mouse is simulated.
	TSharedPtr<SWidget> SlateWidget = Panel->TakeWidget();
	Panel->ViewEnemy(Enemy);
	TestEqual(TEXT("Enemy target passed to actual WBP"), Panel->GetViewState().TargetActor.Get(), static_cast<AActor*>(Enemy));
	TestEqual(TEXT("Enemy name passed to actual WBP"), Panel->GetViewState().TargetName, Enemy->GetActorNameOrLabel());
	TestEqual(TEXT("Initial enemy health passed as raw value"), Panel->GetViewState().Health, 100.f);
	const FGameplayEffectSpecHandle DamageSpec = ASC->MakeOutgoingSpec(UUmbraDebugDamageEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
	ASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());
	TestEqual(TEXT("Enemy Health updates by delegate"), Panel->GetViewState().Health, 90.f);
	TestTrue(TEXT("Enemy attribute listener installed"), ASC->GetGameplayAttributeValueChangeDelegate(
		UUmbraAttributeSet::GetHealthAttribute()).IsBoundToObject(Panel));
	Enemy->Destroy();
	TestTrue(TEXT("Destroyed enemy falls back to player mode"), Panel->GetViewState().bViewingPlayer);
	TestFalse(TEXT("No local player in isolated world is not ready"), Panel->GetViewState().bReady);
	Panel->ShutdownPanel();
	TestFalse(TEXT("Widget lifecycle listener removed"), UUmbraAbilitySystemComponent::OnLifecycleChanged.IsBoundToObject(Panel));
	Panel->ShutdownPanel();
	SlateWidget.Reset();
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
