#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UmbraStatEntry.generated.h"

class UTexture2D;
class UUmbraStatTooltip;

/** Stable identifiers for the eight slots in the character sheet. */
UENUM(BlueprintType)
enum class EUmbraCharacterStat : uint8
{
	AttackPower,
	AbilityPower,
	Armor,
	MagicResistance,
	AttackSpeed,
	AbilityHaste,
	CriticalChance,
	MoveSpeed
};

/** Compact icon/value cell. Name and description are shown by a separate tooltip widget. */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraStatEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	EUmbraCharacterStat GetStat() const { return Stat; }

	/** The sole value update path, including unavailable values. */
	UFUNCTION(BlueprintCallable, Category = "Character Stats")
	void SetDisplayValue(const FText& Value);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Stats", meta = (ExposeOnSpawn = "true"))
	EUmbraCharacterStat Stat = EUmbraCharacterStat::AttackPower;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Stats", meta = (ExposeOnSpawn = "true"))
	FText StatName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Stats", meta = (ExposeOnSpawn = "true", MultiLine = "true"))
	FText Description;

	/** Configure WBP_StatTooltip on the WBP_StatEntry class. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Stats|Tooltip")
	TSubclassOf<UUmbraStatTooltip> TooltipClass;

	/** Kept so WBP graphs made before the icon migration remain loadable; no longer invoked. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Character Stats", meta = (DisplayName = "Apply Stat Icon", DeprecatedFunction, DeprecationMessage = "Set the Image Brush in the entry WBP; this event is no longer called."))
	void BP_ApplyIcon(UTexture2D* InIcon);

	UFUNCTION(BlueprintImplementableEvent, Category = "Character Stats", meta = (DisplayName = "Apply Stat Value"))
	void BP_ApplyValue(const FText& Value);

private:
	FText DisplayValue;

	UPROPERTY(Transient)
	TObjectPtr<UUmbraStatTooltip> StatTooltip;
};
