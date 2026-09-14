#pragma once
#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "UmbraEnemyHealthBarComponent.generated.h"

/** Assigns the owner whenever the component creates its widget, regardless of ASC ordering. */
UCLASS()
class UMBRA_API UUmbraEnemyHealthBarComponent : public UWidgetComponent
{
	GENERATED_BODY()
public:
	UUmbraEnemyHealthBarComponent();
	virtual void InitWidget() override;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
};

