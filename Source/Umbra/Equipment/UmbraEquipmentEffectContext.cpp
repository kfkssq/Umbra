#include "Equipment/UmbraEquipmentEffectContext.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

const FUmbraEquipmentEffectContext* FUmbraEquipmentEffectContext::Get(const FGameplayEffectContextHandle& Handle)
{
	return Handle.Get() && Handle.Get()->GetScriptStruct() == StaticStruct() ? static_cast<const FUmbraEquipmentEffectContext*>(Handle.Get()) : nullptr;
}

FUmbraEquipmentEffectContext* FUmbraEquipmentEffectContext::Duplicate() const
{
	auto* Copy = new FUmbraEquipmentEffectContext(*this);
	if (GetHitResult()) Copy->AddHitResult(*GetHitResult(), true);
	return Copy;
}

bool FUmbraEquipmentEffectContext::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bool BaseSuccess = false;
	const bool bMapped = FGameplayEffectContext::NetSerialize(Ar, Map, BaseSuccess);
	// Name strings keep tag queries self-contained. No UObject authoring references are sent.
	TArray<uint8> Bytes;
	if (Ar.IsSaving())
	{
		if (DamageBonuses.Num() > 64) { bOutSuccess = false; return false; }
		FMemoryWriter Writer(Bytes);
		FObjectAndNameAsStringProxyArchive Payload(Writer, false);
		int32 Count = DamageBonuses.Num(); Payload << Count;
		for (auto Bonus : DamageBonuses)
		{
			Bonus.Magnitude = FScalableFloat(Bonus.Magnitude.GetValueAtLevel(1));
			FUmbraDamageBonus::StaticStruct()->SerializeItem(Payload, &Bonus, nullptr);
		}
	}
	uint32 Size = Bytes.Num(); Ar.SerializeIntPacked(Size);
	if (Size > 65536) { bOutSuccess = false; return false; }
	if (Ar.IsLoading()) Bytes.SetNum(Size);
	Ar.Serialize(Bytes.GetData(), Size);
	if (Ar.IsLoading() && !Ar.IsError())
	{
		FMemoryReader Reader(Bytes);
		FObjectAndNameAsStringProxyArchive Payload(Reader, false);
		int32 Count = 0; Payload << Count;
		if (Count < 0 || Count > 64) { bOutSuccess = false; return false; }
		DamageBonuses.SetNum(Count);
		for (auto& Bonus : DamageBonuses) FUmbraDamageBonus::StaticStruct()->SerializeItem(Payload, &Bonus, nullptr);
		if (Reader.IsError()) { bOutSuccess = false; return false; }
	}
	bOutSuccess = BaseSuccess && !Ar.IsError();
	return bMapped;
}
