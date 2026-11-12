"""Replace only the resource object in the eight existing stat brushes; retain all styling."""
import unreal as u,json,time
from pathlib import Path
out=Path(u.Paths.project_saved_dir())/'UIExpansion'
mapping={'AttackPower':'broadsword','AbilityPower':'magic-swirl','Armor':'breastplate','MagicResistance':'magic-shield','AttackSpeed':'quick-slash','AbilityCooldown':'stopwatch','CriticalHitChance':'target-arrows','MoveSpeed':'wingfoot'}
items=[];report=[]
# Validate all eight destinations before changing any asset.
for stat,icon in mapping.items():
 bp=u.load_asset('/Game/UI/HUD/StatsPanel/WBP_'+stat)
 tex=u.load_asset('/Game/Asset/Icons/8statsicons/'+icon)
 assert bp and isinstance(tex,u.Texture2D),(stat,icon)
 images=[w for w in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(bp) if isinstance(w,u.Image)]
 assert len(images)==1,(stat,[(w.get_name(),str(w.get_editor_property('brush'))) for w in images])
 w=images[0];brush=w.get_editor_property('brush')
 items.append((bp,tex,w,str(brush),str(brush.get_editor_property('tint_color')),str(w.get_editor_property('color_and_opacity'))))
for bp,tex,w,original,tint,color in items:
 brush=w.get_editor_property('brush');old=brush.get_editor_property('resource_object')
 brush.set_editor_property('resource_object',tex)
 w.set_editor_property('brush',brush)
 assert str(w.get_editor_property('brush').get_editor_property('tint_color'))==tint
 assert str(w.get_editor_property('color_and_opacity'))==color
 report.append({'asset':bp.get_path_name(),'widget':w.get_name(),'texture':tex.get_path_name(),'before_brush':original,'after_brush':str(w.get_editor_property('brush')),'tint':tint,'color':color})
 u.BlueprintEditorLibrary.compile_blueprint(bp)
 assert u.EditorAssetLibrary.save_loaded_asset(bp,False),bp.get_name()
(out/'stats-configured.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('STATS_ICONS_CONFIGURED_8_COLORS_PRESERVED')
u.SystemLibrary.quit_editor()
