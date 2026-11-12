"""Explicit editor migration; copy source WBP trees and verify unchanged template children."""
import unreal as u, json, os, shutil, hashlib, re, datetime
out=os.path.join(u.Paths.project_saved_dir(),'SkillHUD')
project=u.Paths.project_dir()
source_paths=['Asset/fantasy_gui_4/widgets/sample_windows/W_Window_Actionbar',
              'Asset/fantasy_gui_4/widgets/templates/item/W_Actionbar_Item_Slot',
              'Asset/fantasy_gui_4/widgets/templates/item/W_Item_Slot']
def digest(relative):
    with open(os.path.join(project,'Content',relative+'.uasset'),'rb') as f:return hashlib.sha256(f.read()).hexdigest()
before={p:digest(p) for p in source_paths}
backup=os.path.join(out,'BeforeTemplate',datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
os.makedirs(backup,exist_ok=True)
for p in ['UI/Skills/WBP_SkillSlot','UI/Skills/WBP_SkillBar','UI/HUD/WBP_PlayerHUDGroup']:
    shutil.copy2(os.path.join(project,'Content',p+'.uasset'),backup)
assert u.UmbraSkillEditorLibrary.adopt_actionbar_templates(), 'Migration refused or failed; inspect editor log'
def tree(path):return {w.get_name():w for w in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(u.load_asset(path))}
def value(obj,p):return re.sub(r'0x[0-9A-Fa-f]+','PTR',str(obj.get_editor_property(p)))
source=tree('/Game/'+source_paths[1]); dest=tree('/Game/UI/Skills/WBP_SkillSlot')
checked=[]
for name,w in source.items():
    target=dest[name]
    assert w.get_class()==target.get_class(),name
    assert (w.get_parent().get_name() if w.get_parent() else None)==(target.get_parent().get_name() if target.get_parent() else None),name
    for p in ['padding','background','brush','font','text','color_and_opacity','width_override','height_override',
              'override_width_override','override_height_override','render_transform','visibility','item_icon','quantity']:
        try: original=value(w,p)
        except:continue
        assert original==value(target,p),(name,p,original,value(target,p))
        checked.append(name+'.'+p)
    if w.slot:
        for p in ['padding','layout_data','size','auto_size','z_order','horizontal_alignment','vertical_alignment']:
            try:original=value(w.slot,p)
            except:continue
            assert original==value(target.slot,p),(name,p)
            checked.append(name+'.slot.'+p)
bar=tree('/Game/UI/Skills/WBP_SkillBar')
assert len(bar['HorizontalBox_0'].get_all_children())==10
for i in range(10):
    w=bar['SkillSlot_'+str(i)]
    assert w.get_editor_property('slot_index')==i
    padding=w.slot.get_editor_property('padding')
    assert all(getattr(padding,p)==0 for p in ['left','top','right','bottom'])
defaults=u.get_default_object(u.load_asset('/Game/UI/Skills/WBP_SkillSlot').generated_class())
assert defaults.get_editor_property('icon_widget_path')=='W_Item_Slot.item_icon'
assert defaults.get_editor_property('key_widget_path')=='keyboard_shortcut_label'
assert defaults.get_editor_property('cost_widget_path')=='W_Item_Slot.item_quantity'
assert before=={p:digest(p) for p in source_paths},'Source package changed'
report={'result':'passed','backup':backup,'checked_identical_properties':checked,'source_hashes':before,
        'bar_tree':u.UmbraSkillEditorLibrary.describe_widget(u.load_asset('/Game/UI/Skills/WBP_SkillBar')),
        'slot_tree':u.UmbraSkillEditorLibrary.describe_widget(u.load_asset('/Game/UI/Skills/WBP_SkillSlot')),
        'size':[bar['Size_TemplateViewport'].get_editor_property('width_override'),bar['Size_TemplateViewport'].get_editor_property('height_override')]}
with open(os.path.join(out,'template-migration.json'),'w',encoding='utf-8') as f:json.dump(report,f,ensure_ascii=False,indent=2)
u.log('ACTIONBAR_TEMPLATE_MIGRATION_PASSED')
