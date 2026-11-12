#include "UmbraSkillEditorLibrary.h"
#include "Curves/CurveVector.h"
#include "InputKeyEventArgs.h"
#include "UmbraPlayerController.h"

bool UUmbraSkillEditorLibrary::ConfigureTargetLeapCurve(UCurveVector* Curve,float Height)
{
    if (!Curve || !FMath::IsFinite(Height) || Height<=0.f) return false;
    Curve->Modify();
    for (auto& Axis : Curve->FloatCurves) Axis.Reset();
    for (int32 I=0;I<=48;++I)
    {
        const float T=float(I)/48.f;
        const auto Handle=Curve->FloatCurves[2].AddKey(T,4.f*Height*T*(1.f-T));
        Curve->FloatCurves[2].SetKeyInterpMode(Handle,RCIM_Linear);
    }
    Curve->MarkPackageDirty(); return true;
}
bool UUmbraSkillEditorLibrary::InjectTargetPointer(AActor* Player,FKey Key,bool bPressed)
{
    auto* Pawn=Cast<APawn>(Player);
    auto* PC=Pawn ? Cast<AUmbraPlayerController>(Pawn->GetController()) : nullptr;
    if (!PC || PC->GetWorld()->WorldType!=EWorldType::PIE) return false;
    FInputKeyEventArgs Args;
    Args.Key=Key; Args.Event=bPressed ? IE_Pressed : IE_Released;
    return PC->InputKey(Args);
}
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "Engine/LocalPlayer.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "Player/UmbraPlayerState.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/UObjectHash.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimBlueprint.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_Slot.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Materials/MaterialInterface.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "UI/UmbraSkillBar.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Border.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Engine/Texture2D.h"
#include "UObject/SavePackage.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"

