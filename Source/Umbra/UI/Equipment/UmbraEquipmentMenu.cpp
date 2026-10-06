#include "UI/Equipment/UmbraEquipmentMenu.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/Equipment/UmbraEquipmentSlotWidget.h"
#include "Umbra.h"
#include "Equipment/UmbraEquipmentComponent.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "Player/UmbraPlayerState.h"
#include "Engine/Texture2D.h"
#include "UI/Items/UmbraTransferFeedback.h"
#include "UI/Items/UmbraItemTooltip.h"
#include "AbilitySystem/UmbraAbilitySystemComponent.h"
#include "AbilitySystem/UmbraAttributeSet.h"
#include "Async/Async.h"

void UUmbraEquipmentMenu::NativeConstruct()
{
	Super::NativeConstruct();
	OnVisibilityChanged.AddUniqueDynamic(this, &ThisClass::HandleTooltipVisibility);
	if (!TooltipASCLifecycleHandle.IsValid()) TooltipASCLifecycleHandle = UUmbraAbilitySystemComponent::OnLifecycleChanged.AddUObject(this, &ThisClass::OnTooltipASCLifecycle);
	UnbindEquipment();
	// Binding discovery must not invalidate the saved selection against a temporary unbound snapshot.
	bObservingEquipment = false;
	RebuildSlotBindings();
	bObservingEquipment = true;
	if (APlayerController* PC = GetOwningPlayer()) PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &ThisClass::HandlePawnChanged);
	if (!LifecycleHandle.IsValid()) LifecycleHandle = UUmbraAbilitySystemComponent::OnLifecycleChanged.AddUObject(this, &ThisClass::OnASCLifecycle);
	BindEquipment();
}

void UUmbraEquipmentMenu::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer()) PC->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePawnChanged);
	bObservingEquipment = false;
	UnbindEquipment();
	UUmbraAbilitySystemComponent::OnLifecycleChanged.Remove(LifecycleHandle);
	LifecycleHandle.Reset();
	if (bBindEquipmentData) RefreshSlotData();
	bPageActive = false;
	ReleasePreview();
	UnbindSlots();
	ClearHoveredTooltip();
	if (CachedTooltip && CachedTooltip->IsInViewport()) CachedTooltip->RemoveFromParent();
	CachedTooltip = nullptr;
	OnVisibilityChanged.RemoveDynamic(this, &ThisClass::HandleTooltipVisibility);
	UnbindTooltipContext();
	UUmbraAbilitySystemComponent::OnLifecycleChanged.Remove(TooltipASCLifecycleHandle);
	TooltipASCLifecycleHandle.Reset();
	Super::NativeDestruct();
}

void UUmbraEquipmentMenu::UnbindSlots()
{
	ClearHoveredTooltip();
	for (UUmbraEquipmentSlotWidget* EquipmentSlot : BoundSlots)
	{
		if (!EquipmentSlot) continue;
		EquipmentSlot->OnSelectionRequested.RemoveDynamic(this, &ThisClass::HandleSelection);
		EquipmentSlot->OnUnequipRequested.RemoveDynamic(this, &ThisClass::HandleUnequip);
		EquipmentSlot->OnTooltipHoverChanged.RemoveAll(this);
		EquipmentSlot->SetToolTip(nullptr);
		EquipmentSlot->OnSlotTypeChanged.RemoveDynamic(this, &ThisClass::HandleSlotTypeChanged);
	}
	BoundSlots.Reset();
	Slots.Reset();
}

