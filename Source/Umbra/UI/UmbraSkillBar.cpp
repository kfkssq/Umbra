#include "UI/UmbraSkillBar.h"
#include "Player/UmbraPlayerState.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Skills/UmbraSweepDefinition.h"
#include "GameplayTags/UmbraGameplayTags.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Border.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "TimerManager.h"
#include "Blueprint/WidgetLayoutLibrary.h"

FUmbraSkillSlotView UUmbraSkillSlotWidget::ReadViewState() const
{
    FUmbraSkillSlotView View;
    const auto* PlayerState = GetOwningPlayerState<AUmbraPlayerState>();
    const auto* ASC = PlayerState ? PlayerState->GetUmbraAbilitySystemComponent() : nullptr;
    if (!ASC || !PlayerState->InitialSkillSlots.IsValidIndex(SlotIndex)) return View;
    const auto& SkillSlot = PlayerState->InitialSkillSlots[SlotIndex];
    if (!SkillSlot.Definition) return View;
    const auto& Definition = *SkillSlot.Definition;
    View.Name = Definition.DisplayName;
    View.Description = Definition.Description;
    View.KeyLabel = SkillSlot.KeyLabel;
    View.Icon = Definition.Icon;
    View.ResourceCost = Definition.ResourceCost;
    for (const auto& Spec : ASC->GetActivatableAbilities())
    {
        if (Spec.SourceObject.Get() == SkillSlot.Definition && Spec.GetDynamicSpecSourceTags().HasTagExact(SkillSlot.InputTag))
        { View.bGranted = true; View.bActive = Spec.IsActive(); break; }
    }
    View.bAffordable = ASC->GetNumericAttribute(UUmbraAttributeSet::GetResourceAttribute()) >= View.ResourceCost;
    View.bBlocked = ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Dead)
        || ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Stunned)
        || ASC->HasMatchingGameplayTag(UmbraGameplayTags::State_Attacking);
    if (!UmbraSkills::AreCooldownsEnabled()) return View;
    const FGameplayTagContainer Tags(Definition.CooldownTag);
    for (const auto& Pair : ASC->GetActiveEffectsTimeRemainingAndDuration(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(Tags)))
    {
        if (Pair.Key > View.CooldownRemaining)
        { View.CooldownRemaining = Pair.Key; View.CooldownDuration = Pair.Value; }
    }
    View.CooldownFraction = View.CooldownDuration > 0.f
        ? FMath::Clamp(View.CooldownRemaining / View.CooldownDuration, 0.f, 1.f) : 0.f;
    return View;
}

void UUmbraSkillSlotWidget::ResolvePresentationWidgets()
{
    const auto Resolve = [this](const FString& Path) -> UWidget*
    {
        TArray<FString> Parts;
        Path.ParseIntoArray(Parts, TEXT("."), true);
        UUserWidget* Owner = this;
        UWidget* Result = nullptr;
        for (int32 Index = 0; Index < Parts.Num(); ++Index)
        {
            if (!Owner) return nullptr;
            Result = Owner->GetWidgetFromName(*Parts[Index]);
            Owner = Cast<UUserWidget>(Result);
        }
        return Result;
    };
    if (!IconWidgetPath.IsEmpty()) Img_Icon = Cast<UImage>(Resolve(IconWidgetPath));
    if (!KeyWidgetPath.IsEmpty()) Txt_Key = Cast<UTextBlock>(Resolve(KeyWidgetPath));
    if (!CostWidgetPath.IsEmpty()) Txt_Cost = Cast<UTextBlock>(Resolve(CostWidgetPath));
    SkillContent = Resolve(ContentWidgetPath);
    if (auto* Frame = Cast<UBorder>(Resolve(FrameWidgetPath)))
        if (FrameTexture) Frame->SetBrushFromTexture(FrameTexture);
}

void UUmbraSkillSlotWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    ResolvePresentationWidgets();
    if (IsDesignTime())
    {
        ViewState = FUmbraSkillSlotView();
        ViewState.Icon = PreviewIcon;
        ViewState.KeyLabel = PreviewKeyLabel;
        ViewState.ResourceCost = PreviewResourceCost;
        ViewState.bGranted = PreviewIcon != nullptr;
        ViewState.bAffordable = true;
        ViewState.CooldownFraction = FMath::Clamp(PreviewCooldownFraction,0.f,1.f);
        ViewState.CooldownDuration = 1.f;
        ViewState.CooldownRemaining = ViewState.CooldownFraction;
        ApplyViewState();
    }
}

void UUmbraSkillSlotWidget::NativeConstruct()
{
    Super::NativeConstruct();
    ResolvePresentationWidgets();
    SetIsFocusable(false);
    SetVisibility(ESlateVisibility::Visible);
    RefreshSkill();
    // Samples replicated state at 20Hz; never keeps an independent countdown or a stale ASC.
    GetWorld()->GetTimerManager().SetTimer(RefreshTimer, this, &ThisClass::RefreshSkill, 0.05f, true);
}

void UUmbraSkillSlotWidget::NativeDestruct()
{
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(RefreshTimer);
    SetToolTip(nullptr);
    if (SkillTooltip) SkillTooltip->RemoveFromParent();
    SkillTooltip = nullptr;
    Super::NativeDestruct();
}

void UUmbraSkillSlotWidget::RefreshSkill()
{
    ViewState = ReadViewState();
    ApplyViewState();
    RefreshTooltip();
    OnSkillViewUpdated(ViewState);
}

