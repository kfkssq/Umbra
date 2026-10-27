"""Read-only UE AssetRegistry audit; run with PythonScriptPlugin commandlet."""
import json
import os
import unreal

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
audio_classes = {"SoundWave", "SoundCue", "MetaSoundSource", "MetaSoundPatch", "SoundClass", "SoundMix", "SoundAttenuation", "SoundConcurrency", "SoundSubmix"}
report = {"audio": [], "montages": [], "blueprints": []}
for asset in registry.get_assets_by_path("/Game", recursive=True):
    kind = str(asset.asset_class_path.asset_name)
    path = str(asset.package_name)
    if kind in audio_classes:
        report["audio"].append({"path": path, "class": kind})
    if kind == "AnimMontage":
        montage = asset.get_asset()
        record = {"path": path, "dependencies": [str(x) for x in registry.get_dependencies(asset.package_name, unreal.AssetRegistryDependencyOptions())]}
        try:
            record["notifies"] = []
            for event in unreal.AnimationLibrary.get_animation_notify_events(montage):
                notify = event.get_editor_property("notify") or event.get_editor_property("notify_state_class")
                record["notifies"].append({"class": notify.get_class().get_name() if notify else "None", "name": str(event.get_editor_property("notify_name"))})
        except Exception as error:
            record["inspection_unavailable"] = str(error)
        report["montages"].append(record)
    if kind in {"Blueprint", "WidgetBlueprint", "AnimBlueprint"} and not path.startswith(("/Game/Paragon", "/Game/Characters/Mannequins")):
        report["blueprints"].append({"path": path, "parent": str(asset.get_tag_value("ParentClass")), "dependencies": [str(x) for x in registry.get_dependencies(asset.package_name, unreal.AssetRegistryDependencyOptions())]})
output = os.path.join(unreal.Paths.project_saved_dir(), "AudioAssetAudit.json")
with open(output, "w", encoding="utf-8") as stream:
    json.dump(report, stream, ensure_ascii=False, indent=2)
unreal.log("AUDIO_AUDIT_COMPLETE " + output)
