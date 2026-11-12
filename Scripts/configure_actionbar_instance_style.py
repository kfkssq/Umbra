"""Read each source Actionbar slot's actual PreConstruct parameters, then configure project instances."""
import unreal as u, json, os, hashlib
def tree(bp):return {w.get_name():w for w in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(bp)}
source=u.load_asset('/Game/Asset/fantasy_gui_4/widgets/sample_windows/W_Window_Actionbar')
slot=u.load_asset('/Game/UI/Skills/WBP_SkillSlot')
bar=u.load_asset('/Game/UI/Skills/WBP_SkillBar')
cdo=u.get_default_object(slot.generated_class())
for k,v in {'content_widget_path':'W_Item_Slot','frame_widget_path':'ContentBorder','icon_texture_parameter':'IconTexture'}.items():
    cdo.set_editor_property(k,v)
u.BlueprintEditorLibrary.compile_blueprint(slot)
assert u.EditorAssetLibrary.save_loaded_asset(slot,False)
cells=tree(source)['HorizontalBox_0'].get_all_children()
project=tree(bar)
report=[]
for i,cell in enumerate(cells):
    frame=cell.get_editor_property('ContentBorderTexture')
    target=project['SkillSlot_'+str(i)]
    target.set_editor_property('frame_texture',frame)
    report.append({'index':i,'frame':frame.get_path_name(),'source_content_class':str(cell.get_editor_property('Widget'))})
u.BlueprintEditorLibrary.compile_blueprint(bar)
assert u.EditorAssetLibrary.save_loaded_asset(bar,False)
with open(os.path.join(u.Paths.project_saved_dir(),'SkillHUD','template-instance-style.json'),'w',encoding='utf-8') as f:
    json.dump(report,f,ensure_ascii=False,indent=2)
u.log('ACTIONBAR_INSTANCE_STYLE_PASSED')
