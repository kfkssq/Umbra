"""Saved layout/background readback and four simultaneous relics via the live inventory menu."""
import json,time
from pathlib import Path
import unreal as u
level=u.get_editor_subsystem(u.LevelEditorSubsystem);editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
u.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic();state={'stage':'start'};keep=[]
def check(ok,msg):
    if not ok:raise RuntimeError(msg)
def put(obj,**fields):
    for key,value in fields.items():obj.set_editor_property(key,value)
    return obj
def finish(result,detail):
    state.update(result=result,detail=detail)
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
    out=Path(u.Paths.project_saved_dir())/'RelicSlots';out.mkdir(exist_ok=True)
    (out/'pie.json').write_text(json.dumps(state,indent=2),encoding='utf-8')
    u.unregister_slate_post_tick_callback(callback);u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
def tick(delta):
    try:
        check(time.monotonic()-start<150,'Timeout '+state['stage'])
        if state['stage']=='start':level.editor_request_begin_play();state['stage']='inspect';return
        if state['stage']=='inspect' and level.is_in_play_in_editor() and time.monotonic()-start>8:
            world=editor.get_game_world();pc=u.GameplayStatics.get_player_controller(world,0)
            pc.toggle_character_menu()
            menu=next(w for w in u.ObjectIterator(u.UmbraEquipmentMenu) if w.get_owning_player()==pc)
            for names in [('HEAD','CHEST','HANDS','LEGS','MAIN_HAND'),('AMULET','AMULET2','RING1','RING2','OFF_HAND')]:
                cells=[menu.get_equipment_slot(getattr(u.UmbraEquipmentSlot,n)) for n in names]
                check(all(cells),'Missing slot')
                check(list(cells[0].get_parent().get_all_children())==cells,'Wrong top-to-bottom order')
                for n,cell in zip(names,cells):
                    expected=cell.get_editor_property('empty_slot_icons')[getattr(u.UmbraEquipmentSlot,n)]
                    actual=cell.get_editor_property('empty_icon').get_editor_property('brush').get_editor_property('resource_object')
                    check(actual==expected,'Rendered background mismatch '+n)
            u.SystemLibrary.execute_console_command(world,'Shot SHOWUI',pc)
            table=u.load_asset('/Game/Items/NativeAccessoriesV1/DA_Loot_Prototype')
            category=next(c for c in table.categories if c.category==u.UmbraItemCategory.ACCESSORY);group=category.types[0]
            template=group.templates[0]
            one_group=u.UmbraLootTypeGroup();one_group.import_text(group.export_text());put(one_group,templates=[template])
            one_category=u.UmbraLootCategoryGroup();one_category.import_text(category.export_text());put(one_category,types=[one_group])
            targeted=put(u.new_object(u.UmbraLootTable),use_type_groups=True,categories=[one_category])
            keep.append(targeted)
            enemy=u.GameplayStatics.get_all_actors_of_class(world,u.UmbraEnemyCharacter)[0]
            for _ in range(4):
                spawned=u.UmbraEquipmentNetworkTestLibrary.spawn_enemy_for_loot_test(enemy)
                put(spawned,enable_loot=True,loot_table=targeted,loot_item_level=1,loot_scatter_radius=300.0)
                keep.append(spawned);check(u.UmbraEquipmentNetworkTestLibrary.kill_enemy_for_loot_test(spawned),'Death failed')
            state.update(stage='equip',at=time.monotonic());return
        if state['stage']=='equip' and time.monotonic()-state['at']>3:
            world=editor.get_game_world();pc=u.GameplayStatics.get_player_controller(world,0)
            owner=pc.player_state;bag=owner.get_inventory_component();equipment=owner.get_equipment_component()
            menu=next(w for w in u.ObjectIterator(u.UmbraInventoryMenu) if w.get_owning_player()==pc)
            drops=[d for d in u.GameplayStatics.get_all_actors_of_class(world,u.UmbraWorldItem) if str(d.get_item_instance().definition_id).startswith('NativeV1.EquipParamAccessory_') and not isinstance(d.get_owner(),u.UmbraLootTestSpawner)]
            check(len(drops)==4,'Expected four relic drops')
            slots=[u.UmbraEquipmentSlot.AMULET,u.UmbraEquipmentSlot.AMULET2,u.UmbraEquipmentSlot.RING1,u.UmbraEquipmentSlot.RING2]
            ids=[]
            for drop,slot in zip(drops,slots):
                instance=drop.get_item_instance();before=instance.export_text()
                pc.get_controlled_pawn().set_actor_location(drop.get_actor_location()+u.Vector(0,0,100),False,True)
                check(drop.try_pickup(pc)==u.UmbraPickupResult.SUCCESS,'Pickup')
                carried=next(e for e in bag.get_snapshot().items if e.instance_id.to_string()==instance.instance_id.to_string())
                tooltip=u.UmbraItemTooltipDataBuilder.from_inventory(bag.get_snapshot(),carried.slot_index,carried.instance_id,carried.definition,equipment,True,slot)
                check(tooltip.valid and str(tooltip.item_type)=='遗物','Target tooltip/classification')
                check(menu.request_equip_in_slot(instance.instance_id,slot)==u.UmbraTransferResult.SUCCESS,'Live UI rejects relic '+str(slot))
                check(next(e for e in equipment.get_snapshot().items if e.slot==slot).instance.export_text()==before,'Equip changed instance')
                ids.append((slot,instance.instance_id,before))
            check(sum(e.slot in slots for e in equipment.get_snapshot().items)==4,'Four relics not simultaneously equipped')
            state['equipped']=[str(slot) for slot,_,_ in ids]
            u.SystemLibrary.execute_console_command(world,'Shot SHOWUI',pc)
            state['stage']='unequip';state['at']=time.monotonic();keep.extend(ids);return
        if state['stage']=='unequip' and time.monotonic()-state['at']>2:
            pc=u.GameplayStatics.get_player_controller(editor.get_game_world(),0);owner=pc.player_state
            for slot,identity,before in [row for row in keep if isinstance(row,tuple)]:
                result=owner.unequip_to_inventory(slot,identity)
                check(result[0]==u.UmbraTransferResult.SUCCESS,'Unequip')
                check(next(e for e in owner.get_inventory_component().get_snapshot().items if e.instance_id.to_string()==identity.to_string()).instance.export_text()==before,'Unequip changed instance')
            state.update(stage='empty',at=time.monotonic())
        if state['stage']=='empty' and time.monotonic()-state['at']>2:
            world=editor.get_game_world();pc=u.GameplayStatics.get_player_controller(world,0)
            u.SystemLibrary.execute_console_command(world,'Shot SHOWUI',pc)
            state.update(stage='close',at=time.monotonic())
        if state['stage']=='close' and time.monotonic()-state['at']>1:
            pc=u.GameplayStatics.get_player_controller(editor.get_game_world(),0)
            pc.toggle_character_menu();level.editor_request_end_play();state['stage']='end'
        if state['stage']=='end' and not level.is_in_play_in_editor():finish('passed','Saved order/backgrounds; four relics equipped via inventory UI, target tooltip valid, simultaneous occupancy and unchanged payload after unequip')
    except Exception as error:level.editor_request_end_play();finish('failed',str(error))
callback=u.register_slate_post_tick_callback(tick)