namespace
{
    UWidgetBlueprint* Create(const TCHAR* Path, UClass* Parent)
    {
        UPackage* Package = CreatePackage(Path);
        const FString Name = FPackageName::GetLongPackageAssetName(Path);
        return Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(Parent, Package, *Name,
            BPTYPE_Normal, UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
    }
    bool Save(UWidgetBlueprint* BP)
    {
        // Existing WBP assets require editor GUID registration for newly authored widgets.
        BP->ForEachSourceWidget([BP](UWidget* Widget)
        {
            if (!BP->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
                BP->OnVariableAdded(Widget->GetFName());
            else if (!BP->WidgetVariableNameToGuidMap[Widget->GetFName()].IsValid())
                BP->WidgetVariableNameToGuidMap[Widget->GetFName()]=FGuid::NewGuid();
        });
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
        FKismetEditorUtilities::CompileBlueprint(BP);
        if (BP->Status == BS_Error) return false;
        BP->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
        return UPackage::SavePackage(BP->GetPackage(), BP,
            *FPackageName::LongPackageNameToFilename(BP->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
    }
    void Place(UCanvasPanel* Canvas, UWidget* Child, float X, float Y, float W, float H, int32 Z=0)
    {
        auto* Position = Canvas->AddChildToCanvas(Child);
        Position->SetPosition(FVector2D(X,Y)); Position->SetSize(FVector2D(W,H)); Position->SetZOrder(Z);
    }
    UTextBlock* Text(UWidgetTree* Tree, FName Name, const TCHAR* Value, int32 Size, FLinearColor Color)
    {
        auto* Widget = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
        Widget->SetText(FText::FromString(Value));
        auto Font = Widget->GetFont(); Font.Size = Size; Widget->SetFont(Font);
        Widget->SetColorAndOpacity(FSlateColor(Color));
        Widget->SetJustification(ETextJustify::Center);
        Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
        return Widget;
    }
}

bool UUmbraSkillEditorLibrary::CreateSkillWidgets()
{
    const TCHAR* SlotPath = TEXT("/Game/UI/Skills/WBP_SkillSlot");
    auto* SlotBP = LoadObject<UWidgetBlueprint>(nullptr, SlotPath);
    if (!SlotBP)
    {
        SlotBP = Create(SlotPath, UUmbraSkillSlotWidget::StaticClass());
        auto* Tree = SlotBP->WidgetTree.Get();
        auto* Root = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_Root"));
        Root->SetWidthOverride(112); Root->SetHeightOverride(130); Tree->RootWidget = Root;
        auto* Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas_Slot"));
        Root->AddChild(Canvas);
        auto* Back = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Img_Background"));
        Back->SetBrushFromTexture(LoadObject<UTexture2D>(nullptr,TEXT("/Game/Asset/classic-rpg-ui/Assets/ClassicRPGUI/ClassicUI/SpecialFrames/Background_grey")));
        Place(Canvas,Back,20,20,72,72);
        auto* Icon = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Img_Icon"));
        Icon->SetBrushFromTexture(LoadObject<UTexture2D>(nullptr,TEXT("/Game/Asset/Icons/flat_skills_icons_no_color/warrior/warrior_12")));
        Place(Canvas,Icon,24,24,64,64,1);
        auto* Cooldown = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("Pb_Cooldown"));
        FProgressBarStyle Style;
        Style.BackgroundImage.DrawAs = ESlateBrushDrawType::NoDrawType;
        Style.FillImage.DrawAs = ESlateBrushDrawType::Image;
        Style.FillImage.TintColor = FSlateColor(FLinearColor::White);
        Cooldown->SetWidgetStyle(Style);
        Cooldown->SetBarFillStyle(EProgressBarFillStyle::Scale);
        Cooldown->SetBarFillType(EProgressBarFillType::BottomToTop);
        Cooldown->SetFillColorAndOpacity(FLinearColor(0.f,0.f,0.f,0.72f));
        Cooldown->SetBorderPadding(FVector2D::ZeroVector); Cooldown->SetPercent(0.f);
        Place(Canvas,Cooldown,24,24,64,64,2);
        auto* Frame = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Img_Frame"));
        Frame->SetBrushFromTexture(LoadObject<UTexture2D>(nullptr,TEXT("/Game/Asset/classic-rpg-ui/Assets/ClassicRPGUI/ClassicUI/SpecialFrames/Frame_grey")));
        Place(Canvas,Frame,18,18,76,76,3);
        const FLinearColor Cream(0.95f,0.86f,0.65f);
        Place(Canvas,Text(Tree,TEXT("Txt_Name"),TEXT("横扫"),12,Cream),0,0,112,20,4);
        Place(Canvas,Text(Tree,TEXT("Txt_Key"),TEXT("Q"),13,FLinearColor::White),18,76,24,20,5);
        Place(Canvas,Text(Tree,TEXT("Txt_Cost"),TEXT("15"),12,FLinearColor(0.3f,0.7f,1.f)),68,76,26,20,5);
        Place(Canvas,Text(Tree,TEXT("Txt_Cooldown"),TEXT(""),22,FLinearColor::White),24,40,64,32,6);
        Place(Canvas,Text(Tree,TEXT("Txt_Status"),TEXT("就绪"),11,Cream),0,100,112,20,4);
        if (!Save(SlotBP)) return false;
    }
    const TCHAR* BarPath = TEXT("/Game/UI/Skills/WBP_SkillBar");
    if (!LoadObject<UWidgetBlueprint>(nullptr, BarPath))
    {
        auto* BarBP = Create(BarPath, UUmbraSkillBar::StaticClass());
        auto* Tree = BarBP->WidgetTree.Get();
        auto* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("HB_Skills"));
        Tree->RootWidget = Row;
        for (int32 Index=0; Index<2; ++Index)
        {
            auto* SkillWidget = Tree->ConstructWidget<UUmbraSkillSlotWidget>(SlotBP->GeneratedClass.Get(),
                *FString::Printf(TEXT("SkillSlot_%d"),Index));
            SkillWidget->SlotIndex = Index;
            Row->AddChildToHorizontalBox(SkillWidget)->SetPadding(FMargin(4,0));
        }
        if (!Save(BarBP)) return false;
    }
    return true;
}

bool UUmbraSkillEditorLibrary::CreateExperienceBarPlaceholder()
{
    auto* Source = LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/HUD/WBP_PlayerResourceBar"));
    auto* HUD = LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/HUD/WBP_PlayerHUDGroup"));
    if (!Source || !HUD) return false;
    auto* Row = Cast<UVerticalBox>(HUD->WidgetTree->FindWidget(TEXT("VerticalBox_126")));
    auto* Vitals = Cast<USizeBox>(HUD->WidgetTree->FindWidget(TEXT("Size_ExistingVitals")));
    auto* Middle = Cast<USizeBox>(HUD->WidgetTree->FindWidget(TEXT("SizeBox_33")));
    if (!Row || !Vitals || !Middle) return false;
    if (HUD->WidgetTree->FindWidget(TEXT("WBP_PlayerExperienceBar"))) return true;
    const TCHAR* Path = TEXT("/Game/UI/HUD/WBP_PlayerExperienceBar");
    auto* BP = LoadObject<UWidgetBlueprint>(nullptr,Path);
    if (!BP)
    {
        // A plain UserWidget intentionally excludes the source's GAS parent and event graph.
        BP = Create(Path,UUserWidget::StaticClass());
        BP->WidgetTree->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors | REN_NonTransactional);
        BP->WidgetTree = DuplicateObject<UWidgetTree>(Source->WidgetTree,BP,TEXT("WidgetTree"));
        auto* Root = Cast<USizeBox>(BP->WidgetTree->RootWidget);
        if (!Root) return false;
        Root->SetWidthOverride(Middle->GetWidthOverride());
        Root->SetHeightOverride(14.f);
        if (auto* Value = BP->WidgetTree->FindWidget(TEXT("ValueText"))) Value->SetVisibility(ESlateVisibility::Collapsed);
        Root->SetVisibility(ESlateVisibility::HitTestInvisible);
        if (!Save(BP)) return false;
    }
    HUD->Modify();
    auto* Experience = HUD->WidgetTree->ConstructWidget<UUserWidget>(BP->GeneratedClass.Get(),TEXT("WBP_PlayerExperienceBar"));
    auto* Position = Row->AddChildToVerticalBox(Experience);
    Position->SetHorizontalAlignment(HAlign_Fill);
    Position->SetPadding(FMargin(0,-4,0,0));
    // Existing two rows occupy 74 units (40 + 40 - 6). The thin third row adds 10.
    Vitals->SetHeightOverride(FMath::Max(Vitals->GetHeightOverride(),84.f));
    return Save(HUD);
}

FString UUmbraSkillEditorLibrary::DescribeWidget(UObject* Asset)
{
    auto* BP = Cast<UWidgetBlueprint>(Asset);
    if (!BP || !BP->WidgetTree) return TEXT("Not a widget blueprint");
    FString Result;
    BP->WidgetTree->ForEachWidget([&](UWidget* Widget)
    {
        Result += FString::Printf(TEXT("%s : %s parent=%s\n"),*Widget->GetName(),*Widget->GetClass()->GetName(),*GetNameSafe(Widget->GetParent()));
    });
    return Result;
}

bool UUmbraSkillEditorLibrary::AdoptActionbarTemplates()
{
    auto* SlotBP = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/UI/Skills/WBP_SkillSlot"));
    auto* BarBP = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/UI/Skills/WBP_SkillBar"));
    auto* HUD = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/UI/HUD/WBP_PlayerHUDGroup"));
    auto* SourceSlot = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/Asset/fantasy_gui_4/widgets/templates/item/W_Actionbar_Item_Slot"));
    auto* SourceBar = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/Asset/fantasy_gui_4/widgets/sample_windows/W_Window_Actionbar"));
    if (!SlotBP || !BarBP || !HUD || !SourceSlot || !SourceBar) return false;
    // Never silently reapply this migration over subsequent manual Designer edits.
    if (BarBP->WidgetTree->FindWidget(TEXT("Size_TemplateViewport")))
        return Save(SlotBP) && Save(BarBP) && Save(HUD);
    auto* Middle = Cast<USizeBox>(HUD->WidgetTree->FindWidget(TEXT("SizeBox_33")));
    auto* EmbeddedBar = HUD->WidgetTree->FindWidget(TEXT("WBP_SkillBar"));
    auto* EmbeddedSlot = EmbeddedBar ? Cast<UVerticalBoxSlot>(EmbeddedBar->Slot) : nullptr;
    auto* Cooldown = Cast<UProgressBar>(SlotBP->WidgetTree->FindWidget(TEXT("Pb_Cooldown")));
    auto* Countdown = Cast<UTextBlock>(SlotBP->WidgetTree->FindWidget(TEXT("Txt_Cooldown")));
    if (!Middle || !EmbeddedSlot || !Cooldown || !Countdown
        || !SourceSlot->WidgetTree->FindWidget(TEXT("CanvasPanel_2"))
        || !SourceBar->WidgetTree->FindWidget(TEXT("HorizontalBox_0"))) return false;

    const auto CopyTree = [](UWidgetBlueprint* Destination, UWidgetBlueprint* Source)
    {
        Destination->Modify();
        auto* OldTree = Destination->WidgetTree.Get();
        OldTree->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
        Destination->WidgetTree = DuplicateObject<UWidgetTree>(Source->WidgetTree, Destination, TEXT("WidgetTree"));
        Destination->WidgetVariableNameToGuidMap.Empty();
    };
    CopyTree(SlotBP, SourceSlot);
    auto* SlotTree = SlotBP->WidgetTree.Get();
    auto* Interior = CastChecked<UCanvasPanel>(SlotTree->FindWidget(TEXT("CanvasPanel_2")));
    auto* Mask = DuplicateObject<UProgressBar>(Cooldown, SlotTree, TEXT("Pb_Cooldown"));
    auto* Number = DuplicateObject<UTextBlock>(Countdown, SlotTree, TEXT("Txt_Cooldown"));
    // The original W_Item_Slot has another 8-unit inset around its icon.
    for (UWidget* Widget : {static_cast<UWidget*>(Mask), static_cast<UWidget*>(Number)})
    {
        Widget->Slot = nullptr;
        auto* Position = Interior->AddChildToCanvas(Widget);
        Position->SetAnchors(FAnchors(0,0,1,1));
        Position->SetOffsets(FMargin(8.f));
        Position->SetZOrder(Widget == Mask ? 1 : 2);
        Widget->SetVisibility(ESlateVisibility::Hidden);
    }
    auto Font = Number->GetFont(); Font.Size = 36; Number->SetFont(Font);
    // Center the new countdown without changing any of the template's original children.
    auto* NumberSlot = CastChecked<UCanvasPanelSlot>(Number->Slot);
    NumberSlot->SetAnchors(FAnchors(.5f,.5f));
    NumberSlot->SetAlignment(FVector2D(.5f,.5f));
    NumberSlot->SetPosition(FVector2D::ZeroVector);
    NumberSlot->SetSize(FVector2D(96.f,48.f));
    auto* Defaults = CastChecked<UUmbraSkillSlotWidget>(SlotBP->GeneratedClass->GetDefaultObject());
    Defaults->IconWidgetPath = TEXT("W_Item_Slot.item_icon");
    Defaults->KeyWidgetPath = TEXT("keyboard_shortcut_label");
    Defaults->CostWidgetPath = TEXT("W_Item_Slot.item_quantity");
    Defaults->PreviewIcon = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Asset/Icons/flat_skills_icons_no_color/warrior/warrior_12"));
    Defaults->PreviewKeyLabel = FText::FromString(TEXT("Q"));
    Defaults->PreviewResourceCost = 15.f;
    if (!Save(SlotBP)) return false;

    CopyTree(BarBP, SourceBar);
    auto* Tree = BarBP->WidgetTree.Get();
    auto* TemplateRoot = CastChecked<UOverlay>(Tree->RootWidget);
    // Extract the central bar. Glass balls and demo buffs are separate sample HUD features.
    Tree->RemoveWidget(Tree->FindWidget(TEXT("HorizontalBox_52")));
    Tree->RemoveWidget(Tree->FindWidget(TEXT("Buffs")));
    auto* Border = CastChecked<UBorder>(Tree->FindWidget(TEXT("Border_1")));
    CastChecked<UOverlaySlot>(Border->Slot)->SetPadding(FMargin(0));
    auto* Row = CastChecked<UHorizontalBox>(Tree->FindWidget(TEXT("HorizontalBox_0")));
    auto* RowLayout = CastChecked<UOverlaySlot>(Row->Slot);
    FMargin RowPadding = RowLayout->GetPadding();
    RowPadding.Left = RowPadding.Right = 0.f; // Original 450-unit side reserves belong to the balls.
    RowLayout->SetPadding(RowPadding);
    const int32 Count = Row->GetChildrenCount();
    if (Count != 10) return false;
    struct FCellLayout { FMargin Padding; FSlateChildSize Size; EHorizontalAlignment H; EVerticalAlignment V; };
    TArray<FCellLayout> Layouts;
    for (UWidget* Child : Row->GetAllChildren())
    {
        auto* Cell = CastChecked<UHorizontalBoxSlot>(Child->Slot);
        Layouts.Add({Cell->GetPadding(), Cell->GetSize(), Cell->GetHorizontalAlignment(), Cell->GetVerticalAlignment()});
    }
    Row->ClearChildren();
    for (int32 Index=0; Index<Count; ++Index)
    {
        auto* Skill = Tree->ConstructWidget<UUmbraSkillSlotWidget>(SlotBP->GeneratedClass.Get(),
            *FString::Printf(TEXT("SkillSlot_%d"),Index));
        Skill->SlotIndex = Index;
        Skill->PreviewIcon = Index < 2 ? LoadObject<UTexture2D>(nullptr, Index == 0
            ? TEXT("/Game/Asset/Icons/flat_skills_icons_no_color/warrior/warrior_12")
            : TEXT("/Game/Asset/Icons/flat_skills_icons_no_color/warrior/warrior_48")) : nullptr;
        Skill->PreviewKeyLabel = FText::FromString(Index == 0 ? TEXT("Q") : Index == 1 ? TEXT("E") : TEXT(""));
        Skill->PreviewResourceCost = Index == 0 ? 15.f : Index == 1 ? 25.f : 0.f;
        auto* Cell = Row->AddChildToHorizontalBox(Skill);
        Cell->SetPadding(Layouts[Index].Padding); Cell->SetSize(Layouts[Index].Size);
        Cell->SetHorizontalAlignment(Layouts[Index].H); Cell->SetVerticalAlignment(Layouts[Index].V);
    }
    // Keep all source dimensions in source space; only the outer viewport adapts to CombatHUD.
    const float SourceWidth = 120.f * Count;
    const float SourceHeight = CastChecked<USizeBox>(Tree->FindWidget(TEXT("SizeBox_1")))->GetHeightOverride();
    const float HUDWidth = Middle->GetWidthOverride();
    if (HUDWidth <= 0.f || SourceHeight <= 0.f) return false;
    auto* SourceSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_TemplateDesign"));
    SourceSize->SetWidthOverride(SourceWidth); SourceSize->SetHeightOverride(SourceHeight);
    SourceSize->AddChild(TemplateRoot);
    auto* Scale = Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("Scale_Template"));
    Scale->SetStretch(EStretch::ScaleToFit);
    auto* ScaleSlot = CastChecked<UScaleBoxSlot>(Scale->AddChild(SourceSize));
    ScaleSlot->SetHorizontalAlignment(HAlign_Center); ScaleSlot->SetVerticalAlignment(VAlign_Center);
    auto* Viewport = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_TemplateViewport"));
    Viewport->SetWidthOverride(HUDWidth); Viewport->SetHeightOverride(HUDWidth * SourceHeight / SourceWidth);
    Viewport->AddChild(Scale); Viewport->SetVisibility(ESlateVisibility::HitTestInvisible);
    Tree->RootWidget = Viewport;
    if (!Save(BarBP)) return false;
    HUD->Modify();
    const auto* Vitals = CastChecked<USizeBox>(HUD->WidgetTree->FindWidget(TEXT("Size_ExistingVitals")));
    const FMargin Gap = EmbeddedSlot->GetPadding();
    Middle->SetHeightOverride(Viewport->GetHeightOverride() + Gap.Top + Gap.Bottom + Vitals->GetHeightOverride());
    return Save(HUD);
}

