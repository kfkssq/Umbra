"""Rendered PIE: target-card GAS updates and whole-window page lifecycle. No map saves."""
import unreal as u,time,json
from pathlib import Path
out=Path(u.Paths.project_saved_dir()).resolve()/'UIExpansion'
level=u.get_editor_subsystem(u.LevelEditorSubsystem);editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
lib=u.UmbraSkillEditorLibrary
source_widgets={w.get_name():w for w in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(u.load_asset('/Game/UI/HUD/WBP_EnemyTargetPanel'))}
assert isinstance(source_widgets.get('HealthText'),u.TextBlock),'HealthText must exist in saved Designer tree'
assert 'MainSizeBox' in source_widgets and 'HealthValueOverlay' in source_widgets
u.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic();stamp=start;state={'stage':'start'}
world=pc=target=panel=menu=equipment=None

def health_fraction():
 retainer=lib.find_presentation_widget(panel,'W_ProgressBar_Parent.ProgressBar_RetainerBox')
 assert retainer,'Missing original template retainer'
 material=retainer.get_effect_material()
 assert material,'Original progress material was not initialized'
 return material.get_scalar_parameter_value('Value')

def shot(name):u.SystemLibrary.execute_console_command(world,'Shot SHOWUI filename="'+(out/(name+'.png')).as_posix()+'" -nosuffix')
def finish(error=''):
 state.update(result='failed' if error else 'passed',error=error)
 (out/'pie.json').write_text(json.dumps(state,ensure_ascii=False,indent=2),encoding='utf-8')
 u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
 u.unregister_slate_post_tick_callback(callback);u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
