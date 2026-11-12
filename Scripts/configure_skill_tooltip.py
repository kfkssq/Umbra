"""Create a text-only skill tooltip and fill missing descriptions without replacing existing tuning."""
import unreal as u,json
from pathlib import Path
assert u.UmbraSkillEditorLibrary.create_skill_tooltip_presentation()
tooltip_class=u.load_asset('/Game/UI/Skills/WBP_SkillTooltip').generated_class()
slot_bp=u.load_asset('/Game/UI/Skills/WBP_SkillSlot')
u.get_default_object(slot_bp.generated_class()).set_editor_property('skill_tooltip_class',tooltip_class)
u.BlueprintEditorLibrary.compile_blueprint(slot_bp)
assert u.EditorAssetLibrary.save_loaded_asset(slot_bp,False)
bar_bp=u.load_asset('/Game/UI/Skills/WBP_SkillBar')
for widget in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(bar_bp):
 if isinstance(widget,u.UmbraSkillSlotWidget):widget.set_editor_property('skill_tooltip_class',tooltip_class)
u.BlueprintEditorLibrary.compile_blueprint(bar_bp)
assert u.EditorAssetLibrary.save_loaded_asset(bar_bp,False)
ps=u.get_default_object(u.load_asset('/Game/Blueprints/Player/BP_UmbraPlayerState').generated_class())
rows=[]
for slot in ps.get_editor_property('initial_skill_slots'):
 d=slot.get_editor_property('definition')
 if not d:continue
 execution=d.get_editor_property('execution')
 descriptions={u.UmbraSkillExecution.SECTOR:'挥剑斩击前方区域，对范围内的敌人造成伤害。',u.UmbraSkillExecution.PROJECTILE:'挥剑向前释放一道斩切波，命中敌人时造成伤害。',u.UmbraSkillExecution.SWORD_STORM:('按住技能键持续旋转挥剑，对周围敌人周期性造成伤害。期间可以移动，松开按键结束。' if d.get_editor_property('channel_while_held') else '旋转挥剑，对周围敌人连续造成多次伤害。'),u.UmbraSkillExecution.LEAP_SLAM:('选择落点后跃向目标位置，落地时震击周围敌人。' if d.get_editor_property('targeted_leap') else '向前跃起，落地时震击周围敌人。'),u.UmbraSkillExecution.GUARD:'举盾进入防御姿态，在持续期间提高护甲与物理抗性。',u.UmbraSkillExecution.STONE_RECOVERY:'腾空后重击地面，对周围敌人造成伤害，并恢复自身生命。',u.UmbraSkillExecution.BATTLE_FOCUS:'凝聚战意，短时间内提高暴击率。重复施放刷新持续时间，效果不叠加。'}
 if not str(d.get_editor_property('description')).strip():
  d.set_editor_property('description',descriptions[execution]);assert u.EditorAssetLibrary.save_loaded_asset(d,False)
 rows.append({'asset':d.get_path_name(),'name':str(d.display_name),'description':str(d.description)})
Path(u.Paths.project_saved_dir(),'UIExpansion','skill-tooltip-configured.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('SKILL_TOOLTIP_CONFIGURED')
u.SystemLibrary.quit_editor()
