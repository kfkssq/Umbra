"""Configure production WorldItem in UE 5.8; preserve original third-party Niagara assets."""
import json
from pathlib import Path
import unreal as u

path='/Game/Items/Loot/BP_WorldItem'
bp=u.load_asset(path)
cls=u.EditorAssetLibrary.load_blueprint_class(path)
cdo=u.get_default_object(cls)
names=('Common','Uncommon','Rare','Epic','Legendary','Mythic')
rarities=('COMMON','MAGIC','RARE','EPIC','LEGENDARY','UNIQUE')
parameters=['User.C_Circle','User.C_Fade_Magic','User.C_M_Flame',
            'User.C_Magic_Floor','User.C_Rays','User.C_Smoke']
styles={}
report=[]
for i,name in enumerate(names):
    system=u.load_asset('/Game/LootDropVFX/Systems/Rarity/NS_LootDrop_'+name)
    assert isinstance(system,u.NiagaraSystem)
    row=u.UmbraLootVFXStyle()
    row.set_editor_property('system',system)
    row.set_editor_property('override_color',i>=4)
    # Normalized RGB from UE VFXAssetAudit: Mythic red / Legendary gold.
    # Runtime multiplies by each selected system layer's original HDR intensity.
    if i==4: row.set_editor_property('color',u.LinearColor(1,.045,.12,1))
    if i==5: row.set_editor_property('color',u.LinearColor(1,.37,.025,1))
    styles[getattr(u.UmbraItemRarity,rarities[i])]=row
    report.append(dict(rarity=rarities[i],system=system.get_path_name(),override=i>=4,color=str(row.color)))
cdo.set_editor_property('loot_vfx_styles',styles)
cdo.set_editor_property('loot_vfx_color_parameters',parameters)
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
saved=u.get_default_object(u.EditorAssetLibrary.load_blueprint_class(path))
assert len(saved.get_editor_property('loot_vfx_styles'))==6
out=Path(u.Paths.project_saved_dir())/'LootVFX'
out.mkdir(exist_ok=True)
(out/'configuration.json').write_text(json.dumps(dict(blueprint=path,parameters=parameters,styles=report),indent=2),encoding='utf-8')
