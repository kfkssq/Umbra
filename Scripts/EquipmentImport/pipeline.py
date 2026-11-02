"""M1 FBX/PNG authoring. Pure planning is usable without Unreal; writes are create-only."""
from __future__ import annotations
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import time

META = 'UmbraEquipmentImport.Recipe'
VERSION = 2
PRESETS = {'metal': (1.0, .35), 'leather': (0.0, .65), 'cloth': (0.0, .85), 'wood': (0.0, .75), 'neutral': (0.0, .65)}

def check(ok, message):
    if not ok:
        raise ValueError(message)

def stable(value):
    return json.dumps(value, sort_keys=True, ensure_ascii=False, separators=(',', ':'))

def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

def source(base, name, suffix):
    check(isinstance(name, str), 'Missing source filename')
    path = (base / name).resolve()
    check(path.is_relative_to(base), 'Source escapes source_root: ' + name)
    check(path.is_file() and path.suffix.lower() == suffix, 'Missing/unsupported source: ' + name)
    return path

def png_info(path):
    # Header inspection is conservative: alpha availability is not proof of transparency.
    with Path(path).open('rb') as stream:
        check(stream.read(8) == b'\x89PNG\r\n\x1a\n', 'Invalid PNG: ' + str(path))
        check(stream.read(8) == b'\x00\x00\x00\rIHDR', 'Invalid PNG IHDR')
        width, height, depth, color, _, _, _ = struct.unpack('>IIBBBBB', stream.read(13))
        check(width > 0 and height > 0, 'Empty PNG')
        stream.read(4)
        alpha = color in (4, 6)
        while True:
            header = stream.read(8)
            check(len(header) == 8, 'Truncated PNG')
            length, tag = struct.unpack('>I4s', header)
            if tag == b'tRNS':
                alpha = True
            stream.seek(length + 4, 1)
            if tag == b'IEND':
                break
    return {'width': width, 'height': height, 'depth': depth, 'alpha_channel': alpha}

