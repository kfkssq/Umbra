"""Run inside matching Unreal Editor with PythonScriptPlugin enabled after C++ build.
Creates only missing definitions; never overwrites an existing asset.
"""
import unreal

ROWS = [
    ("Head", "1730 Helmet", "HEAD", 8, 4, 3, "/Game/Asset/Equipments/Head/head_01"),
    ("Chest", "1730 Chest Armor", "CHEST", 20, 8, 8, "/Game/Asset/Equipments/Body/body"),
    ("Hands", "1730 Gauntlets", "HANDS", 6, 3, 2, "/Game/Asset/Equipments/Arm/arm"),
    ("Legs", "1730 Leg Armor", "LEGS", 12, 5, 5, "/Game/Asset/Equipments/Legs/legs"),
]
DEST = "/Game/Items/Armor"
cls = unreal.load_class(None, "/Script/Umbra.UmbraItemDefinition")
if not cls:
    raise RuntimeError("Build and load UmbraEditor first.")

# Validate every source before creating any package. Filenames do not prove asset type.
meshes = []
for *_, path in ROWS:
    mesh = unreal.load_asset(path)
    if not isinstance(mesh, unreal.SkeletalMesh):
        raise RuntimeError(f"Expected SkeletalMesh at {path}; select the actual mesh in the editor.")
    meshes.append(mesh)

tools = unreal.AssetToolsHelpers.get_asset_tools()
for row, mesh in zip(ROWS, meshes):
    suffix, name, slot, armor, resistance, weight, _ = row
    asset_name = f"DA_Armor_1730_{suffix}"
    path = f"{DEST}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log_warning(f"Preserving existing asset: {path}; review values manually.")
        continue
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", cls)
    item = tools.create_asset(asset_name, DEST, cls, factory)
    if not item:
        raise RuntimeError(f"Could not create {path}")
    values = {
        "display_name": name, "item_category": unreal.UmbraItemCategory.ARMOR,
        "allowed_slots": [getattr(unreal.UmbraEquipmentSlot, slot)],
        "required_level": 1, "requirements": unreal.UmbraPrimaryValues(),
        "primary_bonuses": unreal.UmbraPrimaryValues(), "attribute_bonuses": [],
        "damage_bonuses": [], "affix_roll_configs": [], "weapon": None,
        "armor": armor, "magic_resistance": resistance, "weight": weight,
        "world_drop_skeletal_mesh": mesh, "world_drop_mesh": None,
        "world_drop_scale": unreal.Vector(1, 1, 1),
        "world_drop_rotation": unreal.Rotator(),
        "world_drop_offset": unreal.Vector(),
    }
    for key, value in values.items():
        item.set_editor_property(key, value)
    if not unreal.EditorAssetLibrary.save_loaded_asset(item):
        raise RuntimeError(f"Could not save {path}")
    unreal.log(f"Created armor: {path}, mesh={mesh.get_path_name()}")

# A new test-only BP provides a ready-to-place fixture without editing any map or enemy.
test_path = "/Game/Items/Loot/BP_LootTest_Armor1730"
if not unreal.EditorAssetLibrary.does_asset_exist(test_path):
    world_class = unreal.load_class(None, "/Game/Items/Loot/BP_WorldItem.BP_WorldItem_C")
    if not world_class:
        raise RuntimeError("Existing BP_WorldItem could not load; configure test spawner manually.")
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/Umbra.UmbraLootTestSpawner"))
    blueprint = tools.create_asset("BP_LootTest_Armor1730", "/Game/Items/Loot", unreal.Blueprint, factory)
    defaults = unreal.get_default_object(blueprint.generated_class())
    defaults.set_editor_property("items", [
        unreal.load_asset(f"{DEST}/DA_Armor_1730_{row[0]}") for row in ROWS
    ])
    defaults.set_editor_property("world_item_class", world_class)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint):
        raise RuntimeError("Could not save armor test Blueprint.")
    unreal.log(f"Created test fixture: {test_path}")

for mesh in meshes:
    materials = [str(m.get_editor_property("material_interface")) for m in mesh.get_editor_property("materials")]
    unreal.log(f"Armor source verified: {mesh.get_path_name()}, materials={materials}")
world_bp = unreal.load_asset("/Game/Items/Loot/BP_WorldItem")
world_defaults = unreal.get_default_object(world_bp.generated_class())
if not isinstance(world_defaults, unreal.UmbraWorldItem):
    raise RuntimeError("BP_WorldItem must derive from UmbraWorldItem.")
unreal.log("World item verified: derives from UmbraWorldItem")
unreal.log(f"World item label class: {world_defaults.get_editor_property('label').get_editor_property('widget_class')}")