FString UUmbraSkillEditorLibrary::DescribeTemplateBehavior(UObject* Asset)
{
    auto* BP = Cast<UWidgetBlueprint>(Asset);
    if (!BP) return FString();
    FString Result;
    const auto Properties = [&Result](UObject* Object)
    {
        Result += TEXT("\nOBJECT ") + Object->GetName() + TEXT("\n");
        for (TFieldIterator<FProperty> It(Object->GetClass(),EFieldIteratorFlags::ExcludeSuper); It; ++It)
        {
            FString Value;
            It->ExportText_InContainer(0,Value,Object,Object,Object,PPF_None);
            Result += It->GetName() + TEXT(" = ") + Value + TEXT("\n");
        }
    };
    Properties(BP->GeneratedClass->GetDefaultObject());
    BP->WidgetTree->ForEachWidget([&](UWidget* Widget) { if (Cast<UUserWidget>(Widget)) Properties(Widget); });
    TArray<UEdGraph*> Graphs;
    BP->GetAllGraphs(Graphs);
    for (UEdGraph* Graph : Graphs)
    {
        Result += TEXT("\nGRAPH ") + Graph->GetName() + TEXT("\n");
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            Result += Node->GetName() + TEXT(" ") + Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString() + TEXT("\n");
            for (UEdGraphPin* Pin : Node->Pins)
            {
                Result += FString::Printf(TEXT("  %s %s %s"),*Pin->PinName.ToString(),*Pin->DefaultValue,*GetPathNameSafe(Pin->DefaultObject));
                for (UEdGraphPin* Link : Pin->LinkedTo) Result += TEXT(" -> ") + Link->GetOwningNode()->GetName() + TEXT(".") + Link->PinName.ToString();
                Result += TEXT("\n");
            }
        }
    }
    return Result;
}

