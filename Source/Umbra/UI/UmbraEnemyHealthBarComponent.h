#pragma once
#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "UmbraEnemyHealthBarComponent.generated.h"

/** Uses the WBP desired size and assigns the owner regardless of ASC ordering. */
UCLASS()
class UMBRA_API UUmbraEnemyHealthBarComponent : public UWidgetComponent
{
	GENERATED_BODY()
public:
	UUmbraEnemyHealthBarComponent();
	virtual void InitWidget() override;
protected:
	virtual void OnRegister() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
};
