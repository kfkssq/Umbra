"""Read back completed bulk results in a new UE process. Never saves assets."""
import json
import os
from pathlib import Path
import sys
import unreal as u

path=Path(os.environ['UMBRA_BULK_REPORT'])
report=json.loads(path.read_text(encoding='utf-8'))
source_plan=json.loads((path.parent/'source_plan.json').read_text(encoding='utf-8'))
sources={r['item']['source_id']:r for r in source_plan['items']}
expected_root='/Game/EquipmentImport/'+source_plan.get('destination','Staging')+'/'+source_plan['batch']+'/'
def check_destination(obj):
    assert obj and obj.get_path_name().startswith(expected_root), 'Reference outside batch: '+str(obj)
recipes={}
for chunk in report['chunks']:
    p=Path(chunk['plan']).parent/'meshes_report.json'
    if p.is_file():
        for row in json.loads(p.read_text(encoding='utf-8'))['items']:
            for spec in row.get('materials',{}).values():
                recipes[row['folder']+'/Materials/MI_'+str(spec['index'])]=spec
result={'ok':False,'items':[],'scope':'fresh process structural and parameter readback; not visual/gameplay certification'}
for row in report['items']:
    if row['status']=='failed':continue
    record={'source_id':row['source_id'],'meshes':[]};result['items'].append(record)
    try:
        icon=u.load_asset(row['icon'])
        check_destination(icon)
        assert isinstance(icon,u.Texture2D) and icon.srgb
        assert icon.compression_settings==u.TextureCompressionSettings.TC_EDITOR_ICON
        assert icon.lod_group==u.TextureGroup.TEXTUREGROUP_UI
        assert icon.mip_gen_settings==u.TextureMipGenSettings.TMGS_NO_MIPMAPS
        paths=row.get('meshes') or [row['mesh']]
        for asset in paths:
            mesh=u.load_asset(asset);skeletal=isinstance(mesh,u.SkeletalMesh)
            check_destination(mesh)
            assert isinstance(mesh,(u.SkeletalMesh,u.StaticMesh))
            if skeletal:check_destination(mesh.skeleton)
            slots=mesh.get_editor_property('materials' if skeletal else 'static_materials');assert slots
            materials=[]
            for slot in slots:
                mi=slot.material_interface;assert isinstance(mi,u.MaterialInstanceConstant)
                check_destination(mi)
                spec=recipes[mi.get_path_name().split('.')[0]]
                assert mi.parent
                check_destination(mi.parent)
                assert mi.parent.get_editor_property('used_with_skeletal_mesh')
                for parameter,field in (('BaseColor','base_color'),('Normal','normal')):
                    if field not in spec:continue
                    tex=u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(mi,parameter)
                    assert isinstance(tex,u.Texture2D)
                    check_destination(tex)
                    actual=Path(tex.get_editor_property('asset_import_data').get_first_filename()).resolve()
                    assert actual==Path(spec[field]).resolve(),str(actual)
                    if field=='normal':assert not tex.srgb and tex.compression_settings==u.TextureCompressionSettings.TC_NORMALMAP
                    else:assert tex.srgb
                for parameter in ('Metallic','Roughness'):
                    actual=u.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(mi,parameter)
                    assert abs(actual-spec[parameter.lower()])<1e-5
                overrides=mi.get_editor_property('base_property_overrides')
                if 'alpha_clip' in spec:
                    assert mi.parent.blend_mode==u.BlendMode.BLEND_MASKED
                    assert overrides.get_editor_property('override_opacity_mask_clip_value')
                    assert abs(overrides.get_editor_property('opacity_mask_clip_value')-spec['alpha_clip'])<1e-5
                if 'two_sided' in spec:
                    assert overrides.get_editor_property('override_two_sided') and overrides.get_editor_property('two_sided')==spec['two_sided']
                materials.append({'asset':mi.get_path_name(),'surface':spec['surface'],'alpha_clip':spec.get('alpha_clip')})
            record['meshes'].append({'asset':asset,'materials':materials})
        if row.get('draft'):
            draft=u.load_asset(row['draft']);source=sources[row['source_id']]['item']
            check_destination(draft)
            assert str(draft.display_name)==source['name']['value'] and draft.icon==icon
            reference='world_drop_skeletal_mesh' if isinstance(mesh,u.SkeletalMesh) else 'world_drop_mesh'
            assert draft.get_editor_property(reference)==mesh
            raw=json.loads(u.EditorAssetLibrary.get_metadata_tag(draft,'UmbraItemImport.Source'))
            assert raw['item']['raw_params']==source['raw_params']
        record['ok']=True
    except Exception as exc:record.update(ok=False,error=str(exc))
    u.SystemLibrary.collect_garbage()
result['ok']=bool(result['items']) and all(i['ok'] for i in result['items'])
(path.parent/'verification.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
if not result['ok']:raise RuntimeError('Bulk readback failed')
