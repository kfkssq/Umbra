"""Apply the approved physical-affix, starter-background and primary-font changes in UE."""
import importlib
import json
import math
import sys
from pathlib import Path
import unreal as u

sys.path.insert(0, str(Path(__file__).parent))
import rarity_palette
palette = importlib.reload(rarity_palette)
out = Path(u.Paths.project_saved_dir()) / 'ItemPresentationFix'
out.mkdir(parents=True, exist_ok=True)
report_path = out / 'assets.json'
previous = json.loads(report_path.read_text(encoding='utf-8')) if report_path.exists() else {}
report = {'affixes': [], 'starters': [], 'fonts': []}

affixes = [u.load_asset('/Game/Items/GeneratedEquipment/V2/Affixes/DA_Affix_' + n + 'A')
           for n in ('Slashing', 'Blunt', 'Piercing')]
assert all(isinstance(a, u.UmbraAffixDefinition) for a in affixes)
starters = [u.load_asset('/Game/Items/Weapons/DA_Item_' + n) for n in ('TestSword', 'TestMace')]
assert all(isinstance(a, u.UmbraItemDefinition) for a in starters)
tip = u.load_asset('/Game/UI/Tooltip/WBP_ItemTooltip')
tip_class = u.load_class(None, '/Game/UI/Tooltip/WBP_ItemTooltip.WBP_ItemTooltip_C')
row_class = u.get_default_object(tip_class).get_editor_property('key_value_entry_class')
assert row_class, 'Tooltip must use an authored key/value entry WBP'
row_bp = u.load_asset(row_class.get_path_name().split('.')[0])
tip_widgets = u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(tip)
row_widgets = u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(row_bp)
header = [w for w in tip_widgets if isinstance(w, u.TextBlock)
          and (w.get_name() == 'TotalDamageText' or str(w.get_text()) == '基础伤害')]
rows = [w for w in row_widgets if isinstance(w, u.TextBlock) and w.get_name() in ('LabelText', 'ValueText')]
assert len(header) == 2 and len(rows) == 2, 'Unexpected WBP primary text contract'
baseline = {v['path']: v for v in previous.get('fonts', [])}

for asset in affixes:
    tiers = list(asset.tiers)
    before = [t.export_text() for t in tiers]
    for tier in tiers:
        values = tier.values
        bonus = values.get_editor_property('damage_bonus')
        # EditDefaultsOnly members in detached struct copies use UE's struct importer.
        bonus.import_text(bonus.export_text().replace('AttackSource=BasicAttack','AttackSource=Any'))
        values.set_editor_property('damage_bonus', bonus)
        tier.set_editor_property('values', values)
    asset.set_editor_property('tiers', tiers)
    assert u.EditorAssetLibrary.save_loaded_asset(asset)
    assert all(t.values.damage_bonus.attack_source == u.UmbraBonusAttackSource.ANY for t in asset.tiers)
    report['affixes'].append(dict(path=asset.get_path_name(), before=before,
                                  after=[t.export_text() for t in asset.tiers]))

for asset in starters:
    index = next(i for i,n in enumerate(palette.NAMES) if asset.rarity == getattr(u.UmbraItemRarity,n))
    before = {k:str(asset.get_editor_property(k)) for k in palette.backgrounds(index)}
    for key, value in palette.backgrounds(index).items(): asset.set_editor_property(key,value)
    asset.set_editor_property('tint_rarity_backgrounds', False)
    assert u.EditorAssetLibrary.save_loaded_asset(asset)
    report['starters'].append(dict(path=asset.get_path_name(), rarity=str(asset.rarity), before=before,
                                   after={k:v.get_path_name() for k,v in palette.backgrounds(index).items()}))

for bp, widgets in ((tip,header),(row_bp,rows)):
    for widget in widgets:
        path = widget.get_path_name()
        font = widget.get_editor_property('font')
        before = int(font.size)
        original = baseline.get(path,{}).get('original',before)
        target = math.ceil(original * 1.25)
        font.set_editor_property('size', target)
        widget.set_editor_property('font',font)
        report['fonts'].append(dict(path=path, original=original, before=before, after=target))
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    assert u.EditorAssetLibrary.save_loaded_asset(bp)

report_path.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('Umbra: updated three physical affixes, two starter items and four primary text styles')
