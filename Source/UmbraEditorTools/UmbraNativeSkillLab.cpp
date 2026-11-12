#include "UmbraSkillEditorLibrary.h"
#include "Animation/UmbraLocomotionAnimInstance.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_BlendListByBool.h"
#include "AnimGraphNode_Slot.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphSchema.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Editor.h"
#include "Builders/CubeBuilder.h"
#include "ActorFactories/ActorFactory.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "EngineUtils.h"
#include "UmbraPlayerController.h"
#include "GameFramework/Pawn.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "UObject/UnrealType.h"

bool UUmbraSkillEditorLibrary::ConfigureLabNavigation(UObject* WorldContext)
{
    UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
    if (!World || World->WorldType != EWorldType::Editor || !World->GetPackage()->GetName().StartsWith(TEXT("/Game/SkillLab/"))) return false;
    ANavMeshBoundsVolume* Bounds = nullptr;
    for (TActorIterator<ANavMeshBoundsVolume> It(World); It; ++It) { Bounds = *It; break; }
    if (!Bounds) Bounds = World->SpawnActor<ANavMeshBoundsVolume>();
    if (Bounds->GetComponentsBoundingBox(true).GetExtent().IsNearlyZero())
    {
        auto* Builder = NewObject<UCubeBuilder>(Bounds);
        Builder->X = 3800.f; Builder->Y = 3800.f; Builder->Z = 600.f;
        UActorFactory::CreateBrushForVolumeActor(Bounds, Builder);
        Bounds->SetActorLabel(TEXT("LabNavigationBounds"));
        Bounds->PostEditChange();
    }
    if (Bounds->GetComponentsBoundingBox(true).GetExtent().GetMin() < 100.f) return false;
    auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    if (!Nav) return false;
    Nav->OnNavigationBoundsUpdated(Bounds);
    // Commandlets lock tile building; persist explicit dynamic nav data and let PIE generate tiles.
    if (!TActorIterator<ARecastNavMesh>(World)) World->SpawnActor<ARecastNavMesh>();
    // This small lab regenerates in PIE; no dependency on stale editor-baked tiles.
    bool bConfigured = false;
    for (TActorIterator<ARecastNavMesh> It(World); It; ++It)
    {
        if (!Nav->GetSupportedAgents().IsEmpty()) It->SetConfig(Nav->GetSupportedAgents()[0]);
        if (auto* Property = FindFProperty<FEnumProperty>(It->GetClass(), TEXT("RuntimeGeneration")))
        {
            It->Modify();
            Property->GetUnderlyingProperty()->SetIntPropertyValue(Property->ContainerPtrToValuePtr<void>(*It), static_cast<int64>(ERuntimeGenerationType::Dynamic));
            It->PostEditChange(); bConfigured = true;
        }
    }
    return bConfigured;
}

TArray<FVector> UUmbraSkillEditorLibrary::GetLabNavigationPath(AActor* Player, FVector Destination)
{
    if (!Player || Player->GetWorld()->WorldType != EWorldType::PIE) return {};
    auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Player->GetWorld());
    FNavLocation Start, End;
    if (!Nav || !Nav->ProjectPointToNavigation(Player->GetActorLocation(), Start)
        || !Nav->ProjectPointToNavigation(Destination, End))
    {
        UE_LOG(LogTemp, Warning, TEXT("Lab nav projection failed: nav=%s data=%s"), *GetNameSafe(Nav), Nav ? *GetNameSafe(Nav->GetDefaultNavDataInstance()) : TEXT("None"));
        return {};
    }
    auto* Path = UNavigationSystemV1::FindPathToLocationSynchronously(Player, Start.Location, End.Location, Player);
    return Path && Path->IsValid() && !Path->IsPartial() ? Path->PathPoints : TArray<FVector>();
}

