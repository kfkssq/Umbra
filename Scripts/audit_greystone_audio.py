import unreal as u,json
from pathlib import Path
r=u.AssetRegistryHelpers.get_asset_registry();r.search_all_assets(True)
out=Path(u.Paths.project_saved_dir())/'GreystoneAudio';out.mkdir(exist_ok=True)
rows=[]
for a in r.get_assets_by_path('/Game',True):
 c=str(a.asset_class_path.asset_name)
 if c not in ['SoundWave','SoundCue','MetaSoundSource']:continue
 o=a.get_asset();row=dict(path=str(a.package_name),kind=c,duration=o.get_editor_property('duration'),dependencies=[str(p) for p in r.get_dependencies(a.package_name,u.AssetRegistryDependencyOptions())])
 if c=='SoundWave':
  for p in ['looping','subtitles']:
   try:row[p]=str(o.get_editor_property(p))
   except Exception:pass
 rows.append(row)
lib=u.load_asset('/Game/Asset/Audio/Libraries/DA_Audio_Default')
report=dict(audio=rows,library=str(lib.get_editor_property('events')))
(out/'audit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('GREYSTONE_AUDIO_AUDIT_DONE')


# Fresh-process readback of the installed skill configuration (no asset saves).
report['skills']=[]
for name in ['BreachSlash','SwordStorm','LeapSlam','Guard','StoneRecovery','BattleFocus']:
 d=u.load_asset('/Game/Skills/Greystone/DA_'+name)
 report['skills'].append(dict(skill=name,cost=d.resource_cost,cast_voice=str(d.cast_voice),resolve_voice=str(d.resolve_voice),strike_sound=str(d.strike_sound),active_sound=str(d.active_sound),attenuation=str(d.skill_attenuation),concurrency=str(d.skill_sound_concurrency)))
(out/'audit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
