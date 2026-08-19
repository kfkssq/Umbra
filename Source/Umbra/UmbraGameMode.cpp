// Copyright Epic Games, Inc. All Rights Reserved.

#include "UmbraGameMode.h"

#include "Player/UmbraPlayerState.h"

AUmbraGameMode::AUmbraGameMode()
{
	PlayerStateClass = AUmbraPlayerState::StaticClass();
}
