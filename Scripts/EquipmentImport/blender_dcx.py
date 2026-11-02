"""Background adapter for the user's installed FLVER Importer. Never edits that plugin.
Blender --background --factory-startup --python-exit-code 1 --python this.py -- job.json
"""
import importlib
import hashlib
import json
from pathlib import Path
import re
import shutil
import sys
import traceback
import bpy
sys.path.insert(0,str(Path(__file__).resolve().parent))
from material_channels import decode_rg_gloss
from normal_rules import validate_direct_normal
from matbin_rules import resolve_material
from flver_layout import primary_vertex_channels

def save_derived(path,pixels,size):
    import numpy as np
    if path.exists(): raise ValueError('Derived output exists: '+str(path))
    image=bpy.data.images.new(path.stem,width=size[0],height=size[1],alpha=True)
    image.colorspace_settings.name='Non-Color'; image.alpha_mode='CHANNEL_PACKED'
    image.pixels.foreach_set(pixels.reshape(-1))
    image.filepath_raw=str(path); image.file_format='PNG'; image.save()
    bpy.data.images.remove(image)
    saved=bpy.data.images.load(str(path),check_existing=False)
    saved.colorspace_settings.name='Non-Color'; saved.alpha_mode='CHANNEL_PACKED'
    actual=np.array(saved.pixels[:],dtype=np.float32).reshape(-1,4)
    bpy.data.images.remove(saved)
    if actual.shape!=pixels.shape or np.max(np.abs(actual-pixels))>1/255+1e-5:
        raise ValueError('Derived PNG quantization check failed')

def identifier(value):
    return re.sub('[^A-Za-z0-9_]', '_', value)

