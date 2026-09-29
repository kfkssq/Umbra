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

void UUmbraEquipmentMenu::NativeConstruct()
{
	Super::NativeConstruct();
	RebuildSlotBindings();
	if (APlayerController* PC = GetOwningPlayer()) PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &ThisClass::HandlePawnChanged);
}

void UUmbraEquipmentMenu::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer()) PC->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePawnChanged);
	bPageActive = false;
	ReleasePreview();
	UnbindSlots();
	Super::NativeDestruct();
}

void UUmbraEquipmentMenu::UnbindSlots()
{
	for (UUmbraEquipmentSlotWidget* EquipmentSlot : BoundSlots)
	{
		if (!EquipmentSlot) continue;
		EquipmentSlot->OnSelectionRequested.RemoveDynamic(this, &ThisClass::HandleSelection);
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
	if (UUmbraEquipmentSlotWidget* EquipmentSlot = GetEquipmentSlot(Type))
	{
		EquipmentSlot->SetItem(Item);
		return true;
	}
	return false;
}

void UUmbraEquipmentMenu::HandleSelection(UUmbraEquipmentSlotWidget* Selected)
{
	for (UUmbraEquipmentSlotWidget* EquipmentSlot : BoundSlots) if (EquipmentSlot) EquipmentSlot->SetSelected(EquipmentSlot == Selected);
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

