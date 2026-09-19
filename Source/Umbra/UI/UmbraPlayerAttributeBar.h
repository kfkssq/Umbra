#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UmbraPlayerAttributeBar.generated.h"

class AUmbraPlayerState;
class UUmbraAbilitySystemComponent;
class UUmbraAttributeSet;
struct FOnAttributeChangeData;

/** Player-owned attribute pair presented by a reusable HUD bar. */
UENUM(BlueprintType)
enum class EUmbraPlayerAttributeBarType : uint8
{
	Health,
	Resource
};

/** Read-only presentation state produced from the owning player's replicated GAS attributes. */
USTRUCT(BlueprintType)
struct UMBRA_API FUmbraPlayerAttributeBarViewState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Player Attribute Bar")
	EUmbraPlayerAttributeBarType AttributeType = EUmbraPlayerAttributeBarType::Health;

	UPROPERTY(BlueprintReadOnly, Category = "Player Attribute Bar")
	float CurrentValue = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Player Attribute Bar")
	float MaxValue = 0.f;

	/** Safe CurrentValue / MaxValue ratio in the [0, 1] range. */
	UPROPERTY(BlueprintReadOnly, Category = "Player Attribute Bar")
	float Normalized = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Player Attribute Bar")
	bool bReady = false;
};

/**
 * Independent, reusable player attribute bar. Each widget instance selects Health or Resource,
 * observes the matching GAS value/max pair, and forwards presentation state to Blueprint.
 */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraPlayerAttributeBar : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Re-evaluates the owning PlayerState; useful if a parent HUD changes player context. */
	void NotifyPlayerContextChanged();
	void Shutdown();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Select per WBP class or per embedded widget instance before construction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Attribute Bar", meta = (ExposeOnSpawn = "true"))
	EUmbraPlayerAttributeBarType AttributeType = EUmbraPlayerAttributeBarType::Health;

	UFUNCTION(BlueprintImplementableEvent, Category = "Player Attribute Bar", meta = (DisplayName = "Apply Player Attribute Bar State"))
	void BP_ApplyViewState(const FUmbraPlayerAttributeBarViewState& State);

private:
	friend class FUmbraPlayerAttributeBarStateTest;
	static FUmbraPlayerAttributeBarViewState MakeViewState(
		const UUmbraAttributeSet* Attributes, bool bReady, EUmbraPlayerAttributeBarType Type);
	void Bind();
	void UnbindASC();
	void Refresh();
	void OnAttributeChanged(const FOnAttributeChangeData& Data);
	void OnLifecycle(UUmbraAbilitySystemComponent* ASC, bool bReady);

	TWeakObjectPtr<AUmbraPlayerState> BoundPlayerState;
	TWeakObjectPtr<UUmbraAbilitySystemComponent> BoundASC;
	EUmbraPlayerAttributeBarType BoundAttributeType = EUmbraPlayerAttributeBarType::Health;
	bool bHasBoundAttributeType = false;
	FDelegateHandle CurrentValueHandle;
	FDelegateHandle MaxValueHandle;
	FDelegateHandle LifecycleHandle;
};
