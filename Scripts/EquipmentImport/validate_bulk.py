"""Fresh UE readback; optional per-component previews in an unsaved diagnostic world."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import re
import dcx_run

ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent


def run(path,preview_ids):
    path=Path(path).resolve();folder=path.parent
    running=subprocess.check_output(['tasklist','/FO','CSV','/NH'],creationflags=subprocess.CREATE_NO_WINDOW).decode(errors='replace')
    if re.search(r'"UnrealEditor(?:-Cmd)?\.exe"',running,re.I):raise ValueError('Close UE before validation')
    settings=json.loads((ROOT/'Saved/EquipmentImport/dcx_tools.json').read_text(encoding='utf-8-sig'))
    engine=Path(settings['ue_root'])/'Engine/Binaries/Win64'
    dcx_run.execute([engine/'UnrealEditor-Cmd.exe',ROOT/'Umbra.uproject','-run=pythonscript',
       '-script='+(HERE/'verify_bulk_editor.py').as_posix(),'-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities',
       '-AllowCommandletRendering','-unattended','-nop4','-nosound','-stdout','-abslog='+str(folder/'verify_engine.log')],
       folder/'verify_process.log',env=dict(os.environ,UMBRA_BULK_REPORT=str(path)),timeout=1800)
    if preview_ids:
        batch=json.loads(path.read_text(encoding='utf-8'))
        plan=json.loads((folder/'source_plan.json').read_text(encoding='utf-8'))
        lookup={r['item']['source_id']:r for r in plan['items']}
        preview=folder/'PreviewReview';preview.mkdir(exist_ok=True)
        jobs=[]
        for row in batch['items']:
            source=lookup[row['source_id']]
            if row['status']=='failed' or source['archive'] not in preview_ids:continue
            meshes=row.get('meshes') or [row['mesh']]
            for i,mesh in enumerate(meshes):
                jobs.append({'source_id':row['source_id'],'name':source['item']['name']['value'],
                    'archive':source['archive']+'_'+str(i),'ue':{**row,'mesh':mesh}})
        if not jobs:raise ValueError('No successful preview items')
        dcx_run.write(preview/'report.json',{'items':jobs})
        dcx_run.write(preview/'plan.json',plan)
        dcx_run.execute([engine/'UnrealEditor.exe',ROOT/'Umbra.uproject',
           '-ExecutePythonScript='+(HERE/'batch_equipment_preview.py').as_posix(),
           '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities','-unattended','-nop4','-nosound',
           '-stdout','-abslog='+str(preview/'preview_engine.log')],preview/'preview_process.log',
           env=dict(os.environ,UMBRA_BATCH_REPORT=str(preview/'report.json')),timeout=1800)
        acceptance=json.loads((preview/'acceptance.json').read_text(encoding='utf-8'))
        if not acceptance['ok']:raise ValueError('Preview readback failed')
    print('VERIFICATION '+str(folder/'verification.json'))


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--report',required=True);p.add_argument('--previews',default='')
    a=p.parse_args();run(a.report,set(a.previews.split(','))-{''})
