"""Optional Blender background adapter: blender --background --python blender_export.py -- job.json.
Job: source_blend, collection, output_fbx, report. Explicit mesh selection; never edits source .blend.
"""
import json
import pathlib
import sys
import bpy

def main():
    job=json.loads(pathlib.Path(sys.argv[sys.argv.index("--")+1]).read_text(encoding="utf-8"))
    source=pathlib.Path(job["source_blend"]).resolve()
    target=pathlib.Path(job["output_fbx"]).resolve()
    if target.exists():
        raise RuntimeError("Existing FBX preserved; choose a new output")
    bpy.ops.wm.open_mainfile(filepath=str(source))
    collection=bpy.data.collections.get(job["collection"])
    if not collection:
        raise RuntimeError("Missing explicit collection")
    objects=[o for o in collection.all_objects if o.type in {"MESH","ARMATURE"}]
    if not objects or not any(o.type=="MESH" for o in objects):
        raise RuntimeError("No mesh objects")
    bpy.ops.object.select_all(action="DESELECT")
    rows=[]
    for obj in objects:
        if any(abs(s-1)>1e-6 for s in obj.scale):
            raise RuntimeError("Unapplied scale: "+obj.name)
        obj.select_set(True)
        if obj.type=="MESH":
            rows.append(dict(name=obj.name,dimensions_scene_units=list(obj.dimensions),vertices=len(obj.data.vertices),
                             uv_layers=len(obj.data.uv_layers),materials=[m.name if m else None for m in obj.data.materials],
                             armature=obj.find_armature().name if obj.find_armature() else None))
    target.parent.mkdir(parents=True,exist_ok=True)
    bpy.ops.export_scene.fbx(filepath=str(target),use_selection=True,object_types={"MESH","ARMATURE"},
        global_scale=1.0,apply_unit_scale=True,axis_forward="-Y",axis_up="Z",add_leaf_bones=False,bake_anim=False)
    pathlib.Path(job["report"]).write_text(json.dumps(dict(source=str(source),output=str(target),
        blender=bpy.app.version_string,unit_scale=bpy.context.scene.unit_settings.scale_length,meshes=rows,
        manual_checks=["grip/orientation", "normals/UV", "weights and skeleton family"]),indent=2),encoding="utf-8")

if __name__=="__main__": main()
