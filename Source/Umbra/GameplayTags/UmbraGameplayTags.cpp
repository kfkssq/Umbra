// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameplayTags/UmbraGameplayTags.h"

namespace UmbraGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input, "Input", "Root tag for ability input routing.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability, "Ability", "Root tag for gameplay abilities.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State, "State", "Root tag for gameplay states.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event, "Event", "Root tag for gameplay events.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage, "Damage", "Root tag for damage classification.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Attack_Primary, "Input.Attack.Primary", "Primary attack input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack_Basic, "Ability.Attack.Basic", "Basic attack ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attacking, "State.Attacking", "Actor is performing an attack.");
}
