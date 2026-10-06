#include "UI/Items/UmbraItemTooltip.h"
#include "UI/Items/UmbraTooltipStatEntry.h"
#include "UI/Items/UmbraTooltipKeyValueEntry.h"
#include "UI/Items/UmbraTooltipScalingEntry.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Engine/Texture2D.h"
#include "Widgets/SWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Umbra.h"

#define LOCTEXT_NAMESPACE "UmbraItemTooltip"

namespace
{
	void Show(UWidget* Widget, bool bShow)
	{
		if (Widget) Widget->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	void Text(UTextBlock* Block, const FText& Value, bool bShow)
	{
		if (Block) { Block->SetText(Value); Show(Block, bShow); }
	}
}

void UUmbraItemTooltip::SetTooltipData(const FUmbraItemTooltipData& Data)
{
	TooltipData = Data.bValid && Data.Result == EUmbraTooltipResult::Success ? Data : FUmbraItemTooltipData();
	RefreshDisplay(true);
}

void UUmbraItemTooltip::ClearTooltipData()
{
	TooltipData = FUmbraItemTooltipData();
	RefreshDisplay(true);
}

void UUmbraItemTooltip::SetVisibility(ESlateVisibility InVisibility)
{
	Super::SetVisibility(InVisibility == ESlateVisibility::Hidden || InVisibility == ESlateVisibility::Collapsed
		? InVisibility : ESlateVisibility::HitTestInvisible);
}

TSharedRef<SWidget> UUmbraItemTooltip::RebuildWidget()
{
	TooltipContent = Super::RebuildWidget();
	if (IsDesignTime()) return TooltipContent.ToSharedRef();
	// Scale the complete authored layout, including its frame, without discarding any rows.
	return SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
		.HAlign(HAlign_Left).VAlign(VAlign_Top)[TooltipContent.ToSharedRef()];
}

void UUmbraItemTooltip::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	TooltipContent.Reset();
}

void UUmbraItemTooltip::PresentBeside(UWidget* AnchorWidget)
{
	if (!AnchorWidget) { HideTooltip(); return; }
	const FGeometry Viewport = UWidgetLayoutLibrary::GetViewportWidgetGeometry(this);
	PresentAt(Viewport.AbsoluteToLocal(AnchorWidget->GetCachedGeometry().GetAbsolutePosition()), Viewport.GetLocalSize());
	TrackedAnchor = AnchorWidget;
}

void UUmbraItemTooltip::PresentAt(const FVector2D& Anchor, const FVector2D& ViewportSize)
{
	TrackedAnchor.Reset();
	LastAnchor = Anchor;
	bPresented = true;
	if (!IsInViewport()) AddToViewport(100000);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetAlignmentInViewport(FVector2D::ZeroVector);
	ApplyViewportPosition(Anchor, ViewportSize);
}

void UUmbraItemTooltip::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bPresented || IsDesignTime()) return;
	const FGeometry Viewport = UWidgetLayoutLibrary::GetViewportWidgetGeometry(this);
	if (TrackedAnchor.IsValid())
		LastAnchor = Viewport.AbsoluteToLocal(TrackedAnchor->GetCachedGeometry().GetAbsolutePosition());
	// Wrapped text settles after layout; keep the fit current when content, DPI or viewport size changes.
	ApplyViewportPosition(LastAnchor, Viewport.GetLocalSize());
}

void UUmbraItemTooltip::ApplyViewportPosition(const FVector2D& Anchor, const FVector2D& ViewportSize)
{
	if (!TooltipContent.IsValid() || ViewportSize.X <= 0.0 || ViewportSize.Y <= 0.0) return;
	ForceLayoutPrepass();
	const FVector2D NaturalSize = FVector2D(TooltipContent->GetDesiredSize()).ComponentMax(FVector2D(1.0, 1.0));
	// Distances are viewport Slate units; SetPositionInViewport must not apply DPI conversion again.
	const double Margin = FMath::Min(12.0, FMath::Min(ViewportSize.X, ViewportSize.Y) * 0.25);
	const FVector2D Available = ViewportSize - FVector2D(2.0 * Margin);
	const double Scale = FMath::Min(1.0, FMath::Min(Available.X / NaturalSize.X, Available.Y / NaturalSize.Y));
	const FVector2D Size = NaturalSize * Scale;
	const double Left = FMath::Clamp(Anchor.X - HorizontalOffsetFromSlot - Size.X, Margin, FMath::Max(Margin, ViewportSize.X - Margin - Size.X));
	const double Top = FMath::Clamp(Anchor.Y, Margin, FMath::Max(Margin, ViewportSize.Y - Margin - Size.Y));
	SetDesiredSizeInViewport(Size);
	SetPositionInViewport(FVector2D(Left, Top), false);
}