void UUmbraEquipmentMenu::RebuildSlotBindings()
{
	UnbindSlots();
	if (!WidgetTree) return;
	TArray<UWidget*> Widgets;
	WidgetTree->GetAllWidgets(Widgets);
	TSet<EUmbraEquipmentSlot> Duplicates;
	for (UWidget* Widget : Widgets)
	{
		UUmbraEquipmentSlotWidget* EquipmentSlot = Cast<UUmbraEquipmentSlotWidget>(Widget);
		if (!EquipmentSlot) continue;
		BoundSlots.Add(EquipmentSlot);
		EquipmentSlot->OnSelectionRequested.AddUniqueDynamic(this, &ThisClass::HandleSelection);
		EquipmentSlot->OnUnequipRequested.AddUniqueDynamic(this, &ThisClass::HandleUnequip);
		EquipmentSlot->OnTooltipHoverChanged.AddUObject(this, &ThisClass::HandleTooltipHover);
		EquipmentSlot->OnSlotTypeChanged.AddUniqueDynamic(this, &ThisClass::HandleSlotTypeChanged);
		const EUmbraEquipmentSlot Type = EquipmentSlot->GetSlotType();
		if (Slots.Contains(Type) || Duplicates.Contains(Type))
		{
			UE_LOG(LogUmbra, Warning, TEXT("Equipment %s: duplicate SlotType %s at %s; ambiguous slot is disabled for data lookup."),
				*GetName(), *UEnum::GetValueAsString(Type), *EquipmentSlot->GetName());
			Slots.Remove(Type);
			Duplicates.Add(Type);
		}
		else Slots.Add(Type, EquipmentSlot);
	}
	if (Slots.Num() != 10) UE_LOG(LogUmbra, Warning, TEXT("Equipment %s has %d/10 unique slots. Configure SlotType on each Designer instance."), *GetName(), Slots.Num());
	if (bObservingEquipment && bBindEquipmentData) RefreshSlotData();
}

void UUmbraEquipmentMenu::HandleSlotTypeChanged(UUmbraEquipmentSlotWidget* EquipmentSlot)
{
	RebuildSlotBindings();
}

UUmbraEquipmentSlotWidget* UUmbraEquipmentMenu::GetEquipmentSlot(EUmbraEquipmentSlot Type) const
{
	const TObjectPtr<UUmbraEquipmentSlotWidget>* Found = Slots.Find(Type);
	return Found ? Found->Get() : nullptr;
}

bool UUmbraEquipmentMenu::SetSlotItem(EUmbraEquipmentSlot Type, const FUmbraEquipmentItemDisplay& Item)
{
	// Live snapshots own this view; manual preview data must not masquerade as equipped items.
	if (bObservingEquipment && bBindEquipmentData) return false;
	if (UUmbraEquipmentSlotWidget* EquipmentSlot = GetEquipmentSlot(Type))
	{
		EquipmentSlot->SetItem(Item);
		return true;
	}
	return false;
}

void UUmbraEquipmentMenu::HandleSelection(UUmbraEquipmentSlotWidget* Selected)
{
	SelectedInstanceId = Selected ? Selected->GetItemDisplay().InstanceId : FGuid();
	for (UUmbraEquipmentSlotWidget* EquipmentSlot : BoundSlots) if (EquipmentSlot) EquipmentSlot->SetSelected(EquipmentSlot == Selected);
}

void UUmbraEquipmentMenu::HandleUnequip(UUmbraEquipmentSlotWidget* EquipmentSlot)
{
	RequestUnequipSlot(EquipmentSlot);
}

