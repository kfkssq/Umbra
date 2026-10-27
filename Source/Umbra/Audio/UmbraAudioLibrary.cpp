#include "Audio/UmbraAudioLibrary.h"

FUmbraAudioContext FUmbraAudioContext::FromItem(const UUmbraItemDefinition* Item)
{
	FUmbraAudioContext Context;
	if (IsValid(Item))
	{
		Context.Weapon = Item->WeaponType;
		Context.bHasRarity = true;
		Context.Rarity = Item->Rarity;
	}
	return Context;
}

int32 FUmbraAudioEntry::MatchScore(EUmbraAudioEvent InEvent, const FUmbraAudioContext& Context) const
{
	if (Event != InEvent) return INDEX_NONE;
	int32 Score = 0;
	if (Match.Weapon != EUmbraWeaponType::Unknown)
	{
		if (Match.Weapon != Context.Weapon) return INDEX_NONE;
		++Score;
	}
	if (Match.bHasRarity)
	{
		if (!Context.bHasRarity || Match.Rarity != Context.Rarity) return INDEX_NONE;
		++Score;
	}
	if (Match.Surface != SurfaceType_Default)
	{
		if (Match.Surface != Context.Surface) return INDEX_NONE;
		++Score;
	}
	if (!Match.CharacterType.IsNone())
	{
		if (Match.CharacterType != Context.CharacterType) return INDEX_NONE;
		++Score;
	}
	return Score;
}

const FUmbraAudioEntry* UUmbraAudioLibrary::Find(EUmbraAudioEvent Event, const FUmbraAudioContext& Context) const
{
	const FUmbraAudioEntry* Best = nullptr;
	int32 BestScore = INDEX_NONE;
	for (const FUmbraAudioEntry& Entry : Events)
	{
		const int32 Score = Entry.MatchScore(Event, Context);
		if (Score > BestScore) { Best = &Entry; BestScore = Score; }
	}
	return Best;
}