void UUmbraItemTooltip::HideTooltip()
{
	bPresented = false;
	TrackedAnchor.Reset();
	SetVisibility(ESlateVisibility::Collapsed);
}
void UUmbraItemTooltip::CaptureBackgrounds()
{
	if (quality_bg && !bQualityBackgroundCaptured) { QualityBackgroundBrush = quality_bg->GetBrush(); bQualityBackgroundCaptured = true; }
	if (SlotBackground && !bSlotBackgroundCaptured) { SlotBackgroundBrush = SlotBackground->GetBrush(); bSlotBackgroundCaptured = true; }
}

void UUmbraItemTooltip::ApplyBackground(UImage* Image, UTexture2D* Texture, const FSlateBrush& Default, bool bCaptured)
{
	if (!Image || !bCaptured) return;
	if (Texture)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Image->SetBrush(Brush);
	}
	else
	{
		Image->SetBrush(Default);
	}
	Image->SetColorAndOpacity(FLinearColor::White);
}

EUmbraTooltipRowStyle UUmbraItemTooltip::ToRowStyle(EUmbraTooltipEntryStyle Style) const
{
	switch (Style)
	{
	case EUmbraTooltipEntryStyle::Met: return EUmbraTooltipRowStyle::Met;
	case EUmbraTooltipEntryStyle::Unmet: return EUmbraTooltipRowStyle::Unmet;
	case EUmbraTooltipEntryStyle::Warning: return EUmbraTooltipRowStyle::Unknown;
	default: return EUmbraTooltipRowStyle::Neutral;
	}
}

void UUmbraItemTooltip::ClearEntryWidget(UWidget* Child)
{
	if (auto* Entry = Cast<UUmbraTooltipStatEntry>(Child)) Entry->SetEntryText(FText::GetEmpty());
	else if (auto* Key = Cast<UUmbraTooltipKeyValueEntry>(Child)) Key->SetEntry(FText::GetEmpty(), FText::GetEmpty());
	else if (auto* Scaling = Cast<UUmbraTooltipScalingEntry>(Child)) Scaling->SetEntry(FText::GetEmpty(), FText::GetEmpty());
	else if (auto* Block = Cast<UTextBlock>(Child)) Block->SetText(FText::GetEmpty());
}

void UUmbraItemTooltip::ClearRows()
{
	UPanelWidget* Rows[] = { DamageRows.Get(), ScalingRows.Get(), StatRows.Get(), RequirementRows.Get(), SpecialEffectRows.Get() };
	for (UPanelWidget* Container : Rows)
	{
		if (!Container) continue;
		// Clear detached row objects too: external references must not keep old item text.
		for (UWidget* Child : Container->GetAllChildren()) ClearEntryWidget(Child);
		Container->ClearChildren();
	}
}

void UUmbraItemTooltip::AddKeyValueEntry(UVerticalBox* Container, const FText& Label, const FText& Value)
{
	if (!Container) return;
	if (KeyValueEntryClass && !KeyValueEntryClass->HasAnyClassFlags(CLASS_Abstract))
	{
		if (auto* Entry = CreateWidget<UUmbraTooltipKeyValueEntry>(this, KeyValueEntryClass))
		{
			Entry->SetEntry(Label, Value);
			Container->AddChildToVerticalBox(Entry);
			return;
		}
		UE_LOG(LogUmbra, Warning, TEXT("Tooltip %s could not create KeyValueEntryClass; using a TextBlock."), *GetName());
	}
	auto* Block = NewObject<UTextBlock>(this);
	Block->SetText(FText::Format(LOCTEXT("KeyValueFallback", "{0}  {1}"), Label, Value));
	Block->SetAutoWrapText(true);
	Block->SetVisibility(ESlateVisibility::HitTestInvisible);
	Container->AddChildToVerticalBox(Block);
}