def main(job_path):
    job = json.loads(Path(job_path).read_text(encoding='utf-8-sig'))
    addon = Path(job['addon']).resolve()
    sys.path.insert(0,str(addon.parent))
    importer = importlib.import_module(addon.name+'.importer')
    read_flver = importlib.import_module(addon.name+'.flver_utils').read_flver
    unpacked = Path(job['unpacked']).resolve()
    output = Path(job['output']).resolve()
    output.mkdir(parents=True,exist_ok=True)
    report = {'blender':bpy.app.version_string,'models':[],'textures':[],'warnings':[], 'items':[]}
    textures = {}
    derived = {}
    material_index=json.loads(Path(job['material_index']).read_text(encoding='utf-8')) if job.get('material_index') else None
    shared={}
    if job.get('shared_textures'):
        shared_report=json.loads(Path(job['shared_textures']).read_text(encoding='utf-8'))
        for texture in shared_report['textures']:
            key=texture['name'].casefold()
            if key in shared:raise ValueError('Duplicate shared texture: '+key)
            if texture.get('error'):raise ValueError('Invalid shared texture conversion: '+key)
            shared[key]=texture
    def get_texture(key):
        if key in textures:return textures[key]
        if key not in shared:return None
        original=shared[key]
        target=output/Path(original['output']).name
        if target.exists():raise ValueError('Shared texture output collision')
        shutil.copy2(original['output'],target)
        texture={**original,'output':str(target),'shared_source':original['output']}
        textures[key]=texture;report['textures'].append(texture)
        return texture
    # Convert each source independently, retaining RGBA. Never infer a normal encoding from _n.
    for dds in sorted(unpacked.rglob('*.dds')):
        record={'source':str(dds),'name':dds.stem}
        report['textures'].append(record)
        try:
            key=dds.stem.casefold()
            if key in textures:
                raise ValueError('Duplicate texture name in archive: '+dds.stem)
            image=bpy.data.images.load(str(dds),check_existing=False)
            image.colorspace_settings.name='Non-Color'
            image.alpha_mode='CHANNEL_PACKED'
            size=list(image.size)  # Force Blender's lazy image decoding before has_data.
            if size[0]==0 or not image.has_data:
                raise ValueError('Blender could not decode DDS')
            import numpy as np
            pixels=np.empty(len(image.pixels),dtype=np.float32)
            image.pixels.foreach_get(pixels)
            rgba=pixels.reshape(-1,4)
            if not np.isfinite(rgba).all() or rgba.min() < 0 or rgba.max() > 1.00001:
                raise ValueError('HDR/out-of-range DDS requires an explicit non-PNG conversion rule')
            record.update(size=list(image.size), channel_min=rgba.min(axis=0).tolist(),channel_max=rgba.max(axis=0).tolist())
            target=output/(dds.stem+'.png')
            if target.exists():
                raise ValueError('Output already exists: '+str(target))
            image.filepath_raw=str(target)
            image.file_format='PNG'
            image.save()
            bpy.data.images.remove(image)
            # Verify decoded channel preservation rather than assuming Blender's color settings.
            saved=bpy.data.images.load(str(target),check_existing=False)
            saved.colorspace_settings.name='Non-Color'
            saved.alpha_mode='CHANNEL_PACKED'
            check_pixels=np.empty(len(saved.pixels),dtype=np.float32)
            saved.pixels.foreach_get(check_pixels)
            if len(check_pixels)!=len(pixels): raise ValueError('PNG pixel count changed')
            error=float(np.max(np.abs(check_pixels-pixels)))
            record['png_max_channel_error']=error
            bpy.data.images.remove(saved)
            if error>1.0/255+1e-5: raise ValueError('PNG color/alpha changed beyond 8-bit quantization')
            record['output']=str(target)
            textures[key]=record
            if key.endswith('_m'):
                report['warnings'].append(dds.name+': channels preserved, unknown mask semantics; not connected')
        except Exception as exc:
            record['error']=str(exc)

    if job.get('textures_only'):
        (output/'conversion.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        if not textures or any(t.get('error') for t in report['textures']):raise ValueError('Shared texture conversion incomplete')
        return

    for flver in sorted(unpacked.rglob('*.flver')):
        row={'source':str(flver),'materials':[],'warnings':[]}
        report['models'].append(row)
        try:
            bpy.ops.wm.read_factory_settings(use_empty=True)
            data=read_flver(flver)
            material_specs={}
            used={m.material_index for m in data.meshes}
            for index in sorted(used):
                mat=data.materials[index]
                slot='Slot_'+str(index).zfill(3)
                record={'index':index,'slot':slot,'name':mat.name,'template':mat.mtd_path,
                        'references':[vars(t) for t in data.textures[mat.texture_index:mat.texture_index+mat.texture_count]]}
                row['materials'].append(record)
                resolved=resolve_material(record,material_index) if material_index else None
                if resolved:record['source_material']=resolved
                # Rules are explicit and recorded; missing templates never silently become metal.
                match=re.search(r'P\[([^\]]+)\](?:_?([^\\/]+?))?\.(?:matxml|mtd)$',mat.mtd_path,re.I)
                if not match and not resolved:
                    raise ValueError('Unrecognized material template: '+mat.mtd_path)
                stem,kind=match.groups() if match else ('',''); kind=kind or ''
                override=job['rules'].get('materials',{}).get(mat.mtd_path)
                override=override or job['rules'].get('material_files',{}).get(Path(mat.mtd_path.replace('\\','/')).name.casefold())
                rule=override or job['rules'].get('templates',{}).get(kind.casefold())
                if not rule and resolved:
                    rule={'surface':'Opaque','preset':'neutral','normal_encoding':'er_rg_direct'}
                    row['warnings'].append(slot+': source shader resolved, material class unclassified; neutral PBR preview preset only')
                if not rule:
                    raise ValueError('No reviewed template rule for '+kind)
                tex_stem=rule.get('texture_stem',stem+rule.get('texture_suffix',''))
                refs=[] if resolved else [t for t in record['references'] if t['path'] and 'albedo' in t['type_name'].lower()]
                explicit={Path(t['path'].replace('\\','/')).stem.casefold() for t in refs}
                if explicit:
                    if len(explicit)!=1 or next(iter(explicit))!=(tex_stem+'_a').casefold():
                        raise ValueError('Explicit FLVER albedo references conflict with template rule: '+str(sorted(explicit)))
                base_key=resolved['base_key'] if resolved else (tex_stem+'_a').casefold()
                normal_key=resolved['normal_key'] if resolved else (tex_stem+'_n').casefold()
                albedo=get_texture(base_key)
                if not albedo:
                    raise ValueError('Missing/ambiguous BaseColor: '+base_key)
                alpha=rule.get('alpha_policy') or (resolved['alpha_policy'] if resolved else None)
                if albedo['channel_min'][3] < .999 and alpha not in ('ignore','mask'):
                    raise ValueError('Nonopaque BaseColor alpha requires explicit rule: '+tex_stem)
                spec={'base_color':str(Path(albedo['output']).relative_to(output)),
                      'surface':rule['surface'],'preset':rule['preset'],'alpha_policy':alpha or 'ignore'}
                if resolved:
                    spec.update(two_sided=resolved['two_sided'])
                    if alpha=='mask':spec.update(surface='Masked',alpha_clip=resolved['alpha_clip'])
                    row['warnings'].append(slot+': MATBIN primary textures/AlphaRef resolved; detail, masks and secondary AlphaRef remain unimplemented')
                normal=get_texture(normal_key)
                if resolved and not normal:raise ValueError('Missing primary normal texture: '+normal_key)
                if normal:
                    spec.update(normal=str(Path(normal['output']).relative_to(output)),normal_encoding=rule.get('normal_encoding','unknown'))
                    if spec['normal_encoding']=='er_rg_direct':
                        normal_record={'references':[resolved['normal_reference']]} if resolved else record
                        normal_stem=normal_key[:-2] if resolved and normal_key.endswith('_n') else tex_stem
                        record['normal_binding']=validate_direct_normal(normal_record,normal,albedo,normal_stem)
                        if record['normal_binding']['unimplemented_detail_layers']:
                            row['warnings'].append(slot+': detail normal layer retained in conversion report, not blended; primary normal bound')
                        row['warnings'].append(tex_stem+': ordinary ER RG normal bound directly; B/A unused; per-asset visual fidelity not certified')
                    if spec['normal_encoding']=='er_rg_gloss_candidate':
                        expected=rule.get('normal_dds_sha256')
                        actual=hashlib.sha256(Path(normal['source']).read_bytes()).hexdigest()
                        if not expected or expected!=actual:
                            raise ValueError('Packed normal rule requires matching audited DDS hash')
                        if type(rule.get('flip_green')) is not bool:
                            raise ValueError('Packed normal rule must explicitly declare flip_green')
                        key=(normal['name'],rule['flip_green'])
                        if key not in derived:
                            source_image=bpy.data.images.load(normal['output'],check_existing=False)
                            source_image.colorspace_settings.name='Non-Color'; source_image.alpha_mode='CHANNEL_PACKED'
                            size=list(source_image.size)
                            p=np.array(source_image.pixels[:],dtype=np.float32).reshape(-1,4)
                            n,r,metrics=decode_rg_gloss(p,rule['flip_green'])
                            bpy.data.images.remove(source_image)
                            suffix='_FlipG' if rule['flip_green'] else ''
                            n_path=output/(tex_stem+'_ue_normal'+suffix+'.png')
                            r_path=output/(tex_stem+'_ue_roughness'+suffix+'.png')
                            save_derived(n_path,n,size); save_derived(r_path,r,size)
                            derived[key]=(n_path.name,r_path.name,metrics)
                        n_name,r_name,metrics=derived[key]
                        spec.update(normal=n_name,normal_encoding='standard_dx',roughness_texture=r_name)
                        record['channel_conversion']={'encoding':'er_rg_gloss_candidate','source_sha256':actual,**metrics}
                        row['warnings'].append(tex_stem+': audited RG/B candidate; normal green orientation and appearance require UE visual review; alpha remains unused')
                    if spec['normal_encoding'] not in ('standard_dx','er_rg_direct'):
                        row['warnings'].append(tex_stem+': normal converted to PNG but encoding unconfirmed; not connected')
                for key in ('metallic','roughness'):
                    if key in rule: spec[key]=rule[key]
                material_specs[slot]=spec
                record.update(rule=rule,resolved_base_color=albedo['name'])
            original=importer.create_mesh
            def create_with_slot(base_name,collection,flver_data,flver_mesh,inflated_mesh,armature,z_up):
                layout=primary_vertex_channels(flver_data,flver_mesh,inflated_mesh)
                if layout['omitted_secondary_semantics']:
                    row.setdefault('vertex_layouts',[]).append({'material_index':flver_mesh.material_index,**layout})
                previous=set(collection.objects)
                original(base_name,collection,flver_data,flver_mesh,inflated_mesh,armature,z_up)
                for obj in set(collection.objects)-previous:
                    if obj.type=='MESH':
                        name='Slot_'+str(flver_mesh.material_index).zfill(3)
                        material=bpy.data.materials.get(name) or bpy.data.materials.new(name)
                        obj.data.materials.append(material)
                        obj.name=identifier(base_name)+'_Mesh_'+str(len(previous)).zfill(3)
            importer.create_mesh=create_with_slot
            try:
                importer.import_flver(flver,z_up=True,connect_bones=False)
            finally:
                importer.create_mesh=original
            meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
            if not meshes:
                raise ValueError('No imported meshes')
            skeletal=not flver.stem.lower().startswith('wp_')
            if skeletal:
                if not any(o.type=='ARMATURE' for o in bpy.context.scene.objects):
                    raise ValueError('Armor has no armature')
                unweighted=sum(1 for obj in meshes for v in obj.data.vertices if sum(g.weight for g in v.groups)<=0)
                if unweighted:
                    raise ValueError('Importer produced unweighted vertices: '+str(unweighted))
            objects=[o for o in bpy.context.scene.objects if o.type in ({'MESH','ARMATURE'} if skeletal else {'MESH'})]
            bpy.ops.object.select_all(action='DESELECT')
            for obj in objects: obj.select_set(True)
            target=output/(flver.stem+'.fbx')
            if target.exists(): raise ValueError('Existing FBX preserved')
            bpy.context.scene.unit_settings.system='METRIC'
            bpy.context.scene.unit_settings.scale_length=1.0
            bpy.ops.export_scene.fbx(filepath=str(target),use_selection=True,object_types={'MESH','ARMATURE'} if skeletal else {'MESH'},
                global_scale=1.0,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',
                add_leaf_bones=False,bake_anim=False,use_mesh_modifiers=False,mesh_smooth_type='FACE')
            row.update(output=str(target),mesh_count=len(meshes),vertex_count=sum(len(o.data.vertices) for o in meshes),
                       dimensions_m={o.name:list(o.dimensions) for o in meshes},status='converted')
            report['items'].append({'id':identifier(flver.stem),'model':target.name,'kind':'skeletal' if skeletal else 'static',
                'usage':'WorldDrop','scale':1.0,'provenance':job['provenance'],'materials':material_specs})
        except Exception:
            row.update(status='failed',error=traceback.format_exc())
    (output/'conversion.json').write_text(json.dumps(report,ensure_ascii=False,indent=2,default=str),encoding='utf-8')
    manifest={'schema_version':1,'source_root':str(output),'output_root':job['ue_root'],'items':report['items'],
              'scan_warnings':report['warnings']+[w for r in report['models'] for w in r['warnings']]}
    (output/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    if any(r.get('error') for r in report['models']+report['textures']) or not report['items']:
        raise RuntimeError('Conversion incomplete; see conversion.json')

if __name__=='__main__': main(sys.argv[sys.argv.index('--')+1])
