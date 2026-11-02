"""Manifest-driven UE equipment authoring. Default dry run; import never replaces existing assets.
Standalone Python validates source/manifest; Unreal Python supplies asset inspection and writes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import time

VERSION = "1.0"
META = "UmbraPipeline.Baseline"
PHYSICS = ("world_drop_physics_extent", "world_drop_physics_center", "world_drop_physics_rotation")
VECTORS = {"world_drop_scale", "world_drop_offset", *PHYSICS[:2]}
ROTATIONS = {"world_drop_rotation", PHYSICS[2]}
ENUMS = {"item_category": "UmbraItemCategory", "weapon_type": "UmbraWeaponType", "rarity": "UmbraItemRarity"}
REFERENCES = {"weapon": "UmbraWeaponProfile", "generation_profile": "UmbraItemGenerationProfile",
              "icon": "Texture2D", "world_drop_mesh": "StaticMesh", "world_drop_skeletal_mesh": "SkeletalMesh"}
NUMBERS = {"armor", "magic_resistance", "weight", "required_level", "item_level"}
FIELDS = set(ENUMS) | set(REFERENCES) | VECTORS | ROTATIONS | NUMBERS | {
    "definition_id", "display_name", "flavor_text", "allowed_slots", "requirements", "primary_bonuses"}

class PipelineError(ValueError):
    pass

def require(ok, message):
    if not ok:
        raise PipelineError(message)

def digest(path):
    with Path(path).open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()

def stable(value):
    return json.dumps(value, sort_keys=True, ensure_ascii=False, separators=(",", ":"))

def package(path):
    return isinstance(path, str) and bool(re.fullmatch(r"/Game/(?:[A-Za-z0-9_]+/)*[A-Za-z0-9_]+", path))

def source_path(base, relative):
    path = (base / relative).resolve()
    require(path.is_relative_to(base.resolve()), "Source path escapes manifest folder: " + relative)
    require(path.is_file(), "Source file missing: " + relative)
    return path

def load_manifest(path):
    path = Path(path).resolve()
    data = json.loads(path.read_text(encoding="utf-8"))
    require(data.get("schema_version") == 1, "Unsupported schema_version")
    root = data.get("output_root", "")
    require(package(root) and root.startswith(("/Game/Items/Tests/", "/Game/Items/Staging/")), "Output must be an isolated Tests/Staging root")
    require(re.fullmatch(r"[A-Za-z0-9_.-]+", data.get("manifest_id", "")), "Missing manifest_id")
    rows = data.get("items", [])
    require(isinstance(rows, list) and rows, "Empty items")
    ids, targets, imports = set(), set(), {}
    for row in rows:
        identity = row.get("id", "")
        require(re.fullmatch(r"[A-Za-z0-9_.-]+", identity), "Invalid stable id")
        require(identity.casefold() not in ids, "Duplicate id: " + identity)
        ids.add(identity.casefold())
        target = row.get("asset_path", "")
        require(package(target) and target.startswith(root + "/"), "Definition outside output_root: " + target)
        require(target.casefold() not in targets, "Duplicate definition path: " + target)
        targets.add(target.casefold())
        provenance = row.get("provenance", {})
        require(all(provenance.get(k) for k in ("author", "license", "record", "status")), identity + ": incomplete provenance")
        source_path(path.parent, provenance["record"])
        require(provenance["status"] in ("project_authored", "verified"), identity + ": provenance pending; cannot import")
        fields = row.get("fields", {})
        require(isinstance(fields, dict) and set(fields) <= FIELDS, identity + ": unsupported fields")
        require(fields.get("item_category") in ("WEAPON", "ARMOR", "ACCESSORY"), identity + ": invalid category")
        slots = fields.get("allowed_slots", [])
        valid_slots = {"HEAD","CHEST","HANDS","LEGS","FEET","RING1","RING2","AMULET","MAIN_HAND","OFF_HAND"}
        require(slots and len(set(slots)) == len(slots) and set(slots) <= valid_slots, identity + ": invalid slots")
        if fields["item_category"] == "WEAPON":
            require(slots == ["MAIN_HAND"] and fields.get("weapon"), identity + ": weapon requires MainHand and profile")
        else:
            require(not fields.get("weapon"), identity + ": nonweapon cannot carry weapon profile")
        require(fields.get("generation_profile"), identity + ": generation_profile required for this pipeline")
        for key, value in fields.items():
            if key in REFERENCES and value is not None:
                require(package(value), identity + ": invalid reference " + key)
            if key in VECTORS | ROTATIONS:
                require(isinstance(value,list) and len(value)==3 and all(isinstance(v,(int,float)) and math.isfinite(v) for v in value), identity + ": invalid vector " + key)
                if key == "world_drop_scale":
                    require(all(v > 0 for v in value), identity + ": nonpositive scale")
            if key in NUMBERS:
                require(isinstance(value,(int,float)) and not isinstance(value,bool) and math.isfinite(value) and value >= (1 if key.endswith("level") else 0), identity + ": invalid number " + key)
                if key.endswith("level"):
                    require(isinstance(value,int), identity + ": level must be integer")
            if key in ("requirements", "primary_bonuses"):
                require(isinstance(value,dict) and set(value)=={"strength","dexterity","intelligence","faith"} and all(isinstance(v,(int,float)) and math.isfinite(v) and v>=0 for v in value.values()), identity + ": invalid primaries")
        managed = row.get("managed_fields", [])
        locks = row.get("locked_fields", [])
        require(len(set(managed))==len(managed) and set(managed)<=FIELDS, identity + ": invalid managed_fields")
        require(set(locks)<=FIELDS, identity + ": invalid locked_fields")
        require("definition_id" not in locks, identity + ": identity cannot be locked to another id")
        require(isinstance(row.get("fit_physics",False),bool), identity + ": fit_physics must be boolean")
        if row.get("fit_physics"):
            require(not set(PHYSICS).intersection(set(locks) | set(fields)), identity + ": automatic fit conflicts with authored/locked physics")
        for label in ("mesh", "icon"):
            spec = row.get(label, {})
            require(package(spec.get("asset_path")), identity + ": missing " + label)
            if label == "mesh":
                require(spec.get("kind") in ("static", "skeletal"), identity + ": mesh kind must be explicit")
                if spec["kind"] == "skeletal":
                    require(package(spec.get("skeleton")), identity + ": skeletal imports require explicit existing skeleton")
            if spec.get("source_file"):
                require(spec["asset_path"].startswith(root+"/"), identity + ": import outside output_root")
                source = source_path(path.parent, spec["source_file"])
                require(source.suffix.lower() in ((".png", ".tga") if label=="icon" else ((".fbx",) if spec["kind"]=="skeletal" else (".fbx", ".obj"))), identity + ": unsupported source format")
                require(spec.get("sha256") == digest(source), identity + ": source hash mismatch: " + label)
                recipe = stable(spec)
                destination=spec["asset_path"].casefold()
                require(destination not in imports or imports[destination]==recipe, "Conflicting import recipes")
                imports[destination] = recipe
        desired = desired_fields(row)
        require(set(desired) <= set(managed), identity + ": every authored field must be managed (locks can protect it)")
    require(not targets.intersection(imports), "Definition and import destinations overlap")
    return data

def desired_fields(row):
    fields = dict(row["fields"])
    fields.update(definition_id=row["id"], icon=row["icon"]["asset_path"],
                  world_drop_mesh=row["mesh"]["asset_path"] if row["mesh"]["kind"]=="static" else None,
                  world_drop_skeletal_mesh=row["mesh"]["asset_path"] if row["mesh"]["kind"]=="skeletal" else None)
    return fields

def compare_fields(desired, current, baseline, locks):
    changes, conflicts, locked = {}, {}, []
    for key, wanted in desired.items():
        if key in locks:
            if current.get(key)!=wanted:
                locked.append(key)
            continue
        actual = current.get(key)
        if actual == wanted:
            continue
        if key not in baseline or actual != baseline[key]:
            conflicts[key] = {"baseline":baseline.get(key), "actual":actual, "desired":wanted}
        else:
            changes[key] = wanted
    return changes, conflicts, locked

class UnrealBackend:
    def __init__(self, base):
        import unreal
        self.u = unreal
        self.base = Path(base)
        self.saved = []

    def load(self, path, expected):
        asset = self.u.load_asset(path)
        require(isinstance(asset, getattr(self.u,expected)), "Missing/wrong type: " + path + " expected " + expected)
        return asset

    def exists(self, path):
        return self.u.EditorAssetLibrary.does_asset_exist(path)

    def metadata(self, asset, key=META):
        value = self.u.EditorAssetLibrary.get_metadata_tag(asset,key)
        return json.loads(value) if value else {}

    def read(self, asset, keys):
        result={}
        for key in keys:
            value=asset.get_editor_property(key)
            if key in REFERENCES:
                result[key]=value.get_path_name().split(".")[0] if value else None
            elif key in VECTORS | ROTATIONS:
                result[key]=[round(float(getattr(value,c)),6) for c in (("pitch","yaw","roll") if key in ROTATIONS else ("x","y","z"))]
            elif key in ENUMS:
                result[key]=str(value).split(".")[-1].split(":")[0].strip(">")
            elif key=="allowed_slots":
                result[key]=[str(v).split(".")[-1].split(":")[0].strip(">") for v in value]
            elif key in ("requirements","primary_bonuses"):
                result[key]={k:round(float(getattr(value,k)),6) for k in ("strength","dexterity","intelligence","faith")}
            elif key in NUMBERS:
                result[key]=round(float(value),6)
            else:
                result[key]=str(value)
        return result

    def set_fields(self, asset, values):
        u=self.u
        for key,value in values.items():
            if key in REFERENCES:
                value=self.load(value,REFERENCES[key]) if value else None
            elif key in ENUMS:
                value=getattr(getattr(u,ENUMS[key]),value)
            elif key in VECTORS:
                value=u.Vector(*value)
            elif key in ROTATIONS:
                value=u.Rotator(pitch=value[0],yaw=value[1],roll=value[2])
            elif key=="allowed_slots":
                value=[getattr(u.UmbraEquipmentSlot,v) for v in value]
            elif key in ("requirements","primary_bonuses"):
                value=u.UmbraPrimaryValues(**value)
            asset.set_editor_property(key,value)

    def import_preflight(self,spec,kind):
        if self.exists(spec["asset_path"]):
            asset=self.load(spec["asset_path"],kind)
            if spec.get("source_file"):
                meta=self.metadata(asset,"UmbraPipeline.Import")
                require(meta.get("recipe")==stable(spec), "Existing import changed/unmanaged; use a new destination: "+spec["asset_path"])
        else:
            require(spec.get("source_file"),"Missing reference without import source: "+spec["asset_path"])
        if kind=="SkeletalMesh":
            self.load(spec["skeleton"],"Skeleton")
        for path in spec.get("materials",{}).values():
            self.load(path,"MaterialInterface")

    def native_validate(self, asset):
        result, errors, warnings = self.u.get_editor_subsystem(self.u.EditorValidatorSubsystem).is_object_valid(asset,self.u.DataValidationUsecase.SCRIPT)
        require(result != self.u.DataValidationResult.INVALID,"Native validation: "+"; ".join(str(e) for e in errors))
        return [str(w) for w in warnings]

    def preflight(self,row):
        for key,kind in (("mesh","StaticMesh" if row["mesh"]["kind"]=="static" else "SkeletalMesh"),("icon","Texture2D")):
            self.import_preflight(row[key],kind)
        values=desired_fields(row)
        pending={row[k]["asset_path"] for k in ("mesh","icon") if not self.exists(row[k]["asset_path"])}
        for key,value in values.items():
            if key in REFERENCES and value and value not in pending:
                self.load(value,REFERENCES[key])
        # Check generation rules on an unsaved transient object, never a destination package.
        candidate=self.u.new_object(self.u.UmbraItemDefinition)
        self.set_fields(candidate,{k:v for k,v in values.items() if k not in REFERENCES or v not in pending})
        return self.native_validate(candidate)

    def save(self,asset):
        require(self.u.EditorAssetLibrary.save_loaded_asset(asset),"Save failed: "+asset.get_path_name())
        self.saved.append(asset.get_path_name().split(".")[0])

    def import_missing(self,spec,kind):
        if self.exists(spec["asset_path"]):
            return self.load(spec["asset_path"],kind)
        u=self.u
        task=u.AssetImportTask()
        task.filename=str(source_path(self.base,spec["source_file"]))
        task.destination_path,task.destination_name=spec["asset_path"].rsplit("/",1)
        task.automated=True; task.replace_existing=False; task.save=False
        if kind in ("StaticMesh","SkeletalMesh"):
            options=u.FbxImportUI()
            options.automated_import_should_detect_type=False
            options.import_mesh=True; options.import_materials=False; options.import_textures=False; options.import_animations=False
            options.import_as_skeletal=kind=="SkeletalMesh"
            options.mesh_type_to_import=u.FBXImportType.FBXIT_SKELETAL_MESH if kind=="SkeletalMesh" else u.FBXImportType.FBXIT_STATIC_MESH
            options.create_physics_asset=False
            if kind=="SkeletalMesh":
                options.skeleton=self.load(spec["skeleton"],"Skeleton")
            else:
                options.static_mesh_import_data.combine_meshes=True
                options.static_mesh_import_data.auto_generate_collision=False
                options.static_mesh_import_data.import_uniform_scale=1.0
            task.options=options; task.factory=u.FbxFactory()
        else:
            task.factory=u.TextureFactory()
        u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        imported={str(v).split(".")[0] for v in task.imported_object_paths}
        require(imported=={spec["asset_path"]},"Unexpected import outputs: "+str(sorted(imported)))
        asset=self.load(spec["asset_path"],kind)
        if kind=="Texture2D":
            asset.set_editor_property("compression_settings",u.TextureCompressionSettings.TC_EDITOR_ICON)
            asset.set_editor_property("lod_group",u.TextureGroup.TEXTUREGROUP_UI)
            asset.set_editor_property("srgb",True)
        if kind=="SkeletalMesh":
            require(asset.get_editor_property("skeleton")==self.load(spec["skeleton"],"Skeleton"),"Imported skeleton mismatch")
        if kind in ("StaticMesh","SkeletalMesh"):
            slots=asset.get_editor_property("static_materials" if kind=="StaticMesh" else "materials")
            for name,path in spec.get("materials",{}).items():
                matches=[i for i,s in enumerate(slots) if str(s.material_slot_name)==name]
                require(len(matches)==1,"Material slot missing/ambiguous: "+name)
                material=self.load(path,"MaterialInterface")
                if kind=="StaticMesh":
                    asset.set_material(matches[0],material)
                else:
                    slots[matches[0]].set_editor_property("material_interface",material)
            if kind=="SkeletalMesh" and spec.get("materials"):
                asset.set_editor_property("materials",slots)
        u.EditorAssetLibrary.set_metadata_tag(asset,"UmbraPipeline.Import",stable({"recipe":stable(spec),"tool":VERSION}))
        self.save(asset)
        return asset

    def create(self,path):
        folder,name=path.rsplit("/",1)
        factory=self.u.DataAssetFactory(); factory.set_editor_property("data_asset_class",self.u.UmbraItemDefinition)
        asset=self.u.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,self.u.UmbraItemDefinition,factory)
        require(asset is not None,"Cannot create definition: "+path)
        return asset

    def finalize(self,asset,baseline):
        self.u.EditorAssetLibrary.set_metadata_tag(asset,META,stable(baseline))
        self.save(asset)

def run(manifest,backend,mode="CreateMissingOnly",apply=False,ids=None,report_path=None):
    require(mode in ("CreateMissingOnly","UpdateManagedFields"),"Invalid mode")
    requested=set(ids or [])
    require(not requested or requested <= {r["id"] for r in manifest["items"]},"Unknown selected IDs")
    report={"tool":VERSION,"manifest_id":manifest["manifest_id"],"mode":mode,"dry_run":not apply,"rows":[],"saved_packages":[],"status":"planned"}
    started=time.perf_counter()
    plans=[]
    for row in manifest["items"]:
        if requested and row["id"] not in requested:
            continue
        entry={"id":row["id"],"asset_path":row["asset_path"],"recipe_hash":hashlib.sha256(stable(row).encode()).hexdigest(),"errors":[],"manual_checks":["Icon crop/readability", "mesh scale/materials/collision", "world-only; wearing compatibility not certified"]}
        report["rows"].append(entry)
        try:
            entry["warnings"]=backend.preflight(row)
            wanted=desired_fields(row)
            asset=backend.load(row["asset_path"],"UmbraItemDefinition") if backend.exists(row["asset_path"]) else None
            old=backend.metadata(asset) if asset else {}
            if asset:
                current=backend.read(asset,set(wanted)|set(PHYSICS))
                require(current["definition_id"]==row["id"],"Stable ID mismatch")
                if mode=="CreateMissingOnly":
                    entry.update(action="preserve_existing",differences={k:{"actual":current[k],"desired":v} for k,v in wanted.items() if current[k]!=v})
                    plans.append((row,entry,asset,old,{},False)); continue
                require(old.get("manifest_id")==manifest["manifest_id"] and old.get("id")==row["id"],"Existing definition has no matching ownership baseline")
                changes,conflicts,locks=compare_fields(wanted,current,old.get("fields",{}),row.get("locked_fields",[]))
                entry.update(changes=changes,conflicts=conflicts,locked_fields=locks)
                require(not conflicts,"Manual field conflict")
                fit=bool(row.get("fit_physics")) and (old.get("fit_recipe") != stable(row["mesh"]) or any(k.startswith("world_drop_") for k in changes))
                if fit:
                    require(not set(PHYSICS).intersection(row.get("locked_fields",[])),"Physics fields locked; fit requires manual handling")
                    require(all(current[k]==old.get("fields",{}).get(k) for k in PHYSICS),"Manual physics conflict")
                entry["action"]="update" if changes or fit else "unchanged"
            else:
                changes=wanted
                require(not set(wanted).intersection(row.get("locked_fields",[])),"New asset cannot initialize locked fields")
                fit=bool(row.get("fit_physics"))
                entry.update(action="create",changes=changes)
            plans.append((row,entry,asset,old,changes,fit))
        except Exception as error:
            entry.update(action="blocked",errors=[str(error)])
    if any(e["errors"] for e in report["rows"]):
        report["status"]="blocked"
    elif apply:
        report["status"]="success"
        for row,entry,asset,old,changes,fit in plans:
            if entry["action"] in ("preserve_existing","unchanged"):
                continue
            try:
                backend.import_missing(row["mesh"],"StaticMesh" if row["mesh"]["kind"]=="static" else "SkeletalMesh")
                backend.import_missing(row["icon"],"Texture2D")
                asset=asset or backend.create(row["asset_path"])
                backend.set_fields(asset,changes)
                if fit:
                    require(asset.try_fit_world_drop_physics(),"Physics fit failed; definition not saved")
                backend.native_validate(asset)
                fields=dict(old.get("fields",{}))
                # Preserve locked baselines, so unlocking a manual override still requires conflict resolution.
                keys=set(changes)|(set(PHYSICS) if fit else set())
                fields.update(backend.read(asset,keys))
                baseline={"manifest_id":manifest["manifest_id"],"id":row["id"],"fields":fields,"recipe_hash":entry["recipe_hash"],"tool":VERSION,
                          "provenance":row["provenance"],"fit_recipe":stable(row["mesh"]) if fit else old.get("fit_recipe")}
                backend.finalize(asset,baseline)
                entry["status"]="saved"
            except Exception as error:
                entry["errors"].append(str(error)); entry["status"]="failed"; report["status"]="partial_failure"
                # Stop rather than saving other packages after an unexpected import/save error.
                break
    report["saved_packages"]=list(backend.saved)
    report["elapsed_seconds"]=round(time.perf_counter()-started,3)
    if report_path:
        target=Path(report_path); target.parent.mkdir(parents=True,exist_ok=True)
        temporary=target.with_suffix(".tmp")
        temporary.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding="utf-8")
        temporary.replace(target)
    return report

def main():
    try:
        import unreal
    except ImportError:
        parser=argparse.ArgumentParser(description=__doc__)
        parser.add_argument("--manifest",required=True)
        args=parser.parse_args()
        data=load_manifest(args.manifest)
        print(json.dumps({"status":"source_valid","items":len(data["items"]),"asset_checks":"requires Unreal Editor"}))
        return
    command=unreal.SystemLibrary.get_command_line()
    def arg(name,default=None):
        match=re.search(r"-"+name+r'=(?:"([^"]+)"|([^\s]+))',command,re.I)
        return (match.group(1) or match.group(2)) if match else default
    manifest=Path(arg("UmbraManifest",str(Path(__file__).parent/"EquipmentPipeline"/"samples.json"))).resolve()
    report_path=arg("UmbraPipelineReport",str(Path(unreal.Paths.project_saved_dir())/"EquipmentPipeline"/"report.json"))
    result=run(load_manifest(manifest),UnrealBackend(manifest.parent),arg("UmbraPipelineMode","CreateMissingOnly"),
               bool(re.search(r"-UmbraPipelineApply(?:\s|$)",command,re.I)),[x for x in arg("UmbraPipelineIds","").split(",") if x],report_path)
    unreal.log("EQUIPMENT_PIPELINE "+stable({"status":result["status"],"saved":len(result["saved_packages"]),"report":report_path}))
    if result["status"] in ("blocked","partial_failure"):
        raise PipelineError("Pipeline failed; inspect "+report_path)

if __name__=="__main__":
    main()
