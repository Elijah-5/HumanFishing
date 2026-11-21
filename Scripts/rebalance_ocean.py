"""Readable ocean lighting, broader lower Cove entrance, no floating markers."""
import unreal
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/HumanFishing/Maps/L_DeepOcean')
for a in list(actors.get_all_level_actors()):
    label=a.get_actor_label()
    if label.startswith('OceanFill_'):actors.destroy_actor(a);continue
    if isinstance(a,unreal.HumanFishingTravelZone):
        if 'L_ScenicCove' in str(a.get_editor_property('destination_map')):
            a.set_actor_location(unreal.Vector(-2630,0,-900),False,False)
            a.travel_volume.set_box_extent(unreal.Vector(250,360,850),False)
        elif 'L_ShorePrototype' in str(a.get_editor_property('destination_map')):
            a.set_actor_location(unreal.Vector(2650,0,-900),False,False)
            a.travel_volume.set_box_extent(unreal.Vector(220,340,850),False)
        for component in a.get_components_by_class(unreal.StaticMeshComponent):
            component.set_static_mesh(None);component.set_visibility(False);component.set_hidden_in_game(True)
    for light in a.get_components_by_class(unreal.PointLightComponent):
        light.set_editor_property('intensity_units',unreal.LightUnits.LUMENS)
        light.set_intensity(420 if 'MarketWarm' in label else 750 if 'CoveBlue' in label else 110)
    if isinstance(a,unreal.SkyLight):
        component=a.get_component_by_class(unreal.SkyLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
        component.set_editor_property('cubemap',unreal.load_asset('/Engine/MapTemplates/Sky/DaylightAmbientCubemap'))
        component.set_light_color(unreal.LinearColor(.46,.72,.80,1));component.set_intensity(1.15)
    if isinstance(a,unreal.DirectionalLight):
        a.get_component_by_class(unreal.DirectionalLightComponent).set_intensity(4.5)
    if label=='OceanArt_ColorAndExposure':
        settings=a.get_editor_property('settings')
        settings.set_editor_property('auto_exposure_min_brightness',1.4)
        settings.set_editor_property('auto_exposure_max_brightness',1.4)
        settings.set_editor_property('bloom_intensity',.08)
        a.set_editor_property('settings',settings)
# Broad soft fills across the reef, wreck and both entry routes, not just the market.
for i,(x,y,z) in enumerate([(1700,-900,-750),(1500,1050,-900),(-600,1500,-900),(-1400,-1200,-800),(-2200,0,-750),(200,-1800,-1000)]):
    a=actors.spawn_actor_from_class(unreal.PointLight,unreal.Vector(x,y,z));a.set_actor_label('OceanFill_%02d'%i)
    c=a.get_component_by_class(unreal.PointLightComponent);c.set_mobility(unreal.ComponentMobility.MOVABLE)
    c.set_editor_property('intensity_units',unreal.LightUnits.LUMENS);c.set_intensity(1600)
    c.set_attenuation_radius(2300);c.set_source_radius(240);c.set_cast_shadows(False)
    c.set_light_color(unreal.LinearColor(.45,.75,.82,1))
levels.save_current_level()
unreal.log('HF_OCEAN_REBALANCED')
