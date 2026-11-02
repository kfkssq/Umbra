"""Single-player PIE integration smoke; edits the transient editor level only, never saves it."""
import json
import os
import time
import unreal

pipeline_m5 = "-UmbraPipelineM5" in unreal.SystemLibrary.get_command_line()
fixture_path = ("/Game/Items/Tests/PipelineM5/BP_LootTest_PipelineM5.BP_LootTest_PipelineM5_C" if pipeline_m5
                else "/Game/Items/Tests/RandomMVP/BP_LootTest_RandomMVP.BP_LootTest_RandomMVP_C")
definition_prefix = "PipelineM5." if pipeline_m5 else "MVP."
report_name = "EquipmentM5PIESmoke.json" if pipeline_m5 else "RandomMVPPIESmoke.json"
previous_play = unreal.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0, 1, True)
if len(previous_play) != 3:
    raise RuntimeError("Cannot configure standalone PIE")
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
starts = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.PlayerStart)]
origin = starts[0].get_actor_location() if starts else unreal.Vector()
fixture = actors.spawn_actor_from_class(unreal.load_class(None, fixture_path), origin)
if not fixture:
    raise RuntimeError("Cannot place test fixture")
started = time.monotonic()
state = dict(requested=False, checked=False, ended=False, result="pending", items=[])

def payload(instance):
    return dict(guid=instance.instance_id.to_string(), definition=str(instance.definition_id),
                schema=instance.schema_version, level=instance.item_level, rarity=str(instance.rarity),
                seed=instance.generation_seed, rules=instance.generation_rules_version,
                affixes=[dict(id=str(r.affix_id), tier=str(r.affix_tier_id), value=r.value,
                              selected=r.selected_tier, candidates=list(r.candidate_values)) for r in instance.rolled_affixes])

def check(condition, detail):
    if not condition:
        raise RuntimeError(detail)

def finish(result, detail):
    state.update(result=result, detail=detail)
    unreal.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous_play[0],previous_play[1],bool(previous_play[2]))
    actors.destroy_actor(fixture)
    with open(os.path.join(unreal.Paths.project_saved_dir(), report_name), "w", encoding="utf-8") as stream:
        json.dump(state, stream, indent=2)
    unreal.log("RANDOM_MVP_PIE " + result + " " + detail)
    unreal.unregister_slate_post_tick_callback(callback)
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    unreal.SystemLibrary.quit_editor()

def tick(delta):
    try:
        age = time.monotonic() - started
        if age > 100:
            level.editor_request_end_play()
            finish("failed", "Timed out")
            return
        if not state["requested"]:
            state["requested"] = True
            level.editor_request_begin_play()
            return
        if level.is_in_play_in_editor() and not state["checked"] and age > 8:
            world = editor.get_game_world()
            if not world:
                return
            controller = unreal.GameplayStatics.get_player_controller(world, 0)
            pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
            if not pawn:
                return
            owner = controller.player_state
            inventory = owner.get_inventory_component()
            equipment = owner.get_equipment_component()
            check(equipment.get_editor_property("enable_equipment"), "Equipment disabled in prototype")
            # Preserve prototype starting gear by returning it to the bag before testing the same slots.
            for existing in equipment.get_snapshot().items:
                owner.unequip_to_inventory(existing.slot, existing.instance_id)
            drops = [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.UmbraWorldItem)
                     if str(a.get_item_instance().definition_id).startswith(definition_prefix)]
            check(len(drops) >= 2, "Need at least two generated drops")
            for drop in drops:
                instance = drop.get_item_instance()
                before = payload(instance)
                pawn.set_actor_location(drop.get_actor_location() + unreal.Vector(0,0,100), False, True)
                result = drop.try_pickup(controller)
                check(result == unreal.UmbraPickupResult.SUCCESS, "Pickup failed: " + str(result))
                carried = [e for e in inventory.get_snapshot().items if e.instance_id.to_string() == before["guid"]]
                check(len(carried) == 1 and payload(carried[0].instance) == before, "Pickup changed payload")
                slot = instance.definition.get_editor_property("allowed_slots")[0]
                result = owner.equip_from_inventory(slot, instance.instance_id)
                check(result == unreal.UmbraTransferResult.SUCCESS, "Equip failed: " + str(result))
                worn = [e for e in equipment.get_snapshot().items if e.instance_id.to_string() == before["guid"]]
                check(len(worn) == 1 and payload(worn[0].instance) == before, "Equip changed payload")
                owner.unequip_to_inventory(slot, instance.instance_id)
                returned = [e for e in inventory.get_snapshot().items if e.instance_id.to_string() == before["guid"]]
                check(len(returned) == 1 and payload(returned[0].instance) == before, "Unequip changed payload")
                state["items"].append(before)
            check(len({r["guid"] for r in state["items"]}) == len(drops), "Repeated GUID")
            check(len({r["definition"] for r in state["items"]}) == (10 if pipeline_m5 else 2), "All templates required")
            state.update(checked=True, checked_at=age)
        if state["checked"] and not state["ended"] and age-state["checked_at"] > 3:
            level.editor_request_end_play()
            state["ended"] = True
        if state["ended"] and not level.is_in_play_in_editor():
            finish("passed", "Generated test assets: world pickup, equip, unequip, complete payload and clean EndPIE")
    except Exception as error:
        level.editor_request_end_play()
        finish("failed", str(error))

callback = unreal.register_slate_post_tick_callback(tick)
