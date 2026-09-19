#include "UI/UmbraEnemyHealthBar.h"
#include "Characters/UmbraEnemyCharacter.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"

void UUmbraEnemyHealthBar::SetEnemy(AUmbraEnemyCharacter* InEnemy)
{
	if (Enemy.Get() != InEnemy)
	{
		Shutdown();
		Enemy = InEnemy;
	}
	Bind();
}

void UUmbraEnemyHealthBar::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(false);
	Bind();
}

void UUmbraEnemyHealthBar::NativeDestruct()
{
	// Keep the weak target for a later Slate reconstruction, but release every listener.
	Shutdown();
	Super::NativeDestruct();
}

void UUmbraEnemyHealthBar::UnbindASC()
{
	if (auto* ASC = BoundASC.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetHealthAttribute()).Remove(HealthHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthHandle);
	}
	BoundASC.Reset();
	HealthHandle.Reset();
	MaxHealthHandle.Reset();
}

void UUmbraEnemyHealthBar::Shutdown()
{
	UnbindASC();
	UUmbraAbilitySystemComponent::OnLifecycleChanged.Remove(LifecycleHandle);
	LifecycleHandle.Reset();
	if (Enemy.IsValid()) Enemy->OnEndPlay.RemoveDynamic(this, &ThisClass::OnEnemyEndPlay);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UUmbraEnemyHealthBar::Bind()
{
	if (!Enemy.IsValid()) { Refresh(); return; }
	Enemy->OnEndPlay.AddUniqueDynamic(this, &ThisClass::OnEnemyEndPlay);
	if (!LifecycleHandle.IsValid())
		LifecycleHandle = UUmbraAbilitySystemComponent::OnLifecycleChanged.AddUObject(this, &ThisClass::OnLifecycle);
	auto* ASC = Enemy->GetUmbraAbilitySystemComponent();
	if (!ASC || !ASC->IsActorInfoReady() || !ASC->GetSet<UUmbraAttributeSet>())
	{
		UnbindASC();
		Refresh();
		return;
	}
	if (BoundASC.Get() != ASC)
	{
		UnbindASC();
		BoundASC = ASC;
		HealthHandle = ASC->GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetHealthAttribute())
			.AddUObject(this, &ThisClass::OnAttributeChanged);
		MaxHealthHandle = ASC->GetGameplayAttributeValueChangeDelegate(UUmbraAttributeSet::GetMaxHealthAttribute())
			.AddUObject(this, &ThisClass::OnAttributeChanged);
	}
	Refresh();
}

void UUmbraEnemyHealthBar::OnLifecycle(UUmbraAbilitySystemComponent* ASC, bool bReady)
{
	if (!Enemy.IsValid() || ASC != Enemy->GetUmbraAbilitySystemComponent()) return;
	if (bReady) Bind();
	else { UnbindASC(); Refresh(); }
}

void UUmbraEnemyHealthBar::OnEnemyEndPlay(AActor* Actor, EEndPlayReason::Type Reason)
{
	Shutdown();
	Enemy.Reset();
}

void UUmbraEnemyHealthBar::OnAttributeChanged(const FOnAttributeChangeData& Data)
{
	Refresh();
}

void UUmbraEnemyHealthBar::Refresh()
{
	const auto* ASC = BoundASC.Get();
	const auto* Attributes = ASC ? ASC->GetSet<UUmbraAttributeSet>() : nullptr;
	FUmbraEnemyHealthBarViewState State;
	State.Health = Attributes ? Attributes->GetHealth() : 0.f;
	State.MaxHealth = Attributes ? Attributes->GetMaxHealth() : 0.f;
	State.HealthNormalized = FMath::IsFinite(State.Health) && FMath::IsFinite(State.MaxHealth) && State.MaxHealth > 0.f
		? FMath::Clamp(State.Health / State.MaxHealth, 0.f, 1.f) : 0.f;
	State.bVisible = Enemy.IsValid() && FMath::IsFinite(State.Health) && State.Health > 0.f;

	// HitTestInvisible applies to this widget AND all descendants.
	SetVisibility(State.bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	BP_ApplyViewState(State);
}
