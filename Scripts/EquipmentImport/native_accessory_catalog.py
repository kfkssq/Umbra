"""Read-only ER catalog for talismans and the explicit nineteen smithing materials."""
import csv
import json
import re
import struct
from pathlib import Path
from item_catalog import icon_index, digest, valid_text, resolve_text

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent

def build(config):
    source = Path(config['source_directory'])
    text_path = source / 'msg/itemtext_chinese.json'
    texts = {}
    for wrapper in json.loads(text_path.read_text(encoding='utf-8-sig'))['FmgWrappers']:
        match = re.fullmatch(r'(Accessory|Goods)(Name|Info|Caption)(?:_dlc\d+)?\.fmg', wrapper['Name'])
        if not match:
            continue
        for entry in wrapper['Fmg']['Entries']:
            if valid_text(entry.get('Text')):
                texts.setdefault((match[1], match[2], int(entry['ID'])), []).append(
                    {'file': wrapper['Name'], 'text': entry['Text']})
    icons = icon_index(config['icons'])
    report = {'config': config, 'inputs': {str(text_path): digest(text_path)}, 'items': [], 'excluded': []}
    for kind in ('Accessory', 'Goods'):
        path = source / ('EquipParam' + kind + '.csv')
        report['inputs'][str(path)] = digest(path)
        with path.open(encoding='utf-8-sig', newline='') as stream:
            reader = csv.DictReader(stream)
            if not {'ID', 'iconId', 'rarity'}.issubset(reader.fieldnames or []):
                raise ValueError('Missing required fields: ' + str(path))
            rows = list(reader)
        seen = set()
        for raw in rows:
            if None in raw or any(v is None for v in raw.values()):
                raise ValueError('Malformed CSV: ' + str(path))
            number = int(raw['ID'])
            if number in seen:
                raise ValueError('Duplicate row ID: ' + str(number))
            seen.add(number)
            if kind == 'Goods' and number not in config['stone_ids']:
                continue
            name = resolve_text(texts, kind, 'Name', number)
            row = {'source_id': f'ER:EquipParam{kind}:{number}', 'table': 'EquipParam'+kind,
                   'row_id': number, 'category': 'Accessory' if kind == 'Accessory' else 'Material',
                   'name': name, 'description': resolve_text(texts, kind, 'Caption', number),
                   'raw_params': raw, 'icon_id': int(raw['iconId']), 'source_rarity': int(raw['rarity']),
                   'visual_policy': 'shared_placeholder', 'visual_verified': False}
            reasons = []
            if name['status'] != 'matched': reasons.append('name_' + name['status'])
            if row['source_rarity'] not in (0,1,2,3): reasons.append('invalid_rarity')
            if row['description']['status'] == 'conflict': reasons.append('description_conflict')
            if kind == 'Goods' and (raw.get('goodsType') != '14' or not name['value'] or '锻造石' not in name['value']):
                reasons.append('stone_identity_mismatch')
            matches = icons.get(row['icon_id'], [])
            if len(matches) != 1:
                reasons.append('icon_missing' if not matches else 'icon_ambiguous')
            else:
                icon_path = Path(matches[0])
                data = icon_path.read_bytes()
                if data[:8] != b'\x89PNG\r\n\x1a\n': reasons.append('invalid_png')
                else:
                    width, height = struct.unpack('>II', data[16:24])
                    row.update(icon_path=str(icon_path), icon_sha256=digest(icon_path), width=width, height=height,
                               png_color_type=data[25])
                    if not width or not height: reasons.append('empty_image')
                    if data[25] not in (4,6) and b'tRNS' not in data: reasons.append('no_alpha_channel')
            if reasons:
                report['excluded'].append(dict(row, reasons=reasons))
            else:
                report['items'].append(row)
        if kind == 'Goods' and not set(config['stone_ids']).issubset(seen):
            raise ValueError('Missing selected smithing stone rows')
    report['counts'] = {category: sum(i['category']==category for i in report['items']) for category in ('Accessory','Material')}
    report['unique_icons'] = len({i['icon_id'] for i in report['items']})
    return report

if __name__ == '__main__':
    config = json.loads((HERE/'native_accessories.json').read_text(encoding='utf-8'))
    result = build(config)
    output = ROOT/'Saved/NativeAccessories'
    output.mkdir(parents=True, exist_ok=True)
    (output/'plan.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'counts': result['counts'], 'unique_icons':result['unique_icons'],
                      'excluded':[(i['source_id'],i['reasons']) for i in result['excluded']]}, ensure_ascii=True))
