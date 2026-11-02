"""Selected source items -> existing DCX pipeline -> icons and presentation-only drafts."""
import argparse
import json
import os
from pathlib import Path
import re
from types import SimpleNamespace
import uuid
import dcx_run
from item_catalog import build, read_json, resource_gate, digest

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[1]


def prepare(config, selection):
    if not re.fullmatch(r"[A-Za-z][A-Za-z0-9_]*", selection["batch"]):
        raise ValueError("Invalid batch")
    destination = selection.get('destination', 'Staging')
    if destination not in ('Staging', 'Library'):
        raise ValueError('Invalid destination')
    catalog, _ = build(config)
    lookup = {i["source_id"]: i for i in catalog["items"]}
    chosen, seen = [], set()
    if not selection["items"]:
        raise ValueError("Explicit nonempty selection required")
    for spec in selection["items"]:
        identity = spec["source_id"]
        if identity in seen:
            raise ValueError("Duplicate selection " + identity)
        seen.add(identity)
        item = lookup[identity]
        reasons = resource_gate(item)
        if reasons:
            raise ValueError(identity + ": " + ", ".join(reasons))
        if item["name"]["status"] != "matched" or (not item["umbra_suggestion"] and not selection.get('allow_unmapped_asset_only')):
            raise ValueError(identity + ": named and mapped item required for draft")
        if spec.get("usage") != "WorldDrop":
            raise ValueError("Draft bridge currently supports explicit WorldDrop only; no wearable adaptation")
        icon = item["icons"][spec["icon_field"]]
        model = item["models"][0]
        primary = spec["primary_model"]
        if not re.fullmatch(r"[A-Za-z0-9_]+", primary):
            raise ValueError("Explicit primary FLVER model ID required")
        chosen.append({"item": item, "archive": model["archive"], "primary_model": primary,
                       "usage": spec["usage"], "icon_path": icon["paths"][0], "icon_id": icon["id"],
                       "icon_sha256": digest(icon["paths"][0]), "model_sha256": digest(model["path"])})
    return {"batch": selection["batch"], "destination": destination, "items": chosen, "provenance": catalog["provenance"],
            "source_version": catalog["source_version"], "mode": "presentation_drafts_only",
            'combine_world_drop_components':bool(selection.get('combine_world_drop_components',True)),
            'allow_unmapped_asset_only':bool(selection.get('allow_unmapped_asset_only'))}


def select_converted(chosen, converted, manifest):
    """One bad archive must not suppress unrelated valid items or leak partial models."""
    states={r['id']:r for r in converted['items']}
    rows={r['id']:r for r in manifest['items']}
    passed, failed=[],[]
    for item in chosen:
        state=states.get(item['archive'],{})
        key=item['archive']+'_'+item['primary_model']
        if state.get('status')!='converted' or key not in rows:
            failed.append({'source_id':item['item']['source_id'],'archive':item['archive'],
                           'status':'failed','error':state.get('conversion_errors') or state.get('error') or 'Primary model missing'})
            continue
        item['mesh_row_id']=key
        passed.append(item)
    allowed={i['archive'] for i in passed}
    manifest['items']=[r for r in manifest['items'] if any(r['id'].startswith(a+'_') for a in allowed)]
    return passed,failed


def run(args):
    config, selection = read_json(args.config), read_json(args.selection)
    plan = prepare(config, selection)
    folder = PROJECT / "Saved/EquipmentImport/Items" / (selection["batch"] + "_" + uuid.uuid4().hex[:8])
    folder.mkdir(parents=True)
    report = {"ok": False, "apply": args.apply, "plan": str(folder / "plan.json"), 'items':[]}
    dcx_run.write(folder / "plan.json", plan)
    try:
        if not args.apply:
            report.update(ok=True, actions=["convert selected DCX", "import meshes and materials", "import UI icons", "create presentation-only ItemDefinition drafts"])
            return report
        tools = read_json(args.tools)
        # The existing DCX import checks this only when it launches UE itself.
        import subprocess
        running = subprocess.check_output(['tasklist', '/FO', 'CSV', '/NH'], creationflags=subprocess.CREATE_NO_WINDOW).decode(errors='replace')
        if re.search(r'"UnrealEditor(?:-Cmd)?\.exe"', running, re.I):
            raise ValueError("Close Unreal Editor before applying")
        version = read_json(Path(tools['ue_root']) / 'Engine/Build/Build.version')
        if f"{version['MajorVersion']}.{version['MinorVersion']}" != read_json(PROJECT / 'Umbra.uproject')['EngineAssociation']:
            raise ValueError("UE version mismatch")
        converted = dcx_run.run(SimpleNamespace(source=config['parts'], batch=selection['batch'], tools=args.tools,
                                    rules=args.rules, names=','.join(sorted({i['archive'] for i in plan['items']})),
                                    apply=True, convert_only=True))
        report['conversion'] = converted
        manifest = read_json(converted['manifest'])
        manifest['output_root'] = '/Game/EquipmentImport/' + plan['destination'] + '/' + plan['batch']
        plan['items'],report['items']=select_converted(plan['items'],converted,manifest)
        if not plan['items']:
            raise ValueError('No successful conversions; see per-item errors')
        rows = {r['id']: r for r in manifest['items']}
        for chosen in plan['items']:
            key = chosen['archive'] + '_' + chosen['primary_model']
            if key not in rows:
                raise ValueError("Selected primary model not in converted manifest: " + key)
            chosen['mesh_row_id'] = key
            rows[key]['usage'] = chosen['usage']
            if plan['combine_world_drop_components'] and rows[key]['kind']=='static':
                rows[key]['combine_static_meshes']=True
            # Conversion must not race source replacement.
            if digest(chosen['item']['models'][0]['path']) != chosen['model_sha256'] or digest(chosen['icon_path']) != chosen['icon_sha256']:
                raise ValueError("Selected source changed during conversion")
        dcx_run.write(folder / 'meshes.json', manifest)
        plan.update(mesh_manifest=str(folder / 'meshes.json'), output_dir=str(folder))
        dcx_run.write(folder / 'plan.json', plan)
        env = dict(os.environ, UMBRA_ITEM_PLAN=str(folder / 'plan.json'))
        try:
            dcx_run.execute([Path(tools['ue_root']) / 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe', PROJECT / 'Umbra.uproject',
                '-run=pythonscript', '-script=' + (HERE / 'import_items_editor.py').as_posix(),
                '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities', '-unattended', '-nop4',
                '-AllowCommandletRendering', '-nosound', '-stdout', '-abslog=' + str(folder / 'engine.log')],
                folder / 'process.log', env=env, timeout=7200)
        finally:
            if (folder/'editor.json').is_file():
                report['editor'] = read_json(folder / 'editor.json')
                report['items'].extend(report['editor']['items'])
        report['ok'] = converted['ok'] and report['editor']['ok']
    except Exception as exc:
        report['error'] = str(exc)
    finally:
        report['failed_ids']=[r['source_id'] for r in report['items'] if r['status']=='failed']
        dcx_run.write(folder / 'report.json', report)
        print('ITEM REPORT ' + str(folder / 'report.json'), flush=True)
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', default=str(HERE / 'item_catalog.local.example.json'))
    parser.add_argument('--selection', required=True)
    parser.add_argument('--tools', default=str(PROJECT / 'Saved/EquipmentImport/dcx_tools.json'))
    parser.add_argument('--rules', default=str(HERE / 'dcx_rules.json'))
    parser.add_argument('--apply', action='store_true')
    raise SystemExit(0 if run(parser.parse_args())['ok'] else 1)
