// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameplayTags/UmbraGameplayTags.h"

namespace UmbraGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_ResultCritical, "Damage.ResultCritical", "Per-hit execution result, consumed by IncomingDamage settlement.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input, "Input", "Root tag for ability input routing.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability, "Ability", "Root tag for gameplay abilities.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State, "State", "Root tag for gameplay states.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event, "Event", "Root tag for gameplay events.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage, "Damage", "Root tag for damage classification.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Type, "Damage.Type", "SetByCaller: 0 Physical, 1 Magical. Required.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_AbilityPowerCoefficient, "Damage.AbilityPowerCoefficient", "SetByCaller ability power scaling.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_AttackPowerCoefficient, "Damage.AttackPowerCoefficient", "SetByCaller attack power scaling.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Attack_Primary, "Input.Attack.Primary", "Primary attack input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack_Basic, "Ability.Attack.Basic", "Basic attack ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack_EnemyBasic, "Ability.Attack.EnemyBasic", "Enemy basic attack ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_HitReact, "Ability.HitReact", "Cosmetic hit-reaction ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attacking, "State.Attacking", "Actor is performing an attack.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_HitReact, "State.HitReact", "Actor is playing a hit reaction.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "Actor is dead and cannot react to hits.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_SuperArmor, "State.SuperArmor", "Actor ignores ordinary hit reactions.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Attack_ComboInput, "Event.Attack.ComboInput", "Queues the next basic-attack combo step.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Attack_HitWindow, "Event.Attack.HitWindow", "Root event for melee weapon sweep windows.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Attack_HitWindowBegin, "Event.Attack.HitWindow.Begin", "Begins a melee weapon sweep window.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Attack_HitWindowTick, "Event.Attack.HitWindow.Tick", "Advances a melee weapon sweep window.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Attack_HitWindowEnd, "Event.Attack.HitWindow.End", "Ends a melee weapon sweep window.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combat_HitReceived, "Event.Combat.HitReceived", "Reports a confirmed combat hit to the target ASC.");
}
