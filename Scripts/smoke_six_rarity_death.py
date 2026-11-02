"""Transient standalone PIE: kill six enemies through GAS; validate generated loot and presentation."""
import json,time
from pathlib import Path
import unreal as u
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
u.EditorPythonScripting.set_keep_python_script_alive(True)
started=time.monotonic()
state={"result":"pending","stage":"start","items":[]}
keep=[]
def check(ok,why):
    if not ok: raise RuntimeError(why)
def finish(result,detail):
    state.update(result=result,detail=detail)
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
    Path(u.Paths.project_saved_dir(),"SixRarityDeathPIE.json").write_text(json.dumps(state,indent=2),encoding="utf-8")
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()
def tick(delta):
    try:
        if time.monotonic()-started>100: raise RuntimeError("Timed out "+state["stage"])
        if state["stage"]=="start":
            level.editor_request_begin_play();state["stage"]="kill";return
        if state["stage"]=="kill" and time.monotonic()-started>8 and level.is_in_play_in_editor():
            world=editor.get_game_world()
            enemies=u.GameplayStatics.get_all_actors_of_class(world,u.UmbraEnemyCharacter)
            check(bool(enemies),"No prototype enemy to supply a nav-ground test location")
            origin=enemies[0].get_actor_transform()
            source=u.load_asset("/Game/Items/Tests/RandomMVP/DA_Item_RandomSword")
            profile=source.get_editor_property("generation_profile")
            rows=list(profile.get_editor_property("rarities"))
            check(len(rows)==6,"MVP not six rarities")
            for i,row in enumerate(rows):
                item=u.new_object(u.UmbraItemDefinition)
                for key in ("weapon","item_category","weapon_type","allowed_slots","world_drop_mesh","world_drop_skeletal_mesh","icon"):
                    item.set_editor_property(key,source.get_editor_property(key))
                item.set_editor_property("definition_id","SixCheck."+str(i))
                item.set_editor_property("display_name","SixCheck "+str(row.rarity))
                generation=u.new_object(u.UmbraItemGenerationProfile)
                generation.set_editor_property("pool",profile.get_editor_property("pool"))
                generation.set_editor_property("rarities",[row])
                item.set_editor_property("generation_profile",generation)
                table=u.new_object(u.UmbraLootTable)
                entry=u.UmbraLootEntry();entry.set_editor_property("item_definition",item)
                table.set_editor_property("entries",[entry])
                enemy=u.UmbraEquipmentNetworkTestLibrary.spawn_enemy_for_loot_test(enemies[0])
                enemy.set_editor_property("enable_loot",True)
                enemy.set_editor_property("loot_table",table)
                enemy.set_editor_property("loot_item_level",10)
                keep.extend((item,generation,table,enemy))
                check(u.UmbraEquipmentNetworkTestLibrary.kill_enemy_for_loot_test(enemy),"Death did not trigger")
            state["stage"]="inspect";state["killed_at"]=time.monotonic();return
        if state["stage"]=="inspect" and time.monotonic()-state["killed_at"]>2:
            world=editor.get_game_world()
            drops=[d for d in u.GameplayStatics.get_all_actors_of_class(world,u.UmbraWorldItem)
                   if str(d.get_item_instance().definition_id).startswith("SixCheck.")]
            check(len(drops)==6,"Expected six death drops, found "+str(len(drops)))
            pc=u.GameplayStatics.get_player_controller(world,0)
            for drop in drops:
                instance=drop.get_item_instance()
                row=instance.definition.get_editor_property("generation_profile").get_editor_property("rarities")[0]
                pc.get_controlled_pawn().set_actor_location(drop.get_actor_location()+u.Vector(0,0,100),False,True)
                check(drop.try_pickup(pc)==u.UmbraPickupResult.SUCCESS,"Pickup failed")
                bag=pc.player_state.get_inventory_component().get_snapshot()
                entry=next(e for e in bag.items if e.instance_id.to_string()==instance.instance_id.to_string())
                data=u.UmbraItemTooltipDataBuilder.from_inventory(bag,entry.slot_index,entry.instance_id,entry.definition,
                    pc.player_state.get_equipment_component(),False,u.UmbraEquipmentSlot.MAIN_HAND)
                check(data.valid,"Tooltip invalid")
                check(data.rarity==row.rarity and data.rarity_color==row.color,"Rarity/color mismatch")
                check(data.tint_rarity_backgrounds,"Background tint missing")
                state["items"].append(dict(id=str(instance.definition_id),rarity=str(instance.rarity),actor=drop.get_name(),color=str(data.rarity_color)))
            level.editor_request_end_play();state["stage"]="end"
        if state["stage"]=="end" and not level.is_in_play_in_editor(): finish("passed","Six real health-deaths generated all rarities; pickup and tooltip projections verified")
    except Exception as error:
        level.editor_request_end_play()
        finish("failed",type(error).__name__+": "+str(error))
callback=u.register_slate_post_tick_callback(tick)
