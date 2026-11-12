"""Copy native storm FX into project-owned loop; preserve native animation and facing."""
import unreal as u,json,datetime,shutil
from pathlib import Path
root=Path(u.Paths.project_dir()).resolve();out=root/'Saved/SwordStorm';out.mkdir(exist_ok=True)
backup=out/'Backup'/datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
def save(o):
 src=root/'Content'/(o.get_path_name().split('.')[0].removeprefix('/Game/')+'.uasset')
 if src.exists():
  dst=backup/src.relative_to(root/'Content');dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst)
 assert u.EditorAssetLibrary.save_loaded_asset(o,False),o.get_path_name()
d=u.load_asset('/Game/Skills/Greystone/DA_SwordStorm');a=u.load_asset('/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Ability_E')
p='/Game/Skills/Greystone/FX/P_SwordStorm_Channel'
r=dict(source=d.active_effect.get_path_name(),animation_length=a.sequence_length,play_rate=d.play_rate,old_interval=d.pulse_interval,old_offset=str(d.active_effect_offset),old_strike=str(d.strike_effect))
r['attachment_modules']=u.UmbraSkillEditorLibrary.describe_particle_system(d.active_effect)
original=u.load_asset('/Game/ParagonGreystone/FX/Particles/Greystone/Abilities/ClearAPath/FX/P_Greystone_SwordSpinning_01')
r['before']=u.UmbraSkillEditorLibrary.configure_storm_particle(original,0.)
fx=u.load_asset(p) if u.EditorAssetLibrary.does_asset_exist(p) else u.EditorAssetLibrary.duplicate_asset(original.get_path_name().split('.')[0],p)
assert fx
u.UmbraSkillEditorLibrary.configure_storm_particle(fx,a.sequence_length);save(fx)
hp='/Game/Skills/Greystone/FX/P_SwordStorm_HitSparks'
hit=u.load_asset(hp) if u.EditorAssetLibrary.does_asset_exist(hp) else u.EditorAssetLibrary.duplicate_asset(original.get_path_name().split('.')[0],hp)
assert u.UmbraSkillEditorLibrary.configure_storm_hit_particle(hit);save(hit)
d.set_editor_properties(dict(active_effect=fx,sync_storm_to_animation=True,storm_pulses_per_cycle=1,strike_effect=hit,strike_niagara=None,effect_offset=u.Vector(0,0,0),effect_scale=u.Vector(.2,.2,.2)))
save(d)
r['after']=u.UmbraSkillEditorLibrary.configure_storm_particle(fx,0.);r['interval']=a.sequence_length/d.play_rate
r['hit_emitters']=u.UmbraSkillEditorLibrary.configure_storm_particle(hit,0.)
assert not any('Sparks' in line and 'enabled=1' in line for line in r['after'].splitlines()),'Saved channel still contains sparks'
assert all('Sparks' in line for line in r['hit_emitters'].splitlines() if 'enabled=1' in line),'Saved hit effect contains non-spark layers'
(out/'fx-configuration.json').write_text(json.dumps(r,indent=2),encoding='utf-8')
u.log('STORM_FX_CONFIGURED')
