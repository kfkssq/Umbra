"""Align release with the inspected forward sword pose; preserve duration/rate and gameplay tuning."""
import unreal as u,json,shutil,datetime
from pathlib import Path
root=Path(u.Paths.project_dir()).resolve();out=root/'Saved/SlashWave'
d=u.load_asset('/Game/Skills/Greystone/DA_BreachSlash');assert d and str(d.display_name)=='斩切波'
old=dict(strike_time=d.strike_time,windup_end_time=d.windup_end_time,windup_duration=d.windup_duration,play_rate=d.play_rate)
backup=out/('DA_BreachSlash-before-mid-swing-'+datetime.datetime.now().strftime('%Y%m%d_%H%M%S')+'.uasset');shutil.copy2(root/'Content/Skills/Greystone/DA_BreachSlash.uasset',backup)
d.set_editor_properties(dict(windup_end_time=.1,strike_time=.175))
assert u.EditorAssetLibrary.save_loaded_asset(d,False)
(out/'mid-swing-configuration.json').write_text(json.dumps(dict(previous=old,strike_time=d.strike_time,windup_end_time=d.windup_end_time,windup_duration=d.windup_duration,play_rate=d.play_rate,release_delay=d.get_release_delay()),indent=2),encoding='utf-8')
u.log('SLASH_WAVE_MID_SWING_CONFIGURED')
