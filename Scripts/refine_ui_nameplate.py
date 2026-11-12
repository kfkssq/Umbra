"""Copy W_Nameplate_2 structure and add a Designer-editable health text; preserve later manual edits."""
import unreal as u
assert u.UmbraSkillEditorLibrary.adopt_enemy_nameplate_template()
u.log('EDITABLE_NAMEPLATE2_CONFIGURED')
u.SystemLibrary.quit_editor()
