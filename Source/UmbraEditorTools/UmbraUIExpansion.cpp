#include "UmbraSkillEditorLibrary.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UI/UmbraEnemyTargetPanel.h"
#include "UI/UmbraSkillBar.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Border.h"
#include "Components/ScaleBox.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "UObject/UnrealType.h"

namespace UmbraUIExpansion
{
 bool Save(UWidgetBlueprint* BP)
 {
  BP->ForEachSourceWidget([BP](UWidget* W) { if (!BP->WidgetVariableNameToGuidMap.Contains(W->GetFName())) BP->OnVariableAdded(W->GetFName()); });
  FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
  FKismetEditorUtilities::CompileBlueprint(BP);
  if (BP->Status == BS_Error) return false;
  BP->MarkPackageDirty(); FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone;
  return UPackage::SavePackage(BP->GetPackage(),BP,*FPackageName::LongPackageNameToFilename(BP->GetPackage()->GetName(),FPackageName::GetAssetPackageExtension()),Args);
 }
 UCanvasPanelSlot* Place(UCanvasPanel* C,UWidget* W,float X,float Y,float Width,float Height)
 { auto* S=C->AddChildToCanvas(W);S->SetPosition({X,Y});S->SetSize({Width,Height});return S; }
 UTextBlock* Label(UWidgetTree* T,const TCHAR* Name,int32 Size)
 { auto* W=T->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);auto F=W->GetFont();F.Size=Size;W->SetFont(F);W->SetColorAndOpacity(FSlateColor(FLinearColor(.92f,.85f,.69f)));return W; }
}

