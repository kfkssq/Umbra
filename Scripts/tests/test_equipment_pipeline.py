import copy
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("equipment_pipeline",ROOT/"equipment_pipeline.py")
p=importlib.util.module_from_spec(spec); spec.loader.exec_module(p)

class Asset:
    def __init__(self): self.values={k:[1,1,1] for k in p.PHYSICS}; self.meta={}
    def try_fit_world_drop_physics(self): self.values.update({k:[2,2,2] for k in p.PHYSICS}); return True

class Backend:
    def __init__(self,store=None,fail_after=0): self.store=store if store is not None else {}; self.saved=[]; self.fail_after=fail_after
    def exists(self,path): return path in self.store
    def load(self,path,kind): return self.store[path]
    def metadata(self,asset): return copy.deepcopy(asset.meta)
    def read(self,asset,keys): return {k:copy.deepcopy(asset.values.get(k)) for k in keys}
    def preflight(self,row):
        if row.get("test_error"): raise p.PipelineError("wrong reference type")
        return []
    def import_missing(self,*args): pass
    def create(self,path): self.store[path]=Asset(); return self.store[path]
    def set_fields(self,asset,fields): asset.values.update(copy.deepcopy(fields))
    def native_validate(self,asset): return []
    def finalize(self,asset,baseline):
        asset.meta=copy.deepcopy(baseline)
        self.saved.append(next(path for path,value in self.store.items() if value is asset))
        if self.fail_after and len(self.saved)==self.fail_after: raise RuntimeError("interrupted after package commit")

class PipelineTests(unittest.TestCase):
    def setUp(self): self.manifest=p.load_manifest(ROOT/"EquipmentPipeline/samples.json")
    def test_dry_run_never_writes(self):
        backend=Backend(); report=p.run(self.manifest,backend)
        self.assertEqual(report["status"],"planned"); self.assertFalse(backend.store); self.assertFalse(report["saved_packages"])
    def test_second_run_zero_saves(self):
        backend=Backend(); p.run(self.manifest,backend,apply=True)
        second=Backend(backend.store); report=p.run(self.manifest,second,mode="UpdateManagedFields",apply=True)
        self.assertEqual(report["status"],"success"); self.assertFalse(report["saved_packages"])
        self.assertTrue(all(r["action"]=="unchanged" for r in report["rows"]))
    def test_manual_conflict_blocks_entire_batch(self):
        backend=Backend(); p.run(self.manifest,backend,apply=True)
        row=self.manifest["items"][0]; backend.store[row["asset_path"]].values["weight"]=999
        edited=copy.deepcopy(self.manifest); edited["items"][1]["fields"]["weight"]=42
        report=p.run(edited,Backend(backend.store),mode="UpdateManagedFields",apply=True)
        self.assertEqual(report["status"],"blocked"); self.assertFalse(report["saved_packages"])
        self.assertEqual(backend.store[row["asset_path"]].values["weight"],999)
    def test_managed_update_and_locked_override(self):
        backend=Backend(); p.run(self.manifest,backend,apply=True)
        row=self.manifest["items"][0]; asset=backend.store[row["asset_path"]]
        asset.values["weight"]=999; row["locked_fields"]=["weight"]; row["fields"]["display_name"]="Updated"
        report=p.run(self.manifest,Backend(backend.store),mode="UpdateManagedFields",apply=True)
        self.assertEqual(report["status"],"success"); self.assertEqual(asset.values["weight"],999)
        self.assertEqual(asset.values["display_name"],"Updated")
        row["locked_fields"]=[]
        self.assertEqual(p.run(self.manifest,Backend(backend.store),mode="UpdateManagedFields")["status"],"blocked")
    def test_create_missing_preserves_edits(self):
        backend=Backend(); p.run(self.manifest,backend,apply=True)
        row=self.manifest["items"][0]; backend.store[row["asset_path"]].values["weight"]=999
        report=p.run(self.manifest,Backend(backend.store),apply=True)
        self.assertFalse(report["saved_packages"]); self.assertEqual(report["rows"][0]["action"],"preserve_existing")
    def test_resume_after_commit(self):
        backend=Backend(fail_after=1)
        self.assertEqual(p.run(self.manifest,backend,apply=True)["status"],"partial_failure")
        recovered=Backend(backend.store); report=p.run(self.manifest,recovered,mode="UpdateManagedFields",apply=True)
        self.assertEqual(report["status"],"success"); self.assertEqual(len(recovered.saved),9)
        self.assertEqual(len(recovered.store),10)
    def test_selected_ids_and_unknown_ids(self):
        backend=Backend(); p.run(self.manifest,backend,apply=True,ids=[self.manifest["items"][2]["id"]])
        self.assertEqual(len(backend.store),1)
        with self.assertRaises(p.PipelineError): p.run(self.manifest,backend,ids=["unknown"])
    def test_wrong_reference_prevents_creation(self):
        self.manifest["items"][-1]["test_error"]=True
        backend=Backend(); report=p.run(self.manifest,backend,apply=True)
        self.assertEqual(report["status"],"blocked"); self.assertFalse(backend.store)
    def test_source_validator_rejects_bad_recipes(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp); shutil.copytree(ROOT/"EquipmentPipeline/Source",root/"Source")
            shutil.copy(ROOT/"EquipmentPipeline/PROVENANCE.md",root/"PROVENANCE.md")
            changes=[lambda m:m["items"][0]["mesh"].update(sha256="bad"),
                     lambda m:m["items"][0].pop("icon"),
                     lambda m:m["items"][0]["fields"].pop("generation_profile"),
                     lambda m:m["items"][0]["provenance"].update(status="pending"),
                     lambda m:m["items"][1].update(id=m["items"][0]["id"]),
                     lambda m:m["items"][1].update(id=m["items"][0]["id"].lower()),
                     lambda m:m["items"][0].update(locked_fields=["world_drop_physics_extent"]),
                     lambda m:m["items"][0].update(fit_physics="false"),
                     lambda m:m["items"][0].update(asset_path="/Game/Production/Overwrite"),
                     lambda m:m["items"][0]["mesh"].update(source_file="../outside.obj")]
            for change in changes:
                manifest=copy.deepcopy(self.manifest); change(manifest)
                path=root/"bad.json"; path.write_text(json.dumps(manifest),encoding="utf-8")
                with self.assertRaises(p.PipelineError): p.load_manifest(path)

if __name__=="__main__": unittest.main()
