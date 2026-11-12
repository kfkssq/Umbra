"""Configure existing sounds through UE; never edits vendor audio. Rerunnable, with backups."""
import unreal as u,json,datetime,shutil
from pathlib import Path
root=Path(u.Paths.project_dir()).resolve();out=root/'Saved/GreystoneAudio';out.mkdir(exist_ok=True)
backup=out/'Backup'/datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
def load(p):
 o=u.load_asset(p);assert o,p;return o
def save(o):
 p=o.get_path_name().split('.')[0];src=root/'Content'/(p.removeprefix('/Game/')+'.uasset')
 if src.exists():
  dst=backup/src.relative_to(root/'Content');dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst)
 assert u.EditorAssetLibrary.save_loaded_asset(o,False)
def create(name,cls,factory):
 p='/Game/Skills/Greystone/Audio/'+name
 return load(p) if u.EditorAssetLibrary.does_asset_exist(p) else u.AssetToolsHelpers.get_asset_tools().create_asset(name,'/Game/Skills/Greystone/Audio',cls,factory)
att=create('ATT_GreystoneSkills',u.SoundAttenuation,u.SoundAttenuationFactory())
s=att.get_editor_property('attenuation');s.set_editor_properties(dict(attenuation_shape_extents=u.Vector(250,0,0),falloff_distance=2200.,spatialize=True));att.set_editor_property('attenuation',s);save(att)
con=create('CON_GreystoneSkills',u.SoundConcurrency,u.SoundConcurrencyFactory())
s=con.get_editor_property('concurrency');s.set_editor_properties(dict(max_count=8,resolution_rule=u.MaxConcurrentResolutionRule.STOP_OLDEST));con.set_editor_property('concurrency',s);save(con)
voice='/Game/ParagonGreystone/Audio/Cues/Greystone_'
attack='/Game/Asset/Audio/Asset/Attack/'
# Effort/voice choices are provisional until subjective listening; no invented original skill mapping.
config=[('BreachSlash','Effort_Swing',None,'swordswing_slow'),('SwordStorm','Effort_Ability_Q',None,'swordswing_fast_02'),('LeapSlam','Effort_JumpHeavy',None,None),('Guard','Effort_Block',None,None),('StoneRecovery',None,'Effort_Ability_Ultimate_Rebirth',None),('BattleFocus',None,'Status_Buffed',None)]
rows=[]
for name,cast,resolved,strike in config:
 d=load('/Game/Skills/Greystone/DA_'+name)
 assert d.resource_cost==0
 d.set_editor_properties(dict(cast_voice=load(voice+cast) if cast else None,resolve_voice=load(voice+resolved) if resolved else None,
  cast_sound=None,strike_sound=load(attack+strike) if strike else None,active_sound=None,skill_attenuation=att,skill_sound_concurrency=con,skill_sound_volume=.65,skill_voice_volume=.65))
 save(d)
 rows.append(dict(skill=name,cast_voice=cast,resolve_voice=resolved,strike_sound=strike,active_sound=None,cost=d.resource_cost))
# Confirmed skill hits previously had no library row. Match by recipient; never play at empty space.
lib=load('/Game/Asset/Audio/Libraries/DA_Audio_Default');entries=list(lib.get_editor_property('events'))
existing=[e for e in entries if e.event==u.UmbraAudioEvent.SKILL_HIT]
if not existing:
 e=u.UmbraAudioEntry();e.set_editor_properties(dict(event=u.UmbraAudioEvent.SKILL_HIT,category=u.UmbraAudioCategory.COMBAT,
 sounds=[load(attack+'sword_hit_01'),load(attack+'sword_hit_02')],spatial=True,volume=.65,probability=1.,cooldown=.1,global_cooldown=.035,attenuation=att,concurrency=con))
 entries.append(e)
lib.set_editor_property('events',entries);save(lib)
# Audit both montage and source sequence notify tracks to expose any duplicate audio routes.
notifies=[]
for name,*_ in config:
 d=load('/Game/Skills/Greystone/DA_'+name);assets=[d.montage]
 for track in d.montage.get_editor_property('slot_anim_tracks'):
  for seg in track.get_editor_property('anim_track').get_editor_property('anim_segments'):
   a=seg.get_editor_property('anim_reference')
   if a and a not in assets:assets.append(a)
 for a in assets:
  events=[]
  for ev in u.AnimationLibrary.get_animation_notify_events(a):
   n=ev.get_editor_property('notify') or ev.get_editor_property('notify_state_class')
   events.append(dict(name=str(ev.get_editor_property('notify_name')),kind=n.get_class().get_name() if n else 'None'))
  notifies.append(dict(asset=a.get_path_name(),events=events))
(out/'configuration.json').write_text(json.dumps(dict(skills=rows,notifies=notifies,backup=str(backup)),ensure_ascii=False,indent=2),encoding='utf-8')
u.log('GREYSTONE_SKILL_AUDIO_CONFIGURED')