def plan(manifest_path):
    path = Path(manifest_path).resolve()
    data = json.loads(path.read_text(encoding='utf-8-sig'))
    check(data.get('schema_version') == 1, 'schema_version must be 1')
    root = data.get('output_root', '')
    check(re.fullmatch(r'/Game/EquipmentImport/(?:Staging|Library)/[A-Za-z0-9_]+', root), 'Use isolated Staging or Library batch root')
    base = (path.parent / data.get('source_root', '.')).resolve()
    check(base.is_dir(), 'source_root missing')
    rows = data.get('items', [])
    check(isinstance(rows, list) and rows, 'No items')
    ids = [r.get('id', '') for r in rows]
    check(all(re.fullmatch('[A-Za-z][A-Za-z0-9_]*', i) for i in ids), 'IDs must be ASCII identifiers')
    check(len({i.casefold() for i in ids}) == len(ids), 'Duplicate IDs (case insensitive)')
    result = []
    for item in rows:
        row = {'id': item['id'], 'warnings': [], 'errors': [], 'status': 'planned'}
        result.append(row)
        try:
            check(set(item) <= {'id','model','kind','usage','materials','scale','icon_asset','provenance','combine_static_meshes'}, 'Unknown item fields')
            check(item.get('kind') in ('static','skeletal'), 'Explicit kind static/skeletal required')
            check(item.get('usage') in ('WorldDrop','Wearable'), 'Explicit usage WorldDrop/Wearable required')
            combine=item.get('combine_static_meshes',False)
            check(type(combine) is bool and (not combine or (item['kind']=='static' and item['usage']=='WorldDrop')),'Combine is limited to one static WorldDrop FBX')
            check(item.get('provenance'), 'Record provenance; this tool does not verify licensing')
            model = source(base, item.get('model'), '.fbx')
            scale = item.get('scale', 1.0)
            check(type(scale) in (float,int) and math.isfinite(scale) and scale > 0, 'Invalid scale')
            mats = item.get('materials', {})
            check(isinstance(mats, dict) and mats, 'Explicit slot-to-material map required')
            recipes = {}
            for index, (slot, material) in enumerate(mats.items()):
                check(slot and isinstance(material,dict), 'Invalid material slot mapping')
                check(set(material) <= {'base_color','normal','normal_encoding','surface','preset','metallic','roughness','roughness_texture','alpha_policy','alpha_clip','two_sided'}, 'Unknown material fields: '+slot)
                surface = material.get('surface', 'Opaque')
                check(surface in ('Opaque','Masked','Cloth'), 'Unsupported surface')
                preset = material.get('preset', 'cloth' if surface == 'Cloth' else 'leather')
                check(preset in PRESETS, 'Unknown preset')
                color = source(base, material.get('base_color'), '.png')
                info = png_info(color)
                alpha = material.get('alpha_policy')
                if info['alpha_channel']:
                    check(alpha in ('ignore','mask'), 'PNG has alpha/tRNS; explicitly choose alpha_policy ignore/mask: '+slot)
                    row['warnings'].append(slot + ': alpha semantics explicitly declared; visual review required')
                check(surface != 'Masked' or (info['alpha_channel'] and alpha == 'mask'), 'Masked requires confirmed BaseColor alpha')
                check(alpha != 'mask' or surface == 'Masked', 'alpha mask requires Masked surface')
                values = [material.get('metallic', PRESETS[preset][0]), material.get('roughness', 1.0 if material.get('roughness_texture') else PRESETS[preset][1])]
                check(all(type(v) in (int,float) and math.isfinite(v) and 0 <= v <= 1 for v in values), 'PBR scalar outside [0,1]')
                spec = {'index':index, 'surface':surface, 'base_color':str(color), 'base_sha':sha(color), 'metallic':values[0], 'roughness':values[1]}
                if 'alpha_clip' in material:
                    clip=material['alpha_clip']
                    check(surface=='Masked' and type(clip) in (int,float) and math.isfinite(clip) and 0<clip<=1,'Invalid mask threshold')
                    spec['alpha_clip']=clip
                if 'two_sided' in material:
                    check(type(material['two_sided']) is bool,'Invalid two_sided')
                    spec['two_sided']=material['two_sided']
                if material.get('roughness_texture'):
                    roughness = source(base,material['roughness_texture'],'.png')
                    png_info(roughness)
                    spec.update(roughness_texture=str(roughness),roughness_sha=sha(roughness))
                if material.get('normal'):
                    normal = source(base, material['normal'], '.png')
                    png_info(normal)
                    if material.get('normal_encoding') in ('standard_dx','er_rg_direct'):
                        spec.update(normal=str(normal), normal_sha=sha(normal))
                    else:
                        row['warnings'].append(slot + ': normal encoding unconfirmed; left disconnected (M2 review)')
                recipes[slot] = spec
            row.update(folder=f"{root}/{item['usage']}/{item['id']}", model=str(model), model_sha=sha(model),
                       kind=item['kind'], scale=scale, materials=recipes, icon_asset=item.get('icon_asset'), provenance=item['provenance'])
            if combine:row['combine_static_meshes']=True
            row['recipe'] = stable({'version':VERSION, **{k:v for k,v in row.items() if k not in ('warnings','errors','status')}})
        except Exception as exc:
            row['errors'].append(str(exc))
            row['status'] = 'failed'
    return {'version':VERSION, 'output_root':root, 'manifest':str(path), 'source_root':str(base), 'items':result,
            'scan_warnings':data.get('scan_warnings', [])}