bool UUmbraSkillEditorLibrary::EmbedSkillBarInHUD()
{
    auto* SlotBP=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/Skills/WBP_SkillSlot"));
    auto* BarBP=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/Skills/WBP_SkillBar"));
    auto* GroupBP=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/HUD/WBP_PlayerHUDGroup"));
    if (!SlotBP || !BarBP || !GroupBP) return false;
    auto* Middle=Cast<USizeBox>(GroupBP->WidgetTree->FindWidget(TEXT("SizeBox_33")));
    auto* Vitals=Cast<UVerticalBox>(GroupBP->WidgetTree->FindWidget(TEXT("VerticalBox_126")));
    auto* Row=Cast<UHorizontalBox>(BarBP->WidgetTree->FindWidget(TEXT("HB_Skills")));
    if (!Middle || !Vitals || !Row) return false;
    // This is a one-time authoring migration. Subsequent runs preserve manual Designer adjustments.
    if (GroupBP->WidgetTree->FindWidget(TEXT("VB_SkillsAndVitals")))
        return Save(SlotBP) && Save(BarBP) && Save(GroupBP);
    if (Middle->GetContent()!=Vitals || BarBP->WidgetTree->RootWidget!=Row) return false;

    SlotBP->Modify();
    auto* SlotTree=SlotBP->WidgetTree.Get();
    auto* SlotRoot=Cast<USizeBox>(SlotTree->RootWidget);
    if (!SlotRoot) return false;
    SlotRoot->SetWidthOverride(92.f); SlotRoot->SetHeightOverride(96.f);
    const auto Layout=[SlotTree](const TCHAR* Name,FVector2D Position,FVector2D Size)
    {
        if (auto* Widget=SlotTree->FindWidget(Name))
            if (auto* CanvasSlot=Cast<UCanvasPanelSlot>(Widget->Slot))
            { CanvasSlot->SetPosition(Position); CanvasSlot->SetSize(Size); }
    };
    Layout(TEXT("Img_Background"),{12,18},{68,68});
    Layout(TEXT("Img_Icon"),{16,22},{60,60});
    Layout(TEXT("Pb_Cooldown"),{16,22},{60,60});
    Layout(TEXT("Img_Frame"),{10,16},{72,72});
    Layout(TEXT("Txt_Name"),{0,0},{92,17});
    Layout(TEXT("Txt_Key"),{12,68},{20,18});
    Layout(TEXT("Txt_Cost"),{57,68},{24,18});
    Layout(TEXT("Txt_Cooldown"),{16,36},{60,32});
    if (auto* Status=SlotTree->FindWidget(TEXT("Txt_Status"))) Status->SetVisibility(ESlateVisibility::Collapsed);
    if (!Save(SlotBP)) return false;

    BarBP->Modify();
    auto* Tree=BarBP->WidgetTree.Get();
    auto* Root=Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("Size_BarRoot"));
    Root->SetWidthOverride(500.f); Root->SetHeightOverride(112.f);
    auto* Overlay=Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(),TEXT("Overlay_Bar"));
    Root->AddChild(Overlay);
    auto* Frame=Tree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("Img_BarFrame"));
    auto* Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Asset/classic-rpg-ui/Assets/ClassicRPGUI/ClassicUI/GUI_elements/skill_bar"));
    if (!Texture) return false;
    Frame->SetBrushFromTexture(Texture);
    auto* FrameSlot=Overlay->AddChildToOverlay(Frame);
    FrameSlot->SetHorizontalAlignment(HAlign_Fill); FrameSlot->SetVerticalAlignment(VAlign_Fill);
    auto* RowSlot=Overlay->AddChildToOverlay(Row);
    RowSlot->SetHorizontalAlignment(HAlign_Center); RowSlot->SetVerticalAlignment(VAlign_Center);
    RowSlot->SetPadding(FMargin(12,8));
    Root->SetVisibility(ESlateVisibility::HitTestInvisible);
    Tree->RootWidget=Root;
    if (!Save(BarBP)) return false;

    GroupBP->Modify();
    auto* GroupTree=GroupBP->WidgetTree.Get();
    const FMargin VitalsPadding=CastChecked<USizeBoxSlot>(Vitals->Slot)->GetPadding();
    Vitals->RemoveFromParent();
    auto* Stack=GroupTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("VB_SkillsAndVitals"));
    Middle->AddChild(Stack); Middle->SetHeightOverride(200.f);
    auto* Bar=GroupTree->ConstructWidget<UUserWidget>(BarBP->GeneratedClass.Get(),TEXT("WBP_SkillBar"));
    Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* BarSlot=Stack->AddChildToVerticalBox(Bar);
    BarSlot->SetPadding(FMargin(0,0,0,8));
    BarSlot->SetHorizontalAlignment(HAlign_Fill);
    auto* VitalsSize=GroupTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("Size_ExistingVitals"));
    VitalsSize->SetHeightOverride(80.f);
    Stack->AddChildToVerticalBox(VitalsSize)->SetHorizontalAlignment(HAlign_Fill);
    CastChecked<USizeBoxSlot>(VitalsSize->AddChild(Vitals))->SetPadding(VitalsPadding);
    return Save(GroupBP);
}

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Characters/UmbraPlayerCharacter.h"
#include "Characters/UmbraEnemyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

