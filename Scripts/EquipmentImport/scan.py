"""Build a manifest from folders: WorldDrop|Wearable / static|skeletal / item / FBX + PNG.
Texture stem must equal the imported material slot. Ambiguity is reported, never guessed.
Usage: python scan.py SOURCE_ROOT NEW_MANIFEST --batch Batch01
"""
import argparse
import json
from pathlib import Path
import re

def scan(root, batch):
    root = Path(root).resolve()
    data = {'schema_version':1,'source_root':str(root),'output_root':'/Game/EquipmentImport/Staging/'+batch,'items':[], 'scan_warnings':[]}
    for model in sorted(root.rglob('*')):
        if model.suffix.lower() != '.fbx':
            if model.suffix.lower() in ('.dds','.blend'):
                data['scan_warnings'].append(str(model.relative_to(root))+': not processed in M1 importer')
            continue
        relative = model.relative_to(root)
        parts = relative.parts
        if len(parts)!=4 or parts[0] not in ('WorldDrop','Wearable') or parts[1] not in ('static','skeletal'):
            data['scan_warnings'].append(str(relative)+': expected usage/kind/item/model.fbx; omitted')
            continue
        identity = re.sub('[^A-Za-z0-9_]','_',parts[2])
        if not identity or not identity[0].isalpha():
            identity = 'Item_'+identity
        materials = {}
        for texture in sorted(model.parent.glob('*.png')):
            if texture.stem.endswith('_a'):
                slot = texture.stem[:-2]
                normal = texture.with_name(slot+'_n.png')
                spec = {'base_color':str(texture.relative_to(root)), 'preset':'leather', 'surface':'Opaque'}
                if normal.is_file():
                    spec.update(normal=str(normal.relative_to(root)),normal_encoding='unknown')
                materials[slot] = spec
            elif not texture.stem.endswith('_n'):
                data['scan_warnings'].append(str(texture.relative_to(root))+': unclassified/_m texture not connected')
        sidecar = model.parent / 'item.json'
        row = {'id':identity,'usage':parts[0],'kind':parts[1],'model':str(relative),'materials':materials,
               'provenance':'UNCONFIRMED: record source and permitted use in item.json'}
        if sidecar.exists():
            overrides = json.loads(sidecar.read_text(encoding='utf-8-sig'))
            row.update(overrides)
        data['items'].append(row)
    return data

if __name__=='__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source_root')
    parser.add_argument('manifest')
    parser.add_argument('--batch',required=True)
    args = parser.parse_args()
    output = Path(args.manifest)
    with output.open('x',encoding='utf-8') as stream:
        json.dump(scan(args.source_root,args.batch),stream,ensure_ascii=False,indent=2)
    print(output.resolve())
