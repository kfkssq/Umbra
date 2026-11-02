"""Content/source preservation audit, independent of Unreal metadata."""
import argparse
import json
from pathlib import Path
from pipeline import sha
from dcx_run import write


def run(before,folder):
    before=Path(before);folder=Path(folder)
    baseline=json.loads(before.read_text(encoding='utf-8'))
    changes=[p for p,h in baseline.items() if not Path(p).is_file() or sha(p)!=h]
    result={'baseline':str(before.resolve()),'protected_count':len(baseline),'protected_changes':changes}
    plan=json.loads((folder/'source_plan.json').read_text(encoding='utf-8'))
    changed_sources=[]
    for row in plan['items']:
        for path,h in ((row['item']['models'][0]['path'],row['model_sha256']),(row['icon_path'],row['icon_sha256'])):
            if not Path(path).is_file() or sha(path)!=h:changed_sources.append(path)
    result['source_changes']=changed_sources
    if (folder/'before_repeat.json').exists():
        expected=json.loads((folder/'before_repeat.json').read_text())
        result['batch_count']=len(expected)
        result['batch_changes']=[p for p,h in expected.items() if not Path(p).is_file() or sha(p)!=h]
    result['ok']=not changes and not changed_sources and not result.get('batch_changes')
    write(folder/'preservation.json',result)
    print(json.dumps(result,ensure_ascii=False))
    return result


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--before',required=True);p.add_argument('--folder',required=True)
    a=p.parse_args();raise SystemExit(0 if run(a.before,a.folder)['ok'] else 1)
