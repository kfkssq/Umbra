"""Bind the prototype melee enemy to the validated equipment table, through UE only.
Default audit. UMBRA_RANDOM_ACTIVATE=1 writes only the BP loot defaults and explicit
prototype enemy overrides. Never touches test spawners or source equipment.
"""
import json
import os
from pathlib import Path
import unreal as u

write = os.environ.get('UMBRA_RANDOM_ACTIVATE') == '1'
config=json.loads(Path(os.environ.get('UMBRA_RANDOM_CONFIG',str(Path(__file__).parent/'random_equipment_v2.json'))).read_text(encoding='utf-8'))
table = u.load_asset(config['destination']+'/'+config.get('loot_table_name','DA_Loot_Equipment'))
assert isinstance(table,u.UmbraLootTable) and table.get_editor_property('use_type_groups')
bp = u.load_asset('/Game/Blueprints/Enemies/BP_Enemy_Melee_01')
cdo = u.get_default_object(bp.generated_class())
assert isinstance(cdo,u.UmbraEnemyCharacter)
report = {'apply':write,'targets':[],'saved':[]}


def update(obj):
    old = obj.get_editor_property('loot_table')
    enabled = obj.get_editor_property('enable_loot')
    row = {'object':obj.get_path_name(),'previous_table':old.get_path_name() if old else None,'previous_enabled':enabled}
    report['targets'].append(row)
    changed = old != table or not enabled
    row['changed'] = changed
    if write and changed:
        obj.set_editor_property('loot_table',table)
        obj.set_editor_property('enable_loot',True)
    return changed


bp_changed = update(cdo)
if write and bp_changed:
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    assert u.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
    report['saved'].append(bp.get_path_name())
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
assert level.load_level('/Game/Maps/L_Prototype')
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
changed = False
for actor in actors.get_all_level_actors():
    if isinstance(actor,u.UmbraEnemyCharacter) and actor.get_class()==bp.generated_class():
        changed = update(actor) or changed
if write and changed:
    assert level.save_current_level()
    report['saved'].append('/Game/Maps/L_Prototype')
output = Path(u.Paths.project_saved_dir())/config.get('report_directory','RandomEquipment')
output.mkdir(exist_ok=True)
(output/'activation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
