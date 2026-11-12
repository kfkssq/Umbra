"""Actual enemy drops, pickups and live menu quantity updates; no test assets saved."""
import json,time
from pathlib import Path
import unreal as u

level=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
u.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic();state={'stage':'start','pickups':[]};keep=[]
def check(ok,msg):
    if not ok: raise RuntimeError(msg)
def put(obj,**fields):
    for key,value in fields.items():obj.set_editor_property(key,value)
    return obj
def finish(result,detail):
    state.update(result=result,detail=detail)
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
    out=Path(u.Paths.project_saved_dir())/'SmithingStacks';out.mkdir(exist_ok=True)
    (out/'pie.json').write_text(json.dumps(state,indent=2),encoding='utf-8')
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()
def tick(delta):
    try:
        check(time.monotonic()-start<150,'Timeout '+state['stage'])
        if state['stage']=='start':
            level.editor_request_begin_play();state['stage']='spawn';return
        if state['stage']=='spawn' and level.is_in_play_in_editor() and time.monotonic()-start>8:
            world=editor.get_game_world();pc=u.GameplayStatics.get_player_controller(world,0)
            enemy=u.GameplayStatics.get_all_actors_of_class(world,u.UmbraEnemyCharacter)[0]
            table=u.load_asset('/Game/Items/NativeAccessoriesV1/DA_Loot_Prototype')
            category=next(c for c in table.categories if c.category==u.UmbraItemCategory.MATERIAL)
            group=category.types[0]
            for identity,rank in ((10100,0),(10100,0),(10100,1),(10101,0)):
                template=next(t for t in group.templates if str(t.item_definition.definition_id)=='NativeV1.EquipParamGoods_'+str(identity))
                profile=template.item_definition.generation_profile;weights=list(profile.affix_count_weights)
                target_group=u.UmbraLootTypeGroup();target_group.import_text(group.export_text());put(target_group,templates=[template])
                target_category=u.UmbraLootCategoryGroup();target_category.import_text(category.export_text());put(target_category,types=[target_group])
                targeted=put(u.new_object(u.UmbraLootTable),use_type_groups=True,categories=[target_category])
                spawned=u.UmbraEquipmentNetworkTestLibrary.spawn_enemy_for_loot_test(enemy)
                put(spawned,enable_loot=True,loot_table=targeted,loot_item_level=1,loot_scatter_radius=300.0)
                keep.extend((spawned,targeted))
                forced=[0.0]*4;forced[rank]=1.0
                try:
                    put(profile,affix_count_weights=forced)
                    check(u.UmbraEquipmentNetworkTestLibrary.kill_enemy_for_loot_test(spawned),'Death failed')
                finally:put(profile,affix_count_weights=weights)
            pc.toggle_character_menu();check(pc.is_character_menu_open(),'Menu failed')
            state.update(stage='pickup',at=time.monotonic());return
        if state['stage']=='pickup' and time.monotonic()-state['at']>3:
            world=editor.get_game_world();pc=u.GameplayStatics.get_player_controller(world,0);bag=pc.player_state.get_inventory_component()
            # Enemy drops do not retain their destroyed enemy as owner; identify this controlled batch
            # by stone definitions, excluding the existing startup spawner's owned drop.
            drops=[d for d in u.GameplayStatics.get_all_actors_of_class(world,u.UmbraWorldItem)
                   if str(d.get_item_instance().definition_id) in ('NativeV1.EquipParamGoods_10100','NativeV1.EquipParamGoods_10101')
                   and not isinstance(d.get_owner(),u.UmbraLootTestSpawner)]
            check(len(drops)==4,'Expected four controlled drops')
            menu=next(w for w in u.ObjectIterator(u.UmbraInventoryMenu) if w.get_owning_player()==pc)
            initial=len(bag.get_snapshot().items)
            for drop in drops:
                instance=drop.get_item_instance()
                pc.get_controlled_pawn().set_actor_location(drop.get_actor_location()+u.Vector(0,0,100),False,True)
                check(drop.try_pickup(pc)==u.UmbraPickupResult.SUCCESS,'Pickup failed')
                entries=[e for e in bag.get_snapshot().items if e.definition==instance.definition and e.instance.rarity==instance.rarity]
                check(len(entries)==1,'Matching stones occupy multiple cells')
                entry=entries[0];cell=menu.get_inventory_slot(entry.slot_index)
                count=cell.get_editor_property('stack_count_text')
                check(str(count.get_text())==str(entry.instance.quantity),'Live quantity text mismatch')
                check(count.get_visibility()==u.SlateVisibility.HIT_TEST_INVISIBLE,'Quantity hidden')
                state['pickups'].append(dict(id=str(instance.definition_id),rarity=int(instance.rarity.value),quantity=entry.instance.quantity))
            snapshot=bag.get_snapshot();check(len(snapshot.items)==initial+3,'Four stones should occupy three cells')
            stacked=next(e for e in snapshot.items if e.instance.quantity==2)
            check(bag.move_item(stacked.instance_id,90)==u.UmbraInventoryResult.SUCCESS,'Stack move failed')
            moved=next(e for e in bag.get_snapshot().items if e.instance_id.to_string()==stacked.instance_id.to_string())
            check(moved.instance.quantity==2 and moved.slot_index==90,'Move lost quantity')
            check(str(menu.get_inventory_slot(90).get_editor_property('stack_count_text').get_text())=='2','Moved cell count')
            check(menu.get_inventory_slot(stacked.slot_index).get_editor_property('stack_count_text').get_visibility()==u.SlateVisibility.COLLAPSED,'Old cell stale count')
            u.SystemLibrary.execute_console_command(world,'Shot SHOWUI',pc)
            state.update(stage='capture',at=time.monotonic())
        if state['stage']=='capture' and time.monotonic()-state['at']>2:
            pc=u.GameplayStatics.get_player_controller(editor.get_game_world(),0)
            pc.toggle_character_menu();level.editor_request_end_play();state['stage']='end'
        if state['stage']=='end' and not level.is_in_play_in_editor():finish('passed','Four actual drops -> three cells, matching quantity 2, different quality/type separated, live count and move update')
    except Exception as error:
        level.editor_request_end_play();finish('failed',str(error))
callback=u.register_slate_post_tick_callback(tick)
