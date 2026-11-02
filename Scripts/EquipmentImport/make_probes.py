"""Local validation fixtures. PNGs generated from explicit RGB/alpha bytes, not external artwork."""
import json
from pathlib import Path
from test_pipeline import png

root=Path(__file__).resolve().parents[2]
folder=root/'Saved/EquipmentImport/Probes'
folder.mkdir(parents=True,exist_ok=True)
for name,alpha,normal in [('color',False,False),('masked',True,False),('normal',False,True)]:
    target=folder/(name+'.png')
    if not target.exists(): png(target,alpha,normal)
audit=json.loads((root/'Saved/EquipmentImport/audit_existing.json').read_text(encoding='utf-8'))
model=Path(audit[4]['sources'][0])
common=Path.home()
rows=[]
for surface in ['Opaque','Masked','Cloth']:
    rows.append({'id':'Probe'+surface,'model':str(model.relative_to(common)), 'kind':'static','usage':'WorldDrop',
        'provenance':'Explicit generated test PNG; existing user-local sword FBX, original provenance retained',
        'materials':{'材质':{'base_color':str((folder/('masked.png' if surface=='Masked' else 'color.png')).relative_to(common)),
             'normal':str((folder/'normal.png').relative_to(common)),'normal_encoding':'standard_dx',
             'surface':surface,'preset':'cloth' if surface=='Cloth' else 'metal',
             'alpha_policy':'mask' if surface=='Masked' else 'ignore'}}})
data={'schema_version':1,'source_root':str(common),'output_root':'/Game/EquipmentImport/Staging/M1Probes','items':rows}
with (folder/'manifest.json').open('x',encoding='utf-8') as stream: json.dump(data,stream,ensure_ascii=False,indent=2)