class Editor:
    def __init__(self, root):
        import unreal as u
        self.u = u
        self.root = root
        self.saved = []
        self.tools = u.AssetToolsHelpers.get_asset_tools()
        self.lib = u.EditorAssetLibrary
        self.ml = u.MaterialEditingLibrary

    def existing(self, path, cls, recipe):
        if not self.lib.does_asset_exist(path):
            return None
        obj = self.u.load_asset(path)
        check(isinstance(obj, cls), 'Wrong existing type: '+path)
        check(self.lib.get_metadata_tag(obj, META) == recipe, 'Existing asset unmanaged/recipe changed; preserve and use new id: '+path)
        return obj

    def save(self, obj, recipe):
        self.lib.set_metadata_tag(obj, META, recipe)
        check(self.lib.save_loaded_asset(obj), 'Save failed: '+obj.get_path_name())
        self.saved.append(obj.get_path_name())

    def create(self, path, cls, factory):
        check(not self.lib.does_asset_exist(path), 'Destination exists: '+path)
        folder, name = path.rsplit('/',1)
        obj = self.tools.create_asset(name, folder, cls, factory)
        check(obj is not None, 'Creation failed: '+path)
        return obj

    def master(self, surface, roughness_map=False, roughness_default=None):
        u, ml = self.u, self.ml
        path = self.root + '/Masters/M_Equipment_' + surface + ('_RoughnessMap' if roughness_map else '')
        recipe = stable({'master':surface, 'version':VERSION, **({'roughness_map':True} if roughness_map else {})})
        obj = self.existing(path, u.Material, recipe)
        if obj:
            return obj
        obj = self.create(path, u.Material, u.MaterialFactoryNew())
        # Shared masters must compile skeletal permutations before first viewport use.
        obj.set_editor_property('used_with_skeletal_mesh',True)
        if surface == 'Masked':
            obj.set_editor_property('blend_mode', u.BlendMode.BLEND_MASKED)
            obj.set_editor_property('two_sided', True)
        for param, prop, tex, sampler in (
            ('BaseColor',u.MaterialProperty.MP_BASE_COLOR,'/Engine/EngineResources/WhiteSquareTexture',u.MaterialSamplerType.SAMPLERTYPE_COLOR),
            ('Normal',u.MaterialProperty.MP_NORMAL,'/Engine/EngineMaterials/DefaultNormal',u.MaterialSamplerType.SAMPLERTYPE_NORMAL)):
            node = ml.create_material_expression(obj, u.MaterialExpressionTextureSampleParameter2D)
            node.set_editor_property('parameter_name', param)
            texture = u.load_asset(tex)
            check(texture is not None, 'Missing engine default texture: '+tex)
            node.set_editor_property('texture',texture)
            node.set_editor_property('sampler_type',sampler)
            check(ml.connect_material_property(node,'RGB',prop), 'Cannot wire '+param)
            if param == 'BaseColor' and surface == 'Masked':
                check(ml.connect_material_property(node,'A',u.MaterialProperty.MP_OPACITY_MASK), 'Cannot wire alpha')
        for name, prop, default in (('Metallic',u.MaterialProperty.MP_METALLIC,0.0),('Roughness',u.MaterialProperty.MP_ROUGHNESS,.85 if surface=='Cloth' else .65)):
            node = ml.create_material_expression(obj,u.MaterialExpressionScalarParameter)
            node.set_editor_property('parameter_name',name)
            node.set_editor_property('default_value',default)
            if name=='Roughness' and roughness_map:
                tex=ml.create_material_expression(obj,u.MaterialExpressionTextureSampleParameter2D)
                tex.set_editor_property('parameter_name','RoughnessMap')
                check(roughness_default is not None,'Linear roughness default required')
                tex.set_editor_property('texture',roughness_default)
                tex.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
                product=ml.create_material_expression(obj,u.MaterialExpressionMultiply)
                check(ml.connect_material_expressions(tex,'R',product,'A'),'Roughness map wiring failed')
                check(ml.connect_material_expressions(node,'',product,'B'),'Roughness scale wiring failed')
                check(ml.connect_material_property(product,'',prop),'Roughness output wiring failed')
            else:
                check(ml.connect_material_property(node,'',prop), 'Cannot wire '+name)
        ml.layout_material_expressions(obj)
        compile_errors=ml.recompile_material(obj)
        check(not compile_errors,'Master compile errors: '+str(compile_errors))
        self.save(obj,recipe)
        return obj

    def texture(self, filename, path, normal, digest, linear=False):
        u = self.u
        recipe = stable({'sha':digest,'normal':normal,'version':VERSION, **({'linear':True} if linear else {})})
        obj = self.existing(path,u.Texture2D,recipe)
        if obj:
            return obj
        task = u.AssetImportTask()
        task.filename = filename
        task.destination_path, task.destination_name = path.rsplit('/',1)
        task.automated = True
        task.replace_existing = False
        task.save = False
        task.factory = u.TextureFactory()
        self.tools.import_asset_tasks([task])
        obj = u.load_asset(path)
        check(isinstance(obj,u.Texture2D), 'Texture import failed: '+path)
        obj.set_editor_property('srgb',not (normal or linear))
        obj.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP if normal else u.TextureCompressionSettings.TC_DEFAULT)
        if normal:
            obj.set_editor_property('flip_green_channel',False)
            obj.set_editor_property('lod_group',u.TextureGroup.TEXTUREGROUP_WORLD_NORMAL_MAP)
        self.save(obj,recipe)
        return obj

    def instance(self, row, spec):
        u, ml = self.u, self.ml
        path = row['folder'] + '/Materials/MI_' + str(spec['index'])
        recipe = stable(spec)
        obj = self.existing(path,u.MaterialInstanceConstant,recipe)
        if obj:
            return obj
        textures = {'BaseColor': self.texture(spec['base_color'],row['folder']+'/Textures/T_'+str(spec['index'])+'_a',False,spec['base_sha'])}
        if spec.get('normal'):
            textures['Normal'] = self.texture(spec['normal'],row['folder']+'/Textures/T_'+str(spec['index'])+'_n',True,spec['normal_sha'])
        if spec.get('roughness_texture'):
            textures['RoughnessMap'] = self.texture(spec['roughness_texture'],row['folder']+'/Textures/T_'+str(spec['index'])+'_r',False,spec['roughness_sha'],linear=True)
        parent = self.master(spec['surface'],bool(spec.get('roughness_texture')),textures.get('RoughnessMap'))
        obj = self.create(path,u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew())
        ml.set_material_instance_parent(obj,parent)
        for name, texture in textures.items():
            ml.set_material_instance_texture_parameter_value(obj,name,texture)
            check(ml.get_material_instance_texture_parameter_value(obj,name)==texture,'Texture parameter readback failed')
        for name in ('Metallic','Roughness'):
            ml.set_material_instance_scalar_parameter_value(obj,name,spec[name.lower()])
            check(abs(ml.get_material_instance_scalar_parameter_value(obj,name)-spec[name.lower()])<1e-5,'Scalar readback failed')
        if 'alpha_clip' in spec or 'two_sided' in spec:
            overrides=obj.get_editor_property('base_property_overrides')
            if 'alpha_clip' in spec:
                overrides.set_editor_property('override_opacity_mask_clip_value',True)
                overrides.set_editor_property('opacity_mask_clip_value',spec['alpha_clip'])
            if 'two_sided' in spec:
                overrides.set_editor_property('override_two_sided',True)
                overrides.set_editor_property('two_sided',spec['two_sided'])
            obj.set_editor_property('base_property_overrides',overrides)
        ml.update_material_instance(obj)
        self.save(obj,recipe)
        return obj

    def mesh_paths(self, row):
        return self.lib.list_assets(row['folder']+'/Meshes',recursive=True,include_folder=False)

    def inspect_existing_meshes(self, row):
        paths = self.mesh_paths(row)
        meshes = []
        for path in paths:
            obj = self.u.load_asset(path)
            check(self.lib.get_metadata_tag(obj,META)==row['recipe'], 'Unmanaged/changed mesh output: '+path)
            if isinstance(obj,(self.u.StaticMesh,self.u.SkeletalMesh)):
                expected = self.lib.get_metadata_tag(obj,META+'.Outputs')
                if expected:
                    check(set(json.loads(expected)) == set(str(p) for p in paths), 'Partial saved mesh group; inspect outputs and use a new id')
                meshes.append(obj)
        check(not paths or meshes, 'Incomplete mesh outputs; use new id or inspect failed batch')
        return meshes

    def preflight(self, row):
        meshes = self.inspect_existing_meshes(row)
        if meshes:
            self.describe(meshes)
        for spec in row['materials'].values():
            mapped=bool(spec.get('roughness_texture'))
            master = self.existing(self.root+'/Masters/M_Equipment_'+spec['surface']+('_RoughnessMap' if mapped else ''), self.u.Material,
                          stable({'master':spec['surface'],'version':VERSION,**({'roughness_map':True} if mapped else {})}))
            instance = self.existing(row['folder']+'/Materials/MI_'+str(spec['index']),self.u.MaterialInstanceConstant,stable(spec))
            if meshes:
                check(master is not None and instance is not None, 'Missing generated material dependency')
                check(instance.get_editor_property('parent') is not None, 'Missing instance parent')
            for key, suffix, normal in (('base_color','a',False),('normal','n',True),('roughness_texture','r',False)):
                if key in spec:
                    linear=key=='roughness_texture'
                    digest = spec['roughness_sha' if linear else 'normal_sha' if normal else 'base_sha']
                    texture = self.existing(row['folder']+'/Textures/T_'+str(spec['index'])+'_'+suffix,self.u.Texture2D,
                                  stable({'sha':digest,'normal':normal,'version':VERSION,**({'linear':True} if linear else {})}))
                    if meshes:
                        check(texture is not None, 'Missing generated texture dependency')
        return meshes

    def apply(self, row):
        u = self.u
        check(sha(row['model'])==row['model_sha'],'Source model changed after planning')
        for spec in row['materials'].values():
            check(sha(spec['base_color'])==spec['base_sha'],'BaseColor changed after planning')
            if spec.get('normal'):
                check(sha(spec['normal'])==spec['normal_sha'],'Normal changed after planning')
            if spec.get('roughness_texture'):
                check(sha(spec['roughness_texture'])==spec['roughness_sha'],'Roughness map changed after planning')
        existing = self.preflight(row)
        if existing:
            return 'preserved', self.describe(existing)
        materials = {slot:self.instance(row,spec) for slot,spec in row['materials'].items()}
        task = u.AssetImportTask()
        task.filename = row['model']
        task.destination_path = row['folder']+'/Meshes'
        task.destination_name = ('SK_' if row['kind']=='skeletal' else 'SM_')+row['id']
        task.automated = True
        task.replace_existing = False
        task.save = False
        task.factory = u.FbxFactory()
        options = u.FbxImportUI()
        options.automated_import_should_detect_type = False
        options.import_mesh = True
        options.import_as_skeletal = row['kind']=='skeletal'
        options.mesh_type_to_import = u.FBXImportType.FBXIT_SKELETAL_MESH if options.import_as_skeletal else u.FBXImportType.FBXIT_STATIC_MESH
        options.import_materials = False
        options.import_textures = False
        options.import_animations = False
        options.create_physics_asset = False
        # Never pass a character/existing skeleton: imports must not mutate its bone tree.
        options.skeleton = None
        settings = options.skeletal_mesh_import_data if options.import_as_skeletal else options.static_mesh_import_data
        settings.import_uniform_scale = row['scale']
        settings.convert_scene = True
        settings.convert_scene_unit = True
        if not options.import_as_skeletal:
            settings.combine_meshes = row.get('combine_static_meshes',False)
            settings.auto_generate_collision = False
        task.options = options
        self.tools.import_asset_tasks([task])
        objects = list(task.get_objects())
        meshes = [o for o in objects if isinstance(o,(u.StaticMesh,u.SkeletalMesh))]
        check(meshes, 'FBX produced no meshes')
        all_slots = set()
        for mesh in meshes:
            check(isinstance(mesh,u.SkeletalMesh if row['kind']=='skeletal' else u.StaticMesh),'Unexpected mesh kind')
            slots = mesh.get_editor_property('materials' if row['kind']=='skeletal' else 'static_materials')
            check(slots, 'Mesh has no material slots')
            for slot in slots:
                name = str(slot.material_slot_name)
                check(name in materials, 'Unmapped slot: '+name)
                all_slots.add(name)
            if row['kind']=='skeletal':
                updated_slots = []
                for slot in slots:
                    slot.set_editor_property('material_interface',materials[str(slot.material_slot_name)])
                    updated_slots.append(slot)
                mesh.set_editor_property('materials',updated_slots)
                skeleton = mesh.get_editor_property('skeleton')
                check(skeleton is not None,'Missing imported skeleton')
                if skeleton not in objects:
                    objects.append(skeleton)
            else:
                for index, slot in enumerate(slots):
                    mesh.set_material(index,materials[str(slot.material_slot_name)])
        check(all_slots == set(materials),'Configured slot absent in imported FBX: '+str(set(materials)-all_slots))
        descriptions = self.describe(meshes)
        # Validate all outputs before saving any meshes. No Save All, no original package writes.
        for obj in objects:
            check(obj.get_path_name().startswith(row['folder']+'/Meshes/'),'Unexpected import destination')
        expected = stable(sorted(o.get_path_name() for o in objects))
        for mesh in meshes:
            self.lib.set_metadata_tag(mesh,META+'.Outputs',expected)
        for obj in sorted(objects,key=lambda o:isinstance(o,(u.StaticMesh,u.SkeletalMesh))):
            self.save(obj,row['recipe'])
        return 'created', descriptions

    def describe(self, meshes):
        result = []
        for mesh in meshes:
            skeletal = isinstance(mesh,self.u.SkeletalMesh)
            if skeletal:
                check(mesh.get_editor_property('skeleton') is not None,'Missing skeleton reference')
            slots = mesh.get_editor_property('materials' if skeletal else 'static_materials')
            check(all(s.material_interface for s in slots),'Missing material reference')
            result.append({'asset':mesh.get_path_name(), 'kind':'skeletal' if skeletal else 'static',
                           'skeleton':mesh.get_editor_property('skeleton').get_path_name() if skeletal else None,
                           'slots':{str(s.material_slot_name):s.material_interface.get_path_name() for s in slots}})
        return result

