"""Unattended empty-library PIE smoke. No assets are saved; no audible output is asserted."""
import json
import os
import time
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)

level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
started = time.monotonic()
state = {"requested": False, "checked": False, "ended": False, "result": "pending"}

def finish(result, detail):
    state["result"] = result
    state["detail"] = detail
    with open(os.path.join(unreal.Paths.project_saved_dir(), "AudioPIESmoke.json"), "w", encoding="utf-8") as stream:
        json.dump(state, stream, indent=2)
    unreal.log("AUDIO_PIE_SMOKE " + result + " " + detail)
    unreal.unregister_slate_post_tick_callback(callback)
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    unreal.SystemLibrary.quit_editor()

def tick(delta):
    try:
        age = time.monotonic() - started
        if age > 90:
            level.editor_request_end_play()
            finish("failed", "PIE startup or shutdown timed out")
            return
        if not state["requested"]:
            state["requested"] = True
            level.editor_request_begin_play()
            return
        if level.is_in_play_in_editor() and not state["checked"]:
            world = editor.get_game_world()
            if not world:
                return
            player = unreal.GameplayStatics.get_player_controller(world, 0)
            pawn = unreal.GameplayStatics.get_player_pawn(world, 0) if player else None
            if not pawn:
                return
            component = pawn.get_component_by_class(unreal.UmbraAudioComponent)
            if not component:
                raise RuntimeError("PIE player has no native audio component")
            context = component.get_context()
            for event in (unreal.UmbraAudioEvent.UI_HOVER, unreal.UmbraAudioEvent.MENU_OPEN, unreal.UmbraAudioEvent.PICKUP_FAILED):
                unreal.UmbraAudioFunctionLibrary.play_ui(player, event, context)
            component.play_local(unreal.UmbraAudioEvent.ATTACK_SWING, context, pawn.get_actor_location())
            state["checked"] = True
            state["checked_at"] = age
            state["pawn"] = pawn.get_class().get_name()
        if state["checked"] and not state["ended"] and age - state["checked_at"] > 5:
            level.editor_request_end_play()
            state["ended"] = True
        if state["ended"] and not level.is_in_play_in_editor():
            finish("passed", "Prototype PIE, native component, empty-library UI/world calls, five seconds and clean EndPIE")
    except Exception as error:
        level.editor_request_end_play()
        finish("failed", str(error))

callback = unreal.register_slate_post_tick_callback(tick)