AActor* UUmbraSkillEditorLibrary::PrepareSkillTest(UObject* WorldContext)
{
    UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
    if (!World || World->WorldType != EWorldType::PIE) return nullptr;
    auto* Player = Cast<AUmbraPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World,0));
    if (!Player) return nullptr;
    auto* ASC = Player->GetAbilitySystemComponent();
    ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetResourceAttribute(),100.f);
    ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetResourceRegenAttribute(),0.f);
    ASC->SetNumericAttributeBase(UUmbraAttributeSet::GetCriticalChanceAttribute(),0.f);
    Player->SetActorRotation(FRotator(0,90,0));
    Player->GetCharacterMovement()->StopMovementImmediately();
    AUmbraEnemyCharacter* Target = nullptr;
    int32 Index=0;
    for (TActorIterator<AUmbraEnemyCharacter> It(World); It; ++It)
    {
        auto* Enemy = *It;
        if (auto* AI = Cast<AAIController>(Enemy->GetController()))
        {
            if (AI->GetBrainComponent()) AI->GetBrainComponent()->StopLogic(TEXT("Skill prototype fixture"));
            AI->StopMovement();
        }
        Enemy->SetActorTickEnabled(false);
        Enemy->GetCharacterMovement()->DisableMovement();
        const FVector Position = Player->GetActorLocation() + (Index==0 ? FVector(0,180,0) : FVector(5000 + Index*300,0,0));
        Enemy->SetActorLocation(Position,false,nullptr,ETeleportType::TeleportPhysics);
        if (Index++ == 0)
        {
            Target=Enemy;
            auto* TargetASC=Enemy->GetAbilitySystemComponent();
            TargetASC->SetNumericAttributeBase(UUmbraAttributeSet::GetMaxHealthAttribute(),10000.f);
            TargetASC->SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(),10000.f);
            TargetASC->SetNumericAttributeBase(UUmbraAttributeSet::GetHealthRegenAttribute(),0.f);
        }
    }
    return Target;
}

float UUmbraSkillEditorLibrary::ReadTestAttribute(AActor* Actor,FName Attribute)
{
    auto* ASC=UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
    if (!ASC) return -1.f;
    if (Attribute==TEXT("Resource")) return ASC->GetNumericAttribute(UUmbraAttributeSet::GetResourceAttribute());
    if (Attribute==TEXT("Health")) return ASC->GetNumericAttribute(UUmbraAttributeSet::GetHealthAttribute());
    if (Attribute==TEXT("Armor")) return ASC->GetNumericAttribute(UUmbraAttributeSet::GetArmorAttribute());
    if (Attribute==TEXT("CriticalChance")) return ASC->GetNumericAttribute(UUmbraAttributeSet::GetCriticalChanceAttribute());
    if (Attribute==TEXT("SlashingResistance")) return ASC->GetNumericAttribute(UUmbraAttributeSet::GetSlashingResistanceAttribute());
    if (Attribute==TEXT("MaxHealth")) return ASC->GetNumericAttribute(UUmbraAttributeSet::GetMaxHealthAttribute());
    return -1.f;
}

bool UUmbraSkillEditorLibrary::InjectSkillTestInput(AActor* Actor, int32 SlotIndex, bool bPressed)
{
    auto* Player = Cast<AUmbraPlayerCharacter>(Actor);
    if (!Player || Player->GetWorld()->WorldType != EWorldType::PIE) return false;
    auto* PS = Player->GetPlayerState<AUmbraPlayerState>();
    auto* PC = Cast<APlayerController>(Player->GetController());
    auto* Subsystem = PC && PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
    if (!PS || !Subsystem || !PS->InitialSkillSlots.IsValidIndex(SlotIndex)) return false;
    const auto* Property = FindFProperty<FArrayProperty>(Player->GetClass(), TEXT("AbilityInputActions"));
    if (!Property) return false;
    const auto& Bindings = *Property->ContainerPtrToValuePtr<TArray<FUmbraTaggedInputAction>>(Player);
    for (const auto& Binding : Bindings)
    {
        if (Binding.InputTag == PS->InitialSkillSlots[SlotIndex].InputTag && Binding.InputAction)
        {
            Subsystem->InjectInputForAction(Binding.InputAction, FInputActionValue(bPressed), {}, {});
            return true;
        }
    }
    return false;
}