EUmbraUnequipResult UUmbraEquipmentMenu::RequestUnequipSlot(UUmbraEquipmentSlotWidget* EquipmentSlot)
{
	EUmbraUnequipResult Result = EUmbraUnequipResult::NotReady;
	LastUnequipTransferResult = EUmbraTransferResult::NotReady;
	bool bTransferAttempted = false;
	FUmbraEquippedItem RemovedItem;
	if (!IsValid(EquipmentSlot) || !BoundSlots.Contains(EquipmentSlot) || GetEquipmentSlot(EquipmentSlot->GetSlotType()) != EquipmentSlot)
		Result = EUmbraUnequipResult::InvalidSlot;
	else if (!bObservingEquipment || !bBindEquipmentData || !bPageActive || !bEquipmentDataReady || !BoundEquipment.IsValid()
		|| BoundEquipment->GetOwner() != GetOwningPlayerState<AUmbraPlayerState>())
		Result = EUmbraUnequipResult::NotReady;
	else if (EquipmentSlot->IsLocked()) Result = EUmbraUnequipResult::Locked;
	else if (!EquipmentSlot->HasItem()) Result = EUmbraUnequipResult::EmptySlot;
	else
	{
		const auto Display = EquipmentSlot->GetItemDisplay();
		Result = EUmbraUnequipResult::StaleInstance;
		if (Display.bFromEquipmentSnapshot)
		{
			FUmbraInventoryItem Returned;
			bTransferAttempted = true;
			LastUnequipTransferResult = GetOwningPlayerState<AUmbraPlayerState>()->UnequipToInventory(
				EquipmentSlot->GetSlotType(), Display.InstanceId, Returned);
			switch (LastUnequipTransferResult)
			{
			case EUmbraTransferResult::Success: Result = EUmbraUnequipResult::Success; break;
			case EUmbraTransferResult::AppliedInvalid: Result = EUmbraUnequipResult::AppliedInvalid; break;
			case EUmbraTransferResult::AuthorityRequired: Result = EUmbraUnequipResult::AuthorityRequired; break;
			case EUmbraTransferResult::NotReady: Result = EUmbraUnequipResult::NotReady; break;
			case EUmbraTransferResult::InvalidSlot: Result = EUmbraUnequipResult::InvalidSlot; break;
			case EUmbraTransferResult::InvalidIdentity:
			case EUmbraTransferResult::NotFound: Result = EUmbraUnequipResult::StaleInstance; break;
			default: Result = EUmbraUnequipResult::Failed; break;
			}
			if (Result == EUmbraUnequipResult::Success || Result == EUmbraUnequipResult::AppliedInvalid)
			{
				RemovedItem.Slot = EquipmentSlot->GetSlotType();
				RemovedItem.Definition = Returned.Definition;
				RemovedItem.InstanceId = Returned.InstanceId;
				RemovedItem.bRequirementsMet = Display.bRequirementsMet;
			}
		}
	}
	switch (Result)
	{
	case EUmbraUnequipResult::Success: LastUnequipMessage = NSLOCTEXT("UmbraEquipment", "Returned", "Equipment returned to inventory."); break;
	case EUmbraUnequipResult::AuthorityRequired: LastUnequipMessage = NSLOCTEXT("UmbraEquipment", "Authority", "Unequipping requires server authority."); break;
	case EUmbraUnequipResult::EmptySlot: LastUnequipMessage = NSLOCTEXT("UmbraEquipment", "Empty", "This slot is empty."); break;
	case EUmbraUnequipResult::StaleInstance: LastUnequipMessage = NSLOCTEXT("UmbraEquipment", "Stale", "Equipment changed. Refresh and try again."); break;
	case EUmbraUnequipResult::Locked: LastUnequipMessage = NSLOCTEXT("UmbraEquipment", "Locked", "This slot is locked."); break;
	case EUmbraUnequipResult::InvalidSlot: LastUnequipMessage = NSLOCTEXT("UmbraEquipment", "InvalidSlot", "This equipment slot is unavailable."); break;
	case EUmbraUnequipResult::AppliedInvalid: LastUnequipMessage = NSLOCTEXT("UmbraEquipment", "AppliedInvalid", "Equipment removed, but combat stats are invalid."); break;
	default: LastUnequipMessage = NSLOCTEXT("UmbraEquipment", "NotReady", "Equipment cannot be removed right now."); break;
	}
	if (bTransferAttempted && Result != EUmbraUnequipResult::Success)
		LastUnequipMessage = UmbraTransferFeedback::Message(LastUnequipTransferResult);
	// Snapshot notifications update all views; never optimistically clear a slot on a rejected request.
	BP_UnequipFinished(Result, RemovedItem, LastUnequipMessage);
	return Result;
}

