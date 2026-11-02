"""Use read-only audit source paths to prepare local six-item M1 fixtures; no copying sources."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
audit = json.loads((ROOT/'Saved/EquipmentImport/audit_existing.json').read_text(encoding='utf-8'))
source_root = Path(audit[0]['sources'][0]).parents[1]
textures = [
    'hd_m_1730-partsbnd-dcx/HD_M_1730-tpf/HD_M_1730_a.png',
    'bd_m_1731-partsbnd-dcx/BD_M_1731-tpf/BD_M_1730_a.png',
    'am_m_1730-partsbnd-dcx/AM_M_1730-tpf/AM_M_1730_a.png',
    'lg_m_1730-partsbnd-dcx/LG_M_1730-tpf/LG_M_1730_a.png',
    'wp_a_0200-partsbnd-dcx/WP_A_0200-tpf/WP_A_0200_a.png',
    'wp_a_0812-partsbnd-dcx/WP_A_0812_a.png',
]
rows = []
for record, texture, identity in zip(audit,textures,['Head1730','Chest1730','Hands1730','Legs1730','Longsword','MorningStar']):
    rows.append({'id':identity,'model':str(Path(record['sources'][0]).relative_to(source_root)),
        'kind':'skeletal' if record['class']=='SkeletalMesh' else 'static','usage':'WorldDrop',
        'provenance':'Existing user-local Elden Ring sample; same local prototype use only; original provenance retained',
        # Legacy FBX importer output differs from manually authored original UE slots.
        'materials':{('None' if record['class']=='SkeletalMesh' else 'WorldGridMaterial' if identity=='MorningStar' else slot['name']):
                     {'base_color':texture,'alpha_policy':'ignore','surface':'Opaque','preset':'metal'} for slot in record['slots']}})
data = {'schema_version':1,'source_root':str(source_root),'output_root':'/Game/EquipmentImport/Staging/M1Samples','items':rows,
        'scan_warnings':['1730 textures are provisional explicit test mappings, not verified original material reconstruction.',
                         'Special/packed normal encoding unconfirmed: no normal connected for these six samples.',
                         'Head Tree/TreeColor textures not auto-selected; alpha and detailed slot partition need visual review.',
                         'DDS and _m intentionally excluded from M1.']}
target = ROOT/'Saved/EquipmentImport/samples.json'
with target.open('x',encoding='utf-8') as stream:
    json.dump(data,stream,ensure_ascii=False,indent=2)
print(target)
