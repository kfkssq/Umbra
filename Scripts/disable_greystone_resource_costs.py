"""UE-only targeted cost override; preserves all animation and gameplay tuning."""
import unreal as u,json
from pathlib import Path
bp=u.load_asset('/Game/Blueprints/Player/BP_UmbraPlayerState')
slots=list(u.get_default_object(bp.generated_class()).get_editor_property('initial_skill_slots'))
assert [str(s.definition.skill_id) for s in slots]==['BreachSlash','SwordStorm','LeapSlam','Guard','StoneRecovery','BattleFocus']
rows=[]
for s in slots:
    d=s.definition;rows.append(dict(skill=str(d.skill_id),previous_cost=d.resource_cost,cost=0))
    d.set_editor_property('resource_cost',0.)
    assert u.EditorAssetLibrary.save_loaded_asset(d,False)
bar=u.load_asset('/Game/UI/Skills/WBP_SkillBar')
for w in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(bar):
    if isinstance(w,u.UmbraSkillSlotWidget):w.set_editor_property('preview_resource_cost',0.)
u.BlueprintEditorLibrary.compile_blueprint(bar)
assert u.EditorAssetLibrary.save_loaded_asset(bar,False)
out=Path(u.Paths.project_saved_dir())/'GreystoneDirection'
(out/'no-cost-configuration.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
u.log('GREYSTONE_ZERO_COST_SAVED')
