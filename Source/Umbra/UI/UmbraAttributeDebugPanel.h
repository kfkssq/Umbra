#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Blueprint/UserWidget.h"
#include "UmbraAttributeDebugPanel.generated.h"

class UButton;
class UTextBlock;
class UUmbraAbilitySystemComponent;
class UUmbraAttributeSet;
struct FOnAttributeChangeData;
enum class EUmbraAttributeDebugOperation : uint8;

/** Event-driven, read-only attribute view. All test mutations go through the owning controller. */
UCLASS(Abstract, Blueprintable)
class UMBRA_API UUmbraAttributeDebugPanel : public UUserWidget
{
	GENERATED_BODY()
public:
	void ViewPlayer();
	void ViewEnemy(AActor* Enemy);
	void NotifyPlayerContextChanged();
	void SetSelectionFeedback(const FText& Message);
	void ShutdownPanel();
	static FText FormatAttributeSnapshot(const UUmbraAttributeSet& Attributes);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TargetNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AttributesText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HintText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AddEffectButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RemoveEffectButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> DamageButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HealButton;

private:
	void RefreshBinding();
	void BindTarget(AActor* Actor, UUmbraAbilitySystemComponent* ASC);
	void UnbindTarget();
	void RefreshSnapshot();
	void HandleAttributeChanged(const FOnAttributeChangeData& Data);
	void HandleASCLifecycle(UUmbraAbilitySystemComponent* ASC, bool bReady);
	void SendOperation(EUmbraAttributeDebugOperation Operation);

	UFUNCTION()
	void HandlePawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandleTargetEndPlay(AActor* Actor, EEndPlayReason::Type Reason);

	UFUNCTION()
	void AddEffectClicked();
	UFUNCTION()
	void RemoveEffectClicked();
	UFUNCTION()
	void DamageClicked();
	UFUNCTION()
	void HealClicked();

	TWeakObjectPtr<AActor> ViewedActor;
	TWeakObjectPtr<UUmbraAbilitySystemComponent> ViewedASC;
	TArray<TPair<FGameplayAttribute, FDelegateHandle>> AttributeListeners;
	FDelegateHandle LifecycleListener;
	bool bViewingPlayer = true;
	bool bShuttingDown = false;
};
