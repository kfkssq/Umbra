#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UmbraEnemyHealthBar.generated.h"
class AUmbraEnemyCharacter;
class UUmbraAbilitySystemComponent;
class UProgressBar;
struct FOnAttributeChangeData;

/** Presentation only; listens to replicated GAS attributes. */
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
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthProgressBar;
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