bool UUmbraSkillEditorLibrary::InjectMovementTestInput(AActor* Actor, FVector2D Axis)
{
    auto* Player = Cast<AUmbraPlayerCharacter>(Actor);
    if (!Player || Player->GetWorld()->WorldType != EWorldType::PIE) return false;
    auto* PC = Cast<APlayerController>(Player->GetController());
    auto* Subsystem = PC && PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
    const auto* Property = FindFProperty<FObjectPropertyBase>(Player->GetClass(), TEXT("MoveAction"));
    auto* Action = Property ? Cast<UInputAction>(Property->GetObjectPropertyValue_InContainer(Player)) : nullptr;
    if (!Subsystem || !Action) return false;
    Subsystem->InjectInputForAction(Action, FInputActionValue(Axis), {}, {});
    return true;
}

bool UUmbraSkillEditorLibrary::SetSkillTestAttribute(AActor* Actor, FName Attribute, float Value)
{
    if (!Actor || Actor->GetWorld()->WorldType != EWorldType::PIE) return false;
    auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
    if (!ASC) return false;
    FGameplayAttribute Target;
    if (Attribute == TEXT("Health")) Target = UUmbraAttributeSet::GetHealthAttribute();
    else if (Attribute == TEXT("HealthRegen")) Target = UUmbraAttributeSet::GetHealthRegenAttribute();
    else if (Attribute == TEXT("Resource")) Target = UUmbraAttributeSet::GetResourceAttribute();
    else if (Attribute == TEXT("CriticalChance")) Target = UUmbraAttributeSet::GetCriticalChanceAttribute();
    else return false;
    ASC->SetNumericAttributeBase(Target, Value);
    return true;
}

void UUmbraSkillEditorLibrary::InterruptSkillTest(AActor* Actor, bool bMovement)
{
    if (!Actor || Actor->GetWorld()->WorldType != EWorldType::PIE) return;
    if (auto* ASC = Cast<UUmbraAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor)))
    {
        if (bMovement) ASC->RequestPrimaryAttackMovementCancellation();
        else ASC->CancelAllAbilities();
    }
}

AActor* UUmbraSkillEditorLibrary::SpawnSkillTestWall(AActor* Player)
{
    if (!Player || Player->GetWorld()->WorldType != EWorldType::PIE) return nullptr;
    auto* Wall = Player->GetWorld()->SpawnActor<AStaticMeshActor>();
    Wall->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
    Wall->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube")));
    Wall->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
    Wall->SetActorLocation(Player->GetActorLocation() + FVector(0,220,100));
    Wall->SetActorScale3D(FVector(3,.2,5));
    return Wall;
}

FString UUmbraSkillEditorLibrary::DescribeParticleSystem(UObject* Asset)
{
    FString Result;
    if (!Asset) return Result;
    ForEachObjectWithOuter(Asset, [&Result](UObject* Object)
    {
        const FString Class = Object->GetClass()->GetName();
        if (!Class.Contains(TEXT("Location")) && !Class.Contains(TEXT("TypeDataMesh")) && !Class.Contains(TEXT("Required"))) return;
        Result += Object->GetName() + TEXT(" ") + Class + TEXT("\n");
        for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
        {
            const FString Name = It->GetName();
            if (!Name.Contains(TEXT("Param")) && !Name.Contains(TEXT("Mesh")) && !Name.Contains(TEXT("Material")) && !Name.Contains(TEXT("Source"))) continue;
            FString Value; It->ExportText_InContainer(0,Value,Object,Object,Object,PPF_None);
            Result += Name + TEXT("=") + Value + TEXT("\n");
        }
    }, true);
    return Result;
}

bool UUmbraSkillEditorLibrary::ScaleSkillRootHeight(UAnimSequence* Source, UAnimSequence* Destination, float MaximumHeight)
{
    if (!Source || !Destination || Source == Destination || !FMath::IsFinite(MaximumHeight) || MaximumHeight <= 0.f) return false;
    TArray<FTransform> Poses;
    Source->GetDataModel()->GetBoneTrackTransforms(TEXT("root"), Poses);
    float Peak = 0.f;
    for (const auto& Pose : Poses) Peak = FMath::Max(Peak, float(Pose.GetTranslation().Z));
    if (Peak <= KINDA_SMALL_NUMBER) return false;
    TArray<FVector> Positions, Scales;
    TArray<FQuat> Rotations;
    for (const auto& Pose : Poses)
    {
        FVector Position = Pose.GetTranslation();
        Position.Z *= MaximumHeight / Peak;
        Positions.Add(Position); Rotations.Add(Pose.GetRotation()); Scales.Add(Pose.GetScale3D());
    }
    Destination->Modify();
    if (!Destination->GetController().SetBoneTrackKeys(TEXT("root"),Positions,Rotations,Scales)) return false;
    Destination->bEnableRootMotion = false;
    Destination->bForceRootLock = false;
    Destination->PostEditChange();
    Destination->MarkPackageDirty();
    return true;
}

bool UUmbraSkillEditorLibrary::SetSkillMontageSequence(UAnimMontage* Montage, UAnimSequence* Animation, int32 Loops)
{
    if (!Montage || !Animation || Loops < 1 || Loops > 10 || Montage->GetSkeleton() != Animation->GetSkeleton()) return false;
    Montage->Modify();
    Montage->SlotAnimTracks.SetNum(1);
    auto& Slot = Montage->SlotAnimTracks[0];
    Slot.SlotName = TEXT("UpperBody");
    FAnimSegment Segment;
    Segment.SetAnimReference(Animation,true);
    Segment.LoopingCount = Loops;
    Slot.AnimTrack.AnimSegments = {Segment};
    // Nested Python property edits do not refresh the montage's cached composite duration.
    Montage->SetCompositeLength(Montage->CalculateSequenceLength());
    Montage->PostEditChange();
    Montage->MarkPackageDirty();
    return true;
}

