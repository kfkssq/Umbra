"""Run in matching Unreal Editor. Cook triangle query data for the four armor meshes."""
import pathlib
import shutil
import datetime
import unreal

root = pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
backup = root / "Saved" / "ArmorCursorCollision" / datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
seen = set()
for suffix in ("Head", "Chest", "Hand", "Legs"):
    definition = unreal.load_asset(f"/Game/Items/Armor/DA_Armor_1730_{suffix}")
    mesh = definition.get_editor_property("world_drop_skeletal_mesh")
    if not isinstance(mesh, unreal.SkeletalMesh):
        raise RuntimeError(f"Missing skeletal model for {suffix}")
    path = mesh.get_path_name().split(".")[0]
    if path in seen:
        continue
    seen.add(path)
    relative = path.removeprefix("/Game/") + ".uasset"
    destination = backup / relative
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(root / "Content" / relative, destination)
    mesh.set_editor_property("enable_per_poly_collision", True)
    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh):
        raise RuntimeError(f"Could not save collision data: {path}")
    unreal.log(f"ARMOR_CURSOR_COLLISION {path}: {mesh.get_editor_property('enable_per_poly_collision')}")
