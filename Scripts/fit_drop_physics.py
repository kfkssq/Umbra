"""Run with matching UE 5.8 editor Python after build, with other editors closed.
Only fits/saves per-item collision geometry. Backs up original bytes before UE saves.
No gameplay, visual transform, Blueprint or map values are edited.
"""
import json
import pathlib
import shutil
import datetime
import unreal

project = pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
backup = project / "Saved" / "DropPhysicsFit" / datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
backup.mkdir(parents=True, exist_ok=True)
rows = []
for path in unreal.EditorAssetLibrary.list_assets("/Game/Items", recursive=True, include_folder=False):
    data = unreal.EditorAssetLibrary.find_asset_data(path)
    if str(data.asset_class_path.asset_name) != "UmbraItemDefinition":
        continue
    item = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(item, unreal.UmbraItemDefinition):
        continue
    if not item.get_editor_property("world_drop_mesh") and not item.get_editor_property("world_drop_skeletal_mesh"):
        continue
    package = str(data.package_name)
    source = project / "Content" / (package.removeprefix("/Game/") + ".uasset")
    destination = backup / (package.removeprefix("/Game/") + ".uasset")
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
    before = {key: str(item.get_editor_property(key)) for key in ("world_drop_physics_extent", "world_drop_physics_center", "world_drop_physics_rotation")}
    item.fit_world_drop_physics()
    after = {key: str(item.get_editor_property(key)) for key in before}
    if not unreal.EditorAssetLibrary.save_loaded_asset(item):
        raise RuntimeError(f"Cannot save fitted geometry: {package}")
    rows.append({"asset": package, "before": before, "after": after})
    unreal.log(f"DROP_FIT {package}: {after}")
(backup / "report.json").write_text(json.dumps(rows, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log(f"DROP_FIT_COMPLETE count={len(rows)} report={backup / 'report.json'}")
if not rows:
    raise RuntimeError("No item definitions were fitted")
