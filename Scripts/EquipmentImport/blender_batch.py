"""Optional Blender adapter: blender --background --python blender_batch.py -- jobs.json
Never saves a blend or overwrites FBX. One input file/collection per equipment piece.
"""
import json
from pathlib import Path
import sys
import traceback
import bpy

def export(job, base):
    source = (base/job['source']).resolve()
    target = (base/job['output']).resolve()
    if target.exists() or source==target:
        raise ValueError('Existing output preserved: '+str(target))
    if source.suffix.lower()=='.blend':
        bpy.ops.wm.open_mainfile(filepath=str(source))
    elif source.suffix.lower()=='.fbx':
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=str(source),use_anim=False)
    else:
        raise ValueError('Expected .blend or .fbx')
    if job.get('collection'):
        collection = bpy.data.collections.get(job['collection'])
        if collection is None:
            raise ValueError('Missing requested collection')
        candidates = list(collection.all_objects)
    else:
        candidates = list(bpy.context.scene.objects)
    objects = [o for o in candidates if o.type in {'MESH','ARMATURE'}]
    meshes = [o for o in objects if o.type=='MESH']
    if not meshes:
        raise ValueError('No mesh objects')
    armatures = set()
    warnings = []
    for obj in meshes:
        armature = obj.find_armature()
        if armature:
            armatures.add(armature)
            if armature not in objects:
                objects.append(armature)
            if not obj.vertex_groups:
                raise ValueError('Skinned mesh has no vertex groups: '+obj.name)
        if not obj.data.uv_layers:
            warnings.append(obj.name+': missing UV')
        if not obj.material_slots or any(s.material is None for s in obj.material_slots):
            raise ValueError('Missing material assignments: '+obj.name)
        for slot in obj.material_slots:
            # Rename only when explicitly mapped; preserve polygon indices and shared materials.
            slot.material.name = job.get('material_names',{}).get(slot.material.name,slot.material.name)
    if len(armatures)>1:
        raise ValueError('Multiple skeleton families; split input explicitly')
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects:
        if any(abs(s-1)>1e-6 for s in obj.scale):
            raise ValueError('Unapplied scale requires DCC review: '+obj.name)
        obj.hide_set(False)
        obj.select_set(True)
    target.parent.mkdir(parents=True,exist_ok=True)
    bpy.ops.export_scene.fbx(filepath=str(target),use_selection=True,object_types={'MESH','ARMATURE'},
        global_scale=1.0,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',
        add_leaf_bones=False,bake_anim=False,use_mesh_modifiers=False)
    return {'source':str(source),'output':str(target),'meshes':[o.name for o in meshes],
            'slots':{o.name:[s.material.name for s in o.material_slots] for o in meshes},
            'unit_scale':bpy.context.scene.unit_settings.scale_length,'warnings':warnings,
            'manual_review':['weights','orientation','special normals','shape keys/modifiers']}

if __name__=='__main__':
    config = Path(sys.argv[sys.argv.index('--')+1]).resolve()
    jobs = json.loads(config.read_text(encoding='utf-8-sig'))
    report = []
    report_path = (config.parent/jobs['report']).resolve()
    report_path.parent.mkdir(parents=True,exist_ok=True)
    for job in jobs['jobs']:
        try:
            report.append({'status':'exported',**export(job,config.parent)})
        except Exception:
            report.append({'status':'failed','source':job.get('source'),'error':traceback.format_exc()})
        report_path.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    if any(r['status']=='failed' for r in report):
        raise RuntimeError('Some exports failed; see '+str(report_path))
