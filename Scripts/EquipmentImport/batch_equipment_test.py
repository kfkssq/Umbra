"""Explicit representative batch; preserves each failure and imports successful conversions."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
from types import SimpleNamespace
import uuid
from item_catalog import build, read_json, resource_gate, digest
import dcx_run

ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent


def run(selection):
    spec=read_json(selection)
    assert re.fullmatch('[A-Za-z][A-Za-z0-9_]*',spec['batch'])
    assert len(spec['source_ids'])==len(set(spec['source_ids']))
    config=read_json(HERE/'item_catalog.local.example.json')
    settings=read_json(ROOT/'Saved/EquipmentImport/dcx_tools.json')
    running=subprocess.check_output(['tasklist','/FO','CSV','/NH'],creationflags=subprocess.CREATE_NO_WINDOW).decode(errors='replace')
    if re.search(r'"UnrealEditor(?:-Cmd)?\.exe"',running,re.I): raise ValueError('Close Unreal Editor before test')
    version=read_json(Path(settings['ue_root'])/'Engine/Build/Build.version')
    assert f"{version['MajorVersion']}.{version['MinorVersion']}"==read_json(ROOT/'Umbra.uproject')['EngineAssociation']
    catalog,_=build(config); lookup={i['source_id']:i for i in catalog['items']}
    out=ROOT/'Saved/EquipmentImport/BatchTests'/(spec['batch']+'_'+uuid.uuid4().hex[:8]);out.mkdir(parents=True)
    report={'batch':spec['batch'],'items':[],'ok':False}
    chosen=[]
    for source_id in spec['source_ids']:
        item=lookup[source_id]
        assert not resource_gate(item),source_id
        assert item['name']['value'],source_id
        model=item['models'][0]
        icon_field='iconId' if item['table']=='EquipParamWeapon' else 'iconIdM'
        icon=item['icons'][icon_field]
        chosen.append({'item':item,'archive':model['archive'],'primary_model':model['archive'].upper(),
                       'usage':'WorldDrop','icon_path':icon['paths'][0],'icon_id':icon['id'],
                       'icon_sha256':digest(icon['paths'][0]),'model_sha256':digest(model['path'])})
        report['items'].append({'source_id':source_id,'name':item['name']['value'],'category':item['source_category'],
                                'archive':model['archive'],'has_umbra_mapping':bool(item['umbra_suggestion'])})
    dcx_run.write(out/'selection.json',spec)
    # Snapshot existing project packages before running external tools. Only newly isolated targets may be saved.
    before={str(p):digest(p) for p in (ROOT/'Content').rglob('*') if p.suffix in ('.uasset','.umap')}
    dcx_run.write(out/'before.json',before)
    try:
        converted=dcx_run.run(SimpleNamespace(source=config['parts'],batch=spec['batch'],
                    tools=str(ROOT/'Saved/EquipmentImport/dcx_tools.json'),rules=str(HERE/'dcx_rules.json'),
                    names=','.join(sorted({c['archive'] for c in chosen})),apply=True,convert_only=True))
        report['conversion']=converted
        states={r['id']:r for r in converted['items']}
        manifest=read_json(converted['manifest'])
        rows={r['id']:r for r in manifest['items']}
        passed=[]
        for selection_row,test in zip(chosen,report['items']):
            state=states[selection_row['archive']]
            test['conversion_status']=state['status']
            test['conversion_report']=state.get('conversion_report')
            if state['status']=='failed':
                test['errors']=state.get('conversion_errors',state.get('error'));continue
            key=selection_row['archive']+'_'+selection_row['primary_model']
            if key not in rows:
                test['errors']=['Expected primary FLVER absent'];continue
            selection_row['mesh_row_id']=key
            passed.append(selection_row)
        manifest['items']=[r for r in manifest['items'] if any(r['id'].startswith(c['archive']+'_') for c in passed)]
        dcx_run.write(out/'meshes.json',manifest)
        plan={'batch':spec['batch'],'items':passed,'provenance':catalog['provenance'],
              'source_version':catalog['source_version'],'mesh_manifest':str(out/'meshes.json'),'output_dir':str(out),
              'allow_unmapped_asset_only':True}
        dcx_run.write(out/'plan.json',plan)
        if passed:
            env=dict(os.environ,UMBRA_ITEM_PLAN=str(out/'plan.json'))
            try:
                dcx_run.execute([Path(settings['ue_root'])/'Engine/Binaries/Win64/UnrealEditor-Cmd.exe',ROOT/'Umbra.uproject',
                    '-run=pythonscript','-script='+(HERE/'import_items_editor.py').as_posix(),
                    '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities','-AllowCommandletRendering',
                    '-unattended','-nop4','-nosound','-stdout','-abslog='+str(out/'engine.log')],out/'process.log',env=env,timeout=3600)
            except Exception as exc:
                report['editor_process_error']=str(exc)
            if (out/'editor.json').exists(): report['editor']=read_json(out/'editor.json')
        editor_rows={r['source_id']:r for r in report.get('editor',{}).get('items',[])}
        for row in report['items']:
            row['ue']=editor_rows.get(row['source_id'],{'status':'not_imported'})
        report['summary']={'selected':len(chosen),'converted':len(passed),
             'drafts_created':sum(r['ue']['status']=='created' for r in report['items']),
             'asset_only':sum(r['ue']['status']=='asset_only' for r in report['items']),
             'conversion_failures':sum(r['conversion_status']=='failed' for r in report['items'])}
        report['ok']=converted['ok'] and report.get('editor',{}).get('ok',False)
    finally:
        report['existing_package_changes']=[p for p,h in before.items() if not Path(p).is_file() or digest(p)!=h]
        dcx_run.write(out/'report.json',report)
        print('BATCH REPORT '+str(out/'report.json'),flush=True)
    return report


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--selection',default=str(HERE/'batch_test_selection.json'))
    result=run(parser.parse_args().selection)
    raise SystemExit(0 if result['ok'] else 1)
