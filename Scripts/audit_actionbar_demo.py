import unreal as u,json,os
root='/Game/Asset/fantasy_gui_4/widgets/'
paths=[root+'sample_windows/W_Window_ActionbarDemo',root+'sample_windows/W_Window_Actionbar',root+'templates/item/W_Actionbar_Item_Slot',root+'templates/item/W_Skill_Slot',root+'templates/slots/W_Slot_Labeled_1',root+'templates/slots/W_Slot_1']
report={}
for path in paths:
    bp=u.load_asset(path)
    if not bp:continue
    rows=[]
    for w in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(bp):
        row={'name':w.get_name(),'class':w.get_class().get_name(),'parent':str(w.get_parent())}
        if w.slot:
            for p in ['layout_data','padding','size','horizontal_alignment','vertical_alignment']:
                try:row[p]=str(w.slot.get_editor_property(p))
                except:pass
        for p in ['brush','background','widget_style','width_override','height_override','text']:
            try:row[p]=str(w.get_editor_property(p))
            except:pass
        rows.append(row)
    report[path]=rows
out=os.path.join(u.Paths.project_saved_dir(),'SkillHUD','actionbar-demo.json')
with open(out,'w',encoding='utf-8') as f:json.dump(report,f,ensure_ascii=False,indent=2)
u.log('ACTIONBAR_AUDIT_COMPLETE')
