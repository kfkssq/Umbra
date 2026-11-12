"""Real death/drop/pickup/dual-amulet checks in PIE. No test assets saved."""
import json
import time
from pathlib import Path
import unreal as u

u.EditorPythonScripting.set_keep_python_script_alive(True)
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
old_play=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
started=time.monotonic()
state={'stage':'start','items':[]}
keep=[]

def check(value,message):
    if not value: raise RuntimeError(message)

def put(obj,**fields):
    for key,value in fields.items(): obj.set_editor_property(key,value)
    return obj

def finish(result,detail):
    state.update(result=result,detail=detail)
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(old_play[0],old_play[1],bool(old_play[2]))
    (Path(u.Paths.project_saved_dir())/'NativeAccessories/pie.json').write_text(json.dumps(state,ensure_ascii=False,indent=2),encoding='utf-8')
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()

def tick(delta):
    try:
        age=time.monotonic()-started
        check(age<150,'Timeout: '+state['stage'])
        if state['stage']=='start':
            level.editor_request_begin_play();state['stage']='kill';return
        if state['stage']=='kill' and age>8 and level.is_in_play_in_editor():
            world=editor.get_game_world()
            enemies=u.GameplayStatics.get_all_actors_of_class(world,u.UmbraEnemyCharacter)
            check(bool(enemies),'No enemy fixture')
            table=u.load_asset('/Game/Items/NativeAccessoriesV1/DA_Loot_Prototype')
            check(enemies[0].get_editor_property('loot_table')==table,'Saved map table mismatch')
            for category in table.categories:
                if category.category not in (u.UmbraItemCategory.ACCESSORY,u.UmbraItemCategory.MATERIAL): continue
                group=category.types[0]
                # Low and highest source-rarity examples of both new categories.
                for template in (min(group.templates,key=lambda t:t.item_definition.source_rarity),max(group.templates,key=lambda t:t.item_definition.source_rarity)):
                    target_group=u.UmbraLootTypeGroup();target_group.import_text(group.export_text())
                    put(target_group,templates=[template])
                    target_category=u.UmbraLootCategoryGroup();target_category.import_text(category.export_text())
                    put(target_category,types=[target_group])
                    targeted=put(u.new_object(u.UmbraLootTable),use_type_groups=True,categories=[target_category])
                    enemy=u.UmbraEquipmentNetworkTestLibrary.spawn_enemy_for_loot_test(enemies[0])
                    check(enemy.get_editor_property('loot_table')==table,'Saved BP table mismatch')
                    put(enemy,enable_loot=True,loot_table=targeted,loot_item_level=1,loot_scatter_radius=300.0)
                    keep.extend((targeted,enemy))
                    check(u.UmbraEquipmentNetworkTestLibrary.kill_enemy_for_loot_test(enemy),'Enemy death failed')
            state.update(stage='inspect',killed=time.monotonic());return
        if state['stage']=='inspect' and time.monotonic()-state['killed']>3:
            world=editor.get_game_world()
            drops=[d for d in u.GameplayStatics.get_all_actors_of_class(world,u.UmbraWorldItem) if str(d.get_item_instance().definition_id).startswith('NativeV1.')]
            check(len(drops)==4,'Expected four targeted drops, got '+str(len(drops)))
            # Check material rejection while the destination slot is empty; an occupied
            # slot correctly takes precedence in the inventory transfer contract.
            drops.sort(key=lambda d:d.get_item_instance().definition.item_category!=u.UmbraItemCategory.MATERIAL)
            pc=u.GameplayStatics.get_player_controller(world,0)
            owner=pc.player_state;bag=owner.get_inventory_component();equipment=owner.get_equipment_component()
            amulets=[]
            ranks=[u.UmbraItemRarity.COMMON,u.UmbraItemRarity.MAGIC,u.UmbraItemRarity.RARE,u.UmbraItemRarity.EPIC,u.UmbraItemRarity.LEGENDARY,u.UmbraItemRarity.UNIQUE]
            for drop in drops:
                instance=drop.get_item_instance();before=instance.export_text()
                material=instance.definition.item_category==u.UmbraItemCategory.MATERIAL
                rank=ranks.index(instance.rarity);source=instance.definition.source_rarity
                check(rank<=3 if source==1 else rank in (3,4) if source==2 else rank>=4,'Rarity gate')
                check(len(instance.rolled_affixes)==(0 if material else min(rank+1,4)),'Affix count')
                check(drop.get_actor_location().z>-1000,'Drop fell through ground')
                pc.get_controlled_pawn().set_actor_location(drop.get_actor_location()+u.Vector(0,0,100),False,True)
                check(drop.try_pickup(pc)==u.UmbraPickupResult.SUCCESS,'Pickup failed')
                carried=next(e for e in bag.get_snapshot().items if e.instance_id.to_string()==instance.instance_id.to_string())
                check(carried.instance.export_text()==before,'Pickup changed payload')
                tooltip=u.UmbraItemTooltipDataBuilder.from_inventory(bag.get_snapshot(),carried.slot_index,carried.instance_id,carried.definition,equipment,False,u.UmbraEquipmentSlot.AMULET)
                check(tooltip.valid and tooltip.rarity==instance.rarity,'Tooltip invalid')
                if material:
                    check(not tooltip.has_special_effect,'Material displays unimplemented combat power')
                    revision=bag.get_snapshot().revision
                    check(owner.equip_from_inventory(u.UmbraEquipmentSlot.AMULET,instance.instance_id)==u.UmbraTransferResult.EQUIPPING_UNAVAILABLE,'Material equipped')
                    check(bag.get_snapshot().revision==revision,'Rejected equip changed bag')
                else:
                    slot=u.UmbraEquipmentSlot.AMULET if not amulets else u.UmbraEquipmentSlot.AMULET2
                    check(owner.equip_from_inventory(slot,instance.instance_id)==u.UmbraTransferResult.SUCCESS,'Amulet equip failed')
                    amulets.append((slot,instance.instance_id,before))
                state['items'].append({'id':str(instance.definition_id),'rarity':str(instance.rarity),'source_rarity':source,'material':material})
            check(len(amulets)==2,'Missing amulets')
            worn=equipment.get_snapshot().items
            for slot,identity,before in amulets:
                check(next(e for e in worn if e.slot==slot).instance.export_text()==before,'Dual slot changed payload')
                result=owner.unequip_to_inventory(slot,identity)
                check((result[0] if isinstance(result,tuple) else result)==u.UmbraTransferResult.SUCCESS,'Unequip failed')
                check(next(e for e in bag.get_snapshot().items if e.instance_id.to_string()==identity.to_string()).instance.export_text()==before,'Unequip rerolled item')
            pc.toggle_character_menu();check(pc.is_character_menu_open(),'Menu failed to open');pc.toggle_character_menu()
            level.editor_request_end_play();state['stage']='end'
        if state['stage']=='end' and not level.is_in_play_in_editor():
            finish('passed','Saved enemy table; death, drop, pickup, tooltip, simultaneous dual amulets, unequip; stones reject equip without inventory mutation')
    except Exception as error:
        level.editor_request_end_play();finish('failed',str(error))

callback=u.register_slate_post_tick_callback(tick)