bool UUmbraSkillEditorLibrary::ConfigureGuardMontage(UAnimMontage* Montage, UAnimSequence* Animation, float Duration)
{
    if (!Montage || !Animation || Montage->GetSkeleton() != Animation->GetSkeleton()
        || !FMath::IsFinite(Duration) || Duration < Animation->GetPlayLength()) return false;
    Montage->Modify();
    Montage->SlotAnimTracks.SetNum(1);
    auto& Slot = Montage->SlotAnimTracks[0];
    Slot.SlotName = TEXT("UpperBody");
    Slot.AnimTrack.AnimSegments.Reset();
    float Cursor = 0.f;
    const auto Add = [&](float Start, float End, float Rate)
    {
        FAnimSegment Segment;
        Segment.SetAnimReference(Animation, true);
        Segment.AnimStartTime = Start;
        Segment.AnimEndTime = End;
        Segment.AnimPlayRate = Rate;
        Segment.StartPos = Cursor;
        Cursor += (End - Start) / Rate;
        Slot.AnimTrack.AnimSegments.Add(Segment);
    };
    Add(0.f, .35f, 1.f);
    const float HoldDuration = Duration - .35f - (Animation->GetPlayLength() - .38f);
    Add(.35f, .38f, .03f / HoldDuration);
    Add(.38f, Animation->GetPlayLength(), 1.f);
    Montage->SetCompositeLength(Montage->CalculateSequenceLength());
    Montage->PostEditChange();
    Montage->MarkPackageDirty();
    return true;
}

bool UUmbraSkillEditorLibrary::InstallRadialCooldown(UMaterialInterface* Material)
{
    auto* BP = LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/Skills/WBP_SkillSlot"));
    if (!BP || !Material) return false;
    auto* Old = BP->WidgetTree->FindWidget(TEXT("Pb_Cooldown"));
    auto* Position = Old ? Cast<UCanvasPanelSlot>(Old->Slot) : nullptr;
    auto* Parent = Old ? Cast<UCanvasPanel>(Old->GetParent()) : nullptr;
    if (!Position || !Parent) return false;
    BP->Modify();
    auto* Image = Cast<UImage>(BP->WidgetTree->FindWidget(TEXT("Img_Cooldown")));
    if (!Image)
    {
        Image = BP->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("Img_Cooldown"));
        auto* NewSlot = Parent->AddChildToCanvas(Image);
        NewSlot->SetLayout(Position->GetLayout());
        NewSlot->SetZOrder(Position->GetZOrder());
    }
    Image->SetBrushFromMaterial(Material);
    Image->SetVisibility(ESlateVisibility::Hidden);
    Old->SetVisibility(ESlateVisibility::Hidden);
    return Save(BP);
}

FString UUmbraSkillEditorLibrary::ReadCooldownText(UUserWidget* Widget)
{
    const auto* Text=Widget ? Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("Txt_Cooldown"))) : nullptr;
    return Text && Text->GetVisibility()!=ESlateVisibility::Hidden ? Text->GetText().ToString() : FString();
}

UWidget* UUmbraSkillEditorLibrary::FindPresentationWidget(UUserWidget* Widget, const FString& Path)
{
    TArray<FString> Parts;
    Path.ParseIntoArray(Parts,TEXT("."),true);
    UWidget* Result = Widget;
    for (const FString& Part : Parts)
    {
        auto* Owner = Cast<UUserWidget>(Result);
        if (!Owner) return nullptr;
        Result = Owner->GetWidgetFromName(*Part);
    }
    return Result;
}

bool UUmbraSkillEditorLibrary::ConfigureSwordStormAnimation(UAnimSequence* Upper, UAnimSequence* Legs, UAnimSequence* Destination)
{
    if (!Upper || !Legs || !Destination || Upper == Destination || Legs == Destination
        || Upper->GetSkeleton() != Legs->GetSkeleton() || Destination->GetSkeleton() != Upper->GetSkeleton()) return false;
    const auto& Ref = Upper->GetSkeleton()->GetReferenceSkeleton();
    const int32 Spine = Ref.FindBoneIndex(TEXT("spine_01"));
    if (Spine == INDEX_NONE) return false;
    const auto Sample = [](const TArray<FTransform>& Keys, float Phase, const FTransform& Fallback)
    {
        if (Keys.IsEmpty()) return Fallback;
        const float Position = FMath::Clamp(Phase, 0.f, 1.f) * (Keys.Num() - 1);
        const int32 A = FMath::FloorToInt(Position), B = FMath::Min(A + 1, Keys.Num() - 1);
        FTransform Result; Result.Blend(Keys[A], Keys[B], Position - A); return Result;
    };
    Destination->Modify();
    auto& Controller = Destination->GetController();
    Controller.OpenBracket(NSLOCTEXT("Umbra", "StormBake", "Author mobile sword storm loop"));
    Controller.RemoveAllBoneTracks();
    Controller.SetFrameRate(FFrameRate(30, 1));
    constexpr int32 Frames = 38; // Preserve Ability_E's original loop duration and current montage tuning.
    Controller.SetNumberOfFrames(FFrameNumber(Frames));
    bool bSuccess = true;
    for (int32 Bone = 0; Bone < Ref.GetRawBoneNum(); ++Bone)
    {
        const FName Name = Ref.GetBoneName(Bone);
        bool bUpper = Bone == Spine;
        for (int32 Parent = Ref.GetParentIndex(Bone); Parent != INDEX_NONE; Parent = Ref.GetParentIndex(Parent))
            bUpper |= Parent == Spine;
        // Weapon IK belongs to the held sword pose; feet and pelvis follow the stepping cycle.
        bUpper |= Name.ToString().StartsWith(TEXT("ik_hand"));
        TArray<FTransform> Keys;
        const auto* Model = (bUpper ? Upper : Legs)->GetDataModel();
        if (Model->IsValidBoneTrackName(Name)) Model->GetBoneTrackTransforms(Name, Keys);
        TArray<FVector> Positions, Scales; TArray<FQuat> Rotations;
        for (int32 Frame = 0; Frame <= Frames; ++Frame)
        {
            const float Phase = float(Frame) / Frames;
            FTransform Pose = Sample(Keys, bUpper ? .5f : (Frame == Frames ? 0.f : Phase), Ref.GetRefBonePose()[Bone]);
            if (Name == TEXT("root"))
            {
                Pose.SetTranslation(FVector::ZeroVector);
                Pose.SetRotation(FQuat(FVector::UpVector, -2.f * PI * Phase));
            }
            Positions.Add(Pose.GetTranslation()); Rotations.Add(Pose.GetRotation()); Scales.Add(Pose.GetScale3D());
        }
        bSuccess &= Controller.AddBoneCurve(Name);
        bSuccess &= Controller.SetBoneTrackKeys(Name, Positions, Rotations, Scales);
    }
    Controller.CloseBracket();
    Destination->Notifies.Reset();
    Destination->bEnableRootMotion = false;
    Destination->bForceRootLock = false;
    Destination->PostEditChange();
    Destination->MarkPackageDirty();
    return bSuccess;
}