bool UUmbraSkillEditorLibrary::CreateTargetAndMenuPages()
{
 using namespace UmbraUIExpansion;
 auto* Menu=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/CharacterMenu/WBP_CharacterMenu"));
 auto* HUD=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/HUD/WBP_CombatHUD"));
 auto* Template=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/Asset/fantasy_gui_4/widgets/templates/buttons/W_ButtonBrownSquare_Icon"));
 if(!Menu||!HUD||!Template)return false;
 auto* Overlay=Cast<UOverlay>(Menu->WidgetTree->FindWidget(TEXT("Overlay_95")));
 auto* Existing=Menu->WidgetTree->FindWidget(TEXT("HorizontalBox_0"));
 auto* Background=Cast<UImage>(Menu->WidgetTree->FindWidget(TEXT("Image_162")));
 auto* Canvas=Cast<UCanvasPanel>(HUD->WidgetTree->RootWidget);
 UButton* SourceButton=nullptr;
 Template->WidgetTree->ForEachWidget([&](UWidget* W){if(auto* B=Cast<UButton>(W))SourceButton=B;});
 auto* Helmet=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Asset/fantasy_gui_4/textures/icons/items/fg4_icons_helmet"));
 auto* Skill=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Asset/fantasy_gui_4/textures/icons/items/fg4_icons_firesword"));
 if(!Overlay||!Existing||!Background||!Canvas||!SourceButton||!Helmet||!Skill)return false;
 if(!Menu->WidgetTree->FindWidget(TEXT("MainPageSwitcher")))
 {
  auto* T=Menu->WidgetTree.Get();
  auto* Switch=T->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(),TEXT("MainPageSwitcher"));
  Existing->RemoveFromParent();
  Switch->AddChild(Existing);
  auto* Empty=T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("SkillsPage"));
  Switch->AddChild(Empty);
  auto* PageSlot=Overlay->AddChildToOverlay(Switch);PageSlot->SetHorizontalAlignment(HAlign_Fill);PageSlot->SetVerticalAlignment(VAlign_Fill);PageSlot->SetPadding(FMargin(0,56,0,0));
  auto* Tabs=T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("MainPageTabs"));
  auto* TabSlot=Overlay->AddChildToOverlay(Tabs);TabSlot->SetHorizontalAlignment(HAlign_Center);TabSlot->SetVerticalAlignment(VAlign_Top);TabSlot->SetPadding(FMargin(0,4));
  for(int32 I=0;I<2;++I)
  {
   auto* Size=T->ConstructWidget<USizeBox>(USizeBox::StaticClass(),I==0?TEXT("CharacterTabSize"):TEXT("SkillsTabSize"));Size->SetWidthOverride(48);Size->SetHeightOverride(48);
   auto* B=T->ConstructWidget<UButton>(UButton::StaticClass(),I==0?TEXT("CharacterPageButton"):TEXT("SkillsPageButton"));B->SetStyle(SourceButton->GetStyle());
   B->SetToolTipText(I==0?NSLOCTEXT("Umbra","CharacterTab","角色"):NSLOCTEXT("Umbra","SkillsTab","技能"));
   auto* Icon=T->ConstructWidget<UImage>(UImage::StaticClass(),I==0?TEXT("CharacterTabIcon"):TEXT("SkillsTabIcon"));Icon->SetBrushFromTexture(I==0?Helmet:Skill);Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
   B->AddChild(Icon);Size->AddChild(B);Tabs->AddChildToHorizontalBox(Size)->SetPadding(FMargin(3,0));
  }
  if(!Save(Menu))return false;
 }
 // Keep the original content area intact and place the tabs above its frame.
 auto* Switch=Menu->WidgetTree->FindWidget(TEXT("MainPageSwitcher"));
 CastChecked<UOverlaySlot>(Switch->Slot)->SetPadding(FMargin(0));
 auto* Tabs=Menu->WidgetTree->FindWidget(TEXT("MainPageTabs"));
 Tabs->RemoveFromParent();
 auto* TabsSlot=Overlay->AddChildToOverlay(Tabs);
 TabsSlot->SetHorizontalAlignment(HAlign_Center);TabsSlot->SetVerticalAlignment(VAlign_Top);
 Tabs->SetRenderTranslation(FVector2D(0,-48));
 if(!Save(Menu))return false;
 const TCHAR* TargetPath=TEXT("/Game/UI/HUD/WBP_EnemyTargetPanel");
 auto* Target=LoadObject<UWidgetBlueprint>(nullptr,TargetPath);
 if(!Target)
 {
  auto* Package=CreatePackage(TargetPath);
  Target=Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(UUmbraEnemyTargetPanel::StaticClass(),Package,TEXT("WBP_EnemyTargetPanel"),BPTYPE_Normal,UWidgetBlueprint::StaticClass(),UWidgetBlueprintGeneratedClass::StaticClass()));
  auto* T=Target->WidgetTree.Get();auto* C=T->ConstructWidget<UCanvasPanel>();T->RootWidget=C;
  auto* Back=T->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("TargetBackground"));Back->SetBrush(Background->GetBrush());Place(C,Back,0,0,360,100);
  auto* Frame=T->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("TargetFrame"));Frame->SetBrushFromTexture(LoadObject<UTexture2D>(nullptr,TEXT("/Game/Asset/classic-rpg-ui/Assets/ClassicRPGUI/ClassicUI/GUI_elements/big_bar_frame")));Place(C,Frame,0,0,360,100);
  auto* Portrait=T->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("Portrait"));Portrait->SetBrushFromTexture(Helmet);Place(C,Portrait,12,14,68,68);
  auto* Name=Label(T,TEXT("EnemyName"),18);Name->SetText(NSLOCTEXT("Umbra","EnemyPreview","敌人"));Place(C,Name,94,12,190,27);
  auto* Level=Label(T,TEXT("EnemyLevel"),14);Level->SetText(FText::FromString(TEXT("Lv. 1")));Place(C,Level,288,16,62,23);
  auto* Health=T->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(),TEXT("HealthFill"));Health->SetFillColorAndOpacity(FLinearColor(.65f,.025f,.02f));Health->SetPercent(1);Place(C,Health,94,46,248,24);
  auto* Text=Label(T,TEXT("HealthText"),14);Text->SetText(FText::FromString(TEXT("100 / 100")));Text->SetJustification(ETextJustify::Center);Place(C,Text,94,46,248,24);
  if(!Save(Target))return false;
 }
 if(!AdoptEnemyNameplateTemplate())return false;
 if(!HUD->WidgetTree->FindWidget(TEXT("EnemyTargetPanel")))
 {
  auto* W=HUD->WidgetTree->ConstructWidget<UUserWidget>(Target->GeneratedClass.Get(),TEXT("EnemyTargetPanel"));
  Place(Canvas,W,24,24,360,100)->SetZOrder(10);
  if(!Save(HUD))return false;
 }
 return true;
}


