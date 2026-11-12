// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameplayTags/UmbraGameplayTags.h"

namespace UmbraGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Skill_Eight, "Input.Skill.Eight", "Isolated targeting prototype slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Skill_Nine, "Input.Skill.Nine", "Directional targeting prototype.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Skill_Zero, "Input.Skill.Zero", "Directional targeting prototype.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_TargetSector, "Cooldown.Skill.TargetSector", "Directional targeting prototype.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_TargetWave, "Cooldown.Skill.TargetWave", "Directional targeting prototype.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_TargetLeap, "Cooldown.Skill.TargetLeap", "Isolated targeting prototype cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Vulnerable, "State.Vulnerable", "Target vulnerability consumed by opt-in A/X typed damage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_ResultCritical, "Damage.ResultCritical", "Per-hit execution result, consumed by IncomingDamage settlement.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_SourcePrimaryAttack, "Damage.SourcePrimaryAttack", "Per-hit marker for authoritative basic-attack damage measurement.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Source_BasicAttack, "Damage.Source.BasicAttack", "Damage spec source: basic attack, independent of ability implementation.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Source_Skill, "Damage.Source.Skill", "Damage spec source: active skill, not a basic attack.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input, "Input", "Root tag for ability input routing.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability, "Ability", "Root tag for gameplay abilities.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State, "State", "Root tag for gameplay states.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event, "Event", "Root tag for gameplay events.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage, "Damage", "Root tag for damage classification.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Type, "Damage.Type", "SetByCaller: 0 Physical, 1 Magical. Required.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_AbilityPowerCoefficient, "Damage.AbilityPowerCoefficient", "SetByCaller intelligence scaling.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_AttackPowerCoefficient, "Damage.AttackPowerCoefficient", "SetByCaller strength scaling.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Attack_Primary, "Input.Attack.Primary", "Primary attack input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Skill_One, "Input.Skill.One", "First configured skill slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Skill_Two, "Input.Skill.Two", "Second configured skill slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Skill_Three, "Input.Skill.Three", "Third configured skill slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Skill_Four, "Input.Skill.Four", "Fourth configured skill slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Skill_Five, "Input.Skill.Five", "Fifth configured skill slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Skill_Six, "Input.Skill.Six", "Sixth configured skill slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Skill_Seven, "Input.Skill.Seven", "Seventh configured skill slot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_GlintstonePebble, "Cooldown.Skill.GlintstonePebble", "Glintstone pebble cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_SwordStorm, "Cooldown.Skill.SwordStorm", "Sword storm cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_LeapSlam, "Cooldown.Skill.LeapSlam", "Leap slam cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_Guard, "Cooldown.Skill.Guard", "Guard cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_BattleFocus, "Cooldown.Skill.BattleFocus", "Battle focus cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_StoneRecovery, "Cooldown.Skill.StoneRecovery", "Stone recovery cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Skill_Sweep, "Ability.Skill.Sweep", "Data-configured sector attack.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_Sweep, "Cooldown.Skill.Sweep", "Sweep cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Skill_WideSweep, "Cooldown.Skill.WideSweep", "Wide sweep cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack_Basic, "Ability.Attack.Basic", "Basic attack ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack_EnemyBasic, "Ability.Attack.EnemyBasic", "Enemy basic attack ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_HitReact, "Ability.HitReact", "Cosmetic hit-reaction ability.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attacking, "State.Attacking", "Actor is performing an attack.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_SwordStorm, "State.Skill.SwordStorm", "Mobile spinning attack; movement does not cancel the skill.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_HitReact, "State.HitReact", "Actor is playing a hit reaction.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "Actor is dead and cannot react to hits.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Stunned, "State.Stunned", "Actor is stunned and cannot continue attacks.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_SuperArmor, "State.SuperArmor", "Actor ignores ordinary hit reactions.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Attack_ComboInput, "Event.Attack.ComboInput", "Queues the next basic-attack combo step.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Attack_MovementCommand, "Event.Attack.MovementCommand", "Requests phase-aware movement cancellation of the current basic attack.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Attack_HitWindow, "Event.Attack.HitWindow", "Root event for melee weapon sweep windows.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Attack_HitWindowBegin, "Event.Attack.HitWindow.Begin", "Begins a melee weapon sweep window.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Attack_HitWindowTick, "Event.Attack.HitWindow.Tick", "Advances a melee weapon sweep window.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Attack_HitWindowEnd, "Event.Attack.HitWindow.End", "Ends a melee weapon sweep window.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Attack_ChainPoint, "Event.Attack.ChainPoint", "Author-authored point at which the next basic attack may replace the current montage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combat_HitReceived, "Event.Combat.HitReceived", "Reports a confirmed combat hit to the target ASC.");
}
