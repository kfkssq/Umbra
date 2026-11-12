"""Read-only inspection of current HUD slots and source brushes, exporting texture previews."""
import unreal as u
import json, os
out=os.path.join(u.Paths.project_saved_dir(),'SkillHUD')
os.makedirs(out,exist_ok=True)
report={}
textures={}
for path in ['/Game/UI/HUD/WBP_CombatHUD','/Game/UI/HUD/WBP_PlayerHUDGroup',
             '/Game/UI/HUD/WBP_PlayerHealthBar','/Game/UI/Skills/WBP_SkillBar','/Game/UI/Skills/WBP_SkillSlot',
             '/Game/Asset/classic-rpg-ui/Assets/ClassicRPGUI/Widgets/WB_HUD_SkillBar']:
    bp=u.load_asset(path)
    if not bp: continue
    rows=[]
    for w in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(bp):
        row={'name':w.get_name(),'class':w.get_class().get_name(),'parent':str(w.get_parent()),'slot':str(w.slot)}
        for prop in ['layout_data','padding','size','horizontal_alignment','vertical_alignment','z_order']:
            try: row['slot_'+prop]=str(w.slot.get_editor_property(prop))
            except Exception: pass
        for prop in ['width_override','height_override','brush','background','widget_style']:
            try: row[prop]=str(w.get_editor_property(prop))
            except Exception: pass
        if isinstance(w,u.Image):
            tex=w.get_editor_property('brush').get_editor_property('resource_object')
            if isinstance(tex,u.Texture2D): textures[tex.get_path_name()]=tex
        rows.append(row)
    report[path]=rows
for name in ['skill_bar','skill_bar_background','skill_frame','Thin_frame']:
    tex=u.load_asset('/Game/Asset/classic-rpg-ui/Assets/ClassicRPGUI/ClassicUI/GUI_elements/'+name)
    if isinstance(tex,u.Texture2D): textures[tex.get_path_name()]=tex
for path,tex in textures.items():
    task=u.AssetExportTask()
    task.object=tex
    task.filename=os.path.join(out,tex.get_name()+'.png')
    task.automated=True
    task.prompt=False
    task.exporter=u.TextureExporterPNG()
    u.Exporter.run_asset_export_task(task)
report['textures']=list(textures)
with open(os.path.join(out,'audit.json'),'w',encoding='utf-8') as f: json.dump(report,f,ensure_ascii=False,indent=2)
u.log('SKILL_HUD_AUDIT_PASSED')
