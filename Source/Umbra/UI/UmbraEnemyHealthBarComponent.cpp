#include "UI/UmbraEnemyHealthBarComponent.h"
#include "UI/UmbraEnemyHealthBar.h"
#include "Characters/UmbraEnemyCharacter.h"

UUmbraEnemyHealthBarComponent::UUmbraEnemyHealthBarComponent()
{
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawSize(FVector2D(120.f, 12.f));
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetWindowFocusable(false);
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