def tick(delta):
 global world,pc,target,panel,menu,equipment,stamp
 try:
  assert time.monotonic()-start<120,'Timeout '+state['stage']
  if state['stage']=='start':level.editor_request_begin_play();state['stage']='prepare';return
  if state['stage']=='prepare' and level.is_in_play_in_editor() and time.monotonic()-start>10:
   world=editor.get_game_world();pc=u.GameplayStatics.get_player_controller(world,0);player=u.GameplayStatics.get_player_character(world,0)
   target=lib.prepare_skill_test(world)
   panel=next(w for w in u.WidgetLibrary.get_all_widgets_of_class(world,u.UserWidget,False) if isinstance(w,u.UmbraEnemyTargetPanel))
   for w in u.WidgetLibrary.get_all_widgets_of_class(world,u.UserWidget,False):
    if isinstance(w,u.UmbraAttributeDebugPanel):w.set_visibility(u.SlateVisibility.COLLAPSED)
   pos=u.GameplayStatics.project_world_to_screen(pc,target.get_actor_location()+u.Vector(0,0,30))
   state['projection']=str(pos)
   pc.set_mouse_location(int(pos.x),int(pos.y));stamp=time.monotonic();state['stage']='target';return
  if state['stage'] in ['target','health']:
   pos=u.GameplayStatics.project_world_to_screen(pc,target.get_actor_location()+u.Vector(0,0,30))
   pc.set_mouse_location(int(pos.x),int(pos.y))
  if state['stage']=='target' and time.monotonic()-stamp>1:
   assert lib.find_presentation_widget(panel,'MainSizeBox'),'Missing copied template root'
   assert lib.find_presentation_widget(panel,'HealthValueOverlay'),'Missing Designer health overlay'
   assert pc.get_highlighted_enemy_for_ui()==target,'Actual hover did not select fixture'
   assert panel.get_visibility()==u.SlateVisibility.HIT_TEST_INVISIBLE,'Target panel hidden'
   assert str(lib.find_presentation_widget(panel,'TextBlock_0').get_text())=='近战守卫'
   name_font=lib.find_presentation_widget(panel,'TextBlock_0').get_editor_property('font').font_object.get_path_name()
   level_font=lib.find_presentation_widget(panel,'TextBlock_5').get_editor_property('font').font_object.get_path_name()
   health_font=lib.find_presentation_widget(panel,'HealthText').get_editor_property('font').font_object.get_path_name()
   assert name_font=='/Game/Asset/Font/SourceHanSerifSC-Regular_Font.SourceHanSerifSC-Regular_Font',name_font
   assert level_font==health_font=='/Game/Asset/Font/SimplifiedChinese/SourceHanSansSC-Regular_Font.SourceHanSansSC-Regular_Font',(level_font,health_font)
   material=lib.find_presentation_widget(panel,'W_ProgressBar_Parent.ProgressBar_RetainerBox').get_effect_material()
   parent=material.get_editor_property('parent').get_path_name()
   assert parent=='/Game/Asset/fantasy_gui_4/materials/progressbar/Mat_GUI_ProgressBar_Red.Mat_GUI_ProgressBar_Red',parent
   state['style']={'name_font':name_font,'numeric_font':health_font,'health_material':parent}

   assert abs(health_fraction()-1)<.001
   lib.set_skill_test_attribute(target,'Health',5000);stamp=time.monotonic();state['stage']='health';return
  if state['stage']=='health' and time.monotonic()-stamp>.5:
   assert abs(health_fraction()-.5)<.001,'GAS health did not update'
   state['health_text']=str(lib.find_presentation_widget(panel,'HealthText').get_text());shot('enemy-target')
   stamp=time.monotonic();state['stage']='target-shot';return
  if state['stage']=='target-shot':
   pos=u.GameplayStatics.project_world_to_screen(pc,target.get_actor_location()+u.Vector(0,0,30));pc.set_mouse_location(int(pos.x),int(pos.y))
   if time.monotonic()-stamp>.5:
    pc.set_mouse_location(5,5);stamp=time.monotonic();state['stage']='clear'
   return
  if state['stage']=='clear':
   pc.set_mouse_location(5,5)
   if time.monotonic()-stamp<=1 or pc.get_highlighted_enemy_for_ui() is not None:return
   assert panel.get_visibility()==u.SlateVisibility.COLLAPSED,'No-target panel remained visible'
   pc.toggle_character_menu();stamp=time.monotonic();state['stage']='character';return
  if state['stage']=='character' and time.monotonic()-stamp>2:
   menu=next(w for w in u.ObjectIterator(u.UmbraCharacterMenu) if w.get_owning_player()==pc)
   equipment=next(w for w in u.ObjectIterator(u.UmbraEquipmentMenu) if w.get_owning_player()==pc)
   assert equipment.is_page_active(),'Character page inactive'
   shot('character-page');stamp=time.monotonic();state['stage']='character-shot';return
  if state['stage']=='character-shot' and time.monotonic()-stamp>.5:
   menu.show_skills_page();stamp=time.monotonic();state['stage']='skills';return
  if state['stage']=='skills' and time.monotonic()-stamp>1:
   switch=lib.find_presentation_widget(menu,'MainPageSwitcher');assert switch.get_active_widget_index()==1, 'Skills switch index='+str(switch.get_active_widget_index())
   assert not equipment.is_page_active(),'Hidden equipment preview still active'
   assert switch.get_active_widget().get_children_count()==0,'Skills page not empty'
   shot('skills-page');stamp=time.monotonic();state['stage']='skills-shot';return
  if state['stage']=='skills-shot' and time.monotonic()-stamp>.5:
   menu.show_character_page();stamp=time.monotonic();state['stage']='return';return
  if state['stage']=='return' and time.monotonic()-stamp>1:
   assert equipment.is_page_active(),'Returning failed to reactivate equipment'
   pc.toggle_character_menu();assert not equipment.is_page_active(),'Closing failed to deactivate equipment'
   level.editor_request_end_play();state['stage']='end';return
  if state['stage']=='end' and not level.is_in_play_in_editor():finish()
 except Exception as e:
  u.log_error('UI_EXPANSION_PIE_FAILED '+repr(e));finish(repr(e))
callback=u.register_slate_post_tick_callback(tick)