void UUmbraSkillSlotWidget::ApplyViewState()
{
    const bool bCooling = ViewState.CooldownRemaining > 0.f;
    if (Img_Icon)
    {
        auto* Material = !IconTextureParameter.IsNone() ? Img_Icon->GetDynamicMaterial() : nullptr;
        if (Material) Material->SetTextureParameterValue(IconTextureParameter, ViewState.Icon);
        else Img_Icon->SetBrushFromTexture(ViewState.Icon);
        Img_Icon->SetVisibility(ViewState.Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
        Img_Icon->SetRenderOpacity(!ViewState.bGranted || !ViewState.bAffordable || ViewState.bBlocked ? 0.4f : 1.f);
    }
    if (SkillContent) SkillContent->SetVisibility(ViewState.Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    if (Txt_Name) Txt_Name->SetText(ViewState.Name);
    if (Txt_Key) Txt_Key->SetText(ViewState.KeyLabel);
    if (Txt_Cost)
    {
        Txt_Cost->SetText(FText::AsNumber(ViewState.ResourceCost));
        Txt_Cost->SetVisibility(ViewState.bGranted ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    }
    if (Txt_Cooldown)
    {
        Txt_Cooldown->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), ViewState.CooldownRemaining)));
        Txt_Cooldown->SetVisibility(bCooling ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    }
    if (Pb_Cooldown)
    {
        Pb_Cooldown->SetPercent(ViewState.CooldownFraction);
        Pb_Cooldown->SetVisibility(bCooling && !Img_Cooldown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    }
    if (Img_Cooldown)
    {
        if (auto* Material = Img_Cooldown->GetDynamicMaterial())
            Material->SetScalarParameterValue(TEXT("Remaining"),ViewState.CooldownFraction);
        Img_Cooldown->SetVisibility(bCooling ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    }
    if (Txt_Status) Txt_Status->SetText(FText::FromString(!ViewState.bGranted ? TEXT("未授予")
        : ViewState.bActive ? TEXT("施放中") : bCooling ? TEXT("冷却中")
        : !ViewState.bAffordable ? TEXT("资源不足") : ViewState.bBlocked ? TEXT("不可施放") : TEXT("就绪")));
}

void UUmbraSkillTooltip::SetSkillText(const FText& Name, const FText& Description)
{
    if (SkillName && !SkillName->GetText().EqualTo(Name)) SkillName->SetText(Name);
    if (DescriptionText && !DescriptionText->GetText().EqualTo(Description)) DescriptionText->SetText(Description);
}

void UUmbraSkillTooltip::PresentAbove(UWidget* Anchor)
{
    if (!Anchor) return;
    const FGeometry Viewport = UWidgetLayoutLibrary::GetViewportWidgetGeometry(this);
    const FVector2D ViewSize = Viewport.GetLocalSize();
    const FGeometry& AnchorGeometry = Anchor->GetCachedGeometry();
    if (ViewSize.X <= 0 || ViewSize.Y <= 0 || AnchorGeometry.GetLocalSize().IsNearlyZero()) return;
    if (!IsInViewport()) AddToViewport(100000);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    ForceLayoutPrepass();
    const FVector2D Size = GetDesiredSize();
    const FVector2D Top = Viewport.AbsoluteToLocal(AnchorGeometry.LocalToAbsolute(FVector2D(AnchorGeometry.GetLocalSize().X * .5, 0)));
    const FVector2D Bottom = Viewport.AbsoluteToLocal(AnchorGeometry.LocalToAbsolute(FVector2D(AnchorGeometry.GetLocalSize().X * .5, AnchorGeometry.GetLocalSize().Y)));
    constexpr double Gap = 8.0;
    constexpr double Margin = 8.0;
    double Y = Top.Y - Gap - Size.Y;
    if (Y < Margin) Y = Bottom.Y + Gap;
    const double X = FMath::Clamp(Top.X - Size.X * .5, Margin, FMath::Max(Margin, ViewSize.X - Size.X - Margin));
    Y = FMath::Clamp(Y, Margin, FMath::Max(Margin, ViewSize.Y - Size.Y - Margin));
    SetAlignmentInViewport(FVector2D::ZeroVector);
    SetDesiredSizeInViewport(Size);
    // Geometry is already in viewport Slate units; do not apply DPI conversion twice.
    SetPositionInViewport(FVector2D(X, Y), false);
}

void UUmbraSkillSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
    RefreshSkill();
}

void UUmbraSkillSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);
    if (SkillTooltip) SkillTooltip->RemoveFromParent();
}

void UUmbraSkillSlotWidget::RefreshTooltip()
{
    // Empty/ungranted slots never keep a previous skill's tooltip. Cooldown/blocked skills remain inspectable.
    if (!ViewState.bGranted || ViewState.Name.IsEmpty() || !SkillTooltipClass)
    {
        if (SkillTooltip) { SkillTooltip->RemoveFromParent(); SkillTooltip = nullptr; }
        return;
    }
    if (!SkillTooltip)
    {
        SkillTooltip = CreateWidget<UUmbraSkillTooltip>(GetOwningPlayer(), SkillTooltipClass);
        if (!SkillTooltip) return;
        SkillTooltip->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    SkillTooltip->SetSkillText(ViewState.Name, ViewState.Description);
    if (IsHovered()) SkillTooltip->PresentAbove(this);
    else if (SkillTooltip->IsInViewport()) SkillTooltip->RemoveFromParent();
}