void UUmbraItemTooltip::AddFullTextEntry(UVerticalBox* Container, const FText& Text, EUmbraTooltipRowStyle Style)
{
	if (!Container) return;
	if (StatEntryClass && !StatEntryClass->HasAnyClassFlags(CLASS_Abstract))
	{
		if (auto* Entry = CreateWidget<UUmbraTooltipStatEntry>(this, StatEntryClass))
		{
			Entry->SetEntryText(Text, Style);
			Container->AddChildToVerticalBox(Entry);
			return;
		}
		UE_LOG(LogUmbra, Warning, TEXT("Tooltip %s could not create StatEntryClass; using a TextBlock."), *GetName());
	}
	auto* Block = NewObject<UTextBlock>(this);
	Block->SetText(Text);
	Block->SetAutoWrapText(true);
	Block->SetVisibility(ESlateVisibility::HitTestInvisible);
	Container->AddChildToVerticalBox(Block);
}

void UUmbraItemTooltip::AddScalingEntry(UUniformGridPanel* Panel, const FText& Attribute, const FText& Grade, int32 Index)
{
	if (!Panel) return;
	UWidget* Cell = nullptr;
	if (ScalingEntryClass && !ScalingEntryClass->HasAnyClassFlags(CLASS_Abstract))
	{
		if (auto* Entry = CreateWidget<UUmbraTooltipScalingEntry>(this, ScalingEntryClass))
		{
			Entry->SetEntry(Attribute, Grade);
			Cell = Entry;
		}
		else UE_LOG(LogUmbra, Warning, TEXT("Tooltip %s could not create ScalingEntryClass; using a TextBlock."), *GetName());
	}
	if (!Cell)
	{
		auto* Block = NewObject<UTextBlock>(this);
		Block->SetText(FText::Format(LOCTEXT("ScalingFallback", "{0} {1}"), Attribute, Grade));
		Block->SetAutoWrapText(true);
		Block->SetVisibility(ESlateVisibility::HitTestInvisible);
		Cell = Block;
	}
	Panel->AddChildToUniformGrid(Cell, Index / 2, Index % 2);
}

