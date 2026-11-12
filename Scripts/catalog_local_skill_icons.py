"""Read-only local ER PNG catalog; filenames never substitute for a missing GemParam mapping."""
import csv, hashlib, json, re, struct
from pathlib import Path

root=Path('C:/baidunetdiskdownload/艾尔登法环图标/艾尔登法环图标')
source=Path('C:/Users/kfkssq/Desktop/EldenRingAsset')
out=Path(__file__).resolve().parents[1]/'Saved/SkillHUD'
out.mkdir(parents=True,exist_ok=True)
texts=json.loads((source/'msg/itemtext_chinese.json').read_text(encoding='utf-8-sig'))
goods_names={}
for w in texts['FmgWrappers']:
    if re.fullmatch(r'GoodsName(?:_dlc\d+)?\.fmg',w['Name']):
        for e in w['Fmg']['Entries']:
            if e.get('Text'): goods_names.setdefault(int(e['ID']),set()).add(e['Text'])
by_icon={}
with (source/'EquipParamGoods.csv').open(encoding='utf-8-sig',newline='') as f:
    for row in csv.DictReader(f):
        by_icon.setdefault(int(row['iconId']),[]).append({'goods_id':int(row['ID']),
            'names':sorted(goods_names.get(int(row['ID']),set()))})
rows=[]
for p in sorted(root.rglob('*.png')):
    m=re.fullmatch(r'menu_knowledge_(\d+)out(?:\.obj)?\.png',p.name)
    if not m: continue
    number=int(m[1])
    if not (6000 <= number < 7000 or 8300 <= number < 8500): continue
    data=p.read_bytes()
    width,height=struct.unpack('>II',data[16:24])
    rows.append({'icon_id':number,'path':str(p),'width':width,'height':height,
        'sha256':hashlib.sha256(data).hexdigest(),'goods_matches':by_icon.get(number,[]),
        'name_status':'matched_by_goods_csv' if by_icon.get(number) else 'visual_candidate_only_no_GemParam'})
report={'root':str(root),'source_version':'unverified local dump',
    'spell_icon_count':sum(6000<=r['icon_id']<7000 for r in rows),
    'martial_candidate_count':sum(8300<=r['icon_id']<8500 for r in rows),
    'missing_mapping':'EquipParamGem.csv / SwordArtsParam.csv not present; no official skill-name assertion for 8300-series',
    'icons':rows}
(out/'local-icons.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
selection=[r for r in rows if r['icon_id'] in [8303,8309,8314,8360,6000,6014,6050,6052,6200,6402]]
(out/'icon-candidates.json').write_text(json.dumps(selection,ensure_ascii=False,indent=2),encoding='utf-8')
print('Catalogued',len(rows),'icons; no source files changed')
