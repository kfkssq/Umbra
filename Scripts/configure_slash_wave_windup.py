"""Targeted timing-only edit; retain projectile, damage, icon and audio authoring."""
import unreal as u,json,datetime,shutil
from pathlib import Path
root=Path(u.Paths.project_dir()).resolve();out=root/'Saved/SlashWave';out.mkdir(exist_ok=True)
d=u.load_asset('/Game/Skills/Greystone/DA_BreachSlash');assert d and str(d.display_name)=='斩切波'
src=root/'Content/Skills/Greystone/DA_BreachSlash.uasset';backup=out/('DA_BreachSlash-before-windup-'+datetime.datetime.now().strftime('%Y%m%d_%H%M%S')+'.uasset');shutil.copy2(src,backup)
old=dict(play_rate=d.play_rate,strike_time=d.strike_time,windup_end_time=d.windup_end_time,windup_duration=d.windup_duration)
d.set_editor_properties(dict(windup_end_time=.1,windup_duration=.65,play_rate=.65,strike_time=.175))
assert u.EditorAssetLibrary.save_loaded_asset(d,False)
(out/'windup-configuration.json').write_text(json.dumps(dict(previous=old,play_rate=d.play_rate,strike_time=d.strike_time,windup_end_time=d.windup_end_time,windup_duration=d.windup_duration,release_delay=d.get_release_delay(),backup=str(backup)),ensure_ascii=False,indent=2),encoding='utf-8')
u.log('SLASH_WAVE_WINDUP_CONFIGURED')
