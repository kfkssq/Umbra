#pragma once

#include "CoreMinimal.h"
#include "UI/UmbraStatEntry.h"
#include "UI/Combat/UmbraCombatStatData.h"
#include "UmbraCombatStatEntry.generated.h"

/** Pure view: Designer preview or a typed value supplied by CombatInfo, never its own GAS observer. */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UMBRA_API UUmbraCombatStatEntry : public UUmbraStatEntry
{
	GENERATED_BODY()

public:
	/** None resolves only an exact widget instance name (e.g. AttackPower); labels are not identifiers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat UI|Binding")
	EUmbraCombatStat CombatStat = EUmbraCombatStat::None;
	EUmbraCombatStat ResolveCombatStat() const;
	void ApplyCombatValue(const FUmbraCombatStatValue& Value);
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Combat UI|Binding")
	FUmbraCombatStatValue CombatValue;

protected:
	virtual void NativePreConstruct() override;

	/** WBP_CombatInfo row instance overrides the WBP row default; includes units such as %. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat UI|Test Data", meta = (ExposeOnSpawn = "true"))
	FText TestValue;
};
