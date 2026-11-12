import unreal as u,json
from pathlib import Path
d=u.load_asset('/Game/Skills/Greystone/DA_SwordStorm');r={}
for p in ['play_rate','pulse_interval','strike_time','windup_end_time','windup_duration','active_effect','active_niagara','strike_effect','strike_niagara','cast_socket','active_effect_offset','active_effect_scale','effect_offset','effect_scale']:
 r[p]=str(d.get_editor_property(p))
a=u.load_asset('/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Ability_E');r['animation_length']=a.sequence_length
r['emitters']=u.UmbraSkillEditorLibrary.configure_storm_particle(d.active_effect,0.)
Path(u.Paths.project_saved_dir()+'SwordStorm/fx-audit.json').write_text(json.dumps(r,indent=2),encoding='utf-8')
