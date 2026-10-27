"""Add one cosmetic swing notify to the configured enemy montage using UE 5.8."""
import datetime
import pathlib
import shutil
import unreal

bp = unreal.load_asset('/Game/Blueprints/Enemies/BP_Enemy_Melee_01')
ability = unreal.get_default_object(unreal.get_default_object(bp.generated_class()).get_editor_property('basic_attack_ability_class'))
montage = ability.get_editor_property('attack_montage')
assert montage.get_path_name().split('.')[0] == '/Game/Blueprints/Enemies/Animation/AM_Enemy_BasicAttack'
api = unreal.AnimationLibrary
before = api.get_animation_notify_events(montage)
existing = [e for e in before if isinstance(e.get_editor_property('notify'), unreal.UmbraAnimNotify_Audio) and e.get_editor_property('notify').get_editor_property('event') == unreal.UmbraAudioEvent.ATTACK_SWING]
assert len(existing) <= 1
if not existing:
    hit = [e for e in before if isinstance(e.get_editor_property('notify_state_class'), unreal.UmbraAnimNotifyState_AttackHitWindow)]
    assert len(hit) == 1
    hit_time = api.get_anim_notify_event_trigger_time(hit[0])
    hit_duration = api.get_anim_notify_event_duration(hit[0])
    # Lead the existing contact window by 60 ms; final perceived sync is a PIE listening check.
    swing_time = max(0.0, hit_time - 0.06)
    root = pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
    relative = montage.get_path_name().split('.')[0].removeprefix('/Game/') + '.uasset'
    backup = root / 'Saved' / 'EnemySwingBackup' / datetime.datetime.now().strftime('%Y%m%d_%H%M%S') / relative
    backup.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(root / 'Content' / relative, backup)
    if not api.is_valid_anim_notify_track_name(montage, 'Audio'):
        api.add_animation_notify_track(montage, 'Audio')
    notify = api.add_animation_notify_event(montage, 'Audio', swing_time, unreal.UmbraAnimNotify_Audio)
    assert notify
    notify.set_editor_property('event', unreal.UmbraAudioEvent.ATTACK_SWING)
    after = api.get_animation_notify_events(montage)
    assert len(after) == len(before) + 1
    preserved = [e for e in after if isinstance(e.get_editor_property('notify_state_class'), unreal.UmbraAnimNotifyState_AttackHitWindow)]
    assert len(preserved) == 1
    assert abs(api.get_anim_notify_event_trigger_time(preserved[0]) - hit_time) < 0.00001
    assert abs(api.get_anim_notify_event_duration(preserved[0]) - hit_duration) < 0.00001
    assert unreal.EditorAssetLibrary.save_loaded_asset(montage, only_if_is_dirty=False)
    unreal.log('ENEMY_SWING_CONFIGURED time=' + str(swing_time) + ' hit=' + str(hit_time) + ' backup=' + str(backup))
else:
    unreal.log('ENEMY_SWING_ALREADY_CONFIGURED')