bool UUmbraSkillEditorLibrary::InjectLabGroundClick(AActor* Player, FVector Destination, bool bPressed)
{
    auto* Pawn = Cast<APawn>(Player);
    auto* PC = Pawn ? Cast<AUmbraPlayerController>(Pawn->GetController()) : nullptr;
    if (!PC || PC->GetWorld()->WorldType != EWorldType::PIE || !PC->GetLocalPlayer()) return false;
    auto* Subsystem = PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
    const auto* Property = FindFProperty<FObjectPropertyBase>(PC->GetClass(), TEXT("PrimaryAction"));
    auto* Action = Property ? Cast<UInputAction>(Property->GetObjectPropertyValue_InContainer(PC)) : nullptr;
    FVector2D Screen;
    if (!Subsystem || !Action || !PC->ProjectWorldLocationToScreen(Destination, Screen)) return false;
    PC->SetMouseLocation(FMath::RoundToInt(Screen.X), FMath::RoundToInt(Screen.Y));
    Subsystem->InjectInputForAction(Action, FInputActionValue(bPressed), {}, {});
    return true;
}

bool UUmbraSkillEditorLibrary::CreateNativeLabAnimBlueprint()
{
    const FString Path = TEXT("/Game/SkillLab/Animation/ABP_MannyLab");
    if (FPackageName::DoesPackageExist(Path)) return true;
    auto* Skeleton = LoadObject<USkeleton>(nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin"));
    auto* Idle = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
    auto* Run = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd.MF_Unarmed_Jog_Fwd"));
    if (!Skeleton || !Idle || !Run) return false;
    UPackage* Package = CreatePackage(*Path);
    auto* BP = Cast<UAnimBlueprint>(FKismetEditorUtilities::CreateBlueprint(UUmbraLocomotionAnimInstance::StaticClass(), Package,
        TEXT("ABP_MannyLab"), BPTYPE_Normal, UAnimBlueprint::StaticClass(), UAnimBlueprintGeneratedClass::StaticClass()));
    if (!BP) return false;
    BP->TargetSkeleton = Skeleton;
    TArray<UEdGraph*> Graphs;
    BP->GetAllGraphs(Graphs);
    UAnimGraphNode_Root* Root = nullptr;
    for (UEdGraph* Graph : Graphs)
        for (UEdGraphNode* Node : Graph->Nodes)
            if (auto* Found = Cast<UAnimGraphNode_Root>(Node)) Root = Found;
    if (!Root) return false;
    UEdGraph* Graph = Root->GetGraph();
    auto Add = [Graph](UEdGraphNode* Node, int32 X, int32 Y)
    {
        Graph->AddNode(Node, false, false); Node->CreateNewGuid(); Node->PostPlacedNewNode();
        Node->AllocateDefaultPins(); Node->NodePosX = X; Node->NodePosY = Y;
    };
    auto* IdleNode = NewObject<UAnimGraphNode_SequencePlayer>(Graph);
    IdleNode->Node.SetSequence(Idle); IdleNode->Node.SetLoopAnimation(true); Add(IdleNode, -650, 200);
    auto* RunNode = NewObject<UAnimGraphNode_SequencePlayer>(Graph);
    RunNode->Node.SetSequence(Run); RunNode->Node.SetLoopAnimation(true); Add(RunNode, -650, 0);
    auto* Blend = NewObject<UAnimGraphNode_BlendListByBool>(Graph); Add(Blend, -350, 100);
    auto* Moving = NewObject<UK2Node_VariableGet>(Graph);
    Moving->VariableReference.SetSelfMember(TEXT("bMoving")); Add(Moving, -650, -150);
    auto* Slot = NewObject<UAnimGraphNode_Slot>(Graph);
    Slot->Node.SlotName = TEXT("UpperBody"); Add(Slot, -100, 100);
    const UEdGraphSchema* Schema = Graph->GetSchema();
    auto Link = [Schema](UEdGraphNode* A, const TCHAR* APin, UEdGraphNode* B, const TCHAR* BPin)
    {
        auto* From = A->FindPin(APin); auto* To = B->FindPin(BPin);
        return From && To && Schema->TryCreateConnection(From, To);
    };
    if (!Link(RunNode, TEXT("Pose"), Blend, TEXT("BlendPose_0"))
        || !Link(IdleNode, TEXT("Pose"), Blend, TEXT("BlendPose_1"))
        || !Link(Moving, TEXT("bMoving"), Blend, TEXT("bActiveValue"))
        || !Link(Blend, TEXT("Pose"), Slot, TEXT("Source"))
        || !Link(Slot, TEXT("Pose"), Root, TEXT("Result"))) return false;
    FKismetEditorUtilities::CompileBlueprint(BP);
    if (BP->Status == BS_Error) return false;
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    return UPackage::SavePackage(Package, BP, *FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension()), Args);
}
