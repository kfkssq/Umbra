"""Fresh-editor readback and sequential isolated previews for successful batch test items."""
import json
import os
from pathlib import Path
import time
import traceback
import unreal as u

folder=Path(os.environ['UMBRA_BATCH_REPORT']).parent
batch=json.loads(Path(os.environ['UMBRA_BATCH_REPORT']).read_text(encoding='utf-8'))
plan=json.loads((folder/'plan.json').read_text(encoding='utf-8'))
sources={s['item']['source_id']:s['item'] for s in plan['items']}
jobs=[r for r in batch['items'] if r['ue']['status'] in ('created','preserved','asset_only')]
output=folder/'Previews';output.mkdir(exist_ok=True)
preview_mode=os.environ.get('UMBRA_PREVIEW_MODE','lit')
report={'items':[],'ok':False,'visual_certification':'inspection required; test lighting is not original-game lighting'}
world=u.EditorLoadingAndSavingUtils.new_blank_map(False)
u.SystemLibrary.execute_console_command(world,'r.TextureStreaming 0')
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
light=actors.spawn_actor_from_class(u.DirectionalLight,u.Vector(0,0,200),u.Rotator(-35,-35,0))
light.light_component.set_intensity(3)
fill=actors.spawn_actor_from_class(u.DirectionalLight,u.Vector(0,0,200),u.Rotator(25,145,0))
fill.light_component.set_intensity(1)
sky=actors.spawn_actor_from_class(u.SkyLight,u.Vector())
sky.light_component.set_editor_property('source_type',u.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
sky.light_component.set_editor_property('cubemap',u.load_asset('/Engine/MapTemplates/Sky/DaylightAmbientCubemap'))
sky.light_component.set_intensity(.5);sky.light_component.recapture_sky()
key=actors.spawn_actor_from_class(u.PointLight,u.Vector())
key.point_light_component.set_intensity(200);key.point_light_component.set_attenuation_radius(1000)
capture=actors.spawn_actor_from_class(u.SceneCapture2D,u.Vector())
comp=capture.capture_component2d
comp.set_editor_property('capture_every_frame',False);comp.set_editor_property('capture_on_movement',False)
rt=u.RenderingLibrary.create_render_target2d(world,768,768,u.TextureRenderTargetFormat.RTF_RGBA8)
comp.set_editor_property('texture_target',rt)
comp.set_editor_property('projection_type',u.CameraProjectionMode.ORTHOGRAPHIC)
comp.set_editor_property('capture_source',u.SceneCaptureSource.SCS_BASE_COLOR if preview_mode=='basecolor' else u.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
pp=comp.get_editor_property('post_process_settings')
for k,v in {'override_auto_exposure_method':True,'auto_exposure_method':u.AutoExposureMethod.AEM_MANUAL,
            'override_auto_exposure_apply_physical_camera_exposure':True,'auto_exposure_apply_physical_camera_exposure':False,
            'override_auto_exposure_bias':True,'auto_exposure_bias':0,'override_bloom_intensity':True,'bloom_intensity':0}.items():
    pp.set_editor_property(k,v)
comp.set_editor_property('post_process_settings',pp)
comp.set_editor_property('post_process_blend_weight',1)
state={'index':0,'phase':0,'view':0,'next':time.monotonic()+8,'actor':None}
u.EditorPythonScripting.set_keep_python_script_alive(True)

def finish():
    report['ok']=all(not r.get('error') for r in report['items']) and len(report['items'])==len(jobs)
    (folder/('acceptance_basecolor.json' if preview_mode=='basecolor' else 'acceptance.json')).write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    comp.set_editor_property('texture_target',None)
    u.RenderingLibrary.release_render_target2d(rt)
    for actor in (state.get('actor'),capture,light,fill,sky,key):
        if actor:actors.destroy_actor(actor)
    # Let queued render-resource releases finish before editor shutdown.
    state['quitting']=True;state['next']=time.monotonic()+3

def tick(delta):
    if time.monotonic()<state['next']:return
    if state.get('quitting'):
        u.unregister_slate_post_tick_callback(callback)
        u.EditorPythonScripting.set_keep_python_script_alive(False)
        u.SystemLibrary.quit_editor()
        return
    try:
        if state['index']==len(jobs):finish();return
        row=jobs[state['index']]; paths=row['ue']
        if state['phase']==0:
            check={'source_id':row['source_id'],'name':row['name'],'images':[]}
            report['items'].append(check)
            mesh=u.load_asset(paths['mesh']);icon=u.load_asset(paths['icon'])
            assert isinstance(icon,u.Texture2D) and icon.srgb
            assert icon.compression_settings==u.TextureCompressionSettings.TC_EDITOR_ICON
            if 'draft' in paths:
                draft=u.load_asset(paths['draft'])
                assert str(draft.get_editor_property('display_name'))==row['name']
                assert draft.get_editor_property('icon')==icon
                reference='world_drop_skeletal_mesh' if isinstance(mesh,u.SkeletalMesh) else 'world_drop_mesh'
                assert draft.get_editor_property(reference)==mesh
                raw=json.loads(u.EditorAssetLibrary.get_metadata_tag(draft,'UmbraItemImport.Source'))
                assert raw['item']['raw_params']==sources[row['source_id']]['raw_params']
            skeletal=isinstance(mesh,u.SkeletalMesh)
            if skeletal: assert mesh.skeleton
            slots=mesh.get_editor_property('materials' if skeletal else 'static_materials')
            assert slots and all(s.material_interface for s in slots)
            check['material_slots']=len(slots);check['normal_bindings']=[]
            for s in slots:
                n=u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(s.material_interface,'Normal')
                check['normal_bindings'].append(n.get_path_name() if n else None)
                if n and n.get_path_name().startswith('/Game/'):
                    assert not n.srgb and n.compression_settings==u.TextureCompressionSettings.TC_NORMALMAP
            actor=actors.spawn_actor_from_class(u.SkeletalMeshActor if skeletal else u.StaticMeshActor,u.Vector())
            component=actor.skeletal_mesh_component if skeletal else actor.static_mesh_component
            if skeletal:component.set_skeletal_mesh(mesh)
            else:component.set_static_mesh(mesh)
            state['actor']=actor
            center,extent=actor.get_actor_bounds(False)
            assert max(extent.x,extent.y,extent.z)>0
            state['center'],state['extent']=center,extent
            check['bounds_extent_cm']=[extent.x,extent.y,extent.z]
            state['phase']=1
        elif state['phase']==1:
            center,extent=state['center'],state['extent']
            axis=min(range(3),key=lambda i:[extent.x,extent.y,extent.z][i])
            direction=[0.,0.,0.];direction[axis]=-1
            if state['view']==1:direction[(axis+1)%3]=.35;direction[(axis+2)%3]=.2
            distance=max(extent.x,extent.y,extent.z)*5+100
            position=center+u.Vector(*direction)*distance
            capture.set_actor_location(position,False,False)
            capture.set_actor_rotation(u.MathLibrary.find_look_at_rotation(position,center),False)
            comp.set_editor_property('ortho_width',max(extent.x,extent.y,extent.z)*2.5)
            key.set_actor_location(center+u.Vector(*direction)*100+u.Vector(30,0,40),False,False)
            state['phase']=2
        elif state['phase']==2:
            comp.capture_scene();state['phase']=3
        else:
            name=f"{row['archive']}_{'basecolor_' if preview_mode=='basecolor' else ''}{state['view']}.png"
            u.RenderingLibrary.export_render_target(world,rt,str(output),name)
            report['items'][-1]['images'].append(str(output/name))
            if state['view']==0:state['view']=1;state['phase']=1
            else:
                actors.destroy_actor(state['actor']);state['actor']=None
                state.update(index=state['index']+1,phase=0,view=0)
                u.SystemLibrary.collect_garbage()
        state['next']=time.monotonic()+1
    except Exception:
        if report['items']:report['items'][-1]['error']=traceback.format_exc()
        else:report['error']=traceback.format_exc()
        finish()

callback=u.register_slate_post_tick_callback(tick)
