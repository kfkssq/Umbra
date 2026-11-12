#include "Items/UmbraItemDefinition.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Umbra.h"
#include "Items/UmbraItemGeneration.h"
#include "Misc/DataValidation.h"

TArray<EUmbraEquipmentSlot> UUmbraItemDefinition::GetAllowedEquipmentSlots() const
{
	if (ItemCategory == EUmbraItemCategory::Accessory)
		return {EUmbraEquipmentSlot::Amulet, EUmbraEquipmentSlot::Amulet2, EUmbraEquipmentSlot::Ring1, EUmbraEquipmentSlot::Ring2};
	return AllowedSlots;
}

bool UUmbraItemDefinition::CanEquipInSlot(EUmbraEquipmentSlot Slot) const
{
	if (!IsUmbraEquipmentSlotActive(Slot)) return false;
	return ItemCategory == EUmbraItemCategory::Accessory ? IsUmbraRelicSlot(Slot) : AllowedSlots.Contains(Slot);
}

#if WITH_EDITOR
EDataValidationResult UUmbraItemDefinition::IsDataValid(FDataValidationContext& Context) const
{
	const auto Base = Super::IsDataValid(Context);
	if (!GenerationProfile) return Base;
	FString Error;
	if (!UmbraItemGeneration::Validate(this, Error))
	{
		Context.AddError(FText::FromString(GetPathName() + TEXT(": ") + Error));
		return EDataValidationResult::Invalid;
	}
	return Base == EDataValidationResult::Invalid ? Base : EDataValidationResult::Valid;
}
#endif

void UUmbraItemDefinition::FitWorldDropPhysics()
{
	TryFitWorldDropPhysics();
}

bool UUmbraItemDefinition::TryFitWorldDropPhysics()
{
#if WITH_EDITOR
	FBox Bounds(ForceInit);
	FBox LocalBounds(ForceInit);
	const FTransform Shape(WorldDropRotation, FVector::ZeroVector, WorldDropScale);
	if (!WorldDropSkeletalMesh.IsNull())
	{
		if (const auto* Asset = WorldDropSkeletalMesh.LoadSynchronous())
		{
			LocalBounds = Asset->GetBounds().GetBox();
			Bounds = Asset->GetBounds().GetBox().TransformBy(Shape);
			Bounds = Bounds.ShiftBy(WorldDropOffset + FVector(0, 0, -Bounds.Min.Z));
		}
	}
	else if (const auto* Asset = WorldDropMesh.LoadSynchronous())
	{
		LocalBounds = Asset->GetBoundingBox();
		Bounds = Asset->GetBoundingBox().TransformBy(FTransform(WorldDropRotation, WorldDropOffset, WorldDropScale));
	}
	const FBox ScaledBounds = LocalBounds.IsValid
		? LocalBounds.TransformBy(FTransform(FQuat::Identity, FVector::ZeroVector, WorldDropScale)) : LocalBounds;
	if (!Bounds.IsValid || !ScaledBounds.IsValid || Bounds.Min.ContainsNaN() || Bounds.Max.ContainsNaN()
		|| ScaledBounds.GetExtent().GetMax() > 200 || Bounds.GetCenter().GetAbsMax() > 500)
	{
		UE_LOG(LogUmbra, Warning, TEXT("Cannot fit drop physics for %s: missing mesh or bounds outside supported limits."), *GetName());
		return false;
	}
	Modify();
	WorldDropPhysicsExtent = ScaledBounds.GetExtent().ComponentMax(FVector(1));
	WorldDropPhysicsCenter = Bounds.GetCenter();
	WorldDropPhysicsRotation = WorldDropRotation;
	MarkPackageDirty();
	return true;
#else
	return false;
#endif
}
