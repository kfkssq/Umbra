#include "UI/UmbraEnemyHealthBarComponent.h"
#include "UI/UmbraEnemyHealthBar.h"
#include "Characters/UmbraEnemyCharacter.h"

UUmbraEnemyHealthBarComponent::UUmbraEnemyHealthBarComponent()
{
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawAtDesiredSize(true);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetWindowFocusable(false);
}

void UUmbraEnemyHealthBarComponent::OnRegister()
{
	// The WBP root owns layout size, including for assets with an old fixed-size override.
	SetDrawAtDesiredSize(true);
	Super::OnRegister();
}

void UUmbraEnemyHealthBarComponent::InitWidget()
{
	if (GetNetMode() == NM_DedicatedServer) return;
	Super::InitWidget();
	if (auto* Bar = Cast<UUmbraEnemyHealthBar>(GetUserWidgetObject()))
		Bar->SetEnemy(Cast<AUmbraEnemyCharacter>(GetOwner()));
}

void UUmbraEnemyHealthBarComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (auto* Bar = Cast<UUmbraEnemyHealthBar>(GetUserWidgetObject())) Bar->Shutdown();
	Super::EndPlay(Reason);
}