def run(manifest, report_path, apply=False, ids=None):
    report_path = Path(report_path)
    report_path.parent.mkdir(parents=True,exist_ok=True)
    report = {'apply':apply, 'items':[], 'saved':[], 'fatal':None}
    started = time.monotonic()
    editor = None
    try:
        report.update(plan(manifest))
        if ids:
            check(set(ids) <= {r['id'] for r in report['items']},'Unknown retry ID')
            report['items'] = [r for r in report['items'] if r['id'] in ids]
        editor = Editor(report['output_root'])
        for row in report['items']:
            if row['errors']:
                continue
            try:
                if apply:
                    row['status'], row['outputs'] = editor.apply(row)
                else:
                    meshes = editor.preflight(row)
                    row['status'] = 'preserve' if meshes else 'would_import'
                    row['actions'] = ['import PNGs','create/reuse masters + instances','import FBX without merging static nodes','exact slot binding','save new packages'] if not meshes else []
                    row['warnings'].append('Dry run cannot inspect unimported FBX slots/weights; validated during apply')
            except Exception as exc:
                row['status'] = 'failed'
                row['errors'].append(str(exc))
            finally:
                editor.u.log('EquipmentImport '+row['id']+': '+row['status']+' '+str(row['errors']))
                # Sequential batches keep referenced assets bounded; avoid a full asset-library load.
                editor.u.SystemLibrary.collect_garbage()
                report['saved'] = list(editor.saved)
                report_path.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    except Exception as exc:
        report['fatal'] = str(exc)
    finally:
        report['elapsed_seconds'] = round(time.monotonic()-started,3)
        report['failed_ids'] = [r['id'] for r in report['items'] if r['status']=='failed']
        report['ok'] = not report['fatal'] and not report['failed_ids']
        report_path.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    return report
