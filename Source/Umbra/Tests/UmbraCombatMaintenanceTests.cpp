#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "AbilitySystem/Abilities/UmbraBasicAttackAbility.h"
#include "AbilitySystem/Abilities/UmbraEnemyBasicAttackAbility.h"
#include "AbilitySystem/Damage/UmbraDamageNotification.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/UmbraAnimNotifyState_AttackHitWindow.h"
#include "Characters/UmbraPlayerCharacter.h"
#include "Characters/UmbraEnemyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Player/UmbraPlayerState.h"
#include "UmbraPlayerController.h"
#include "UObject/UnrealType.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraCombatMaintenanceTest, "Umbra.Combat.Maintenance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraCombatMaintenanceTest::RunTest(const FString& Parameters)
{
	UClass* PlayerClass = LoadClass<AUmbraPlayerCharacter>(nullptr,
		TEXT("/Game/Blueprints/Player/BP_UmbraPlayerCharacter.BP_UmbraPlayerCharacter_C"));
	UClass* EnemyClass = LoadClass<AUmbraEnemyCharacter>(nullptr,
		TEXT("/Game/Blueprints/Enemies/BP_Enemy_Melee_01.BP_Enemy_Melee_01_C"));
	UClass* ControllerClass = LoadClass<AUmbraPlayerController>(nullptr,
		TEXT("/Game/Blueprints/Player/BP_UmbraPlayerController.BP_UmbraPlayerController_C"));
	UClass* AttackClass = LoadClass<UUmbraBasicAttackAbility>(nullptr,
		TEXT("/Game/Blueprints/Abilities/Attack/GA_BasicAttack.GA_BasicAttack_C"));
	UClass* EnemyAttackClass = LoadClass<UUmbraEnemyBasicAttackAbility>(nullptr,
		TEXT("/Game/Blueprints/Abilities/Attack/GA_EnemyBasicAttack.GA_EnemyBasicAttack_C"));
	if (!TestNotNull(TEXT("Actual player BP parent"), PlayerClass)
		|| !TestNotNull(TEXT("Actual enemy BP parent"), EnemyClass)
		|| !TestNotNull(TEXT("Actual controller BP parent"), ControllerClass)
		|| !TestNotNull(TEXT("Actual player attack BP parent"), AttackClass)
		|| !TestNotNull(TEXT("Actual enemy attack BP parent"), EnemyAttackClass)) return false;

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Player = World->SpawnActor<AUmbraPlayerCharacter>(PlayerClass, FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	auto* Enemy = World->SpawnActor<AUmbraEnemyCharacter>(EnemyClass, FVector(150.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	auto* PC = World->SpawnActor<AUmbraPlayerController>(ControllerClass);
	// LocalPredicted requires a local player even in an isolated standalone world.
	PC->Player = NewObject<ULocalPlayer>(GEngine);
	auto* PS = World->SpawnActor<AUmbraPlayerState>();
	PC->PlayerState = PS;
	PS->SetOwner(PC);
	if (!PS->GetUmbraAbilitySystemComponent()->HasBeenInitialized())
	{
		PS->GetUmbraAbilitySystemComponent()->InitializeComponent();
	}
	PC->Possess(Player);
	Player->DispatchBeginPlay();
	TestFalse(TEXT("Locomotion does not use controller yaw"), Player->bUseControllerRotationYaw);
	TestTrue(TEXT("CharacterMovement owns locomotion facing"),
		Player->GetCharacterMovement()->bOrientRotationToMovement);
	TestFalse(TEXT("Controller desired rotation is disabled"),
		Player->GetCharacterMovement()->bUseControllerDesiredRotation);
	TestTrue(TEXT("Locomotion yaw rate is finite and positive"),
		FMath::IsFinite(Player->GetCharacterMovement()->RotationRate.Yaw)
		&& Player->GetCharacterMovement()->RotationRate.Yaw > 0.f);
	AddInfo(FString::Printf(TEXT("Enemy asset AI enabled=%d; test enables AI on transient instance only"), Enemy->IsAIBehaviorEnabled()));
	FindFProperty<FBoolProperty>(AUmbraEnemyCharacter::StaticClass(), TEXT("bAIBehaviorEnabled"))->SetPropertyValue_InContainer(Enemy, true);
	Enemy->DispatchBeginPlay();
	auto* ASC = PS->GetUmbraAbilitySystemComponent();
	auto* EnemyASC = Enemy->GetUmbraAbilitySystemComponent();
	TestEqual(TEXT("Player starts with 500 movement speed"),
		ASC->GetNumericAttribute(UUmbraAttributeSet::GetMoveSpeedAttribute()), 500.f);
	TestEqual(TEXT("Initial movement speed uses authored 1x locomotion animation"),
		Player->GetLocomotionAnimationPlayRate(), 1.f);
	TestEqual(TEXT("Initial movement speed preserves 600 degrees per second yaw"),
		Player->GetCharacterMovement()->RotationRate.Yaw, 600.0);
	Player->GetMesh()->InitAnim(true);
	Enemy->GetMesh()->InitAnim(true);

	// Use the same world collision geometry and ray for old vs current selection.
	const FVector RayStart(-300.f, 0.f, 0.f), RayEnd(500.f, 0.f, 0.f);
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	FHitResult OldHit, NewHit;
	World->LineTraceSingleByObjectType(OldHit, RayStart, RayEnd, Objects);
	TestTrue(TEXT("Reproduced close-range selection: old query hits own pawn"), OldHit.GetActor() == Player);
	TestTrue(TEXT("Current selection query hits enemy behind own pawn"),
		PC->TraceAttackablePawn(RayStart, RayEnd, NewHit) && NewHit.GetActor() == Enemy);
	AddInfo(FString::Printf(TEXT("Selection baseline=%s corrected=%s"), *GetNameSafe(OldHit.GetActor()), *GetNameSafe(NewHit.GetActor())));

	// Runtime GE changes must affect the avatar, restore on removal and follow a rebind.
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetMoveSpeedAttribute(), 420.f);
	TestEqual(TEXT("GAS sets actual player speed"), Player->GetCharacterMovement()->MaxWalkSpeed, 420.f);
	const float SlowMovementYawRate = Player->GetCharacterMovement()->RotationRate.Yaw;
	const float SlowLocomotionRate = Player->GetLocomotionAnimationPlayRate();
	UGameplayEffect* SpeedBuff = NewObject<UGameplayEffect>();
	SpeedBuff->DurationPolicy = EGameplayEffectDurationType::Infinite;
	FGameplayModifierInfo& SpeedModifier = SpeedBuff->Modifiers.AddDefaulted_GetRef();
	SpeedModifier.Attribute = UUmbraAttributeSet::GetMoveSpeedAttribute();
	SpeedModifier.ModifierOp = EGameplayModOp::Additive;
	SpeedModifier.ModifierMagnitude = FScalableFloat(80.f);
	const FActiveGameplayEffectHandle Buff = ASC->ApplyGameplayEffectSpecToSelf(FGameplayEffectSpec(SpeedBuff, ASC->MakeEffectContext(), 1.f));
	TestEqual(TEXT("Movement receives active GE aggregate"), Player->GetCharacterMovement()->MaxWalkSpeed, 500.f);
	TestTrue(TEXT("Movement yaw rate increases with the GAS-driven target speed"),
		Player->GetCharacterMovement()->RotationRate.Yaw > SlowMovementYawRate);
	TestTrue(TEXT("Locomotion animation rate follows the GAS-driven target speed"),
		Player->GetLocomotionAnimationPlayRate() > SlowLocomotionRate);
	ASC->RemoveActiveGameplayEffect(Buff);
	TestEqual(TEXT("Movement restores after GE removal"), Player->GetCharacterMovement()->MaxWalkSpeed, 420.f);
	auto* Replacement = World->SpawnActor<AUmbraPlayerCharacter>(FVector(2000.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	ASC->InitAbilityActorInfo(PS, Replacement);
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetMoveSpeedAttribute(), 430.f);
	TestEqual(TEXT("Rebound avatar receives existing GAS speed"), Replacement->GetCharacterMovement()->MaxWalkSpeed, 430.f);
	TestEqual(TEXT("Old avatar is no longer updated"), Player->GetCharacterMovement()->MaxWalkSpeed, 420.f);
	ASC->ClearActorInfo();
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetMoveSpeedAttribute(), 440.f);
	TestEqual(TEXT("ClearActorInfo releases speed binding"), Replacement->GetCharacterMovement()->MaxWalkSpeed, 430.f);
	// The native character defaults provide a deterministic contract for AnimBP start/stop scaling.
	Replacement->GetCharacterMovement()->MaxWalkSpeed = 250.f;
	TestEqual(TEXT("Half reference speed produces 0.5x locomotion animation"),
		Replacement->GetLocomotionAnimationPlayRate(), 0.5f);
	TestEqual(TEXT("Below-reference movement speed produces proportional yaw"),
		Replacement->CalculateMovementYawRate(250.f), 300.f);
	Replacement->GetCharacterMovement()->MaxWalkSpeed = 500.f;
	TestEqual(TEXT("Reference speed produces authored 1x locomotion animation"),
		Replacement->GetLocomotionAnimationPlayRate(), 1.f);
	TestEqual(TEXT("Initial movement speed preserves 600 degrees per second yaw"),
		Replacement->CalculateMovementYawRate(500.f), 600.f);
	Replacement->GetCharacterMovement()->MaxWalkSpeed = 750.f;
	TestEqual(TEXT("One and a half reference speed produces 1.5x locomotion animation"),
		Replacement->GetLocomotionAnimationPlayRate(), 1.5f);
	TestEqual(TEXT("Higher movement speed produces proportional yaw"),
		Replacement->CalculateMovementYawRate(750.f), 900.f);
	Replacement->GetCharacterMovement()->MaxWalkSpeed = 5000.f;
	TestEqual(TEXT("Locomotion animation rate observes the configured upper bound"),
		Replacement->GetLocomotionAnimationPlayRate(), 3.f);
	TestEqual(TEXT("Movement yaw rate observes the configured upper bound"),
		Replacement->CalculateMovementYawRate(5000.f), 1800.f);
	Replacement->GetCharacterMovement()->MaxWalkSpeed = 0.f;
	TestEqual(TEXT("Locomotion animation rate observes the configured lower bound"),
		Replacement->GetLocomotionAnimationPlayRate(), 0.25f);
	ASC->InitAbilityActorInfo(PS, Player);
	TestEqual(TEXT("Rebind does not reseed from stale movement"), Player->GetCharacterMovement()->MaxWalkSpeed, 440.f);
	EnemyASC->SetNumericAttributeBase(UUmbraAttributeSet::GetMoveSpeedAttribute(), 230.f);
	TestEqual(TEXT("GAS sets actual enemy speed"), Enemy->GetCharacterMovement()->MaxWalkSpeed, 230.f);

	const FGameplayAbilitySpecHandle AttackHandle = ASC->GiveAbility(FGameplayAbilitySpec(AttackClass, 1));
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetAttackPowerAttribute(), 20.f);
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetAttackSpeedAttribute(), 1.f);
	ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetCriticalChanceAttribute(), 0.f);
	EnemyASC->SetNumericAttributeBase(UUmbraAttributeSet::GetMaxHealthAttribute(), 1000.f);
	EnemyASC->SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(), 1000.f);
	EnemyASC->AddLooseGameplayTag(UmbraGameplayTags::State_SuperArmor);
	UGameplayEffect* AttackSpeedBuff = NewObject<UGameplayEffect>();
	AttackSpeedBuff->DurationPolicy = EGameplayEffectDurationType::Infinite;
	FGameplayModifierInfo& AttackSpeedModifier = AttackSpeedBuff->Modifiers.AddDefaulted_GetRef();
	AttackSpeedModifier.Attribute = UUmbraAttributeSet::GetAttackSpeedAttribute();
	AttackSpeedModifier.ModifierOp = EGameplayModOp::Additive;
	AttackSpeedModifier.ModifierMagnitude = FScalableFloat(1.f);
	const FActiveGameplayEffectHandle AttackSpeedBuffHandle = ASC->ApplyGameplayEffectSpecToSelf(
		FGameplayEffectSpec(AttackSpeedBuff, ASC->MakeEffectContext(), 1.f));
	IConsoleVariable* MeasureSeconds = IConsoleManager::Get().FindConsoleVariable(TEXT("umbra.Attack.MeasureSeconds"));
	if (TestNotNull(TEXT("Basic attack damage measurement command"), MeasureSeconds))
	{
		MeasureSeconds->Set(1.f);
	}
	PC->BeginAttackTarget(Enemy);
	auto* Attack = Cast<UUmbraBasicAttackAbility>(ASC->FindAbilitySpecFromHandle(AttackHandle)->GetPrimaryInstance());
	if (TestNotNull(TEXT("Player attack instanced"), Attack) && TestTrue(TEXT("Basic attack activates"), Attack->IsActive()))
	{
		TestEqual(TEXT("Attack snapshots activation speed"), Attack->CapturedAttackSpeed, 2.f);
		TestTrue(TEXT("Measurement starts on the attack start"), ASC->bPrimaryAttackDamageMeasurementActive);
		TestEqual(TEXT("Measurement counts the first attack"), ASC->PrimaryAttackDamageMeasurementStarts, 1);
		TestTrue(TEXT("Logical period ignores animation length"),
			FMath::IsNearlyEqual(Attack->CurrentEffectiveAttackPeriod, Attack->ResolveBaseAttackInterval() / 2.f));
		TestTrue(TEXT("Visual strike marker exists"), Attack->ResolveVisualStrikeTime(Attack->CurrentAttackMontage) > 0.f);
		for (const float Multiplier : { 1.f, 2.99f, 3.f, 4.f, 5.f, 10.f })
		{
			const float Period = Attack->ResolveBaseAttackInterval() / Multiplier;
			TestTrue(FString::Printf(TEXT("%.2fx theoretical period"), Multiplier),
				Period > 0.f && FMath::IsNearlyEqual(Period * Multiplier, Attack->ResolveBaseAttackInterval()));
		}
		const uint32 FirstId = Attack->AttackInstanceId;
		const float Before = EnemyASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute());
		FGameplayEventData LegacyWindow;
		LegacyWindow.EventTag = UmbraGameplayTags::Event_Attack_HitWindowBegin;
		Attack->HandleAttackHitWindow(LegacyWindow);
		TestEqual(TEXT("Legacy window cannot damage"), EnemyASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()), Before);
		Attack->ResolveLogicalStrike(FirstId);
		const float After = EnemyASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute());
		TestTrue(TEXT("Server logical strike damages once"), After < Before);
		TestTrue(TEXT("Measurement uses settled Health loss"),
			FMath::IsNearlyEqual(ASC->PrimaryAttackDamageMeasurementTotal, double(Before - After), 0.001));
		TestEqual(TEXT("Measurement counts one damaging hit"), ASC->PrimaryAttackDamageMeasurementHits, 1);
		Attack->ResolveLogicalStrike(FirstId);
		TestEqual(TEXT("Repeated callback cannot double damage"), EnemyASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()), After);
		TestEqual(TEXT("Repeated callback cannot double-count measured damage"), ASC->PrimaryAttackDamageMeasurementHits, 1);
		ASC->FinishPrimaryAttackDamageMeasurement();
		TestFalse(TEXT("Measurement closes after reporting"), ASC->bPrimaryAttackDamageMeasurementActive);
		TestTrue(TEXT("Issued strike commits interval"), ASC->GetPrimaryAttackIntervalRemaining() > 0.f);
		ASC->RemoveActiveGameplayEffect(AttackSpeedBuffHandle);
		ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetAttackSpeedAttribute(), 3.f);
		TestEqual(TEXT("Current speed snapshot remains unchanged"), Attack->CapturedAttackSpeed, 2.f);
		ASC->NextPrimaryAttackAllowedTime = 0.0;
		Attack->EvaluatePendingTransition(FirstId);
		TestTrue(TEXT("Next strike enters high-speed mode"), Attack->bCurrentStepHighSpeed);
		TestEqual(TEXT("Next strike snapshots 3x"), Attack->CapturedAttackSpeed, 3.f);
		const uint32 SecondId = Attack->AttackInstanceId;
		Attack->ResolveLogicalStrike(FirstId);
		TestEqual(TEXT("Old strike cannot damage new strike"), EnemyASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()), After);
		PC->QueueDirectMoveCommand(FVector(0.f, 1.f, 0.f));
		TestFalse(TEXT("Windup movement immediately cancels"), Attack->IsActive());
		Attack->ResolveLogicalStrike(SecondId);
		TestEqual(TEXT("Cancelled windup has no delayed damage"), EnemyASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute()), After);
		Player->ConsumeMovementInputVector();
	}
	FGameplayAbilitySpec* EnemySpec = EnemyASC->FindAbilitySpecFromClass(EnemyAttackClass);
	const FGameplayAbilitySpecHandle EnemyHandle = EnemySpec ? EnemySpec->Handle : EnemyASC->GiveAbility(FGameplayAbilitySpec(EnemyAttackClass, 1));
	Enemy->SetCombatTarget(Player);
	Enemy->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	EnemyASC->TryActivateAbility(EnemyHandle);
	auto* EnemyAttack = Cast<UUmbraEnemyBasicAttackAbility>(EnemyASC->FindAbilitySpecFromHandle(EnemyHandle)->GetPrimaryInstance());
	if (TestNotNull(TEXT("Enemy attack instanced"), EnemyAttack) && TestTrue(TEXT("Actual enemy attack activates"), EnemyAttack->IsActive()))
	{
		TestTrue(TEXT("Enemy attack owns movement lock"), EnemyAttack->bMovementLocked);
		EnemyASC->CancelAllAbilities();
		TestEqual(TEXT("Cancellation restores previous movement mode"), uint8(Enemy->GetCharacterMovement()->MovementMode), uint8(MOVE_Falling));
		TestFalse(TEXT("Movement lock released"), EnemyAttack->bMovementLocked);
		Enemy->SetCombatTarget(Player);
		EnemyASC->TryActivateAbility(EnemyHandle);
		ASC->NextPrimaryAttackAllowedTime = 0.0;
		PC->BeginAttackTarget(Enemy);
		TestTrue(TEXT("Player can be attacking when the target is killed"), Attack && Attack->IsActive());
		EnemyASC->SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(), 0.f);
		TestTrue(TEXT("Enemy death still disables movement"), Enemy->IsDead() && Enemy->GetCharacterMovement()->MovementMode == MOVE_None);
		PC->UpdatePendingAttack();
		TestFalse(TEXT("Dead target immediately cancels the active player attack"), Attack && Attack->IsActive());
		TestFalse(TEXT("Dead target clears pending auto attack"), PC->PendingAttackTarget.IsValid());
	}

	// Notification routing is a snapshot: destroying the victim must not lose a lethal-hit number.
	UGameplayEffect* Effect = NewObject<UGameplayEffect>();
	FGameplayEffectSpec ResultSpec(Effect, ASC->MakeEffectContext(), 1.f);
	ResultSpec.SetSetByCallerMagnitude(UmbraGameplayTags::Damage_Type, 0.f);
	TestFalse(TEXT("Debug/unexecuted damage has no combat notification"), FUmbraDamageNotification::Capture(EnemyASC, ResultSpec, 10.f).HasRecipient());
	ResultSpec.SetSetByCallerMagnitude(UmbraGameplayTags::Damage_ResultCritical, 0.f);
	const FUmbraDamageNotification Notification = FUmbraDamageNotification::Capture(EnemyASC, ResultSpec, 1000.f);
	TestTrue(TEXT("Computed damage routed to attacker's PC"), Notification.HasRecipient());
	Enemy->Destroy();
	TestTrue(TEXT("Notification survives target destruction"), Notification.HasRecipient());
	Notification.Dispatch();

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