bool UUmbraSkillEditorLibrary::AdoptEnemyNameplateTemplate()
{
 auto* Target=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/HUD/WBP_EnemyTargetPanel"));
 auto* Source=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/Asset/fantasy_gui_4/widgets/templates/nameplates/W_Nameplate_2"));
 if(!Target||!Source)return false;
 if(Target->WidgetTree->FindWidget(TEXT("HealthValueOverlay")))return true;
 // One-time copy preserves the template hierarchy and nested vendor widget instances.
 Target->WidgetTree->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
 Target->WidgetTree=DuplicateObject<UWidgetTree>(Source->WidgetTree,Target,TEXT("WidgetTree"));
 Target->WidgetVariableNameToGuidMap.Empty();
 auto* T=Target->WidgetTree.Get();
 auto* OriginalRoot=T->RootWidget.Get();
 auto* Scale=T->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(),TEXT("NameplateViewport"));
 Scale->SetStretch(EStretch::ScaleToFit);Scale->AddChild(OriginalRoot);T->RootWidget=Scale;
 auto* Chinese=LoadObject<UFont>(nullptr,TEXT("/Game/Asset/Font/SourceHanSerifSC-Regular_Font"));
 auto* Numeric=LoadObject<UFont>(nullptr,TEXT("/Game/Asset/Font/SimplifiedChinese/SourceHanSansSC-Regular_Font"));
 if(!Chinese||!Numeric)return false;
 auto* Name=CastChecked<UTextBlock>(T->FindWidget(TEXT("TextBlock_0")));
 auto* Level=CastChecked<UTextBlock>(T->FindWidget(TEXT("TextBlock_5")));
 auto NameFont=Name->GetFont();NameFont.FontObject=Chinese;NameFont.TypefaceFontName=TEXT("Default");Name->SetFont(NameFont);
 auto NumberFont=Level->GetFont();NumberFont.FontObject=Numeric;NumberFont.TypefaceFontName=TEXT("Default");Level->SetFont(NumberFont);
 auto* Bar=CastChecked<UUserWidget>(T->FindWidget(TEXT("W_ProgressBar_Parent")));
 auto* MaterialProperty=FindFProperty<FObjectPropertyBase>(Bar->GetClass(),TEXT("ProgressBarMaterial"));
 auto* Red=LoadObject<UObject>(nullptr,TEXT("/Game/Asset/fantasy_gui_4/materials/progressbar/Mat_GUI_ProgressBar_Red"));
 if(!MaterialProperty||!Red)return false;
 MaterialProperty->SetObjectPropertyValue_InContainer(Bar,Red);
 auto* BarSize=CastChecked<USizeBox>(T->FindWidget(TEXT("SizeBox_2")));
 Bar->RemoveFromParent();
 auto* Overlay=T->ConstructWidget<UOverlay>(UOverlay::StaticClass(),TEXT("HealthValueOverlay"));BarSize->AddChild(Overlay);
 auto* BarSlot=Overlay->AddChildToOverlay(Bar);BarSlot->SetHorizontalAlignment(HAlign_Fill);BarSlot->SetVerticalAlignment(VAlign_Fill);
 auto* Text=T->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("HealthText"));
 Text->SetFont(NumberFont);Text->SetText(FText::FromString(TEXT("100 / 100")));Text->SetJustification(ETextJustify::Center);Text->SetVisibility(ESlateVisibility::HitTestInvisible);
 auto* TextSlot=Overlay->AddChildToOverlay(Text);TextSlot->SetHorizontalAlignment(HAlign_Fill);TextSlot->SetVerticalAlignment(VAlign_Center);
 T->FindWidget(TEXT("Buffs"))->SetVisibility(ESlateVisibility::Collapsed);
 return UmbraUIExpansion::Save(Target);
}

