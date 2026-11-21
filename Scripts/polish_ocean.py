import unreal
from pathlib import Path
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/HumanFishing/Maps/L_DeepOcean')
for a in list(actors.get_all_level_actors()):
    label=a.get_actor_label()
    for c in a.get_components_by_class(unreal.PointLightComponent):
        c.set_editor_property('intensity_units',unreal.LightUnits.LUMENS)
        c.set_intensity(1200 if 'MarketWarm' in label else 1800 if 'CoveBlue' in label else 300)
        c.set_editor_property('use_inverse_squared_falloff',True)
    if label=='OceanArt_ColorAndExposure':actors.destroy_actor(a)
volume=actors.spawn_actor_from_class(unreal.PostProcessVolume,unreal.Vector(0,0,-900))
volume.set_actor_label('OceanArt_ColorAndExposure');volume.set_editor_property('unbound',True)
settings=volume.get_editor_property('settings')
for prop,value in [('auto_exposure_min_brightness',2.),('auto_exposure_max_brightness',2.),
                   ('auto_exposure_bias',0.),('bloom_intensity',.15),('motion_blur_amount',0.),('lens_flare_intensity',0.)]:
    settings.set_editor_property('override_'+prop,True);settings.set_editor_property(prop,value)
volume.set_editor_property('settings',settings)
# The small rowboat is the return-shore landmark, partially settled into sand.
path='/Game/HumanFishing/Ocean/Kenney/SM_boat_row_small'
mesh=unreal.load_asset(path)
if mesh and not any(a.get_actor_label()=='OceanArt_ShoreRowboat' for a in actors.get_all_level_actors()):
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(2060,700,-1790),unreal.Rotator(yaw=-55))
    a.set_actor_label('OceanArt_ShoreRowboat');a.static_mesh_component.set_static_mesh(mesh)
    a.set_actor_scale3d(unreal.Vector(1,1,1)*(660/(mesh.get_bounds().box_extent.x*2)))
    a.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
levels.save_current_level()
unreal.log('HF_OCEAN_LIGHTING_POLISHED')
