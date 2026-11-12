"""Read current user-edited HUD constraints and reusable progress-bar parameters."""
import unreal as u, json, os
paths=['/Game/UI/HUD/WBP_PlayerHUDGroup','/Game/UI/HUD/WBP_PlayerResourceBar','/Game/UI/HUD/WBP_PlayerHealthBar',
 '/Game/Asset/fantasy_gui_4/widgets/templates/progressbar/blue/W_ProgressBar_Blue',
 '/Game/Asset/fantasy_gui_4/widgets/templates/progressbar/W_ProgressBar_Parent']
report={}
for path in paths:
    bp=u.load_asset(path); rows=[]
    for w in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(bp):
        row={'name':w.get_name(),'class':w.get_class().get_path_name(),'parent':w.get_parent().get_name() if w.get_parent() else None}
        for p in ['width_override','height_override','override_width_override','override_height_override','brush','text','padding','render_transform']:
            try:row[p]=str(w.get_editor_property(p))
            except:pass
        if w.slot:
            for p in ['padding','size','horizontal_alignment','vertical_alignment','layout_data']:
                try:row['slot_'+p]=str(w.slot.get_editor_property(p))
                except:pass
        rows.append(row)
    report[path]={'rows':rows,'behavior':u.UmbraSkillEditorLibrary.describe_template_behavior(bp)}
out=os.path.join(u.Paths.project_saved_dir(),'SkillHUD','experience-audit.json')
with open(out,'w',encoding='utf-8') as f:json.dump(report,f,ensure_ascii=False,indent=2)
u.log('EXPERIENCE_AUDIT_COMPLETE')
