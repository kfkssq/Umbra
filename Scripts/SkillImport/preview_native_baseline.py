"""Render both imported bodies on native animations; no production pawn or map saves."""
import json,time
from pathlib import Path
import unreal as u
out=Path(u.Paths.project_saved_dir()).resolve()/'SkillImport/NativeBaseline'
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
assert level.load_level('/Game/SkillLab/Maps/L_CharacterBaseline')
editor.get_editor_world().get_world_settings().set_editor_property('default_game_mode',u.GameModeBase.static_class())
camera=actors.spawn_actor_from_class(u.CameraActor,u.Vector(480,640,300))
camera.set_actor_rotation(u.MathLibrary.find_look_at_rotation(camera.get_actor_location(),u.Vector(0,100,85)),False)
camera.camera_component.set_editor_property('field_of_view',45.)
camera.set_editor_property('tags',['NativeBaselineCamera'])
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,False)
prefix='/Game/Characters/Mannequins/Anims/Unarmed/'
clips=['MM_Idle','Attack/MM_Attack_01','Attack/MM_Attack_02','Attack/MM_Attack_03','Attack/MM_ChargedAttack',
    'Jog/MF_Unarmed_Jog_Fwd','Walk/MF_Unarmed_Walk_Fwd','Jump/MM_Jump','Jump/MM_Land','Jump/MM_Dash']
skeleton=u.load_asset('/Game/Characters/Mannequins/Meshes/SK_Mannequin')
report={'clips':[],'screenshots':[],'stage':'start','purpose':'Native body/animation baseline, not spell animation acceptance'}
for clip in clips:
    animation=u.load_asset(prefix+clip)
    assert isinstance(animation,u.AnimSequence) and animation.get_editor_property('skeleton')==skeleton
    assert animation.get_play_length()>0
    report['clips'].append(dict(path=animation.get_path_name(),duration=animation.get_play_length()))
samples=[('idle','MM_Idle',.1),('charged','Attack/MM_ChargedAttack',.35),('attack','Attack/MM_Attack_01',.25)]
started=time.monotonic();world=None;subjects=[];at=0
u.EditorPythonScripting.set_keep_python_script_alive(True)
def finish(error=''):
    report.update(result='failed' if error else 'passed',error=error)
    if level.is_in_play_in_editor():level.editor_request_end_play()
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
    (out/'preview.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
def tick(delta):
    global world,subjects,at
    try:
        now=time.monotonic();assert now-started<160,'Preview timeout'
        if report['stage']=='start':
            report['stage']='wait';level.editor_request_begin_play();return
        if report['stage']=='wait' and now-started>10 and level.is_in_play_in_editor():
            world=editor.get_game_world();pc=u.GameplayStatics.get_player_controller(world,0)
            if not pc:return
            cam=next(a for a in u.GameplayStatics.get_all_actors_of_class(world,u.CameraActor) if a.actor_has_tag('NativeBaselineCamera'))
            pc.set_view_target_with_blend(cam,0)
            pawn=u.GameplayStatics.get_player_pawn(world,0)
            if pawn:pawn.set_actor_hidden_in_game(True)
            subjects=u.GameplayStatics.get_all_actors_of_class(world,u.SkeletalMeshActor)
            assert len(subjects)==2
            report['stage']='pose'
        if report['stage']=='pose':
            if not samples:finish();return
            name,clip,t=samples.pop(0)
            for actor in subjects:
                component=actor.skeletal_mesh_component
                component.play_animation(u.load_asset(prefix+clip),False)
                component.set_play_rate(0);component.set_position(t,False)
            report['current']=name;at=now;report['stage']='capture'
        elif report['stage']=='capture' and now-at>2:
            image=out/('baseline-'+report['current']+'.png')
            report['screenshots'].append(str(image));report['stage']='next';at=now
            u.SystemLibrary.execute_console_command(world,'HighResShot 1280x720 filename="'+image.as_posix()+'"')
        elif report['stage']=='next' and now-at>3:
            assert Path(report['screenshots'][-1]).exists(),'Missing screenshot'
            report['stage']='pose'
    except Exception as error:finish(str(error))
callback=u.register_slate_post_tick_callback(tick)
