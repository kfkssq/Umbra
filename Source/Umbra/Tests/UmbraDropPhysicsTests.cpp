#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Loot/UmbraWorldItem.h"
#include "Loot/UmbraWorldItemVisualComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/AudioComponent.h"
#include "Audio/UmbraAudioComponent.h"
#include "Audio/UmbraAudioLibrary.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraDropPhysicsTest, "Umbra.Loot.DropPhysics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraDropPhysicsTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	AActor* Floor = World->SpawnActor<AActor>();
	auto* FloorBox = NewObject<UBoxComponent>(Floor);
	Floor->SetRootComponent(FloorBox);
	FloorBox->SetBoxExtent(FVector(2000, 2000, 50));
	FloorBox->SetCollisionObjectType(ECC_WorldStatic);
	FloorBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	FloorBox->SetCollisionResponseToAllChannels(ECR_Block);
	FloorBox->RegisterComponent();
	Floor->SetActorLocation(FVector(0, 0, -50));
	World->BeginPlay();
	auto* Definition = NewObject<UUmbraItemDefinition>(World);
	const auto SpawnDrop = [&](UClass* Class, FVector Position)
	{
		auto* Item = World->SpawnActor<AUmbraWorldItem>(Class, Position, FRotator::ZeroRotator);
		// Deterministic trajectory for contact/settling assertions; production defaults still randomize directions.
		Item->DropPhysics.HorizontalSpeed = 0;
		Item->DropPhysics.AngularSpeedDegrees = 0;
		FUmbraItemInstance Instance;
		FUmbraItemInstance::Create(Floor, Definition, FGuid::NewGuid(), Instance);
		Item->InitializeInstance(Instance);
		// Explicit dispatch also supports automation worlds without a GameMode.
		if (!Item->HasActorBegunPlay()) Item->DispatchBeginPlay();
		return Item;
	};
	const auto Advance = [&](int32 Frames)
	{
		for (int32 Frame = 0; Frame < Frames; ++Frame)
		{
			++GFrameCounter;
			World->Tick(LEVELTICK_All, 1.f / 60.f);
		}
	};
	// A diagonal asset keeps a tight oriented box, while its visible world transform stays unchanged.
	auto* Fitted = NewObject<UUmbraItemDefinition>(World);
	Fitted->WorldDropMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	Fitted->WorldDropScale = FVector(1, 0.1, 0.05);
	Fitted->WorldDropRotation = FRotator(0, 45, 0);
	Fitted->WorldDropOffset = FVector(7, 9, 0);
	Fitted->FitWorldDropPhysics();
	TestTrue(TEXT("Fit retains thin oriented dimensions"), Fitted->WorldDropPhysicsExtent.Equals(FVector(50, 5, 2.5), 0.01));
	TestTrue(TEXT("Fit follows model direction"), Fitted->WorldDropPhysicsRotation.Equals(Fitted->WorldDropRotation));
	auto* Oriented = World->SpawnActor<AUmbraWorldItem>(FVector(0, 300, 100), FRotator::ZeroRotator);
	FUmbraItemInstance FittedInstance;
	FUmbraItemInstance::Create(Floor, Fitted, FGuid::NewGuid(), FittedInstance);
	Oriented->InitializeInstance(FittedInstance);
	if (!Oriented->HasActorBegunPlay()) Oriented->DispatchBeginPlay();
	const FTransform ExpectedVisual(Fitted->WorldDropRotation, FVector(0, 300, 100) + Fitted->WorldDropOffset, Fitted->WorldDropScale);
	TestTrue(TEXT("Proxy rotation preserves visual pose"), (Oriented->GetRestTransform() * Oriented->GetActorTransform()).Equals(ExpectedVisual, 0.01));
	Oriented->Destroy();
	auto* Drop = SpawnDrop(AUmbraWorldItem::StaticClass(), FVector(0, 0, 20));
	TestTrue(TEXT("Proxy is actor root"), Drop->GetRootComponent() == Drop->PhysicsBody);
	TestTrue(TEXT("Authority launches proxy"), Drop->PhysicsBody->IsSimulatingPhysics());
	TestTrue(TEXT("Upward launch"), Drop->PhysicsBody->GetPhysicsLinearVelocity().Z > 0);
	TestFalse(TEXT("Legacy visual tick disabled"), Drop->VisualComponent->IsComponentTickEnabled());
	TestEqual(TEXT("Ignore Pawn"), Drop->PhysicsBody->GetCollisionResponseToChannel(ECC_Pawn), ECR_Ignore);
	TestEqual(TEXT("Ignore other loot"), Drop->PhysicsBody->GetCollisionResponseToChannel(ECC_WorldDynamic), ECR_Ignore);
	const FVector Launch = Drop->GetActorLocation();
	Advance(12);
	TestTrue(TEXT("Chaos actually moves the actor upwards"), Drop->GetActorLocation().Z > Launch.Z);
	TestTrue(TEXT("Pickup volume follows motion"), Drop->Collision->GetComponentLocation().Equals(Drop->GetActorLocation(), 0.1f));
	Advance(120);
	TestTrue(TEXT("Natural support settles before timeout"), Drop->IsDropSettled());
	TestTrue(TEXT("Real Chaos floor collision emits first landing event"), Drop->bLandAudioSent);
	Advance(120);
	TestTrue(TEXT("Bounded simulation settles"), Drop->IsDropSettled());
	TestFalse(TEXT("Physics disabled permanently"), Drop->PhysicsBody->IsSimulatingPhysics());
	TestFalse(TEXT("Idle actor has no tick"), Drop->IsActorTickEnabled());
	TestFalse(TEXT("Idle timer cleared"), World->GetTimerManager().IsTimerActive(Drop->DropPhysicsTimer));
	TestTrue(TEXT("Settled above floor"), Drop->GetActorLocation().Z >= 7.f && Drop->GetActorLocation().Z < 30.f);
	const FTransform SettledPose = Drop->GetActorTransform();
	Advance(60);
	TestTrue(TEXT("Pose stays fixed"), Drop->GetActorTransform().Equals(SettledPose));
	TestTrue(TEXT("Final snapshot matches server pose"), Drop->DropMotion.Location.Equals(SettledPose.GetLocation()));

	// A motionless apex has no support and must not be accepted as settled.
	auto* Airborne = SpawnDrop(AUmbraWorldItem::StaticClass(), FVector(300, 0, 20));
	Airborne->SetActorLocation(FVector(300, 0, 500), false, nullptr, ETeleportType::TeleportPhysics);
	Airborne->PhysicsBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Airborne->PhysicsBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	Airborne->PhysicsStartTime = World->GetTimeSeconds() - 1;
	Airborne->UpdateDropPhysics();
	TestFalse(TEXT("Apex is not settled"), Airborne->IsDropSettled());
	TestFalse(TEXT("Airborne support queries never emit landing audio"), Airborne->bLandAudioSent);
	TestEqual(TEXT("Unsupported rest does not accumulate"), Airborne->StableSeconds, 0.f);
	// The old deadline must not teleport or add aerial damping, even well after three seconds.
	Airborne->SetActorLocation(FVector(5000, 0, 500), false, nullptr, ETeleportType::TeleportPhysics);
	Airborne->PhysicsStartTime = World->GetTimeSeconds() - 20;
	const FTransform AirPose = Airborne->GetActorTransform();
	Airborne->UpdateDropPhysics();
	TestFalse(TEXT("Soft deadline does not freeze falling items"), Airborne->IsDropSettled());
	TestTrue(TEXT("Soft deadline does not change airborne pose"), Airborne->GetActorTransform().Equals(AirPose));
	TestEqual(TEXT("Airborne damping remains normal"), Airborne->PhysicsBody->GetLinearDamping(), Airborne->DropPhysics.LinearDamping);
	// Supported motion receives gradual damping, with no placement or rotation rewrite.
	Airborne->SetActorTransform(Airborne->SafeDropPose, false, nullptr, ETeleportType::TeleportPhysics);
	Airborne->PhysicsBody->SetPhysicsLinearVelocity(FVector(50, 0, 0));
	Airborne->LastPhysicsSampleTime = World->GetTimeSeconds() - 0.05;
	const FTransform RollingPose = Airborne->GetActorTransform();
	Airborne->UpdateDropPhysics();
	TestFalse(TEXT("Rolling body is not frozen"), Airborne->IsDropSettled());
	TestTrue(TEXT("Ground damping starts gradually"), Airborne->PhysicsBody->GetLinearDamping() > Airborne->DropPhysics.LinearDamping
		&& Airborne->PhysicsBody->GetLinearDamping() < Airborne->DropPhysics.SettlingLinearDamping);
	TestTrue(TEXT("Damping does not rewrite pose"), Airborne->GetActorTransform().Equals(RollingPose));
	Airborne->SetActorLocation(FVector(5000, 0, 500), false, nullptr, ETeleportType::TeleportPhysics);
	Airborne->UpdateDropPhysics();
	TestEqual(TEXT("Leaving support restores damping"), Airborne->PhysicsBody->GetLinearDamping(), Airborne->DropPhysics.LinearDamping);
	// Only genuinely falling out of the playable region uses emergency recovery.
	Airborne->SetActorLocation(FVector(5000, 0, -3000), false, nullptr, ETeleportType::TeleportPhysics);
	Airborne->UpdateDropPhysics();
	TestTrue(TEXT("Void recovery settles"), Airborne->IsDropSettled());
	TestTrue(TEXT("Void recovery uses revalidated safe ground"), Airborne->GetActorLocation().Equals(Airborne->SafeDropPose.GetLocation(), 1.f));

	// Exercise the receive path without pretending this is a real network transport test.
	auto* Replica = World->SpawnActor<AUmbraWorldItem>();
	Replica->SetRole(ROLE_SimulatedProxy);
	Replica->DropMotion = Drop->DropMotion;
	Replica->OnRep_DropMotion();
	TestTrue(TEXT("Late client receives exact settled pose"), Replica->GetActorTransform().Equals(SettledPose));
	TestFalse(TEXT("Late client does not simulate"), Replica->PhysicsBody->IsSimulatingPhysics());
	TestFalse(TEXT("Late client does not tick"), Replica->IsActorTickEnabled());
	TestFalse(TEXT("Motion without item payload cannot initialize ambience"), Replica->bLootAmbienceConfigured);
	Replica->ItemInstance = Drop->ItemInstance;
	Replica->AudioEvents->LibraryOverride = NewObject<UUmbraAudioLibrary>(Replica);
	FUmbraAudioEntry SilentRarity;
	SilentRarity.Event = EUmbraAudioEvent::LootRarity;
	Replica->AudioEvents->LibraryOverride->Events.Add(SilentRarity);
	Replica->RefreshPresentation();
	TestTrue(TEXT("Late settled item initializes ambience after payload arrives"), Replica->bLootAmbienceConfigured);
	// Inject a silent component to verify lifecycle cleanup without a sound device or saved asset.
	auto* SilentLoop = NewObject<UAudioComponent>(Replica);
	SilentLoop->bAutoActivate = false;
	SilentLoop->RegisterComponent();
	Replica->LootAmbience = SilentLoop;
	World->GetTimerManager().SetTimer(Replica->LootAmbienceTimer, FTimerDelegate::CreateLambda([] {}), 1.f, true);
	Replica->bPickedUp = true;
	Replica->RefreshPresentation();
	TestNull(TEXT("Pickup clears owned ambience"), Replica->LootAmbience.Get());
	TestFalse(TEXT("Pickup clears ambience retry timer"), World->GetTimerManager().IsTimerActive(Replica->LootAmbienceTimer));
	TestFalse(TEXT("Pickup destroys the audio component"), SilentLoop->IsRegistered());
	Replica->bPickedUp = false;
	Replica->DropMotion.bSettled = false;
	Replica->DropMotion.Location += FVector(100, 0, 0);
	Replica->OnRep_DropMotion();
	Replica->Tick(0.025f);
	TestTrue(TEXT("Client interpolates moving snapshots"), Replica->GetActorLocation().X > SettledPose.GetLocation().X
		&& Replica->GetActorLocation().X < Replica->DropMotion.Location.X);
	Replica->DropMotion.bSettled = true;
	Replica->OnRep_DropMotion();
	TestTrue(TEXT("Final snapshot stops interpolation exactly"), Replica->GetActorLocation().Equals(Replica->DropMotion.Location));
	TestFalse(TEXT("Final snapshot clears client tick"), Replica->IsActorTickEnabled());

	auto* Legacy = World->SpawnActor<AUmbraWorldItem>();
	Legacy->DropPhysics.bEnabled = false;
	FUmbraItemInstance LegacyInstance;
	FUmbraItemInstance::Create(Floor, Definition, FGuid::NewGuid(), LegacyInstance);
	Legacy->InitializeInstance(LegacyInstance);
	if (!Legacy->HasActorBegunPlay()) Legacy->DispatchBeginPlay();
	TestFalse(TEXT("Disabled mode has no physical launch"), Legacy->PhysicsBody->IsSimulatingPhysics());
	TestFalse(TEXT("Disabled mode has no physics timer"), World->GetTimerManager().IsTimerActive(Legacy->DropPhysicsTimer));
	AddExpectedError(TEXT("no safe static ground within recovery range"), EAutomationExpectedErrorFlags::Contains, 1);
	auto* NoGround = SpawnDrop(AUmbraWorldItem::StaticClass(), FVector(5000, 0, 20));
	TestFalse(TEXT("No ground skips simulation"), NoGround->PhysicsBody->IsSimulatingPhysics());
	TestTrue(TEXT("No ground preserves pickup instance"), NoGround->GetItemInstance().IsValid());

	UClass* Blueprint = LoadClass<AUmbraWorldItem>(nullptr, TEXT("/Game/Items/Loot/BP_WorldItem.BP_WorldItem_C"));
	if (TestNotNull(TEXT("Existing BP loads after root change"), Blueprint))
	{
		auto* BPDrop = SpawnDrop(Blueprint, FVector(-300, 0, 20));
		TestTrue(TEXT("Existing BP uses new root"), BPDrop->GetRootComponent() == BPDrop->PhysicsBody);
		TestTrue(TEXT("Existing BP launches"), BPDrop->PhysicsBody->IsSimulatingPhysics());
		BPDrop->Destroy();
		TestFalse(TEXT("Destruction clears physics timer"), World->GetTimerManager().IsTimerActive(BPDrop->DropPhysicsTimer));
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
