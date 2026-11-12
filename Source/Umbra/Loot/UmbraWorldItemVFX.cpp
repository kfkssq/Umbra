#include "Loot/UmbraWorldItem.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

void AUmbraWorldItem::RefreshLootVFX()
{
	if (!HasActorBegunPlay()) return;
	const auto* Style = LootVFXStyles.Find(ItemInstance.GetRarity());
	if (GetNetMode() == NM_DedicatedServer || bPickedUp || !ItemInstance.IsValid()
		|| IsActorBeingDestroyed() || !Style || !Style->System)
	{
		LootVFX->DeactivateImmediate();
		LootVFX->SetAsset(nullptr);
		return;
	}
	const bool bChanged = LootVFX->GetAsset() != Style->System;
	if (bChanged)
	{
		LootVFX->DeactivateImmediate();
		LootVFX->SetAsset(Style->System);
	}
	const FLinearColor Color = Style->bOverrideColor ? Style->Color : ItemInstance.GetRarityColor();
	for (FName Parameter : LootVFXColorParameters)
	{
		const FNiagaraVariable Variable(FNiagaraTypeDefinition::GetColorDef(), Parameter);
		const auto& Defaults = Style->System->GetExposedParameters();
		if (Defaults.IndexOf(Variable) == INDEX_NONE) continue;
		const FLinearColor Authored = Defaults.GetParameterValue<FLinearColor>(Variable);
		// Preserve each layer's authored HDR intensity and opacity when replacing its hue.
		const float Intensity = FMath::Max3(Authored.R, Authored.G, Authored.B);
		LootVFX->SetVariableLinearColor(Parameter, FLinearColor(Color.R * Intensity,
			Color.G * Intensity, Color.B * Intensity, Color.A * Authored.A));
	}
	// Replication, mesh loads and highlight refreshes must not restart a running loop.
	if (bChanged || !LootVFX->IsActive()) LootVFX->Activate(true);
}
