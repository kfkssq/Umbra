#pragma once
#include "CoreMinimal.h"

namespace UmbraWorldLabelLayout
{
	struct FState
	{
		FVector2D Anchor = FVector2D::ZeroVector;
		FVector2D TargetPosition = FVector2D::ZeroVector;
		FVector2D RenderPosition = FVector2D::ZeroVector;
		bool bVisible = false;
	};
	struct FEntry
	{
		FVector2D DesiredMin;
		FVector2D Size;
		FState* State = nullptr;
		bool bPinned = false;
	};
	// Entries retain identity order; all coordinates are viewport Slate units.
	void Update(TArray<FEntry>& Entries, const FVector2D& ViewSize, float Gap,
		float DeltaTime, float Speed, float SwitchThreshold);
	/** Slate viewport units. Finds the nearest non-overlapping candidate, without moving world actors. */
	bool Place(const FVector2D& DesiredMin, const FVector2D& Size, const FVector2D& ViewSize,
		const TArray<FBox2D>& Occupied, float Gap, FBox2D& OutRect);
}
