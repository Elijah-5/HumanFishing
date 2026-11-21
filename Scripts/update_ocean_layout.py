"""Idempotent editor migration: authored low-poly scenery and explicit travel."""
import unreal, json, math, random, shutil
from pathlib import Path
project=Path(unreal.Paths.project_dir()).resolve()
sources=Path('/private/tmp/humanfishing-model-sources')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
library=unreal.EditorAssetLibrary
root='/Game/HumanFishing/Ocean'
manifest=json.loads((project/'Saved/OceanAssets.json').read_text())
rng=random.Random(415)

def vector(values): return unreal.Vector(*values)
def backup(relative):
    target=project/'Saved/UpdateBackups'/relative
    if not target.exists():
        target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(project/relative,target)

for name in ['structure','structure-roof','structure-platform','boat-row-small']:
    t=unreal.AssetImportTask(); t.filename=str(sources/'pirate/Models/FBX format'/(name+'.fbx'))
    t.destination_path=root+'/Kenney';t.destination_name='SM_'+name.replace('-','_');t.automated=True;t.replace_existing=True;t.save=True
    o=unreal.FbxImportUI();o.automated_import_should_detect_type=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    o.import_as_skeletal=False;o.import_animations=False;o.import_materials=True;o.import_textures=True
    o.static_mesh_import_data.combine_meshes=True;o.static_mesh_import_data.force_front_x_axis=True;t.options=o
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    manifest['props'][name]=root+'/Kenney/'+t.destination_name
    shutil.copy2(Path(t.filename),project/'SourceArt/Ocean/Kenney'/(name+'.fbx'))
texture_dir=project/'SourceArt/Ocean/Kenney/Textures';texture_dir.mkdir(exist_ok=True)
for p in (sources/'pirate/Models/FBX format/Textures').glob('*'):shutil.copy2(p,texture_dir/p.name)

def prop(label,path,position,length,yaw=0,height=None,collision=False):
    mesh=unreal.load_asset(path); assert isinstance(mesh,unreal.StaticMesh),path
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,vector(position),unreal.Rotator(yaw=yaw))
    a.set_actor_label('OceanArt_'+label)
    c=a.static_mesh_component;c.set_static_mesh(mesh)
    b=mesh.get_bounds();s=length/(max(b.box_extent.x,b.box_extent.y)*2.)
    scale=unreal.Vector(s,s,s if height is None else height/(b.box_extent.z*2.))
    a.set_actor_scale3d(scale)
    # Imported coral variants retain their source arrangement; recenter each one.
    offset=unreal.MathLibrary.rotate_angle_axis(unreal.Vector(b.origin.x*scale.x,b.origin.y*scale.y,(b.origin.z-b.box_extent.z)*scale.z),yaw,unreal.Vector(0,0,1))
    a.set_actor_location(vector(position)-offset,False,False)
    c.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS if collision else unreal.CollisionEnabled.NO_COLLISION)
    return a

def point(label,location,color,intensity,radius):
    a=actors.spawn_actor_from_class(unreal.PointLight,vector(location));a.set_actor_label('OceanArt_'+label)
    c=a.get_component_by_class(unreal.PointLightComponent);c.set_mobility(unreal.ComponentMobility.MOVABLE);c.set_light_color(unreal.LinearColor(*color))
    c.set_intensity(intensity);c.set_attenuation_radius(radius);c.set_cast_shadows(False)
    return a

def text(label,words,location,yaw,size=32):
    a=actors.spawn_actor_from_class(unreal.TextRenderActor,vector(location),unreal.Rotator(yaw=yaw));a.set_actor_label('OceanArt_'+label)
    c=a.get_component_by_class(unreal.TextRenderComponent);c.set_text(words);c.set_world_size(size)
    c.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER);c.set_text_render_color(unreal.Color(138,237,215,255))
    return a

def travel(label,destination,location,extent,arrival=''):
    a=actors.spawn_actor_from_class(unreal.HumanFishingTravelZone,vector(location));a.set_actor_label('ENTRY_'+label.replace(' ','_'))
    a.set_editor_property('destination_map',unreal.load_asset('/Game/HumanFishing/Maps/'+destination))
    a.set_editor_property('destination_label',label);a.set_editor_property('arrival_tag',arrival)
    a.travel_volume.set_box_extent(vector(extent),False)
    # A clearly visible boundary made from imported rocks, matching the interaction volume.
    return a