bool UUmbraEquipmentMenu::CreatePreview()
{
	if (IsValid(PreviewActor) && IsValid(PreviewComponent)) return true;
	if (!GetWorld() || !Cast<ACharacter>(GetOwningPlayerPawn())) return false;
	// The Designer brush is the default material source, so it also previews the authored RT.
	UMaterialInterface* Material = PreviewMaterial;
	if (!Material && CharacterPreview) Material = Cast<UMaterialInterface>(CharacterPreview->GetBrush().GetResourceObject());
	if (!PreviewActorClass || !Material || !CharacterPreview || PreviewActorClass->IsChildOf(APawn::StaticClass()))
	{
		UE_LOG(LogUmbra, Warning, TEXT("Equipment %s needs an independent Actor preview BP, PreviewMaterial, and CharacterPreview Image."), *GetName());
		return false;
	}
	FActorSpawnParameters Params;
	Params.Owner = GetOwningPlayer();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	PreviewActor = GetWorld()->SpawnActor<AActor>(PreviewActorClass, PreviewActorTransform, Params);
	if (!PreviewActor) return false;
	TInlineComponentArray<USkeletalMeshComponent*> Meshes(PreviewActor);
	TInlineComponentArray<USceneCaptureComponent2D*> Captures(PreviewActor);
	// Never let a second preview instance render into the portrait BP's default shared RT.
	for (USceneCaptureComponent2D* Capture : Captures)
	{
		Capture->bCaptureEveryFrame = false;
		Capture->bCaptureOnMovement = false;
		if (!PreviewSettings.bUseBlueprintConfiguration) Capture->TextureTarget = nullptr;
		Capture->Deactivate();
	}
	if (Meshes.Num() != 1 || Captures.Num() != 1)
	{
		UE_LOG(LogUmbra, Warning, TEXT("Preview %s requires one base skeletal mesh and one capture. Add future equipment meshes through RefreshEquipmentVisuals."), *GetNameSafe(PreviewActor));
		ReleasePreview();
		return false;
	}
	PreviewComponent = PreviewActor->FindComponentByClass<UUmbraCharacterPreviewComponent>();
	if (!PreviewComponent)
	{
		PreviewComponent = NewObject<UUmbraCharacterPreviewComponent>(PreviewActor);
		PreviewActor->AddInstanceComponent(PreviewComponent);
		PreviewComponent->RegisterComponent();
	}
	if (!PreviewComponent->InitializePreview(Cast<ACharacter>(GetOwningPlayerPawn()), Meshes[0], Captures[0], PreviewSettings))
	{
		ReleasePreview();
		return false;
	}
	PreviewMID = UMaterialInstanceDynamic::Create(Material, this);
	PreviewMID->SetTextureParameterValue(TextureParameterName, PreviewComponent->GetRenderTarget());
	CharacterPreview->SetBrushFromMaterial(PreviewMID);
	return true;
}

void UUmbraEquipmentMenu::ReleasePreview()
{
	if (IsValid(PreviewComponent)) PreviewComponent->SetPreviewActive(false);
	if (IsValid(PreviewActor)) PreviewActor->Destroy();
	PreviewComponent = nullptr;
	PreviewActor = nullptr;
	// Keep the authored brush available for reopening/reconstructing the page.
	if (CharacterPreview && PreviewMID) CharacterPreview->SetBrushFromMaterial(PreviewMID->Parent);
	PreviewMID = nullptr;
}

void UUmbraEquipmentMenu::SetPageActive(bool bActive)
{
	if (IsDesignTime()) return;
	if (!bActive) ClearHoveredTooltip();
	if (bActive && bObservingEquipment) BindEquipment();
	if (bPageActive == bActive && (!bActive || IsValid(PreviewComponent))) return;
	bPageActive = bActive;
	if (bActive && CreatePreview())
	{
		PreviewComponent->SetSourceCharacter(Cast<ACharacter>(GetOwningPlayerPawn()));
		PreviewComponent->SetPreviewActive(true);
	}
	else if (PreviewComponent) PreviewComponent->SetPreviewActive(false);
}

void UUmbraEquipmentMenu::HandlePawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	NotifyPlayerContextChanged();
	ReleasePreview();
	if (bPageActive && CreatePreview()) PreviewComponent->SetPreviewActive(true);
}

void UUmbraEquipmentMenu::RefreshAppearance()
{
	if (PreviewComponent) PreviewComponent->RefreshAppearance();
}

void UUmbraEquipmentMenu::RefreshEquipmentVisuals()
{
	if (PreviewComponent) PreviewComponent->RefreshEquipmentVisuals();
}

void UUmbraEquipmentMenu::NotifyPlayerContextChanged()
{
	if (bObservingEquipment) BindEquipment();
}

