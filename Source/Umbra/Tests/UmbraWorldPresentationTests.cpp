#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SizeBox.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "UI/UmbraDamageNumber.h"
#include "UI/UmbraEnemyHealthBarComponent.h"
#include "UmbraPlayerController.h"
#include "Widgets/SWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraWorldPresentationTest, "Umbra.UI.WorldPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FUmbraWorldPresentationTest::RunTest(const FString& Parameters)
{
	// Run in PIE or rendered -game: NullRHI does not supply a usable projection viewport.
	UWorld* World = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.World() && Context.World()->IsGameWorld() && Context.World()->GetGameViewport())
		{
			World = Context.World();
			break;
		}
	}
	if (!TestNotNull(TEXT("Run in PIE or standalone game with a viewport"), World)) return false;
	auto* PC = Cast<AUmbraPlayerController>(World->GetFirstPlayerController());
	if (!TestNotNull(TEXT("Local Umbra controller"), PC) || !TestNotNull(TEXT("Local player"), PC->GetLocalPlayer())) return false;
	int32 ViewWidth = 0, ViewHeight = 0;
	PC->GetViewportSize(ViewWidth, ViewHeight);
	if (!TestTrue(TEXT("Projection test requires nonzero rendered viewport"), ViewWidth > 0 && ViewHeight > 0)) return false;
	AddInfo(FString::Printf(TEXT("Viewport %dx%d, DPI %.3f"), ViewWidth, ViewHeight, UWidgetLayoutLibrary::GetViewportScale(PC)));
	UClass* NumberClass = LoadClass<UUmbraDamageNumber>(nullptr, TEXT("/Game/UI/Combat/WBP_DamageNumber.WBP_DamageNumber_C"));
	UClass* BarClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/Enemy/WBP_EnemyHealthBar_01.WBP_EnemyHealthBar_01_C"));
	if (!TestNotNull(TEXT("Actual damage WBP"), NumberClass) || !TestNotNull(TEXT("Actual health WBP"), BarClass)) return false;

	AActor* OriginalViewTarget = PC->GetViewTarget();
	auto* Camera = World->SpawnActor<ACameraActor>(FVector(-1000, 0, 500), FRotator::ZeroRotator);
	Camera->GetCameraComponent()->bConstrainAspectRatio = false;
	PC->SetViewTarget(Camera);
	PC->PlayerCameraManager->UpdateCamera(0.f);
	const FVector Anchor(0, 0, 500);
	FVector2D AnchorPixels;
	if (!TestTrue(TEXT("Test camera can project the world anchor"), PC->ProjectWorldLocationToScreen(Anchor, AnchorPixels, true)))
	{
		PC->SetViewTarget(OriginalViewTarget);
		PC->PlayerCameraManager->UpdateCamera(0.f);
		Camera->Destroy();
		return false;
	}
	auto* Number = CreateWidget<UUmbraDamageNumber>(PC, NumberClass);
	TSharedPtr<SWidget> NumberSlate = Number->TakeWidget();
	Number->SpreadAngleDegrees = 0.f;
	Number->MinDistance = Number->MaxDistance = 60.f;
	Number->AppearDuration = 0.25f;
	Number->HoldDuration = 0.3f;
	Number->FadeDuration = 0.3f;
	Number->Start(30.f, false, false, Anchor);

	auto Position = [Number]()
	{
		const FMargin Offset = UGameViewportSubsystem::Get()->GetWidgetSlot(Number).Offsets;
		return FVector2D(Offset.Left, Offset.Top);
	};
	auto Project = [PC, Number](FVector Point)
	{
		FVector2D Pixels;
		PC->ProjectWorldLocationToScreen(Point, Pixels, true);
		return Pixels / UWidgetLayoutLibrary::GetViewportScale(Number);
	};
	const FVector2D SpawnScreen = Position();
	// UE deprojection truncates screen coordinates to whole pixels before forming the ray.
	const double DeprojectionTolerance = 1.0 / UWidgetLayoutLibrary::GetViewportScale(Number) + 0.01;
	TestTrue(TEXT("Spawn starts at projected hit, DPI applied once"), SpawnScreen.Equals(Project(Anchor), 0.1));
	TestTrue(TEXT("Screen-up distance converts to world travel"), Number->WorldTravel.Size() > 1.0);
	Number->NativeTick(FGeometry(), 0.25f);
	const FVector Endpoint = Number->WorldOrigin + Number->WorldTravel;
	TestTrue(TEXT("Original spread retains 60 UMG units at spawn camera"), Position().Equals(SpawnScreen + FVector2D(0, -60), DeprojectionTolerance));
	const FVector2D BeforeMove = Position();
	Camera->SetActorLocation(FVector(-1000, 200, 500));
	PC->PlayerCameraManager->UpdateCamera(0.f);
	Number->UpdateAnimation();
	TestFalse(TEXT("Camera movement changes screen position (regression: screen-fixed origin)"), Position().Equals(BeforeMove, 0.1));
	TestTrue(TEXT("Camera movement keeps endpoint fixed in world"), Position().Equals(Project(Endpoint), 0.1));

	Camera->SetActorLocation(FVector(-1500, 100, 500));
	Camera->SetActorRotation(FRotator(0, -5, 0));
	PC->PlayerCameraManager->UpdateCamera(0.f);
	Number->UpdateAnimation();
	TestTrue(TEXT("Camera rotation/distance still project the same world endpoint"), Position().Equals(Project(Endpoint), 0.1));

	ULocalPlayer* LP = PC->GetLocalPlayer();
	const FVector2D OldOrigin = LP->Origin, OldSize = LP->Size;
	LP->Origin = FVector2D(0.5, 0);
	LP->Size = FVector2D(0.5, 1);
	PC->PlayerCameraManager->UpdateCamera(0.f);
	Number->UpdateAnimation();
	TestTrue(TEXT("Projection uses local player subviewport"), Position().Equals(Project(Endpoint), 0.1));
	Number->Start(30.f, false, false, Anchor);
	const FVector2D SplitStart = Position();
	Number->NativeTick(FGeometry(), 0.25f);
	AddInfo(FString::Printf(TEXT("Split spawn %s, end %s, world travel %s"), *SplitStart.ToString(), *Position().ToString(), *Number->WorldTravel.ToString()));
	TestTrue(TEXT("World endpoint conversion also works with offset subviewport"), Position().Equals(SplitStart + FVector2D(0, -60), DeprojectionTolerance));
	LP->Origin = OldOrigin;
	LP->Size = OldSize;

	Camera->SetActorRotation(FRotator(0, 180, 0));
	PC->PlayerCameraManager->UpdateCamera(0.f);
	const FVector2D LastVisible = Position();
	Number->NativeTick(FGeometry(), 0.05f);
	TestEqual(TEXT("Behind-camera number becomes transparent"), Number->GetRenderOpacity(), 0.f);
	TestTrue(TEXT("Behind-camera number retains in-viewport slot for ticking"), Position().Equals(LastVisible));
	TestTrue(TEXT("Behind-camera lifetime still advances"), FMath::IsNearlyEqual(Number->Elapsed, 0.3f));
	Camera->SetActorRotation(FRotator::ZeroRotator);
	PC->PlayerCameraManager->UpdateCamera(0.f);
	Number->UpdateAnimation();
	TestTrue(TEXT("Returning to view restores opacity"), Number->GetRenderOpacity() > 0.f);
	Camera->SetActorRotation(FRotator(0, 180, 0));
	PC->PlayerCameraManager->UpdateCamera(0.f);
	Number->NativeTick(FGeometry(), 1.f);
	TestFalse(TEXT("Offscreen number finishes despite large DeltaTime"), Number->bStarted);
	NumberSlate.Reset();
	PC->SetViewTarget(OriginalViewTarget);
	PC->PlayerCameraManager->UpdateCamera(0.f);
	Camera->Destroy();

	// Reproduce a serialized pre-fix component override, using the real WBP on a transient actor.
	AActor* Host = World->SpawnActor<AActor>();
	auto* Component = NewObject<UUmbraEnemyHealthBarComponent>(Host);
	Component->SetDrawSize(FVector2D(120, 12));
	Component->SetDrawAtDesiredSize(false);
	Component->SetWidgetClass(BarClass);
	Component->RegisterComponent();
	Component->InitWidget();
	TestTrue(TEXT("Registration repairs old fixed-size mode"), Component->GetDrawAtDesiredSize());
	UUserWidget* Bar = Component->GetUserWidgetObject();
	if (TestNotNull(TEXT("Component creates actual health WBP"), Bar))
	{
		TSharedPtr<SWidget> BarSlate = Bar->TakeWidget();
		Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
		auto* Root = Cast<USizeBox>(Bar->GetRootWidget());
		if (TestNotNull(TEXT("Actual health WBP root is SizeBox"), Root))
		{
			for (FVector2D Size : {FVector2D(220, 32), FVector2D(300, 48)})
			{
				Root->SetWidthOverride(Size.X);
				Root->SetHeightOverride(Size.Y);
				BarSlate->SlatePrepass();
				TestTrue(TEXT("Blueprint root determines desired dimensions, not old 120x12"), FVector2D(BarSlate->GetDesiredSize()).Equals(Size, 0.1));
			}
		}
	}
	Component->DestroyComponent();
	Host->Destroy();
	return true;
}
#endif
