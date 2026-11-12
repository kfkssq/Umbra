"""Read-only installed prototype verification in a fresh matching UE process."""
import unreal as u,json
from pathlib import Path
out=Path(u.Paths.project_saved_dir())/'SkillTargeting'
s=u.get_default_object(u.load_asset('/Game/Blueprints/Player/BP_UmbraPlayerState').generated_class())
slots=list(s.initial_skill_slots);d=slots[7].definition
assert str(d.skill_id)=='TargetLeapPrototype' and d.targeted_leap and d.resource_cost==0
assert all(not x.definition.targeted_leap for x in slots[:6])
imc=u.load_asset('/Game/Input/IMC_Default');maps=imc.get_editor_property('default_key_mappings').get_editor_property('mappings')
bound=[str(m.get_editor_property('key').get_editor_property('key_name')) for m in maps if m.action and m.action.get_name()=='IA_Skill8'];assert bound==['Eight'],bound
report=dict(slots=[dict(index=i+1,definition=x.definition.get_path_name() if x.definition else None,key=str(x.key_label),cost=x.definition.resource_cost if x.definition else None) for i,x in enumerate(slots)],range=d.leap_distance,radius=d.radius,material=d.target_indicator_material.get_path_name(),curve=d.target_leap_curve.get_path_name(),input=bound)
(out/'readback.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('TARGETING_READBACK_PASSED')
