"""Transient close-up source pose review, no asset writes."""
import unreal as u, time, os, json
level=u.get_editor_subsystem(u.LevelEditorSubsystem); editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
u.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic(); stage='start'; index=0; stamp=0; player=None; rows=[]
poses=[('Ability_Q',.18),('Ability_Q',.45),('Ability_Q',.7),('Ability_E',.2),('Ability_E',.55),('Ability_E',.9),
       ('Ability_Ultimate',1.1),('Ability_Ultimate',2.3),('Ability_Ultimate',3.1),('Ability_Ultimate',3.35),('Ability_Ultimate',3.6)]
def tick(delta):
    global stage,index,stamp,player
    try:
        if time.monotonic()-start>100:raise RuntimeError('timeout')
        if stage=='start':level.editor_request_begin_play();stage='wait';return
        if stage=='wait' and level.is_in_play_in_editor() and time.monotonic()-start>7:
            world=editor.get_game_world();player=u.GameplayStatics.get_player_character(world,0)
            if not player:return
            u.UmbraSkillEditorLibrary.prepare_skill_test(world)
            p=player.get_actor_location();player.set_actor_location(u.Vector(p.x-1000,p.y,p.z),False,True)
            player.get_components_by_class(u.SpringArmComponent)[0].set_editor_property('target_arm_length',550.)
            for w in u.WidgetLibrary.get_all_widgets_of_class(world,u.UserWidget,True):w.set_visibility(u.SlateVisibility.COLLAPSED)
            stage='pose'
        if stage=='pose':
            if index>=len(poses):level.editor_request_end_play();stage='finish';return
            name,t=poses[index];a=u.load_asset('/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/'+name)
            player.mesh.play_animation(a,False);player.mesh.set_position(t,False);player.mesh.set_play_rate(0.)
            root=u.AnimationLibrary.get_bone_pose_for_time(a,'root',t,False)
            # Counter the original huge Ultimate root only for this pose inspection.
            player.mesh.set_relative_location(u.Vector(0,0,-90-root.translation.z),False,False)
            rows.append(dict(animation=name,t=t,root=str(root)))
            stamp=time.monotonic();stage='shot'
        if stage=='shot' and time.monotonic()-stamp>.5:
            u.SystemLibrary.execute_console_command(editor.get_game_world(),'Shot SHOWUI')
            u.log('SOURCE_POSE '+str(rows[-1]));stamp=time.monotonic();stage='advance'
        if stage=='advance' and time.monotonic()-stamp>.2:index+=1;stage='pose'
        if stage=='finish' and not level.is_in_play_in_editor():
            with open(os.path.join(u.Paths.project_saved_dir(),'SkillPrototype','motion-review.json'),'w') as f:json.dump(rows,f,indent=2)
            u.unregister_slate_post_tick_callback(callback);u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
    except Exception as e:
        u.log_error('MOTION_REVIEW_FAILED '+str(e));u.unregister_slate_post_tick_callback(callback);u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
callback=u.register_slate_post_tick_callback(tick)
