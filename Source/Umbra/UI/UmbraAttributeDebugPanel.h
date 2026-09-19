#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Blueprint/UserWidget.h"
#include "UI/UmbraAttributeDebugTypes.h"
#include "UmbraAttributeDebugPanel.generated.h"

class UUmbraAbilitySystemComponent;
class UUmbraAttributeSet;
struct FOnAttributeChangeData;

/** Event-driven GAS observer. The WBP owns all presentation and invokes the explicit operation entry point. */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraAttributeDebugPanel : public UUserWidget
{
	GENERATED_BODY()
public:
	void ViewPlayer();
	void ViewEnemy(AActor* Enemy);
	void NotifyPlayerContextChanged();
	void SetSelectionFeedback(EUmbraAttributeDebugFeedback Feedback);
	void ShutdownPanel();

	/** Called by WBP buttons. The server still validates the target and operation. */
	UFUNCTION(BlueprintCallable, Category = "Debug|Attributes", meta = (DevelopmentOnly))
	void RequestOperation(EUmbraAttributeDebugOperation Operation);

	UFUNCTION(BlueprintPure, Category = "Debug|Attributes")
	FUmbraAttributeDebugViewState GetViewState() const { return ViewState; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event) override;

	/** Implement in WBP to write raw values into any desired visual hierarchy. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Debug|Attributes", meta = (DisplayName = "Apply Attribute Debug State"))
	void BP_ApplyViewState(const FUmbraAttributeDebugViewState& State);

private:
	friend class FUmbraAttributeDebugInputTest;
	friend class FUmbraAttributeDebugTest;
	static FUmbraAttributeDebugViewState MakeViewState(const UUmbraAttributeSet* Attributes,
		AActor* TargetActor, bool bReady, bool bViewingPlayer, EUmbraAttributeDebugFeedback Feedback);
	void RefreshBinding();
	void BindTarget(AActor* Actor, UUmbraAbilitySystemComponent* ASC);
	void UnbindTarget();
	void RefreshViewState();
	void HandleAttributeChanged(const FOnAttributeChangeData& Data);
	void HandleASCLifecycle(UUmbraAbilitySystemComponent* ASC, bool bReady);

	UFUNCTION()
	void HandlePawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandleTargetEndPlay(AActor* Actor, EEndPlayReason::Type Reason);

	UPROPERTY(Transient)
	FUmbraAttributeDebugViewState ViewState;
	TWeakObjectPtr<AActor> ViewedActor;
	TWeakObjectPtr<UUmbraAbilitySystemComponent> ViewedASC;
	TArray<TPair<FGameplayAttribute, FDelegateHandle>> AttributeListeners;
	FDelegateHandle LifecycleListener;
	EUmbraAttributeDebugFeedback SelectionFeedback = EUmbraAttributeDebugFeedback::Instructions;
	bool bViewingPlayer = true;
	bool bShuttingDown = false;
};
