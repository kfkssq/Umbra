"""Check native AssetRegistryDump package data, not merely names appearing in dependencies."""
import argparse
import json
from pathlib import Path
import re

parser=argparse.ArgumentParser()
parser.add_argument('--dump',required=True)
parser.add_argument('--report',required=True)
args=parser.parse_args()
manifest=json.loads((Path(__file__).parent/'samples.json').read_text(encoding='utf-8'))
packages={}
for file in Path(args.dump).glob('Page_*.txt'):
    raw=file.read_bytes()
    text=raw.decode('utf-16' if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')
    for path,content_hash,size in re.findall(r'^\s*(/game/\S+) : ([0-9a-f]+) : (\d+) bytes\s*$',text,re.M):
        packages[path]=dict(hash=content_hash,bytes=int(size))
expected={manifest['output_root']+'/BP_LootTest_PipelineM5'}
references=set()
for row in manifest['items']:
    expected.update((row['asset_path'],row['mesh']['asset_path'],row['icon']['asset_path']))
    references.update(v for k,v in row['fields'].items() if k in ('generation_profile','weapon') and v)
missing=sorted(p for p in expected|references if packages.get(p.lower(),{}).get('bytes',0)<=0)
report=dict(status='failed' if missing else 'passed',sample_packages=len(expected),missing=missing,
            packages={p:packages.get(p.lower()) for p in sorted(expected|references)})
Path(args.report).write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps(dict(status=report['status'],sample_packages=len(expected),missing=missing)))
if missing: raise SystemExit(1)
