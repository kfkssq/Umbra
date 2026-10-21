#include "UmbraPlayerController.h"
#include "UI/Items/UmbraWorldLabelLayout.h"
#include "UI/Items/UmbraWorldItemLabel.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "EngineUtils.h"

void AUmbraPlayerController::OnWorldItemLabelDestroyed(AActor* Actor)
{
 auto* Item = Cast<AUmbraWorldItem>(Actor);
 WorldItemLabelStates.Remove(Item);
 WorldItemLabelRects.Remove(Item);
 if (HoveredWorldItem.Get() == Item)
 {
  Item->SetHighlighted(false);
  HoveredWorldItem.Reset();
 }
}

void AUmbraPlayerController::LayoutWorldItemLabels(float DeltaTime)
{
 // Read hover from the previous displayed rectangles before replacing the hit cache.
 auto* PinnedItem = GetWorldItemLabelUnderCursor();
 WorldItemLabelRects.Reset();
 if (!IsLocalPlayerController()) return;
 TArray<AUmbraWorldItem*> Items;
 for (TActorIterator<AUmbraWorldItem> It(GetWorld()); It; ++It)
  if (!It->IsActorBeingDestroyed() && !It->IsHidden() && It->GetItemInstance().IsValid()) Items.Add(*It);
 for (auto It = WorldItemLabelStates.CreateIterator(); It; ++It)
 {
  auto* Item = It.Key().Get();
  if (!Items.Contains(Item))
  {
   if (Item) Item->OnDestroyed.RemoveDynamic(this, &ThisClass::OnWorldItemLabelDestroyed);
   if (HoveredWorldItem.Get() == Item)
   {
    if (Item) Item->SetHighlighted(false);
    HoveredWorldItem.Reset();
   }
   It.RemoveCurrent();
  }
 }
 Items.Sort([](const AUmbraWorldItem& A, const AUmbraWorldItem& B)
  { return A.GetItemInstance().InstanceId.ToString() < B.GetItemInstance().InstanceId.ToString(); });
 // Populate before taking map value pointers (insertion can relocate values).
 for (auto* Item : Items)
 {
  WorldItemLabelStates.FindOrAdd(Item);
  Item->OnDestroyed.AddUniqueDynamic(this, &ThisClass::OnWorldItemLabelDestroyed);
 }
 const float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
 const FVector2D ViewSize = Scale > 0 ? UWidgetLayoutLibrary::GetViewportSize(this) / Scale : FVector2D::ZeroVector;
 TArray<UmbraWorldLabelLayout::FEntry> Entries;
 TArray<AUmbraWorldItem*> VisibleItems;
 for (auto* Item : Items)
 {
  auto& State = WorldItemLabelStates.FindChecked(Item);
  auto* Component = Item->GetLabelComponent();
  auto* Widget = Component ? Cast<UUmbraWorldItemLabel>(Component->GetUserWidgetObject()) : nullptr;
  if (!Widget) { State.bVisible = false; continue; }
  FVector2D Screen = FVector2D::ZeroVector, Anchor = FVector2D::ZeroVector;
  const bool bProjected = Scale > 0 && ProjectWorldLocationToScreen(Component->GetComponentLocation(), Screen);
  if (bProjected)
   USlateBlueprintLibrary::ScreenToViewport(this, FVector2D(FMath::RoundToDouble(Screen.X), FMath::RoundToDouble(Screen.Y)), Anchor);
  if (!bProjected || Anchor.X < 0 || Anchor.Y < 0 || Anchor.X > ViewSize.X || Anchor.Y > ViewSize.Y)
  {
   State.bVisible = false; Widget->SetRenderOpacity(0.f); continue;
  }
  // DesiredSize excludes render transforms. Never measure a translated cached geometry.
  Widget->ForceLayoutPrepass();
  const FVector2D Size = Widget->GetDesiredSize();
  Entries.Add({Anchor - Size * Component->GetPivot(), Size, &State, Item == PinnedItem});
  VisibleItems.Add(Item);
 }
 UmbraWorldLabelLayout::Update(Entries, ViewSize,
  FMath::IsFinite(WorldItemLabelGap) ? FMath::Clamp(WorldItemLabelGap, 0.f, 32.f) : 4.f,
  DeltaTime,
  FMath::IsFinite(WorldItemLabelMoveSpeed) ? FMath::Clamp(WorldItemLabelMoveSpeed, 1.f, 60.f) : 16.f,
  FMath::IsFinite(WorldItemLabelSwitchThreshold) ? FMath::Clamp(WorldItemLabelSwitchThreshold, 0.f, 64.f) : 12.f);
 for (int32 I = 0; I < Entries.Num(); ++I)
 {
  const auto& E = Entries[I];
  auto* Widget = VisibleItems[I]->GetLabelComponent()->GetUserWidgetObject();
  Widget->SetRenderOpacity(E.State->bVisible ? 1.f : 0.f);
  if (!E.State->bVisible) continue;
  Widget->SetRenderTranslation(E.State->RenderPosition - E.DesiredMin);
  WorldItemLabelRects.Add(VisibleItems[I], FBox2D(E.State->RenderPosition, E.State->RenderPosition + E.Size));
 }
}
AUmbraWorldItem* AUmbraPlayerController::GetWorldItemLabelUnderCursor() const
{
	if (!IsLocalPlayerController() || bCharacterMenuOpen || IsPointerOverAttributeDebugPanel()) return nullptr;
	float X, Y;
	if (!GetMousePosition(X, Y)) return nullptr;
	FVector2D Mouse;
	USlateBlueprintLibrary::ScreenToViewport(this, FVector2D(X, Y), Mouse);
	return FindWorldItemLabelAt(Mouse);
}

AUmbraWorldItem* AUmbraPlayerController::FindWorldItemLabelAt(const FVector2D& Position) const
{
	for (const auto& Entry : WorldItemLabelRects)
		if (Entry.Key.IsValid() && !Entry.Key->IsActorBeingDestroyed() && !Entry.Key->IsHidden()
			&& Entry.Value.IsInsideOrOn(Position)) return Entry.Key.Get();
	return nullptr;
}