bool UUmbraSkillEditorLibrary::ConfigureSwordStormAnimBlueprint(UAnimBlueprint* Blueprint, UAnimMontage* Montage)
{
    if (!Blueprint || !Montage || !Blueprint->TargetSkeleton || Montage->SlotAnimTracks.Num() != 1) return false;
    const FName SlotName(TEXT("DefaultSlot"));
    if (!Blueprint->TargetSkeleton->ContainsSlotName(SlotName)) return false;
    TArray<UEdGraph*> Graphs; Blueprint->GetAllGraphs(Graphs);
    UAnimGraphNode_Root* Root = nullptr;
    for (auto* Graph : Graphs)
        if (Graph->GetFName() == TEXT("AnimGraph"))
            for (UEdGraphNode* Node : Graph->Nodes)
                if (auto* Found = Cast<UAnimGraphNode_Root>(Node)) Root = Found;
    if (!Root) return false;
    UEdGraph* Graph = Root->GetGraph();
    auto* Input = Root->FindPin(TEXT("Result"));
    if (!Input || Input->LinkedTo.Num() != 1) return false;
    auto* Source = Input->LinkedTo[0];
    auto* Existing = Cast<UAnimGraphNode_Slot>(Source->GetOwningNode());
    if (!Existing || Existing->Node.SlotName != SlotName)
    {
        for (UEdGraphNode* Node : Graph->Nodes)
            if (auto* Slot = Cast<UAnimGraphNode_Slot>(Node); Slot && Slot->Node.SlotName == SlotName) return false;
        Blueprint->Modify();
        auto* Slot = NewObject<UAnimGraphNode_Slot>(Graph);
        Slot->Node.SlotName = SlotName;
        Slot->Node.bAlwaysUpdateSourcePose = true;
        Graph->AddNode(Slot, false, false); Slot->CreateNewGuid(); Slot->PostPlacedNewNode(); Slot->AllocateDefaultPins();
        Slot->NodePosX = Root->NodePosX - 240; Slot->NodePosY = Root->NodePosY;
        Input->BreakAllPinLinks();
        if (!Graph->GetSchema()->TryCreateConnection(Source, Slot->FindPin(TEXT("Source")))
            || !Graph->GetSchema()->TryCreateConnection(Slot->FindPin(TEXT("Pose")), Input)) return false;
    }
    Montage->Modify(); Montage->SlotAnimTracks[0].SlotName = SlotName; Montage->MarkPackageDirty();
    Montage->CompositeSections.Reset();
    Montage->AddAnimCompositeSection(TEXT("Loop"), 0.f);
    Montage->CompositeSections[0].NextSectionName = TEXT("Loop");
    Montage->BlendOut.SetBlendTime(.15f);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    Blueprint->MarkPackageDirty();
    return Blueprint->Status != BS_Error;
}

#include "Particles/ParticleSystem.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleModuleRequired.h"
#include "Particles/ParticleSystemComponent.h"
FString UUmbraSkillEditorLibrary::ConfigureStormParticle(UParticleSystem* System, float CycleSeconds)
{
    if (!System) return TEXT("Missing particle system");
    FString Report;
    for (UParticleEmitter* Emitter : System->Emitters)
        if (Emitter) for (UParticleLODLevel* LOD : Emitter->LODLevels)
            if (LOD && LOD->RequiredModule)
            {
                auto* R = LOD->RequiredModule.Get();
                Report += FString::Printf(TEXT("%s enabled=%d loops=%d duration=%f local=%d\n"),
                    *Emitter->EmitterName.ToString(), LOD->bEnabled, R->EmitterLoops, R->EmitterDuration, R->bUseLocalSpace);
                for (UParticleModule* Module : LOD->Modules)
                    if (Module) Report += Module->GetClass()->GetName() + TEXT("\n");
                if (CycleSeconds > 0.f)
                {
                    R->Modify(); R->EmitterLoops = 0;
                    if (Emitter->EmitterName.ToString().Contains(TEXT("Sparks"))) LOD->bEnabled = false;
                    R->EmitterDuration = R->EmitterDurationLow = CycleSeconds;
                    R->bEmitterDurationUseRange = false;
                    R->bKillOnCompleted = false;
                }
            }
    if (CycleSeconds > 0.f) { System->SetupSoloing(); System->PostEditChange(); System->MarkPackageDirty(); }
    return Report;
}
int32 UUmbraSkillEditorLibrary::CountStormParticles(AActor* Player, UParticleSystem* System)
{
    int32 Count=0;
    if (Player) { TArray<UParticleSystemComponent*> Components; Player->GetComponents(Components);
        for (auto* C : Components) if (C && C->Template==System) Count+=C->GetNumActiveParticles(); }
    return Count;
}



bool UUmbraSkillEditorLibrary::ConfigureStormHitParticle(UParticleSystem* System)
{
    if (!System) return false;
    bool Found=false;
    for (UParticleEmitter* Emitter : System->Emitters)
        if (Emitter) for (UParticleLODLevel* LOD : Emitter->LODLevels)
            if (LOD && LOD->RequiredModule)
            {
                const bool Spark=Emitter->EmitterName.ToString().Contains(TEXT("Sparks"));
                LOD->bEnabled = LOD->bEnabled && Spark;
                if (!Spark) continue;
                Found=true;
                UParticleModuleRequired* R=LOD->RequiredModule;
                R->EmitterLoops=1; R->EmitterDuration=R->EmitterDurationLow=.08f;
                R->EmitterDelay=R->EmitterDelayLow=0.f;
                R->bEmitterDurationUseRange=false; R->bEmitterDelayUseRange=false;
            }
    System->SetupSoloing(); System->PostEditChange(); System->MarkPackageDirty(); return Found;
}

int32 UUmbraSkillEditorLibrary::CountStormHitParticlesNear(AActor* Actor, UParticleSystem* System, float Radius)
{
    if (!Actor || !System) return 0;
    int32 Count=0;
    for (TObjectIterator<UParticleSystemComponent> It; It; ++It)
        if (It->GetWorld()==Actor->GetWorld() && It->Template==System
            && FVector::DistSquared(It->GetComponentLocation(),Actor->GetActorLocation()) <= FMath::Square(Radius))
            Count+=It->GetNumActiveParticles();
    return Count;
}
