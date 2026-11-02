"""Read-only source catalog. Does not import UE assets or write ItemDefinitions."""
import argparse
import collections
import csv
import hashlib
import json
import re
from datetime import datetime, timezone
from pathlib import Path


def read_json(path):
    return json.loads(Path(path).read_text(encoding="utf-8-sig"))


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def valid_text(value):
    return bool(value and value.strip() and value.strip() not in {"[ERROR]", "dummy", "DLC dummy"})


def text_index(document):
    # Preserve conflicts across base/DLC instead of guessing an override order.
    index = collections.defaultdict(list)
    for wrapper in document["FmgWrappers"]:
        name = wrapper["Name"]
        match = re.fullmatch(r"(Weapon|Protector)(Name|Info|Caption)(?:_dlc\d+)?\.fmg", name)
        if not match:
            continue
        for entry in wrapper["Fmg"]["Entries"]:
            if valid_text(entry.get("Text")):
                index[(match[1], match[2], int(entry["ID"]))].append(
                    {"file": name, "text": entry["Text"]})
    return index


def resolve_text(index, kind, field, row_id):
    candidates = index.get((kind, field, row_id), [])
    values = {v["text"] for v in candidates}
    return {"status": "matched" if len(values) == 1 else "conflict" if values else "missing",
            "value": next(iter(values)) if len(values) == 1 else None, "sources": candidates}


def icon_index(root):
    result = collections.defaultdict(list)
    for path in sorted(Path(root).rglob("*")):
        if not path.is_file():
            continue
        match = re.fullmatch(r"menu_knowledge_(\d+)out(?:\.obj)?\.png", path.name, re.I)
        if match:
            result[int(match[1])].append(str(path.resolve()))
    return result


def resolve_icon(index, number):
    paths = index.get(number, [])
    return {"id": number, "status": "matched_by_id" if len(paths) == 1 else "ambiguous" if paths else "missing",
            "paths": paths, "visual_verified": False}


def model_candidates(row, kind, parts):
    model = int(row["equipModelId"])
    category = int(row["equipModelCategory"])
    if model <= 0:
        return [], "no_model"
    if kind == "Weapon":
        if category != 7:
            return [], "unsupported_model_category"
        names = [f"wp_a_{model:04d}"]
    else:
        prefix = {0: "hd", 1: "bd", 2: "am", 3: "lg"}.get(int(row["protectorCategory"]))
        expected = {"hd": 5, "bd": 2, "am": 1, "lg": 6}
        if not prefix or expected[prefix] != category:
            return [], "unsupported_model_category"
        # Presence is indexed, but gender applicability is not inferred from filenames.
        names = [f"{prefix}_{gender}_{model:04d}" for gender in ("m", "f")]
    found = [{"archive": name, "path": str((parts / (name + ".partsbnd.dcx")).resolve()),
              "role": "WorldDrop" if kind == "Weapon" else "WearableSource"}
             for name in names if (parts / (name + ".partsbnd.dcx")).is_file()]
    return found, "matched_candidates" if found else "missing"


def load_rows(path, kind):
    with Path(path).open(encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream)
        required = {"ID", "equipModelId", "equipModelCategory", "equipModelGender"}
        required |= {"wepType", "iconId"} if kind == "Weapon" else {"protectorCategory", "iconIdM", "iconIdF"}
        if not required.issubset(reader.fieldnames or []):
            raise ValueError(f"{path}: missing columns {sorted(required - set(reader.fieldnames or []))}")
        rows = list(reader)
    seen = set()
    for row in rows:
        if None in row or any(value is None for value in row.values()):
            raise ValueError(f"{path}: malformed CSV row")
        number = int(row["ID"])
        if number in seen:
            raise ValueError(f"{path}: duplicate row ID {number}")
        seen.add(number)
        for key in required - {"ID"}:
            int(row[key])
    return rows


def build(config):
    for key in ("weapon_csv", "armor_csv", "text_json", "parts", "icons", "weapon_enum", "mapping"):
        if not Path(config[key]).exists():
            raise ValueError(f"Missing input {key}: {config[key]}")
    mapping = read_json(config["mapping"])
    labels = {int(o["Key"]): next((n["Text"] for n in o["Names"] if n["Language"] == "English"), o["Key"])
              for o in read_json(config["weapon_enum"])["Options"]}
    texts = text_index(read_json(config["text_json"]))
    icons = icon_index(config["icons"])
    parts = Path(config["parts"])
    items, issues = [], []
    resources = collections.defaultdict(list)
    for kind, source in (("Weapon", config["weapon_csv"]), ("Protector", config["armor_csv"])):
        for row in load_rows(source, kind):
            number = int(row["ID"])
            identity = f"ER:EquipParam{kind}:{number}"
            code = row["wepType" if kind == "Weapon" else "protectorCategory"]
            classification = mapping["weapons" if kind == "Weapon" else "armor"].get(code)
            name = resolve_text(texts, kind, "Name", number)
            description = resolve_text(texts, kind, "Caption", number)
            models, model_status = model_candidates(row, kind, parts)
            icon_fields = ["iconId"] if kind == "Weapon" else ["iconIdM", "iconIdF"]
            item_icons = {field: resolve_icon(icons, int(row[field])) for field in icon_fields}
            problems = []
            if name["status"] != "matched":
                problems.append("name_" + name["status"])
            if description["status"] != "matched":
                problems.append("description_" + description["status"])
            if not classification:
                problems.append("category_unmapped")
            if not models:
                problems.append("model_" + model_status)
            for field, value in item_icons.items():
                if value["status"] != "matched_by_id":
                    problems.append(field + "_" + value["status"])
            if kind == "Protector" and models:
                problems.append("wearable_gender_and_skeleton_review")
            item = {"source_id": identity, "table": f"EquipParam{kind}", "row_id": number,
                    "name": name, "description": description,
                    "summary": resolve_text(texts, kind, "Info", number),
                    "source_category_code": int(code),
                    "source_category": labels.get(int(code), f"Unknown ({code})") if kind == "Weapon"
                    else {0: "Head", 1: "Body", 2: "Arms", 3: "Legs", 4: "Hair"}.get(int(code), "Unknown"),
                    "umbra_suggestion": classification,
                    "classification_status": "suggested_requires_review" if classification else "unmapped",
                    "icons": item_icons, "models": models, "model_status": model_status,
                    "raw_params": row, "issues": problems}
            items.append(item)
            issues.extend({"source_id": identity, "code": problem} for problem in problems)
            for model in models:
                resources[model["path"]].append(identity)
            for value in item_icons.values():
                for path in value["paths"]:
                    if identity not in resources[path]:
                        resources[path].append(identity)
    provenance = {key: {"path": str(Path(config[key]).resolve()), "sha256": digest(config[key])}
                  for key in ("weapon_csv", "armor_csv", "text_json", "weapon_enum", "mapping")}
    return {"schema_version": 1, "source_game": "ER", "source_version": config.get("source_version", "unverified"),
            "provenance": provenance, "items": items,
            "resources": [{"path": p, "used_by": users} for p, users in sorted(resources.items())]}, issues


