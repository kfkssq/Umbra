#include "Tests/UmbraEquipmentNetworkTestLibrary.h"
#include "Characters/UmbraEnemyCharacter.h"
#include "Player/UmbraPlayerState.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "Equipment/UmbraEquipmentEffect.h"
#include "Equipment/UmbraEquipmentEffectContext.h"
#include "UObject/UnrealType.h"
#include "UmbraPlayerController.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Engine/Engine.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "UI/Equipment/UmbraEquipmentSlotWidget.h"

bool UUmbraEquipmentNetworkTestLibrary::MigrateRetiredFeetWidget(UUmbraEquipmentSlotWidget* Widget)
{
#if WITH_EDITOR
	if (!IsValid(Widget)) return false;
	const auto* Icon = Widget->EmptySlotIcons.Find(EUmbraEquipmentSlot::Amulet);
	if (!Icon || !IsValid(Icon->Get())) return false;
	const auto AmuletIcon = *Icon;
	Widget->Modify();
	Widget->EmptySlotIcons.Remove(EUmbraEquipmentSlot::Feet);
	Widget->EmptySlotIcons.Add(EUmbraEquipmentSlot::Amulet2, AmuletIcon);
	if (Widget->SlotType == EUmbraEquipmentSlot::Feet) Widget->SlotType = EUmbraEquipmentSlot::Amulet2;
	return true;
#else
	return false;
#endif
}

TArray<UWidget*> UUmbraEquipmentNetworkTestLibrary::GetBlueprintWidgets(UObject* Blueprint)
{
	TArray<UWidget*> Widgets;
#if WITH_EDITOR
	if (IsValid(Blueprint))
		if (const auto* Property = FindFProperty<FObjectPropertyBase>(Blueprint->GetClass(), TEXT("WidgetTree")))
			if (auto* Tree = Cast<UWidgetTree>(Property->GetObjectPropertyValue_InContainer(Blueprint))) Tree->GetAllWidgets(Widgets);
#endif
	return Widgets;
}

bool UUmbraEquipmentNetworkTestLibrary::KillEnemyForLootTest(AUmbraEnemyCharacter* Enemy)
{
#if !UE_BUILD_SHIPPING
	if (!IsValid(Enemy) || !Enemy->HasAuthority() || !Enemy->GetUmbraAbilitySystemComponent()) return false;
	Enemy->GetUmbraAbilitySystemComponent()->SetNumericAttributeBase(UUmbraAttributeSet::GetHealthAttribute(), 0.f);
	return !Enemy->CanBeAttacked_Implementation();
#else
	return false;
#endif
}

AUmbraEnemyCharacter* UUmbraEquipmentNetworkTestLibrary::SpawnEnemyForLootTest(AUmbraEnemyCharacter* Template)
{
#if !UE_BUILD_SHIPPING
	if (!IsValid(Template) || !Template->HasAuthority()) return nullptr;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return Template->GetWorld()->SpawnActor<AUmbraEnemyCharacter>(Template->GetClass(), Template->GetActorTransform(), Params);
#else
	return nullptr;
#endif
}

EUmbraTransferResult UUmbraEquipmentNetworkTestLibrary::SubmitRemoteRequest(AUmbraPlayerController* Controller, EUmbraItemTransferOperation Operation, FGuid Id, EUmbraEquipmentSlot Slot, int32 Cell)
{
#if WITH_EDITOR
	// Python's FEditorScriptExecutionGuard forces Actor RPCs local. Exercise real network callspace instead.
	TGuardValue<bool> Guard(GAllowActorScriptExecutionInEditor, false);
	return Controller ? Controller->RequestItemTransfer(Operation, Id, Slot, Cell) : EUmbraTransferResult::NotReady;
#else
	return EUmbraTransferResult::NotReady;
#endif
}

TArray<int32> UUmbraEquipmentNetworkTestLibrary::ConfigurePlaySettings(int32 Mode, int32 Clients, bool bSingleProcess)
{
#if WITH_EDITOR
	// ULevelEditorPlaySettings is not exported to Python in this engine; avoid a runtime UnrealEd dependency.
	auto* Class = FindObject<UClass>(nullptr, TEXT("/Script/UnrealEd.LevelEditorPlaySettings"));
	if (!Class || Mode < 0 || Mode > 2 || Clients < 1 || Clients > 10) return {};
	auto* NetMode = FindFProperty<FByteProperty>(Class, TEXT("PlayNetMode"));
	auto* Number = FindFProperty<FIntProperty>(Class, TEXT("PlayNumberOfClients"));
	auto* Single = FindFProperty<FBoolProperty>(Class, TEXT("RunUnderOneProcess"));
	if (!NetMode || !Number || !Single) return {};
	auto* Settings = Class->GetDefaultObject();
	TArray<int32> Previous = {NetMode->GetPropertyValue_InContainer(Settings), Number->GetPropertyValue_InContainer(Settings), int32(Single->GetPropertyValue_InContainer(Settings))};
	NetMode->SetPropertyValue_InContainer(Settings, uint8(Mode)); Number->SetPropertyValue_InContainer(Settings, Clients); Single->SetPropertyValue_InContainer(Settings, bSingleProcess);
	return Previous;
#else
	return {};
#endif
}

int32 UUmbraEquipmentNetworkTestLibrary::CountDamageSnapshots(AUmbraPlayerState* State)
{
#if !UE_BUILD_SHIPPING
	if (!State) return 0;
	auto* ASC = State->GetUmbraAbilitySystemComponent();
	int32 Count=0;
	for (const auto Handle : ASC->GetActiveEffects(FGameplayEffectQuery()))
	{
		const auto* Effect=ASC->GetActiveGameplayEffect(Handle);
		if (!Effect || !Effect->Spec.Def->IsA<UUmbraEquipmentEffect>()) continue;
		const auto* Snapshot=FUmbraEquipmentEffectContext::Get(Effect->Spec.GetContext());
		if (!Snapshot || Snapshot->DamageBonuses.IsEmpty()) continue;
		if (!GetDefault<UUmbraEquipmentDamageBonusComponent>()->Validate(Effect->Spec)) return -1;
		Count += Snapshot->DamageBonuses.Num();
	}
	return Count;
#else
	return 0;
#endif
}
