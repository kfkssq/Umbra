#include "UI/UmbraPlayerAttributeBar.h"

#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Player/UmbraPlayerState.h"

namespace
{
	FGameplayAttribute CurrentAttribute(EUmbraPlayerAttributeBarType Type)
	{
		return Type == EUmbraPlayerAttributeBarType::Resource
			? UUmbraAttributeSet::GetResourceAttribute()
			: UUmbraAttributeSet::GetHealthAttribute();
	}

	FGameplayAttribute MaxAttribute(EUmbraPlayerAttributeBarType Type)
	{
		return Type == EUmbraPlayerAttributeBarType::Resource
			? UUmbraAttributeSet::GetMaxResourceAttribute()
			: UUmbraAttributeSet::GetMaxHealthAttribute();
	}
}

void UUmbraPlayerAttributeBar::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(false);
	Bind();
}

void UUmbraPlayerAttributeBar::NativeDestruct()
{
	Shutdown();
	Super::NativeDestruct();
}

void UUmbraPlayerAttributeBar::NotifyPlayerContextChanged()
{
	Bind();
}

void UUmbraPlayerAttributeBar::Shutdown()
{
	UnbindASC();
	UUmbraAbilitySystemComponent::OnLifecycleChanged.Remove(LifecycleHandle);
	LifecycleHandle.Reset();
	BoundPlayerState.Reset();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UUmbraPlayerAttributeBar::Bind()
{
	AUmbraPlayerState* PlayerState = GetOwningPlayerState<AUmbraPlayerState>();
	if (BoundPlayerState.Get() != PlayerState)
	{
		UnbindASC();
		BoundPlayerState = PlayerState;
	}

	if (!LifecycleHandle.IsValid())
	{
		LifecycleHandle = UUmbraAbilitySystemComponent::OnLifecycleChanged.AddUObject(this, &ThisClass::OnLifecycle);
	}

	UUmbraAbilitySystemComponent* ASC = PlayerState ? PlayerState->GetUmbraAbilitySystemComponent() : nullptr;
	if (!ASC || !ASC->IsActorInfoReady() || !ASC->GetSet<UUmbraAttributeSet>())
	{
		UnbindASC();
		Refresh();
		return;
	}

	if (BoundASC.Get() != ASC || !bHasBoundAttributeType || BoundAttributeType != AttributeType)
	{
		UnbindASC();
		BoundASC = ASC;
		BoundAttributeType = AttributeType;
		bHasBoundAttributeType = true;
		CurrentValueHandle = ASC->GetGameplayAttributeValueChangeDelegate(CurrentAttribute(BoundAttributeType))
			.AddUObject(this, &ThisClass::OnAttributeChanged);
		MaxValueHandle = ASC->GetGameplayAttributeValueChangeDelegate(MaxAttribute(BoundAttributeType))
			.AddUObject(this, &ThisClass::OnAttributeChanged);
	}

	Refresh();
}

void UUmbraPlayerAttributeBar::UnbindASC()
{
	if (UUmbraAbilitySystemComponent* ASC = BoundASC.Get(); ASC && bHasBoundAttributeType)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(CurrentAttribute(BoundAttributeType)).Remove(CurrentValueHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(MaxAttribute(BoundAttributeType)).Remove(MaxValueHandle);
	}
	BoundASC.Reset();
	bHasBoundAttributeType = false;
	CurrentValueHandle.Reset();
	MaxValueHandle.Reset();
}

void UUmbraPlayerAttributeBar::OnLifecycle(UUmbraAbilitySystemComponent* ASC, bool bReady)
{
	AUmbraPlayerState* PlayerState = GetOwningPlayerState<AUmbraPlayerState>();
	if (!PlayerState || ASC != PlayerState->GetUmbraAbilitySystemComponent())
	{
		return;
	}
	if (bReady)
	{
		Bind();
	}
	else if (BoundASC.Get() == ASC)
	{
		UnbindASC();
		Refresh();
	}
}

void UUmbraPlayerAttributeBar::OnAttributeChanged(const FOnAttributeChangeData& Data)
{
	Refresh();
}

FUmbraPlayerAttributeBarViewState UUmbraPlayerAttributeBar::MakeViewState(
	const UUmbraAttributeSet* Attributes, bool bReady, EUmbraPlayerAttributeBarType Type)
{
	FUmbraPlayerAttributeBarViewState State;
	State.AttributeType = Type;
	State.bReady = bReady && Attributes;
	if (!Attributes)
	{
		return State;
	}

	if (Type == EUmbraPlayerAttributeBarType::Resource)
	{
		State.CurrentValue = Attributes->GetResource();
		State.MaxValue = Attributes->GetMaxResource();
	}
	else
	{
		State.CurrentValue = Attributes->GetHealth();
		State.MaxValue = Attributes->GetMaxHealth();
	}

	State.Normalized = FMath::IsFinite(State.CurrentValue) && FMath::IsFinite(State.MaxValue) && State.MaxValue > 0.f
		? FMath::Clamp(State.CurrentValue / State.MaxValue, 0.f, 1.f) : 0.f;
	return State;
}

void UUmbraPlayerAttributeBar::Refresh()
{
	const UUmbraAbilitySystemComponent* ASC = BoundASC.Get();
	const UUmbraAttributeSet* Attributes = ASC ? ASC->GetSet<UUmbraAttributeSet>() : nullptr;
	const FUmbraPlayerAttributeBarViewState State = MakeViewState(
		Attributes, ASC && ASC->IsActorInfoReady(), AttributeType);
	SetVisibility(State.bReady ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	BP_ApplyViewState(State);
}