void UUmbraEquipmentMenu::BindEquipment()
{
	if (!bObservingEquipment || !bBindEquipmentData) return;
	auto* PS = GetOwningPlayerState<AUmbraPlayerState>();
	auto* ASC = PS ? PS->GetUmbraAbilitySystemComponent() : nullptr;
	auto* Equipment = PS ? PS->FindComponentByClass<UUmbraEquipmentComponent>() : nullptr;
	if (!ASC || !ASC->IsActorInfoReady() || !IsValid(Equipment))
	{
		UnbindEquipment(); RefreshSlotData(); return;
	}
	if (BoundEquipment.Get() != Equipment || BoundASC.Get() != ASC)
	{
		if (SelectionEquipment.Get() != Equipment) SelectedInstanceId.Invalidate();
		SelectionEquipment = Equipment;
		UnbindEquipment();
		BoundEquipment = Equipment;
		BoundASC = ASC;
		Equipment->OnEquipmentChanged.AddUniqueDynamic(this, &ThisClass::OnEquipmentChanged);
	}
	RefreshSlotData();
}

void UUmbraEquipmentMenu::UnbindEquipment()
{
	ClearHoveredTooltip();
	UnbindTooltipContext();
	if (BoundEquipment.IsValid()) BoundEquipment->OnEquipmentChanged.RemoveDynamic(this, &ThisClass::OnEquipmentChanged);
	BoundEquipment.Reset(); BoundASC.Reset();
}

void UUmbraEquipmentMenu::OnASCLifecycle(UUmbraAbilitySystemComponent* ASC, bool bReady)
{
	if (!bObservingEquipment || !bBindEquipmentData) return;
	if (!bReady && BoundASC.Get() == ASC) { UnbindEquipment(); RefreshSlotData(); return; }
	const auto* PS = GetOwningPlayerState<AUmbraPlayerState>();
	if (bReady && PS && ASC == PS->GetUmbraAbilitySystemComponent()) BindEquipment();
}

void UUmbraEquipmentMenu::OnEquipmentChanged(const FUmbraEquipmentSnapshot&)
{
	RefreshSlotData();
}

void UUmbraEquipmentMenu::RefreshSlotData()
{
	if (!bBindEquipmentData) return;
	const auto Snapshot = BoundEquipment.IsValid() ? BoundEquipment->GetSnapshot() : FUmbraEquipmentSnapshot();
	bEquipmentDataReady = BoundASC.IsValid() && BoundASC->IsActorInfoReady() && Snapshot.bValid;
	for (const auto& Entry : Snapshot.Items)
		if (!IsValid(Entry.Definition) || !Entry.InstanceId.IsValid()) bEquipmentDataReady = false;
	if (bObservingEquipment && (!bEquipmentDataReady || !Snapshot.Items.ContainsByPredicate(
		[this](const auto& Entry) { return Entry.InstanceId == SelectedInstanceId; }))) SelectedInstanceId.Invalidate();
	// Project by Slot enum, not array order or definition pointer: two rings may share one asset.
	for (UUmbraEquipmentSlotWidget* EquipmentSlot : BoundSlots)
	{
		if (!IsValid(EquipmentSlot)) continue;
		const auto* Entry = bEquipmentDataReady && GetEquipmentSlot(EquipmentSlot->GetSlotType()) == EquipmentSlot
			? Snapshot.Items.FindByPredicate([EquipmentSlot](const FUmbraEquippedItem& Item) { return Item.Slot == EquipmentSlot->GetSlotType(); }) : nullptr;
		if (!Entry || !IsValid(Entry->Definition)) { EquipmentSlot->ClearItem(); continue; }
		auto Display = FUmbraEquipmentItemDisplay::FromDefinition(Entry->Definition, Entry->InstanceId);
		Display.bFromEquipmentSnapshot = true;
		Display.bRequirementsMet = Entry->bRequirementsMet;
		EquipmentSlot->SetItem(Display);
		EquipmentSlot->SetSelected(Display.InstanceId == SelectedInstanceId);
	}
	BindTooltipContext();
	RefreshHoveredTooltip();
	QueueTooltipRefresh();
	BP_EquipmentDataChanged(bEquipmentDataReady);
}