for name in ['L_DeepOcean','L_ShorePrototype','L_ScenicCove']:
    backup('Content/HumanFishing/Maps/'+name+'.umap')
    levels.load_level('/Game/HumanFishing/Maps/'+name)
    existing=list(actors.get_all_level_actors())
    for a in existing:
        label=a.get_actor_label()
        if label.startswith(('OceanArt_','ENTRY_','TRAVEL_','TravelDirection_')):
            actors.destroy_actor(a)
    if name!='L_DeepOcean':
        travel('Deep Ocean','L_DeepOcean',(-1130,0,-300),(160,230,220),'ScenicCove' if name=='L_ScenicCove' else 'ShoreSide')
        for y in [-340,340]:
            prop('ShoreEntryRock',manifest['props']['rocks-a'],(-1210,y,-660),380,30 if y<0 else -40,480,True)
            prop('ShoreEntryCoral',manifest['corals']['1'][0],(-990,y,-410),100)
        text('DiveSign','DEEP OCEAN\nApproach the reef · E to dive',(-990,0,-100),0,28)
        levels.save_current_level();continue

    # Remove visible sphere placeholders, retaining floor and invisible map boundaries.
    for a in list(actors.get_all_level_actors()):
        label=a.get_actor_label()
        if label.startswith(('DeepRock_','CoveArch_')):actors.destroy_actor(a)
        if label.startswith('OceanBoundary_'):
            for c in a.get_components_by_class(unreal.PrimitiveComponent):c.set_visibility(False)
        if label=='SeaweedExchange':
            for c in a.get_components_by_class(unreal.StaticMeshComponent):
                c.set_visibility(False);c.set_hidden_in_game(True);c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        if isinstance(a,unreal.DirectionalLight):
            c=a.get_component_by_class(unreal.DirectionalLightComponent);c.set_intensity(3.0)
            c.set_light_color(unreal.LinearColor(.59,.84,1,1))
        if isinstance(a,unreal.ExponentialHeightFog):
            c=a.get_component_by_class(unreal.ExponentialHeightFogComponent);c.set_fog_density(.006);c.set_fog_inscattering_color(unreal.LinearColor(.012,.12,.17,1))
            c.set_fog_height_falloff(.05);c.set_start_distance(180)

    travel('Shore Beach','L_ShorePrototype',(2650,0,-350),(150,240,240))
    travel('Scenic Cove','L_ScenicCove',(-2650,0,-200),(140,240,160))
    # Tall rock landmarks frame a narrow, readable exit instead of a whole-map trigger.
    for x in [-2780,2780]:
        for y in [-470,470]:
            prop('GatewayPillar',manifest['props']['rocks-b'],(x,y,-1800),610,20 if y<0 else 165,1770,True)
        prop('GatewayCrown',manifest['props']['rocks-a'],(x,0,-80),1150,90,150,True)
        for y in [-410,410]:
            prop('GatewayCoral',manifest['corals']['2'][1],(x+ (230 if x<0 else -230),y,-820),240,25,330)
    # A real wooden structure on a reef shelf, surrounded by recognizable trading props.
    prop('MarketReef',manifest['props']['rocks-c'],(380,0,-1800),1100,12,820,True)
    prop('MarketPlatform',manifest['props']['structure-platform'],(380,0,-1025),620,90,100,True)
    prop('MarketFrame',manifest['props']['structure'],(330,0,-1020),550,90,390)
    prop('MarketRoof',manifest['props']['structure-roof'],(330,0,-640),610,90,150)
    for i,(x,y,n) in enumerate([(520,-225,'barrel'),(480,215,'crate'),(230,-215,'chest')]):
        prop('MarketStock'+str(i),manifest['props'][n],(x,y,-1010),130,15*i)
    point('MarketWarmLight',(600,0,-580),(1,.76,.4,1),18000,1050)
    text('MarketDirection','SEAWEED EXCHANGE\nBait · Upgrades · Cove Key',(850,0,-480),0,35)
    # The wreck is an unmistakable landmark on the Cove side of the seabed.
    prop('CoveShipwreck',manifest['props']['ship-wreck'],(-1250,-1120,-1790),1450,-25,1100,True)
    for i,(x,y,n) in enumerate([(-710,-1520,'barrel'),(-620,-1330,'crate'),(-1770,-1430,'chest')]):
        prop('WreckCargo'+str(i),manifest['props'][n],(x,y,-1795),160,30*i)
    point('CoveBlueLight',(-2010,0,-640),(.18,.66,1,1),22000,1650)

    # Deliberate reef islands around a clear east/west swimming corridor.
    islands=[(1910,-800,0),(1620,1050,1),(1150,-1640,2),(200,1480,3),(-700,1800,4),(-2100,1050,5),(-2010,-2000,6),(-80,-2000,7)]
    for x,y,index in islands:
        rock=prop('Reef%02d'%index,manifest['props'][['rocks-a','rocks-b','rocks-c'][index%3]],(x,y,-1805),650+index%3*110,index*39,330+index%2*120,True)
        for j in range(5):
            angle=j*math.tau/5+index
            px=x+math.cos(angle)*250;py=y+math.sin(angle)*235
            group=str(1+(index+j)%6);variants=manifest['corals'][group]
            prop('Coral%02d_%d'%(index,j),variants[(index+j)%len(variants)],(px,py,-1700+(j%2)*90),150+(j%3)*55,j*65,160+(j%3)*80)
        for j in range(3):
            prop('SeaGrass%02d_%d'%(index,j),manifest['props']['grass-plant'],(x-280+j*90,y+180,-1795),180,j*80,240)
    # Small coral clusters make the entry heights readable even from the spawn points.
    for x in [-2330,2250]:
        for side in [-1,1]:
            prop('EntryReefShelf',manifest['props']['rocks-a'],(x,side*500,-1250),600,35*side,500)
            for j in range(3):
                prop('EntryCoral',manifest['corals'][str(j+1)][0],(x+j*75,side*(450+j*60),-810),180,30*j,240)

    for i,(x,y,z) in enumerate([(1720,-720,-1120),(1430,970,-1320),(-850,1500,-1040),(-1700,-1100,-950),(350,-1200,-1250),(-2100,720,-630),(1200,-750,-960),(-1100,-1350,-1050),(550,1400,-1130)]):
        a=actors.spawn_actor_from_class(unreal.HumanFishingAmbientFish,unreal.Vector(x,y,z));a.set_actor_label('OceanArt_ReefFish%02d'%i)
        species='Fish1' if i%2 else 'Fish3'
        mesh=unreal.load_asset(manifest['fauna'][species]['mesh']);animation=unreal.load_asset(manifest['fauna'][species]['animation'])
        a.set_editor_property('fish_mesh',mesh);a.set_editor_property('swim_animation',animation)
        a.visual.set_skeletal_mesh(mesh);a.visual.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        a.set_editor_property('fish_length',60.+i%3*15);a.set_editor_property('swim_radius',200.+i%3*60)
        bounds=mesh.get_bounds();a.set_actor_scale3d(unreal.Vector(1,1,1)*(a.fish_length/(max(bounds.box_extent.x,bounds.box_extent.y)*2)))
    levels.save_current_level()
    unreal.log('HF_OCEAN_LAYOUT_COMPLETE')

library.save_directory(root,only_if_is_dirty=True,recursive=True)
(project/'Saved/OceanAssets.json').write_text(json.dumps(manifest,indent=2))
# Replace the unused prototype visitor's template mannequin with the existing visitor.
legacy=unreal.load_asset('/Game/Characters/Mannequin_UE4/Meshes/Human1')
backup('Content/Characters/Mannequin_UE4/Meshes/Human1.uasset')
replacement=unreal.load_asset('/Game/HumanFishing/Fishing/Art/SK_BeachHuman')
assert replacement,'Missing existing visitor mesh'
unreal.log('HF_TEMPLATE_MESHES_REPLACED '+str(unreal.HumanFishingEditorLibrary.replace_template_mesh(legacy,replacement)))
library.save_loaded_asset(legacy)
