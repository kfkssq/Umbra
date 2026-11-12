"""Read the saved source templates, including exposed instance configuration."""
import unreal as u, json, os
base='/Game/Asset/fantasy_gui_4/widgets/'
paths=[base+'sample_windows/W_Window_Actionbar',base+'templates/item/W_Actionbar_Item_Slot',base+'templates/item/W_Item_Slot']
report={}
for path in paths:
    bp=u.load_asset(path)
    widgets=u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(bp)
    rows=[]
    for w in widgets:
        row={'name':w.get_name(),'class':w.get_class().get_path_name(),'parent':w.get_parent().get_name() if w.get_parent() else None}
        props=['padding','brush','background','font','text','color_and_opacity','width_override','height_override','override_width_override','override_height_override','visibility','render_transform','clipping']
        if isinstance(w,u.UserWidget):
            row['members']=dir(w)
            props+=['icon','item_icon','keyboard_shortcut','keyboard_shortcut_label','texture','item','amount','quantity','count','quality']
        row['properties']={}
        for p in props:
            try:row['properties'][p]=str(w.get_editor_property(p))
            except:pass
        if w.slot:
            row['slot']={}
            for p in ['layout_data','padding','size','horizontal_alignment','vertical_alignment','auto_size','z_order']:
                try:row['slot'][p]=str(w.slot.get_editor_property(p))
                except:pass
        rows.append(row)
    cdo=u.get_default_object(bp.generated_class())
    report[path]={'rows':rows,'cdo_members':dir(cdo),'behavior':u.UmbraSkillEditorLibrary.describe_template_behavior(bp)}
out=os.path.join(u.Paths.project_saved_dir(),'SkillHUD','templates.json')
with open(out,'w',encoding='utf-8') as f:json.dump(report,f,ensure_ascii=False,indent=2)
u.log('TEMPLATE_AUDIT_COMPLETE')
