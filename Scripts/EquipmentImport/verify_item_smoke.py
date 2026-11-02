"""Read-only fresh-editor acceptance for the selected item bridge run."""
import json
import os
from pathlib import Path
import unreal as u

plan_path = Path(os.environ['UMBRA_ITEM_PLAN'])
plan = json.loads(plan_path.read_text(encoding='utf-8'))
report = json.loads((plan_path.parent / 'editor.json').read_text(encoding='utf-8'))
assert report['ok']
records = {r['source_id']: r for r in report['items']}
checked = []
for chosen in plan['items']:
    item = chosen['item']
    row = records[item['source_id']]
    draft = u.load_asset(row['draft'])
    icon = u.load_asset(row['icon'])
    mesh = u.load_asset(row['mesh'])
    assert isinstance(draft, u.UmbraItemDefinition)
    assert str(draft.get_editor_property('display_name')) == item['name']['value']
    assert str(draft.get_editor_property('flavor_text')) == (item['description']['value'] or '')
    assert draft.get_editor_property('icon') == icon
    assert icon.srgb and icon.compression_settings == u.TextureCompressionSettings.TC_EDITOR_ICON
    assert icon.lod_group == u.TextureGroup.TEXTUREGROUP_UI
    assert icon.mip_gen_settings == u.TextureMipGenSettings.TMGS_NO_MIPMAPS
    assert icon.blueprint_get_size_x() > 0 and icon.blueprint_get_size_y() > 0
    field = 'world_drop_skeletal_mesh' if isinstance(mesh, u.SkeletalMesh) else 'world_drop_mesh'
    assert draft.get_editor_property(field) == mesh
    if isinstance(mesh, u.SkeletalMesh):
        assert mesh.skeleton
    data = json.loads(u.EditorAssetLibrary.get_metadata_tag(draft, 'UmbraItemImport.Source'))
    assert data['item']['raw_params'] == item['raw_params']
    assert data['item']['source_id'] == item['source_id']
    assert not draft.get_editor_property('weapon')
    assert not draft.get_editor_property('generation_profile')
    assert not draft.get_editor_property('allowed_slots')
    assert draft.get_editor_property('armor') == 0
    assert draft.get_editor_property('weight') == 0
    if item['table'] == 'EquipParamWeapon':
        assert draft.get_editor_property('item_category') == u.UmbraItemCategory.WEAPON
        assert draft.get_editor_property('weapon_type') == u.UmbraWeaponType.DAGGER
    else:
        assert draft.get_editor_property('item_category') == u.UmbraItemCategory.ARMOR
        assert data['item']['umbra_suggestion']['SuggestedSlot'] == 'Hands'
    for mi_path in [x['asset'] for x in row['material_checks']]:
        mi = u.load_asset(mi_path)
        assert u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(mi, 'BaseColor')
        if item['row_id'] == 100200:
            normal = u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(mi, 'Normal')
            rough = u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(mi, 'RoughnessMap')
            assert normal.compression_settings == u.TextureCompressionSettings.TC_NORMALMAP and not normal.srgb
            assert rough and not rough.srgb
    checked.append({'source_id': item['source_id'], 'name': item['name']['value'], 'raw_fields': len(data['item']['raw_params']),
                    'icon_size': [icon.blueprint_get_size_x(), icon.blueprint_get_size_y()], 'references_valid': True})
(plan_path.parent / 'acceptance.json').write_text(json.dumps({'ok': True, 'items': checked}, ensure_ascii=False, indent=2), encoding='utf-8')
