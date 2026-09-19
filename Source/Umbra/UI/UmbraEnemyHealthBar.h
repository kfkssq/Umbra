#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UmbraEnemyHealthBar.generated.h"
class AUmbraEnemyCharacter;
class UUmbraAbilitySystemComponent;
struct FOnAttributeChangeData;

/** Read-only presentation state produced from the enemy's replicated GAS attributes. */
USTRUCT(BlueprintType)
struct UMBRA_API FUmbraEnemyHealthBarViewState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Enemy Health Bar")
	float Health = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy Health Bar")
	float MaxHealth = 0.f;

	/** Safe Health / MaxHealth ratio in the [0, 1] range. */
	UPROPERTY(BlueprintReadOnly, Category = "Enemy Health Bar")
	float HealthNormalized = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy Health Bar")
	bool bVisible = false;
};

/** Observes replicated GAS attributes and sends widget-agnostic state to Blueprint presentation. */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraEnemyHealthBar : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetEnemy(AUmbraEnemyCharacter* Enemy);
	void Shutdown();
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Implement in the WBP to update any native, material-driven, or composite health-bar control. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy Health Bar", meta = (DisplayName = "Apply Enemy Health Bar State"))
	void BP_ApplyViewState(const FUmbraEnemyHealthBarViewState& State);
private:
	void Bind();
	void UnbindASC();
	void Refresh();
	void OnAttributeChanged(const FOnAttributeChangeData& Data);
	void OnLifecycle(UUmbraAbilitySystemComponent* ASC, bool bReady);
	UFUNCTION()
	void OnEnemyEndPlay(AActor* Actor, EEndPlayReason::Type Reason);
	TWeakObjectPtr<AUmbraEnemyCharacter> Enemy;
	TWeakObjectPtr<UUmbraAbilitySystemComponent> BoundASC;
	FDelegateHandle HealthHandle;
	FDelegateHandle MaxHealthHandle;
	FDelegateHandle LifecycleHandle;
};
