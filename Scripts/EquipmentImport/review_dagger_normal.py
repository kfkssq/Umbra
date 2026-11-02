"""Isolated direct-normal import and offscreen UE comparisons; no existing maps saved."""
import json
import os
from pathlib import Path
import sys
import time
import traceback
import unreal as u
sys.path.insert(0, str(Path(__file__).resolve().parent))
from pipeline import run

project = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
out = project / 'Saved/EquipmentImport/DaggerNormal'
result = run(out / 'manifest.json', out / 'import.json', apply=True)
assert result['ok'], result
row = result['items'][0]
mesh = u.load_asset(row['outputs'][0]['asset'])
mi = u.load_asset(next(iter(row['outputs'][0]['slots'].values())))
ml = u.MaterialEditingLibrary
normal = ml.get_material_instance_texture_parameter_value(mi, 'Normal')
assert normal.compression_settings == u.TextureCompressionSettings.TC_NORMALMAP
assert not normal.srgb and not normal.flip_green_channel
assert not list(ml.recompile_material(mi.parent))
stats = ml.get_statistics(mi)
assert stats.num_samplers > 0
world = u.EditorLoadingAndSavingUtils.new_blank_map(False)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
actor = actors.spawn_actor_from_class(u.StaticMeshActor, u.Vector())
actor.static_mesh_component.set_static_mesh(mesh)
center, ext = actor.get_actor_bounds(False)
light = actors.spawn_actor_from_class(u.DirectionalLight, u.Vector(0,0,200), u.Rotator(-35,-35,0))
light.light_component.set_intensity(.2)
fill = actors.spawn_actor_from_class(u.DirectionalLight, u.Vector(0,0,100), u.Rotator(25,145,0))
fill.light_component.set_intensity(.075)
sky = actors.spawn_actor_from_class(u.SkyLight, u.Vector())
sky.light_component.set_editor_property('source_type', u.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
sky.light_component.set_editor_property('cubemap', u.load_asset('/Engine/MapTemplates/Sky/DaylightAmbientCubemap'))
sky.light_component.set_intensity(.05)
sky.light_component.recapture_sky()
key = actors.spawn_actor_from_class(u.PointLight, center + u.Vector(40,-100,60))
key.point_light_component.set_intensity(5)
key.point_light_component.set_attenuation_radius(1000)
capture = actors.spawn_actor_from_class(u.SceneCapture2D, u.Vector())
comp = capture.capture_component2d
comp.set_editor_property('capture_every_frame', False)
comp.set_editor_property('capture_on_movement', False)
rt = u.RenderingLibrary.create_render_target2d(world, 1024, 1024, u.TextureRenderTargetFormat.RTF_RGBA8)
comp.set_editor_property('texture_target', rt)
comp.set_editor_property('projection_type', u.CameraProjectionMode.ORTHOGRAPHIC)
comp.set_editor_property('ortho_width', max(ext.x, ext.y, ext.z)*2.5)
pp = comp.get_editor_property('post_process_settings')
pp.set_editor_property('override_auto_exposure_bias', True)
pp.set_editor_property('auto_exposure_bias', -9)
pp.set_editor_property('override_bloom_intensity', True)
pp.set_editor_property('bloom_intensity', 0)
pp.set_editor_property('override_auto_exposure_apply_physical_camera_exposure', True)
pp.set_editor_property('auto_exposure_apply_physical_camera_exposure', False)
pp.set_editor_property('override_auto_exposure_method', True)
pp.set_editor_property('auto_exposure_method', u.AutoExposureMethod.AEM_MANUAL)
comp.set_editor_property('post_process_settings', pp)
comp.set_editor_property('post_process_blend_weight', 1.0)
flat = actor.static_mesh_component.create_dynamic_material_instance(0, mi)
flat.set_texture_parameter_value('Normal', u.load_asset('/Engine/EngineMaterials/DefaultNormal'))
direct = actor.static_mesh_component.create_dynamic_material_instance(0, mi)
# Diagnostic diffuse lighting isolates the normal response from metallic environment reflections.
mode_prefix=os.environ.get('UMBRA_NORMAL_REVIEW_MODE','diagnostic')
if mode_prefix=='diagnostic':
    for material in (flat,direct):
        material.set_scalar_parameter_value('Metallic',0)
        material.set_scalar_parameter_value('Roughness',.65)
variants = {'flat': flat, 'direct': direct}
report = {'diagnostic_only_overrides': {'Metallic':0,'Roughness':.65} if mode_prefix=='diagnostic' else {}, 'normal': normal.get_path_name(), 'srgb': normal.srgb, 'compression': str(normal.compression_settings),
          'shader_samplers': stats.num_samplers, 'bounds': {'center': str(center), 'extent': str(ext)}, 'images': []}
jobs = [(view,direction,variant,material,mode,source)
        for view,direction in [('front',u.Vector(0,-1,0)),('angled',u.Vector(.4,-1,.3))]
        for variant,material in variants.items()
        for mode,source in [('lit',u.SceneCaptureSource.SCS_FINAL_COLOR_LDR),('normal',u.SceneCaptureSource.SCS_NORMAL)]]
u.EditorPythonScripting.set_keep_python_script_alive(True)
state = {'index':0, 'phase':0, 'next':time.monotonic()+8}
def finish():
    (out / (mode_prefix+'_render.json')).write_text(json.dumps(report,indent=2),encoding='utf-8')
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()
def tick(delta):
    if time.monotonic()<state['next']: return
    try:
        if state['index'] == len(jobs):
            finish(); return
        view,direction,variant,material,mode,source = jobs[state['index']]
        if state['phase']==0:
            position=center+direction*(max(ext.x,ext.y,ext.z)*5+100)
            capture.set_actor_location(position,False,False)
            capture.set_actor_rotation(u.MathLibrary.find_look_at_rotation(position,center),False)
            actor.static_mesh_component.set_material(0,material)
            comp.set_editor_property('capture_source',source)
            state['phase']=1
        elif state['phase']==1:
            comp.capture_scene(); state['phase']=2
        else:
            name=f'{mode_prefix}_{view}_{variant}_{mode}.png'
            u.RenderingLibrary.export_render_target(world,rt,str(out),name)
            report['images'].append(name)
            state['index']+=1; state['phase']=0
        state['next']=time.monotonic()+1
    except Exception:
        report['error']=traceback.format_exc(); finish()
callback=u.register_slate_post_tick_callback(tick)
