"""Import the authored Jellyshock meshes and explicit low-cost luminous materials."""
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
folder='/Game/HumanFishing/Equipment/Jellyshock'
tools=unreal.AssetToolsHelpers.get_asset_tools();lib=unreal.MaterialEditingLibrary
def node(mat,kind,**properties):
    result=lib.create_material_expression(mat,kind)
    for name,value in properties.items():result.set_editor_property(name,value)
    return result
def wire(source,target,pin,output=''):lib.connect_material_expressions(source,output,target,pin)
def product(mat,a,b):
    out=node(mat,unreal.MaterialExpressionMultiply);wire(a,out,'A');wire(b,out,'B');return out
def constant(mat,value):return node(mat,unreal.MaterialExpressionConstant,r=value)
def make(name,color,mode,opacity=1):
    mat=unreal.load_asset(folder+'/'+name)
    if not mat:mat=tools.create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew())
    lib.delete_all_material_expressions(mat)
    mat.set_editor_property('blend_mode',mode);mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided',True)
    tint=node(mat,unreal.MaterialExpressionVectorParameter,parameter_name='Tint',default_value=unreal.LinearColor(*color,1))
    strength=node(mat,unreal.MaterialExpressionScalarParameter,parameter_name='Strength',default_value=1.)
    emissive=product(mat,tint,strength)
    if mode==unreal.BlendMode.BLEND_TRANSLUCENT:
        fresnel=node(mat,unreal.MaterialExpressionFresnel,exponent=2.5,base_reflect_fraction=.08)
        edge=product(mat,fresnel,constant(mat,.6))
        alpha=node(mat,unreal.MaterialExpressionAdd);wire(edge,alpha,'A');wire(constant(mat,opacity),alpha,'B')
        lib.connect_material_property(product(mat,alpha,strength),'',unreal.MaterialProperty.MP_OPACITY)
        boost=node(mat,unreal.MaterialExpressionAdd);wire(fresnel,boost,'A');wire(constant(mat,.8),boost,'B')
        emissive=product(mat,emissive,boost)
    elif mode==unreal.BlendMode.BLEND_ADDITIVE:
        vertex=node(mat,unreal.MaterialExpressionVertexColor)
        alpha=node(mat,unreal.MaterialExpressionMultiply);wire(constant(mat,opacity),alpha,'A');wire(vertex,alpha,'B','A')
        lib.connect_material_property(alpha,'',unreal.MaterialProperty.MP_OPACITY)
    lib.connect_material_property(emissive,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.set_material_usage(mat,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    lib.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat
opaque=unreal.BlendMode.BLEND_OPAQUE;glass=unreal.BlendMode.BLEND_TRANSLUCENT
materials={
    'SM_JellyBell':make('M_JellyBell',(.08,1.15,.88),glass,.30),
    'SM_JellyInner':make('M_JellyBody',(.035,.62,.49),glass,.16),
    'SM_JellyTentacle':make('M_JellyTendril',(.35,1.7,1.15),opaque),
    'SM_JellyCore':make('M_JellyGold',(2.5,1.25,.18),opaque),
    'SM_JellyRing':make('M_JellyElectric',(.22,2.1,1.5),unreal.BlendMode.BLEND_ADDITIVE,.8),
    'SM_JellyBubble':make('M_JellyBubble',(.25,1.4,1.1),glass,.06),
}
materials['SM_JellyArc']=materials['SM_JellyRing']
for name,mat in materials.items():
    task=unreal.AssetImportTask();task.filename=str(root/'SourceArt/Equipment/Jellyshock'/(name+'.fbx'))
    task.destination_path=folder;task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True
    options=unreal.FbxImportUI();options.automated_import_should_detect_type=False
    options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;options.import_as_skeletal=False
    options.import_materials=False;options.import_textures=False;options.import_animations=False
    options.static_mesh_import_data.auto_generate_collision=False
    options.static_mesh_import_data.generate_lightmap_u_vs=False
    options.static_mesh_import_data.vertex_color_import_option=unreal.VertexColorImportOption.REPLACE
    task.options=options;tools.import_asset_tasks([task])
    asset=unreal.load_asset(folder+'/'+name);assert isinstance(asset,unreal.StaticMesh),name
    asset.set_material(0,mat);unreal.EditorAssetLibrary.save_loaded_asset(asset)
    unreal.log('HF_JELLY_IMPORTED '+name+' '+str(asset.get_bounds().box_extent))
unreal.log('HF_JELLY_ASSETS_READY')