bool UUmbraSkillEditorLibrary::CreateSkillTooltipPresentation()
{
 using namespace UmbraUIExpansion;
 auto* Item=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/Tooltip/WBP_ItemTooltip"));
 auto* Slot=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/Skills/WBP_SkillSlot"));
 auto* Bar=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/Skills/WBP_SkillBar"));
 auto* Group=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/HUD/WBP_PlayerHUDGroup"));
 if(!Item||!Slot||!Bar||!Group)return false;
 auto* SourceFrame=Cast<UBorder>(Item->WidgetTree->FindWidget(TEXT("Border_1")));
 auto* SourceName=Cast<UTextBlock>(Item->WidgetTree->FindWidget(TEXT("ItemName")));
 auto* SourceDescription=Cast<UTextBlock>(Item->WidgetTree->FindWidget(TEXT("FlavorTextBlock")));
 if(!SourceFrame||!SourceName||!SourceDescription)return false;
 const TCHAR* Path=TEXT("/Game/UI/Skills/WBP_SkillTooltip");
 auto* Tooltip=LoadObject<UWidgetBlueprint>(nullptr,Path);
 if(!Tooltip)
 {
  Tooltip=Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(UUmbraSkillTooltip::StaticClass(),CreatePackage(Path),TEXT("WBP_SkillTooltip"),BPTYPE_Normal,UWidgetBlueprint::StaticClass(),UWidgetBlueprintGeneratedClass::StaticClass()));
  auto* T=Tooltip->WidgetTree.Get();
  auto* Size=T->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("TooltipSize"));Size->SetWidthOverride(360);T->RootWidget=Size;
  auto* Frame=T->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("DescriptionFrame"));Frame->SetBrush(SourceFrame->Background);Frame->SetPadding(SourceFrame->GetPadding());Size->AddChild(Frame);
  auto* Texts=T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("TextContent"));Frame->AddChild(Texts);
  auto* Name=DuplicateObject<UTextBlock>(SourceName,T,TEXT("SkillName"));Name->Slot=nullptr;Name->SetVisibility(ESlateVisibility::HitTestInvisible);Name->SetText(NSLOCTEXT("Umbra","SkillTooltipPreviewName","技能名称"));
  auto Font=Name->GetFont();Font.Size=24;Name->SetFont(Font);Name->SetColorAndOpacity(FSlateColor(FLinearColor(.92f,.85f,.69f)));Name->SetAutoWrapText(true);Name->SetWrapTextAt(330);
  Texts->AddChildToVerticalBox(Name)->SetPadding(FMargin(0,0,0,8));
  auto* Description=DuplicateObject<UTextBlock>(SourceDescription,T,TEXT("DescriptionText"));Description->Slot=nullptr;Description->SetVisibility(ESlateVisibility::HitTestInvisible);
  Description->SetText(NSLOCTEXT("Umbra","SkillTooltipPreviewDescription","技能机制说明。可在技能Definition的Description中编辑内容，在此调整字体、字号、颜色和换行。"));
  auto BodyFont=Description->GetFont();BodyFont.SkewAmount=0;Description->SetFont(BodyFont);Description->SetAutoWrapText(true);Description->SetWrapTextAt(330);Texts->AddChildToVerticalBox(Description);
  if(!Save(Tooltip))return false;
 }
 auto* Defaults=Cast<UUmbraSkillSlotWidget>(Slot->GeneratedClass->GetDefaultObject());if(!Defaults)return false;
 Defaults->SkillTooltipClass=Tooltip->GeneratedClass;
 if(!Save(Slot))return false;
 // Open only the existing skill-bar branch for hit testing; retain gameplay/HUD layout.
 auto* Viewport=Bar->WidgetTree->FindWidget(TEXT("Size_TemplateViewport"));
 auto* EmbeddedBar=Group->WidgetTree->FindWidget(TEXT("WBP_SkillBar"));
 if(!Viewport||!EmbeddedBar)return false;
 Viewport->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
 EmbeddedBar->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
 return Save(Bar)&&Save(Group);
}