def resource_gate(item):
    """Recheck disk at selection time; never trust a stale catalog's status alone."""
    reasons = []
    models = item["models"]
    if len(models) != 1 or item["model_status"] != "matched_candidates":
        reasons.append("model_missing_or_requires_variant_selection")
    elif not Path(models[0]["path"]).is_file():
        reasons.append("model_file_missing")
    if not item["icons"]:
        reasons.append("icon_missing")
    for field, icon in item["icons"].items():
        if icon["status"] != "matched_by_id" or len(icon["paths"]) != 1:
            reasons.append(field + "_missing_or_ambiguous")
        elif not Path(icon["paths"][0]).is_file():
            reasons.append(field + "_file_missing")
    return reasons


def run(config, output):
    catalog, issues = build(config)
    selected, rejected = [], []
    for item in catalog["items"]:
        reasons = resource_gate(item)
        item["resource_selection"] = {"eligible": not reasons, "reasons": reasons}
        if reasons:
            rejected.append({"source_id": item["source_id"], "reasons": reasons})
        else:
            selected.append({"source_id": item["source_id"], "name": item["name"]["value"],
                             "model": item["models"][0], "icons": item["icons"],
                             "remaining_review": item["issues"]})
    # Every run gets a new directory. Source folders and hand-edited results stay untouched.
    output = Path(output)
    output.mkdir(parents=True, exist_ok=False)
    def save(name, value):
        (output / name).write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding="utf-8")
    save("catalog.json", catalog)
    save("issues.json", issues)
    save("import_candidates.json", {"schema_version": 1, "scope": "source_resource_presence_and_id_match_only",
                                    "recheck_before_import": True, "items": selected})
    save("rejected_resources.json", rejected)
    with (output / "catalog.csv").open("w", encoding="utf-8-sig", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(["SourceID", "Name", "SourceCategory", "UmbraSuggestion", "ModelArchives", "IconIDs", "Issues", "ResourceEligible", "ExclusionReasons"])
        for item in catalog["items"]:
            # Prevent spreadsheet formula interpretation of external text.
            cells = [item["source_id"], item["name"]["value"] or "", item["source_category"],
                     json.dumps(item["umbra_suggestion"], ensure_ascii=False),
                     ";".join(m["archive"] for m in item["models"]),
                     ";".join(str(v["id"]) for v in item["icons"].values()), ";".join(item["issues"]),
                     str(item["resource_selection"]["eligible"]), ";".join(item["resource_selection"]["reasons"])]
            writer.writerow(["'" + c if c.startswith(("=", "+", "-", "@")) else c for c in cells])
    summary = {"resource_eligible": len(selected), "resource_rejected": len(rejected),
               "rows": len(catalog["items"]), "tables": dict(collections.Counter(i["table"] for i in catalog["items"])),
               "matched_names": sum(i["name"]["status"] == "matched" for i in catalog["items"]),
               "rows_with_models": sum(bool(i["models"]) for i in catalog["items"]),
               "rows_with_all_icons": sum(all(v["status"] == "matched_by_id" for v in i["icons"].values()) for i in catalog["items"]),
               "categories": dict(collections.Counter(i["source_category"] for i in catalog["items"])),
               "issues": dict(collections.Counter(i["code"] for i in issues)),
               "unique_resources": len(catalog["resources"]), "ue_assets_modified": False,
               "source_version": catalog["source_version"], "visual_icon_validation": "pending"}
    save("report.json", summary)
    print(json.dumps(summary, ensure_ascii=True, indent=2))
    print("CATALOG: " + str(output.resolve()))
    return catalog, summary


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", required=True)
    parser.add_argument("--output")
    args = parser.parse_args()
    config = read_json(args.config)
    destination = args.output or str(Path(__file__).resolve().parents[2] / "Saved/EquipmentImport/Catalog" /
                                   datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ"))
    run(config, destination)
