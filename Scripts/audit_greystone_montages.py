import unreal as u
for name in ['Prototype/AM_Sweep','Greystone/AM_SwordStorm','Greystone/AM_LeapSlam','Greystone/AM_Guard','Greystone/AM_StoneRecovery']:
    obj=u.load_asset('/Game/Skills/'+name)
    for p in ['slot_anim_tracks','blend_in','blend_out','sequence_length']:
        try:u.log('MONTAGE_AUDIT '+name+' '+p+' '+str(obj.get_editor_property(p)))
        except Exception as e:u.log('MONTAGE_AUDIT '+str(e))
for name in ['Deflect/FX/P_Greystone_Deflect_Looping','Ultimate/FX/P_Greystone_HToKill_WarmUp','Ultimate/FX/P_Greystone_HToKill_Resurrect',
             'Ultimate/FX/P_Greystone_Ultimate_Stone','Ultimate/FX/P_HToKill_TurnToStone','Ultimate/FX/P_Greystone_HToKill_Start']:
    ps=u.load_asset('/Game/ParagonGreystone/FX/Particles/Greystone/Abilities/'+name)
    try:
        for emitter in ps.get_editor_property('emitters'):
            for lod in emitter.get_editor_property('lod_levels'):
                for module in lod.get_editor_property('modules'):
                    u.log('FX_AUDIT '+name+' '+module.get_class().get_name())
                    for prop in ['skel_mesh_actor_param_name','source_locations','source_type','emitter_name']:
                        try:u.log('FX_AUDIT '+prop+' '+str(module.get_editor_property(prop)))
                        except:pass
    except Exception as e:u.log('FX_AUDIT '+str(e))
    u.log('FX_NATIVE '+name+'\n'+u.UmbraSkillEditorLibrary.describe_particle_system(ps))
