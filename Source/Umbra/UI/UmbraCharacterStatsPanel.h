#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Blueprint/UserWidget.h"
#include "UI/UmbraStatEntry.h"
#include "UmbraCharacterStatsPanel.generated.h"

class APawn;
class AUmbraPlayerState;
class UUmbraAbilitySystemComponent;
struct FOnAttributeChangeData;

USTRUCT(BlueprintType)
struct FUmbraStatDisplayData
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Stats")
	FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Stats")
	TObjectPtr<UTexture2D> Icon = nullptr;
};

/** Observes the local character's GAS state and sends formatted values to registered rows. */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraCharacterStatsPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	UUmbraCharacterStatsPanel(const FObjectInitializer& ObjectInitializer);
	/** Called by the root HUD/controller when the local PlayerState changes. Safe to repeat. */
	UFUNCTION(BlueprintCallable, Category = "Character Stats")
	void NotifyPlayerContextChanged();

	/** Also supports entries added dynamically after construction. */
	UFUNCTION(BlueprintCallable, Category = "Character Stats")
	void RegisterEntry(UUmbraStatEntry* Entry);

	void Shutdown();

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	/** WBP Class Defaults override localized native names and supply textures; no asset paths in C++. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Stats")
	TMap<EUmbraCharacterStat, FUmbraStatDisplayData> StatDisplayData;

private:
	void RebuildEntries();
	void BindStat(EUmbraCharacterStat Stat);
	void BindPlayer();
	void UnbindASC();
	void RefreshAll();
	void RefreshStat(EUmbraCharacterStat Stat);
	bool TryReadStat(EUmbraCharacterStat Stat, float& OutValue) const;
	void OnAttributeChanged(const FOnAttributeChangeData& Data);
	void OnLifecycle(UUmbraAbilitySystemComponent* ASC, bool bReady);

	UFUNCTION()
	void OnPawnChanged(APawn* OldPawn, APawn* NewPawn);

	TMap<EUmbraCharacterStat, TWeakObjectPtr<UUmbraStatEntry>> Entries;
	TMap<FGameplayAttribute, FDelegateHandle> AttributeHandles;
	TWeakObjectPtr<AUmbraPlayerState> BoundPlayerState;
	TWeakObjectPtr<UUmbraAbilitySystemComponent> BoundASC;
	FDelegateHandle LifecycleHandle;
};
