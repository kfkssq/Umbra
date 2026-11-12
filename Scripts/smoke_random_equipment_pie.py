"""Actual enemy health->death->grouped loot->pickup->equip->unequip, no level saves.
Exercises every enabled type plus the production distribution. Temporarily forced counts
are restored immediately after synchronous generation; no output packages are saved.
"""
import json
from pathlib import Path
import time
import unreal as u

CONFIG=json.loads((Path(__file__).parent/'EquipmentImport/random_equipment_v2.json').read_text(encoding='utf-8'))

u.EditorPythonScripting.set_keep_python_script_alive(True)
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
old_play=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
started=time.monotonic()
state={'stage':'start','items':[],'expected':0}
keep=[]


def check(value,message):
    if not value: raise RuntimeError(message)


def put(obj,**values):
    for key,value in values.items(): obj.set_editor_property(key,value)
    return obj


def payload(instance):
    return instance.export_text()


def finish(result,detail):
    state.update(result=result,detail=detail)
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(old_play[0],old_play[1],bool(old_play[2]))
    output=Path(u.Paths.project_saved_dir())/CONFIG['report_directory']/'pie.json'
    output.write_text(json.dumps(state,ensure_ascii=False,indent=2),encoding='utf-8')
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()


def tick(delta):
    try:
        age=time.monotonic()-started
        if age>160: raise RuntimeError('Timeout: '+state['stage'])
        if state['stage']=='start':
            level.editor_request_begin_play();state['stage']='kill';return
        if state['stage']=='kill' and age>8 and level.is_in_play_in_editor():
            world=editor.get_game_world()
            enemies=u.GameplayStatics.get_all_actors_of_class(world,u.UmbraEnemyCharacter)
            check(bool(enemies),'No enemy/nav-ground fixture')
            table=u.load_asset(CONFIG['destination']+'/DA_Loot_Equipment')
            check(enemies[0].get_editor_property('loot_table')==table,'Prototype enemy not bound to production table')
            check(enemies[0].get_editor_property('enable_loot'),'Prototype loot disabled')
            for category in table.get_editor_property('categories'):
                for group in category.types:
                    # One real asset per type, including high-source-rarity definitions when available.
                    template=max(group.templates,key=lambda t:t.item_definition.source_rarity)
                    item=template.item_definition
                    profile=item.generation_profile
                    weights=list(profile.get_editor_property('affix_count_weights'))
                    source_rank=item.source_rarity
                    rank=state['expected']%3 if source_rank==0 else state['expected']%4
                    forced=[0.0]*4;forced[rank]=1.0
                    put(profile,affix_count_weights=forced)
                    try:
                        one_group=u.UmbraLootTypeGroup();one_group.import_text(group.export_text())
                        put(one_group,templates=[template])
                        one_category=u.UmbraLootCategoryGroup();one_category.import_text(category.export_text())
                        put(one_category,types=[one_group])
                        targeted=put(u.new_object(u.UmbraLootTable),use_type_groups=True,categories=[one_category])
                        enemy=u.UmbraEquipmentNetworkTestLibrary.spawn_enemy_for_loot_test(enemies[state['expected']%len(enemies)])
                        put(enemy,enable_loot=True,loot_table=targeted,loot_item_level=10,loot_scatter_radius=400.0)
                        keep.extend((targeted,enemy))
                        check(u.UmbraEquipmentNetworkTestLibrary.kill_enemy_for_loot_test(enemy),'Targeted death failed')
                        state['expected']+=1
                    finally: put(profile,affix_count_weights=weights)
            for _ in range(8):
                enemy=u.UmbraEquipmentNetworkTestLibrary.spawn_enemy_for_loot_test(enemies[state['expected']%len(enemies)])
                put(enemy,loot_scatter_radius=400.0)
                keep.append(enemy)
                check(enemy.get_editor_property('loot_table')==table,'Spawned enemy lost table defaults')
                check(u.UmbraEquipmentNetworkTestLibrary.kill_enemy_for_loot_test(enemy),'Production death failed')
                state['expected']+=1
            state.update(stage='inspect',killed=time.monotonic());return
        if state['stage']=='inspect' and time.monotonic()-state['killed']>3:
            world=editor.get_game_world()
            drops=[d for d in u.GameplayStatics.get_all_actors_of_class(world,u.UmbraWorldItem)
                   if str(d.get_item_instance().definition_id).startswith(CONFIG['id_prefix'])]
            check(len(drops)==state['expected'],'Expected %d drops, got %d'%(state['expected'],len(drops)))
            pc=u.GameplayStatics.get_player_controller(world,0)
            owner=pc.player_state; bag=owner.get_inventory_component(); equipment=owner.get_equipment_component()
            for worn in equipment.get_snapshot().items: owner.unequip_to_inventory(worn.slot,worn.instance_id)
            for drop in drops:
                instance=drop.get_item_instance();before=payload(instance)
                count=len(instance.rolled_affixes)
                check(1<=count<=4,'Invalid affix count')
                ranks=[u.UmbraItemRarity.COMMON,u.UmbraItemRarity.MAGIC,u.UmbraItemRarity.RARE,u.UmbraItemRarity.EPIC,u.UmbraItemRarity.LEGENDARY,u.UmbraItemRarity.UNIQUE]
                rank=ranks.index(instance.rarity);source_rank=instance.definition.source_rarity
                check(count==min(rank+1,4),'Rarity/count mismatch')
                check(rank<=2 if source_rank==0 else rank<=3 if source_rank==1 else rank in (3,4) if source_rank==2 else rank>=4 if source_rank==3 else True,'Source rarity restriction failed')
                state.setdefault('source_rarity_counts',{})[str(source_rank)]=state.get('source_rarity_counts',{}).get(str(source_rank),0)+1
                expected=instance.rarity
                pc.get_controlled_pawn().set_actor_location(drop.get_actor_location()+u.Vector(0,0,100),False,True)
                check(drop.try_pickup(pc)==u.UmbraPickupResult.SUCCESS,'Pickup failed')
                carried=next(e for e in bag.get_snapshot().items if e.instance_id.to_string()==instance.instance_id.to_string())
                check(payload(carried.instance)==before,'Pickup rerolled item')
                slot=instance.definition.allowed_slots[0]
                tooltip=u.UmbraItemTooltipDataBuilder.from_inventory(bag.get_snapshot(),carried.slot_index,carried.instance_id,carried.definition,equipment,False,slot)
                check(tooltip.valid and tooltip.rarity==expected,'Tooltip projection failed')
                check(tooltip.has_special_effect==(rank>=4) and tooltip.is_unique==(rank==5),'High-quality markers lost')
                state.setdefault('rarity_counts',{})[str(rank)]=state.get('rarity_counts',{}).get(str(rank),0)+1
                enabled=instance.definition.get_editor_property('equipping_enabled')
                if not enabled:
                    check(not tooltip.get_editor_property('equipping_enabled'),'Tooltip forgot unavailable state')
                    check(bool(str(tooltip.requirement_status_text)),'Missing unavailable explanation')
                    revision=bag.get_snapshot().revision
                    check(owner.equip_from_inventory(slot,instance.instance_id)==u.UmbraTransferResult.EQUIPPING_UNAVAILABLE,'Staged equip not rejected explicitly')
                    check(bag.get_snapshot().revision==revision,'Rejected equip modified inventory')
                    carried=next(e for e in bag.get_snapshot().items if e.instance_id.to_string()==instance.instance_id.to_string())
                    check(payload(carried.instance)==before,'Rejected equip lost/changed instance')
                    state['items'].append({'id':str(instance.definition_id),'name':str(instance.definition.display_name),
                        'rarity':str(instance.rarity),'count':count,'staged':True,'requirements':instance.definition.requirements.export_text()})
                    continue
                if equipment.character_level < instance.item_level:
                    check(tooltip.requirements.level_known and not tooltip.requirements.level_met and '×' in str(tooltip.item_level_text), 'Low-level tooltip marker missing')
                    revision = bag.get_snapshot().revision
                    check(owner.equip_from_inventory(slot,instance.instance_id)==u.UmbraTransferResult.LEVEL_TOO_LOW, 'Instance level gate not enforced')
                    check(bag.get_snapshot().revision==revision, 'Level rejection changed inventory')
                    state['items'].append({'id':str(instance.definition_id),'level_rejected':True,'item_level':instance.item_level})
                    continue
                check(tooltip.requirements.level_met and '×' not in str(tooltip.item_level_text), 'Level marker not cleared')
                equip_result = owner.equip_from_inventory(slot,instance.instance_id)
                check(equip_result==u.UmbraTransferResult.SUCCESS,'Equip at required level failed: '+str(equip_result))
                worn=next(e for e in equipment.get_snapshot().items if e.instance_id.to_string()==instance.instance_id.to_string())
                check(payload(worn.instance)==before,'Equip changed payload')
                owner.unequip_to_inventory(slot,instance.instance_id)
                returned=next(e for e in bag.get_snapshot().items if e.instance_id.to_string()==instance.instance_id.to_string())
                check(payload(returned.instance)==before,'Unequip changed payload')
                state['items'].append({'id':str(instance.definition_id),'name':str(instance.definition.display_name),
                    'rarity':str(instance.rarity),'count':count,'requirements':instance.definition.requirements.export_text()})
            level.editor_request_end_play();state['stage']='end'
        if state['stage']=='end' and not level.is_in_play_in_editor():
            finish('passed','All types: death/drop/pickup/tooltip/payload; usable equip/unequip; staged equip rejection preserves inventory')
    except Exception as exc:
        level.editor_request_end_play();finish('failed',str(exc))


callback=u.register_slate_post_tick_callback(tick)
