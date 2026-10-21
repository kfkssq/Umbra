#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UI/Items/UmbraWorldLabelLayout.h"
#include "Loot/UmbraWorldItem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ScopeExit.h"
#include "Tests/UmbraLootTestTypes.h"
#include "UmbraPlayerController.h"
#include "Tests/UmbraItemTooltipUITestTypes.h"
#include "Engine/LocalPlayer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraWorldItemInteractionTest, "Umbra.Loot.HoverAndLabelLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraWorldItemInteractionTest::RunTest(const FString& Parameters)
{
	// Already-loaded assets still defer streamable completion by frames in the editor.
	auto* Delay = IConsoleManager::Get().FindConsoleVariable(TEXT("s.StreamableDelegateDelayFrames"));
	const int32 PreviousDelay = Delay->GetInt();
	Delay->Set(0, ECVF_SetByCode);
	ON_SCOPE_EXIT { Delay->Set(PreviousDelay, ECVF_SetByCode); };
	TArray<FBox2D> Rects;
	{
		using namespace UmbraWorldLabelLayout;
		TArray<FState> States;
		States.SetNum(20);
		TArray<FEntry> Entries;
		for (auto& State : States) Entries.Add({FVector2D(500, 300), FVector2D(160, 24), &State, false});
		const FVector2D View(1280, 720);
		Update(Entries, View, 4, 1.f / 60, 16, 12);
		TArray<FVector2D> Initial;
		for (const auto& State : States) Initial.Add(State.RenderPosition);
		auto CheckVisible = [&](const FVector2D& Bounds)
		{
			for (int32 I = 0; I < Entries.Num(); ++I)
			{
				if (!States[I].bVisible) continue;
				const FBox2D A(States[I].RenderPosition, States[I].RenderPosition + Entries[I].Size);
				TestTrue(TEXT("Rendered rect within viewport"), A.Min.X >= 0 && A.Min.Y >= 0 && A.Max.X <= Bounds.X && A.Max.Y <= Bounds.Y);
				for (int32 J = 0; J < I; ++J)
					if (States[J].bVisible)
						TestFalse(TEXT("Every rendered frame remains disjoint"), A.Intersect(FBox2D(States[J].RenderPosition, States[J].RenderPosition + Entries[J].Size)));
			}
		};
		for (int32 Frame = 0; Frame < 300; ++Frame)
		{
			Update(Entries, View, 4, 1.f / 60, 16, 12);
			for (int32 I = 0; I < States.Num(); ++I)
			{
				TestTrue(TEXT("All 20 labels remain visible"), States[I].bVisible);
				TestTrue(TEXT("300 static frames preserve identity positions"), States[I].RenderPosition.Equals(Initial[I], 0.001));
			}
			CheckVisible(View);
		}
		for (int32 Frame = 0; Frame < 120; ++Frame)
		{
			const double Offset = Frame % 2 ? 0.2 : -0.2;
			for (auto& E : Entries) E.DesiredMin = FVector2D(500 + Offset, 300);
			Update(Entries, View, 4, 1.f / 60, 16, 12);
			for (int32 I = 0; I < States.Num(); ++I)
				TestTrue(TEXT("Small camera oscillation preserves slots"), States[I].TargetPosition.Equals(Initial[I] + FVector2D(Offset, 0), 0.001));
			CheckVisible(View);
		}
		Entries[5].bPinned = true;
		const FVector2D Pinned = States[5].RenderPosition;
		for (auto& E : Entries) E.DesiredMin += FVector2D(2, 2);
		Update(Entries, View, 4, 1.f / 60, 16, 12);
		TestTrue(TEXT("Hovered rectangle stays under cursor"), States[5].RenderPosition.Equals(Pinned, 0.001));
		CheckVisible(View);
		Entries[5].bPinned = false;
		// Physical viewport / DPI changes enter the solver as new Slate bounds and anchors.
		for (auto& E : Entries) { E.DesiredMin /= 1.5; E.Size += FVector2D(10, 2); }
		Update(Entries, View / 1.5, 4, 1.f / 60, 16, 12);
		CheckVisible(View / 1.5);
		Entries.SetNum(2);
		States[0] = FState(); States[1] = FState();
		for (int32 Frame = 0; Frame < 300; ++Frame)
		{
			Entries[0].DesiredMin = FVector2D(400 + FMath::Min(Frame, 100), 300);
			Entries[1].DesiredMin = FVector2D(700 - FMath::Min(Frame, 100), 300);
			Update(Entries, View, 4, 1.f / 60, 16, 12);
			CheckVisible(View);
			if (Frame == 100) { Initial[0] = States[0].TargetPosition; Initial[1] = States[1].TargetPosition; }
			if (Frame > 100)
				for (int32 I = 0; I < 2; ++I) TestTrue(TEXT("Converging labels settle without swapping"), States[I].TargetPosition.Equals(Initial[I], 0.001));
		}
		// Isolated exponential interpolation is independent of frame subdivision.
		FState A, B;
		A.bVisible = B.bVisible = true;
		A.Anchor = B.Anchor = FVector2D(100, 100);
		A.TargetPosition = B.TargetPosition = A.RenderPosition = B.RenderPosition = A.Anchor;
		TArray<FEntry> EA{{FVector2D(200, 100), FVector2D(100, 20), &A, false}};
		TArray<FEntry> EB{{FVector2D(200, 100), FVector2D(100, 20), &B, false}};
		Update(EA, View, 4, 1.f / 30, 16, 12);
		Update(EB, View, 4, 1.f / 60, 16, 12); Update(EB, View, 4, 1.f / 60, 16, 12);
		TestTrue(TEXT("DeltaTime independent smoothing"), A.RenderPosition.Equals(B.RenderPosition, 0.001));
		TestFalse(TEXT("Render and target remain distinct during interpolation"), A.RenderPosition.Equals(A.TargetPosition, 0.001));
	}
	for (int32 Index = 0; Index < 20; ++Index)
	{
		FBox2D Rect;
		if (!TestTrue(TEXT("Dense cluster fits"), UmbraWorldLabelLayout::Place(FVector2D(500, 300), FVector2D(160, 24), FVector2D(1280, 720), Rects, 4, Rect))) break;
		for (const auto& Previous : Rects) TestFalse(TEXT("Names do not overlap"), Rect.Intersect(Previous));
		Rects.Add(Rect);
	}
	FBox2D Edge;
	TestTrue(TEXT("Edge label clamped into viewport"), UmbraWorldLabelLayout::Place(FVector2D(-40, -20), FVector2D(160, 24), FVector2D(1280, 720), {}, 4, Edge));
	TestTrue(TEXT("Edge coordinates nonnegative"), Edge.Min.X >= 0 && Edge.Min.Y >= 0);
	TestFalse(TEXT("Impossible layout never overlaps"), UmbraWorldLabelLayout::Place(FVector2D::ZeroVector, FVector2D(160, 24), FVector2D(100, 20), {}, 4, Edge));

	auto* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	auto* Controller = World->SpawnActor<AUmbraLootTestController>();
	for (const TCHAR* Name : {TEXT("Head"), TEXT("Chest"), TEXT("Hand"), TEXT("Legs")})
	{
		const FString Path = FString::Printf(TEXT("/Game/Items/Armor/DA_Armor_1730_%s.DA_Armor_1730_%s"), Name, Name);
		auto* Definition = LoadObject<UUmbraItemDefinition>(nullptr, *Path);
		if (!TestNotNull(*Path, Definition)) continue;
		Definition->WorldDropSkeletalMesh.LoadSynchronous();
		auto* Item = World->SpawnActor<AUmbraWorldItem>();
		Item->DropPhysics.bEnabled = false;
		FUmbraItemInstance Instance;
		FUmbraItemInstance::Create(Item, Definition, FGuid::NewGuid(), Instance);
		Item->InitializeInstance(Instance);
		Item->DispatchBeginPlay();
		Item->RefreshPresentation();
		FlushAsyncLoading();
		Item->CloseDropAnimationWindow();
		// Chaos applies transformed query bodies at the physics boundary, just as it does in PIE.
		for (int32 Frame = 0; Frame < 2; ++Frame) { ++GFrameCounter; World->Tick(LEVELTICK_All, 1.f / 60.f); }
		auto* Mesh = Item->SkeletalMesh.Get();
		TestNotNull(TEXT("Armor presentation loaded"), Mesh->GetSkeletalMeshAsset());
		TestTrue(TEXT("Triangle queries enabled"), Mesh->bEnablePerPolyCollision != 0);
		TestFalse(TEXT("No armor ragdoll"), Mesh->IsSimulatingPhysics());
		Mesh->UpdateBounds();
		const FBox Bounds = Mesh->Bounds.GetBox();
		int32 SurfaceHits = 0, OutsideSphereHits = 0;
		for (int32 Axis = 0; Axis < 3; ++Axis)
			for (int32 U = 1; U < 10; ++U)
				for (int32 V = 1; V < 10; ++V)
				{
					FVector Start = Bounds.Min, End;
					Start[(Axis + 1) % 3] = FMath::Lerp(Bounds.Min[(Axis + 1) % 3], Bounds.Max[(Axis + 1) % 3], U / 10.f);
					Start[(Axis + 2) % 3] = FMath::Lerp(Bounds.Min[(Axis + 2) % 3], Bounds.Max[(Axis + 2) % 3], V / 10.f);
					End = Start;
					Start[Axis] = Bounds.Max[Axis] + 30;
					End[Axis] = Bounds.Min[Axis] - 30;
					FHitResult Hit;
					if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, FCollisionQueryParams(NAME_None, true)) && Hit.GetComponent() == Mesh)
					{
						++SurfaceHits;
						if (FVector::Dist(Hit.ImpactPoint, Item->Collision->GetComponentLocation()) > Item->Collision->GetScaledSphereRadius() + 1) ++OutsideSphereHits;
					}
				}
		TestTrue(*FString::Printf(TEXT("%s surface receives cursor rays"), Name), SurfaceHits > 0);
		TestTrue(*FString::Printf(TEXT("%s hits beyond pickup sphere"), Name), OutsideSphereHits > 0);
		Controller->WorldItemLabelRects.Reset();
		Controller->WorldItemLabelRects.Add(Item, FBox2D(FVector2D(300, 400), FVector2D(480, 428)));
		TestEqual(TEXT("Displaced label selects owning item"), Controller->FindWorldItemLabelAt(FVector2D(350, 414)), Item);
		TestNull(TEXT("Outside label is not a hit"), Controller->FindWorldItemLabelAt(FVector2D(10, 10)));
		Item->SetHighlighted(true);
		TestTrue(TEXT("Armor outline enabled"), Mesh->bRenderCustomDepth);
		Item->SetHighlighted(false);
		TestFalse(TEXT("Armor outline clears"), Mesh->bRenderCustomDepth);
		auto& LayoutState = Controller->WorldItemLabelStates.FindOrAdd(Item);
		LayoutState.bVisible = true;
		LayoutState.RenderPosition = FVector2D(300, 400);
		LayoutState.TargetPosition = FVector2D(800, 400);
		Controller->HoveredWorldItem = Item;
		Item->OnDestroyed.AddUniqueDynamic(Controller, &AUmbraPlayerController::OnWorldItemLabelDestroyed);
		TestNull(TEXT("Unreached target is not hittable"), Controller->FindWorldItemLabelAt(FVector2D(850, 414)));
		Item->Destroy();
		TestEqual(TEXT("Destroy immediately releases layout history"), Controller->WorldItemLabelStates.Num(), 0);
		TestEqual(TEXT("Destroy immediately releases hit rectangles"), Controller->WorldItemLabelRects.Num(), 0);
		TestFalse(TEXT("Destroy immediately releases hover"), Controller->HoveredWorldItem.IsValid());
		TestNull(TEXT("Destroyed label target rejected"), Controller->FindWorldItemLabelAt(FVector2D(350, 414)));
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmbraTooltipTitleColorTest, "Umbra.UI.Items.TooltipTitleColor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmbraTooltipTitleColorTest::RunTest(const FString& Parameters)
{
	auto* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	auto* PC = World->SpawnActor<APlayerController>();
	PC->SetPlayer(NewObject<ULocalPlayer>(GEngine));
	auto* Tooltip = CreateWidget<UUmbraItemTooltipTestWidget>(PC);
	Tooltip->Configure();
	const auto Slate = Tooltip->TakeWidget();
	FUmbraItemTooltipData Data;
	Data.bValid = true;
	Data.Result = EUmbraTooltipResult::Success;
	Data.DisplayName = FText::FromString(TEXT("White title"));
	Data.RarityColor = FLinearColor::Red;
	Tooltip->SetTooltipData(Data);
	const auto Text = [&](const TCHAR* Name) { return CastChecked<UTextBlock>(Tooltip->GetWidgetFromName(Name)); };
	TestEqual(TEXT("Title stays white with red rarity"), Text(TEXT("ItemName"))->GetColorAndOpacity().GetSpecifiedColor(), FLinearColor::White);
	TestEqual(TEXT("Rarity remains red"), Text(TEXT("RarityText"))->GetColorAndOpacity().GetSpecifiedColor(), FLinearColor::Red);
	Data.RarityColor = FLinearColor::Green;
	Tooltip->SetTooltipData(Data);
	TestEqual(TEXT("Reused title still white"), Text(TEXT("ItemName"))->GetColorAndOpacity().GetSpecifiedColor(), FLinearColor::White);
	TestEqual(TEXT("Rarity changes normally"), Text(TEXT("RarityText"))->GetColorAndOpacity().GetSpecifiedColor(), FLinearColor::Green);
	Tooltip->SetTooltipData(FUmbraItemTooltipData());
	TestEqual(TEXT("Cleared title remains white"), Text(TEXT("ItemName"))->GetColorAndOpacity().GetSpecifiedColor(), FLinearColor::White);
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
