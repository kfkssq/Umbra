"""Author a presentation-only XP bar through UE; preserve the current HUD sizing."""
import unreal as u, os, json, shutil, datetime
out=os.path.join(u.Paths.project_saved_dir(),'SkillHUD')
backup=os.path.join(out,'BeforeExperience',datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
os.makedirs(backup,exist_ok=True)
shutil.copy2(os.path.join(u.Paths.project_content_dir(),'UI/HUD/WBP_PlayerHUDGroup.uasset'),backup)
assert u.UmbraSkillEditorLibrary.create_experience_bar_placeholder()
material_path='/Game/UI/HUD/Materials/MI_ExperienceBar_Preview'
material=u.load_asset(material_path)
if not material:
    material=u.AssetToolsHelpers.get_asset_tools().create_asset('MI_ExperienceBar_Preview','/Game/UI/HUD/Materials',u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew())
    u.MaterialEditingLibrary.set_material_instance_parent(material,u.load_asset('/Game/Asset/fantasy_gui_4/materials/progressbar/Mat_GUI_ProgressBar_Violet'))
    u.MaterialEditingLibrary.update_material_instance(material)
    # UE 5.8.2's setter returns false even after applying; verify the actual readback.
    u.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(material,'Value',0.5)
    assert abs(u.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(material,'Value')-.5)<.001
    u.MaterialEditingLibrary.update_material_instance(material)
    assert u.EditorAssetLibrary.save_loaded_asset(material,False)
bp=u.load_asset('/Game/UI/HUD/WBP_PlayerExperienceBar')
widgets={w.get_name():w for w in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(bp)}
progress=widgets['W_ProgressBar_Red']
progress.set_editor_property('ProgressBarMaterial',material)
# Retain the source's horizontal insets; reduce vertical insets for the thin bottom strip.
padding=progress.slot.get_editor_property('padding')
progress.slot.set_padding(u.Margin(padding.left,1,padding.right,1))
inner=progress.get_editor_property('ProgressBar_Padding')
progress.set_editor_property('ProgressBar_Padding',u.Margin(inner.left,1,inner.right,1))
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
report={'asset':bp.get_path_name(),
 'material':material.get_path_name(),'preview_value':0.5,'height':14,'row_padding':[0,-4,0,0],
 'tree':u.UmbraSkillEditorLibrary.describe_widget(bp),'behavior':u.UmbraSkillEditorLibrary.describe_template_behavior(bp)}
with open(os.path.join(out,'experience-created.json'),'w',encoding='utf-8') as f:json.dump(report,f,ensure_ascii=False,indent=2)
u.log('EXPERIENCE_PLACEHOLDER_SAVED')
