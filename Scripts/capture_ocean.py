import unreal, time
from pathlib import Path
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/HumanFishing/Maps/L_DeepOcean')
output=Path(unreal.Paths.project_saved_dir()).resolve()/'Validation/Images';output.mkdir(parents=True,exist_ok=True)
views=[('OceanOverview',unreal.Vector(2300,-2300,-300),unreal.Vector(0,0,-1250)),
       ('SeaweedExchange',unreal.Vector(1450,-800,-620),unreal.Vector(350,0,-950)),
       ('CoveWreck',unreal.Vector(400,-2350,-700),unreal.Vector(-1250,-1120,-1400))]
views=[(n,p,unreal.MathLibrary.find_look_at_rotation(p,t)) for n,p,t in views]
camera=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(unreal.CameraActor,views[0][1],views[0][2])
camera.camera_component.set_field_of_view(80)
state={'index':0,'start':time.time(),'task':None,'next':time.time()+30,'busy':False}
unreal.EditorLevelLibrary.set_level_viewport_camera_info(views[0][1],views[0][2])
def tick(dt):
    try:
        if state['busy']:return
        if time.time()>state['start']+240:raise RuntimeError('Screenshot timeout')
        if time.time()<state['next']:return
        if state['task']:
            if not state['task'].is_task_done():return
            state['index']+=1;state['task']=None
            if state['index']==len(views):
                unreal.unregister_slate_post_tick_callback(handle);unreal.log('HF_SCREENSHOTS_COMPLETE');unreal.SystemLibrary.quit_editor();return
            name,loc,rot=views[state['index']];camera.set_actor_location_and_rotation(loc,rot,False,False);unreal.EditorLevelLibrary.set_level_viewport_camera_info(loc,rot);state['next']=time.time()+5;return
        name,loc,rot=views[state['index']]
        state['busy']=True
        unreal.log('HF_CAMERA '+name+' '+str(loc)+' '+str(rot))
        state['task']=unreal.AutomationLibrary.take_high_res_screenshot(1280,720,str(output/(name+'.png')),camera=camera)
        state['busy']=False
        state['next']=time.time()+3
    except Exception:
        import traceback;unreal.log_error(traceback.format_exc());unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(tick)