void UUmbraItemTooltip::RefreshDisplay(bool bNotify)
{
	if (IsDesignTime())
	{
		// Layout preview is not valid TooltipData: preserve authored placeholders and rows.
		SetIsFocusable(false);
		SetVisibility(ESlateVisibility::HitTestInvisible);
		return;
	}
	ClearRows();
	const bool bValid = TooltipData.bValid && TooltipData.Result == EUmbraTooltipResult::Success;
	const auto& Data = TooltipData;

	Text(ItemName, Data.DisplayName, bValid);
	if (ItemName) ItemName->SetColorAndOpacity(bValid ? FSlateColor(Data.RarityColor) : FSlateColor(FLinearColor::White));
	if (ItemIcon) { ItemIcon->SetBrushFromTexture(bValid ? Data.Icon.Get() : nullptr); Show(ItemIcon, bValid && Data.Icon != nullptr); }
	Text(ItemTypeText, Data.ItemType, bValid);
	Text(RarityText, Data.RarityText, bValid);
	if (RarityText) RarityText->SetColorAndOpacity(bValid ? FSlateColor(Data.RarityColor) : FSlateColor(FLinearColor::White));

	// Quality backgrounds use authored textures, never a color tint; unconfigured restores the designer brush.
	ApplyBackground(quality_bg, bValid ? Data.TooltipTitleBackgroundTexture.Get() : nullptr, QualityBackgroundBrush, bQualityBackgroundCaptured);
	ApplyBackground(SlotBackground, bValid ? Data.TooltipIconBackgroundTexture.Get() : nullptr, SlotBackgroundBrush, bSlotBackgroundCaptured);

	Text(ItemLevelText, Data.ItemLevelText, bValid);
	Show(HeaderSection, bValid);

	const bool bDamage = bValid && !Data.DamageChannels.IsEmpty();
	Text(TotalDamageText, Data.TotalDamageText, bDamage);
	if (bDamage) for (const auto& Entry : Data.DamageEntries) AddKeyValueEntry(DamageRows, Entry.LabelText, Entry.ValueText);
	Show(DamageRows, bDamage);
	Show(DamageSection, bDamage);

	bool bScaling = false;
	if (bValid)
	{
		int32 Index = 0;
		for (const auto& Entry : Data.ScalingEntries)
		{
			bScaling = true;
			AddScalingEntry(ScalingRows, Entry.LabelText, Entry.ValueText, Index);
			++Index;
		}
	}
	Text(ScalingNote, Data.ScalingNote, bScaling);
	Show(ScalingRows, bScaling);
	Show(ScalingSection, bScaling);

	if (bValid)
	{
		for (const auto& Entry : Data.StatEntries)
		{
			if (Entry.DisplayMode == EUmbraTooltipEntryDisplayMode::KeyValue)
				AddKeyValueEntry(StatRows, Entry.LabelText, Entry.ValueText);
			else
				AddFullTextEntry(StatRows, Entry.FullText, ToRowStyle(Entry.Style));
		}
	}
	Show(StatRows, bValid && !Data.StatEntries.IsEmpty());
	Show(StatsSection, bValid && !Data.StatEntries.IsEmpty());

	if (bValid)
	{
		for (const auto& Requirement : Data.RequirementRows)
			AddFullTextEntry(RequirementRows, Requirement.Text, Requirement.Style);
		if (!Data.RequirementStatusText.IsEmpty())
		{
			const EUmbraTooltipRowStyle StatusStyle = Data.RequirementState == EUmbraTooltipRequirementState::Met ? EUmbraTooltipRowStyle::Met
				: Data.RequirementState == EUmbraTooltipRequirementState::LevelTooLow ? EUmbraTooltipRowStyle::Unmet : EUmbraTooltipRowStyle::Unknown;
			AddFullTextEntry(RequirementRows, Data.RequirementStatusText, StatusStyle);
		}
	}
	const bool bRequirements = bValid && (!Data.RequirementRows.IsEmpty() || !Data.RequirementStatusText.IsEmpty() || !Data.PenaltyText.IsEmpty());
	Show(RequirementRows, bRequirements);
	Text(PenaltyText, Data.PenaltyText, bValid && !Data.PenaltyText.IsEmpty());
	if (PenaltyText) PenaltyText->SetAutoWrapText(true);
	Show(RequirementsSection, bRequirements);

	// Zero weight is valid and intentionally displayed for every valid item.
	Text(WeightText, Data.WeightText, bValid);
	Show(WeightSection, bValid);

	if (bValid) for (const auto& Effect : Data.SpecialEffects) AddFullTextEntry(SpecialEffectRows, Effect, EUmbraTooltipRowStyle::Neutral);
	Show(SpecialEffectRows, bValid && !Data.SpecialEffects.IsEmpty());
	Show(SpecialEffectsSection, bValid && !Data.SpecialEffects.IsEmpty());

	Text(FlavorTextBlock, Data.FlavorText, bValid && !Data.FlavorText.IsEmpty());
	Show(FlavorTextSection, bValid && !Data.FlavorText.IsEmpty());

	SetIsFocusable(false);
	SetVisibility(bValid ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (bNotify) BP_TooltipDataChanged(TooltipData);
}

void UUmbraItemTooltip::NativePreConstruct() { Super::NativePreConstruct(); CaptureBackgrounds(); RefreshDisplay(false); }
void UUmbraItemTooltip::NativeConstruct()
{
	Super::NativeConstruct();
	CaptureBackgrounds();
	if (!ItemName) UE_LOG(LogUmbra, Warning, TEXT("ItemTooltip %s requires ItemName (TextBlock)."), *GetName());
	if (!StatRows) UE_LOG(LogUmbra, Warning, TEXT("ItemTooltip %s requires StatRows (VerticalBox)."), *GetName());
	if (StatEntryClass && StatEntryClass->HasAnyClassFlags(CLASS_Abstract))
		UE_LOG(LogUmbra, Warning, TEXT("ItemTooltip %s StatEntryClass is abstract; choose WBP_TooltipStatEntry or leave unset for plain text."), *GetName());
	if (KeyValueEntryClass && KeyValueEntryClass->HasAnyClassFlags(CLASS_Abstract))
		UE_LOG(LogUmbra, Warning, TEXT("ItemTooltip %s KeyValueEntryClass is abstract; choose WBP_TooltipKeyValueEntry or leave unset for plain text."), *GetName());
	if (ScalingEntryClass && ScalingEntryClass->HasAnyClassFlags(CLASS_Abstract))
		UE_LOG(LogUmbra, Warning, TEXT("ItemTooltip %s ScalingEntryClass is abstract; choose WBP_TooltipScalingEntry or leave unset for plain text."), *GetName());
	RefreshDisplay(true);
}
void UUmbraItemTooltip::NativeDestruct()
{
	Super::NativeDestruct();
	TooltipData = FUmbraItemTooltipData();
	RefreshDisplay(false);
}

#undef LOCTEXT_NAMESPACE
