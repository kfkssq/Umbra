#include "UI/Items/UmbraWorldLabelLayout.h"

bool UmbraWorldLabelLayout::Place(const FVector2D& DesiredMin, const FVector2D& Size,
	const FVector2D& ViewSize, const TArray<FBox2D>& Occupied, float Gap, FBox2D& OutRect)
{
	if (Size.X <= 0 || Size.Y <= 0 || Size.X > ViewSize.X || Size.Y > ViewSize.Y) return false;
	const FVector2D MaxMin = ViewSize - Size;
	const FVector2D Base(FMath::Clamp(DesiredMin.X, 0., MaxMin.X), FMath::Clamp(DesiredMin.Y, 0., MaxMin.Y));
	TArray<FVector2D> Candidates{Base};
	for (const auto& Rect : Occupied)
	{
		Candidates.Add(FVector2D(Base.X, Rect.Min.Y - Size.Y - Gap));
		Candidates.Add(FVector2D(Base.X, Rect.Max.Y + Gap));
		Candidates.Add(FVector2D(Rect.Min.X - Size.X - Gap, Base.Y));
		Candidates.Add(FVector2D(Rect.Max.X + Gap, Base.Y));
	}
	// Quantized distance ties preserve insertion order (an epsilon comparator is not transitive).
	Candidates.StableSort([&](const auto& A, const auto& B)
		{ return FMath::RoundToDouble((A - Base).Size() * 100.) < FMath::RoundToDouble((B - Base).Size() * 100.); });
	for (const FVector2D& Point : Candidates)
	{
		if (Point.X < 0 || Point.Y < 0 || Point.X > MaxMin.X || Point.Y > MaxMin.Y) continue;
		const FBox2D Rect(Point, Point + Size);
		if (!Occupied.ContainsByPredicate([&](const FBox2D& Other)
			{ return Rect.Min.X < Other.Max.X + Gap && Rect.Max.X + Gap > Other.Min.X
				&& Rect.Min.Y < Other.Max.Y + Gap && Rect.Max.Y + Gap > Other.Min.Y; }))
		{
			OutRect = Rect;
			return true;
		}
	}
	return false;
}

namespace
{
 bool Conflicts(const FBox2D& A, const FBox2D& B, float Gap)
 {
  const double Epsilon = FMath::Min(0.01, double(Gap) * 0.5);
  return A.Min.X < B.Max.X + Gap - Epsilon && A.Max.X + Gap > B.Min.X + Epsilon
   && A.Min.Y < B.Max.Y + Gap - Epsilon && A.Max.Y + Gap > B.Min.Y + Epsilon;
 }
 bool Fits(const FBox2D& R, const FVector2D& View, const TArray<FBox2D>& Occupied, float Gap)
 {
  return R.GetSize().X > 0 && R.GetSize().Y > 0 && R.Min.X >= 0 && R.Min.Y >= 0
   && R.Max.X <= View.X && R.Max.Y <= View.Y
   && !Occupied.ContainsByPredicate([&](const FBox2D& Other) { return Conflicts(R, Other, Gap); });
 }
}

void UmbraWorldLabelLayout::Update(TArray<FEntry>& Entries, const FVector2D& ViewSize,
 float Gap, float DeltaTime, float Speed, float SwitchThreshold)
{
 TArray<FBox2D> Occupied;
 TArray<bool> Assigned;
 Assigned.Init(false, Entries.Num());
 TArray<bool> WasVisible;
 for (const auto& E : Entries) WasVisible.Add(E.State->bVisible);
 // Reserve history before newcomers. Hover gets first refusal, without changing identity order.
 for (int32 Pass = 0; Pass < 2; ++Pass)
 for (int32 I = 0; I < Entries.Num(); ++I)
 {
  auto& E = Entries[I]; auto& S = *E.State;
  if (!WasVisible[I] || E.bPinned != (Pass == 0)) continue;
  const FVector2D Previous = E.bPinned ? S.RenderPosition : S.TargetPosition + (E.DesiredMin - S.Anchor);
  const FBox2D R(Previous, Previous + E.Size);
  if (Fits(R, ViewSize, Occupied, Gap))
  {
   S.TargetPosition = Previous; Assigned[I] = true; Occupied.Add(R);
  }
 }
 for (int32 I = 0; I < Entries.Num(); ++I)
 {
  auto& E = Entries[I]; auto& S = *E.State;
  if (!Assigned[I])
  {
   FBox2D Best;
   if (Place(E.DesiredMin, E.Size, ViewSize, Occupied, Gap, Best))
   {
    // Prefer a nearby repair around the previous target unless the anchor candidate is
    // substantially better. Invalid history is never retained through hysteresis.
    FBox2D Repair;
    const FVector2D Previous = S.TargetPosition + (E.DesiredMin - S.Anchor);
    if (WasVisible[I] && Place(Previous, E.Size, ViewSize, Occupied, Gap, Repair)
     && FVector2D::Distance(Repair.Min, E.DesiredMin) <= FVector2D::Distance(Best.Min, E.DesiredMin) + SwitchThreshold)
     Best = Repair;
    S.TargetPosition = Best.Min; Assigned[I] = true; Occupied.Add(Best);
   }
  }
 }
 const double Alpha = 1. - FMath::Exp(-FMath::Max(0.f, Speed) * FMath::Max(0.f, DeltaTime));
 TArray<FVector2D> Render;
 TArray<FBox2D> Sweeps;
 bool bSnap = false;
 for (int32 I = 0; I < Entries.Num(); ++I)
 {
  const auto& E = Entries[I]; const auto& S = *E.State;
  FVector2D P = WasVisible[I] ? FMath::Lerp(S.RenderPosition, S.TargetPosition, Alpha) : S.TargetPosition;
  if (P.Equals(S.TargetPosition, 0.01)) P = S.TargetPosition;
  Render.Add(P);
  FBox2D Sweep(P, P + E.Size);
  if (WasVisible[I]) { Sweep += S.RenderPosition; Sweep += S.RenderPosition + E.Size; }
  Sweeps.Add(Sweep);
  if (!Assigned[I]) continue;
  if (!Fits(Sweep, ViewSize, {}, 0)) bSnap = true;
  for (int32 J = 0; J < I; ++J)
   if (Assigned[J] && Conflicts(Sweep, Sweeps[J], Gap)) bSnap = true;
 }
 // Snap the whole batch when swept rectangles collide: no visible intermediate crossing.
 for (int32 I = 0; I < Entries.Num(); ++I)
 {
  auto& E = Entries[I]; auto& S = *E.State;
  S.bVisible = Assigned[I]; S.Anchor = E.DesiredMin;
  if (S.bVisible) S.RenderPosition = bSnap ? S.TargetPosition : Render[I];
 }
}
