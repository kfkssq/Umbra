#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UmbraSkillBar.generated.h"

class UImage;
class UTextBlock;
class UProgressBar;
class UTexture2D;
class UBorder;

USTRUCT(BlueprintType)
struct FUmbraSkillSlotView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FText Name;
    UPROPERTY(BlueprintReadOnly) FText Description;
    UPROPERTY(BlueprintReadOnly) FText KeyLabel;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> Icon;
    UPROPERTY(BlueprintReadOnly) float ResourceCost = 0.f;
    UPROPERTY(BlueprintReadOnly) float CooldownRemaining = 0.f;
    UPROPERTY(BlueprintReadOnly) float CooldownDuration = 0.f;
    /** Remaining / actual GE duration. Zero means ready. */
    UPROPERTY(BlueprintReadOnly) float CooldownFraction = 0.f;
    UPROPERTY(BlueprintReadOnly) bool bGranted = false;
    UPROPERTY(BlueprintReadOnly) bool bAffordable = false;
    UPROPERTY(BlueprintReadOnly) bool bBlocked = false;
    UPROPERTY(BlueprintReadOnly) bool bActive = false;
};

/** Text-only, Designer-owned skill description anchored above the hovered skill slot. */
UCLASS(Blueprintable)
class UMBRA_API UUmbraSkillTooltip : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetSkillText(const FText& Name, const FText& Description);
    void PresentAbove(UWidget* Anchor);
protected:
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> SkillName;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> DescriptionText;
};

/** Designer-owned presentation; each instance observes a configured PlayerState slot. */
UCLASS(Blueprintable)
class UMBRA_API UUmbraSkillSlotWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill", meta=(ClampMin="0")) int32 SlotIndex = 0;
    UPROPERTY(EditDefaultsOnly, Category="Skill|Presentation") TSubclassOf<UUmbraSkillTooltip> SkillTooltipClass;
    /** Optional Designer paths through nested UserWidgets, e.g. W_Item_Slot.item_icon. */
    UPROPERTY(EditAnywhere, Category="Skill|Presentation") FString IconWidgetPath;
    UPROPERTY(EditAnywhere, Category="Skill|Presentation") FString KeyWidgetPath;
    UPROPERTY(EditAnywhere, Category="Skill|Presentation") FString CostWidgetPath;
    UPROPERTY(EditAnywhere, Category="Skill|Presentation") FString ContentWidgetPath;
    UPROPERTY(EditAnywhere, Category="Skill|Presentation") FString FrameWidgetPath;
    UPROPERTY(EditAnywhere, Category="Skill|Presentation") TObjectPtr<UTexture2D> FrameTexture;
    /** Keep a template's UI material when it exposes a texture parameter; None uses a texture brush. */
    UPROPERTY(EditAnywhere, Category="Skill|Presentation") FName IconTextureParameter;
    /** Editor preview only. Runtime values always come from PlayerState/GAS. */
    UPROPERTY(EditAnywhere, Category="Skill|Preview") TObjectPtr<UTexture2D> PreviewIcon;
    UPROPERTY(EditAnywhere, Category="Skill|Preview") FText PreviewKeyLabel;
    UPROPERTY(EditAnywhere, Category="Skill|Preview") float PreviewResourceCost = 0.f;
    UPROPERTY(EditAnywhere, Category="Skill|Preview", meta=(ClampMin="0", ClampMax="1")) float PreviewCooldownFraction = 0.f;
    UPROPERTY(BlueprintReadOnly, Category="Skill") FUmbraSkillSlotView ViewState;
    UFUNCTION(BlueprintPure, Category="Skill") FUmbraSkillSlotView ReadViewState() const;
    UFUNCTION(BlueprintCallable, Category="Skill") void RefreshSkill();
protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
    UFUNCTION(BlueprintImplementableEvent, Category="Skill") void OnSkillViewUpdated(const FUmbraSkillSlotView& NewView);
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> Img_Icon;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Name;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Key;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Cost;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Cooldown;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Status;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> Pb_Cooldown;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> Img_Cooldown;
private:
    void ResolvePresentationWidgets();
    void ApplyViewState();
    void RefreshTooltip();
    UPROPERTY(Transient) TObjectPtr<UUmbraSkillTooltip> SkillTooltip;
    UPROPERTY(Transient) TObjectPtr<UWidget> SkillContent;
    FTimerHandle RefreshTimer;
};

/** Layout-only base for WBP_SkillBar. Slot widgets observe GAS; no second countdown. */
UCLASS(Blueprintable)
class UMBRA_API UUmbraSkillBar : public UUserWidget
{
    GENERATED_BODY()
};
