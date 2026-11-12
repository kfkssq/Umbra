"""Import user-local ER icons through UE and update only skill/Designer icon references."""
import unreal as u, json, os, hashlib, shutil, datetime, re
root='C:/baidunetdiskdownload/艾尔登法环图标/艾尔登法环图标/其它'
destination='/Game/UI/Icons/EldenRing'
selection=[('Sweep','08314','Compact sword pose and short lateral blade trail'),
           ('WideSweep','08309','Broad physical weapon sweep trail')]
out=os.path.join(u.Paths.project_saved_dir(),'SkillHUD')
backup=os.path.join(out,'BeforeERIcons',datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
os.makedirs(backup,exist_ok=True)
for relative in ['Skills/Prototype/DA_Sweep','Skills/Prototype/DA_WideSweep','UI/Skills/WBP_SkillSlot','UI/Skills/WBP_SkillBar']:
    shutil.copy2(os.path.join(u.Paths.project_content_dir(),relative+'.uasset'),backup)
def tree(bp):return {w.get_name():w for w in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(bp)}
def layout(bp):
    result={}
    for name,w in tree(bp).items():
        row={'parent':w.get_parent().get_name() if w.get_parent() else None}
        for obj,props,prefix in [(w,['width_override','height_override','render_transform','visibility'],''),
                                 (w.slot,['padding','size','layout_data','horizontal_alignment','vertical_alignment'],'slot_')]:
            if not obj:continue
            for p in props:
                try:row[prefix+p]=str(obj.get_editor_property(p))
                except:pass
        result[name]=row
    return result
bar=u.load_asset('/Game/UI/Skills/WBP_SkillBar'); slot=u.load_asset('/Game/UI/Skills/WBP_SkillSlot')
assert bar and slot
original_layouts=[layout(bp) for bp in [bar,slot]]
report={'source':'User-supplied local Elden Ring icon dump','source_version':'unverified',
 'license':'No redistribution/commercial-use grant verified; retain existing project source restrictions',
 'original_skill_names':'Not asserted; selection based on inspected image content', 'icons':[]}
icons=[]
for skill,number,reason in selection:
    filename=os.path.join(root,'menu_knowledge_'+number+'out.obj.png')
    with open(filename,'rb') as f:source_hash=hashlib.sha256(f.read()).hexdigest()
    name='T_Skill_'+skill+'_ER'+number
    path=destination+'/'+name
    if not u.EditorAssetLibrary.does_asset_exist(path):
        task=u.AssetImportTask()
        task.set_editor_properties(dict(filename=filename,destination_path=destination,destination_name=name,
                                       automated=True,replace_existing=False,save=False))
        u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        assert task.get_editor_property('imported_object_paths'),filename
    texture=u.load_asset(path)
    assert isinstance(texture,u.Texture2D)
    texture.set_editor_properties(dict(compression_settings=u.TextureCompressionSettings.TC_EDITOR_ICON,
        lod_group=u.TextureGroup.TEXTUREGROUP_UI,mip_gen_settings=u.TextureMipGenSettings.TMGS_NO_MIPMAPS,
        srgb=True,never_stream=True))
    assert u.EditorAssetLibrary.save_loaded_asset(texture,False)
    definition=u.load_asset('/Game/Skills/Prototype/DA_'+skill)
    assert definition
    stable=lambda value:re.sub(r'0x[0-9A-Fa-f]+','PTR',str(value))
    tuning={p:stable(definition.get_editor_property(p)) for p in ['radius','arc_degrees','damage','resource_cost','cooldown_seconds','montage','strike_effect','strike_time','play_rate']}
    old=definition.get_editor_property('icon')
    definition.set_editor_property('icon',texture)
    assert u.EditorAssetLibrary.save_loaded_asset(definition,False)
    assert tuning=={p:stable(definition.get_editor_property(p)) for p in tuning}
    icons.append(texture)
    report['icons'].append(dict(skill=skill,source=filename,sha256=source_hash,asset=texture.get_path_name(),
        previous_icon=old.get_path_name() if old else None,reason=reason,tuning=tuning))
# Keep previews consistent with runtime data without reconstructing any Designer hierarchy.
u.get_default_object(slot.generated_class()).set_editor_property('preview_icon',icons[0])
u.BlueprintEditorLibrary.compile_blueprint(slot)
assert u.EditorAssetLibrary.save_loaded_asset(slot,False)
for w in tree(bar).values():
    if isinstance(w,u.UmbraSkillSlotWidget):
        index=w.get_editor_property('slot_index')
        if index in [0,1]:w.set_editor_property('preview_icon',icons[index])
u.BlueprintEditorLibrary.compile_blueprint(bar)
assert u.EditorAssetLibrary.save_loaded_asset(bar,False)
# Compiling reinstantiates templates, so compare structural values without memory addresses.
normalize=lambda data: re.sub(r'0x[0-9A-Fa-f]+','PTR',json.dumps(data,sort_keys=True))
assert normalize(original_layouts)==normalize([layout(bp) for bp in [bar,slot]]),'Designer layout changed'
report['result']='passed'
with open(os.path.join(out,'er-icon-import.json'),'w',encoding='utf-8') as f:json.dump(report,f,ensure_ascii=False,indent=2)
u.log('ER_SKILL_ICON_IMPORT_PASSED')
