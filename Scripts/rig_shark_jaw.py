"""Run with Blender --background --python Scripts/rig_shark_jaw.py."""
import bpy,bmesh,math
from pathlib import Path
from mathutils import Vector,Matrix
project=Path(__file__).resolve().parents[1]
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=str(project/'SourceArt/Ocean/Quaternius/Shark.fbx'))
rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
rig.name='SharkRig'
mesh=next(o for o in bpy.context.scene.objects if o.type=='MESH')
scene=bpy.context.scene
scene.frame_start=0;scene.frame_end=30;scene.frame_set(0)
bpy.context.view_layer.update()
print('HF_SHARK_VERTICES',len(mesh.data.vertices),'BOUNDS',[(min(v.co[i] for v in mesh.data.vertices),max(v.co[i] for v in mesh.data.vertices)) for i in range(3)])
bpy.ops.object.camera_add(location=(20,-.5,4))
camera=bpy.context.object
evaluated=mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
corners=[mesh.matrix_world@Vector(p) for p in evaluated.bound_box]
center=sum(corners,Vector())/8
camera.location=center+Vector((30,0,5))
camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.type='ORTHO';camera.data.ortho_scale=max((max(p.z for p in corners)-min(p.z for p in corners))*1.777,max(p.y for p in corners)-min(p.y for p in corners))*1.25
scene.camera=camera
scene.render.engine='BLENDER_WORKBENCH'
scene.display.shading.light='STUDIO';scene.display.shading.color_type='MATERIAL'
scene.display.shading.show_shadows=True;scene.display.shading.show_cavity=True
scene.render.resolution_x=960;scene.render.resolution_y=540;scene.render.resolution_percentage=100
scene.render.film_transparent=True;scene.view_settings.view_transform='Standard'
output=project/'Saved/Validation/Images';output.mkdir(parents=True,exist_ok=True)
scene.render.filepath=str(output/'SharkOriginal.png');bpy.ops.render.render(write_still=True)
matrix=rig.matrix_world.inverted()@mesh.matrix_world
face=mesh.vertex_groups['Face'].index
head=[matrix@v.co for v in mesh.data.vertices if any(g.group==face and g.weight>.35 for g in v.groups)]
front_y=min(p.y for p in head);back_y=max(p.y for p in head)
bottom_z=min(p.z for p in head);top_z=max(p.z for p in head)
seam_z=bottom_z+(top_z-bottom_z)*.45
hinge_y=back_y-(back_y-front_y)*.12
print('HF_JAW_REGION',front_y,back_y,bottom_z,top_z)
bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);bpy.context.view_layer.objects.active=rig
bpy.ops.object.mode_set(mode='EDIT')
jaw=rig.data.edit_bones.new('Jaw')
jaw.head=(0,hinge_y,seam_z);jaw.tail=(0,front_y,seam_z)
jaw.parent=rig.data.edit_bones['Face']
bpy.ops.object.mode_set(mode='OBJECT')
group=mesh.vertex_groups.new(name='Jaw')
bm=bmesh.new();bm.from_mesh(mesh.data);deform=bm.verts.layers.deform.verify()
head_faces=[f for f in bm.faces if (matrix@f.calc_center_median()).y<hinge_y]
geometry=set(head_faces)
for f in head_faces:geometry.update(f.edges);geometry.update(f.verts)
inverse=matrix.inverted()
cut=bmesh.ops.bisect_plane(bm,geom=list(geometry),dist=.00001,
    plane_co=inverse@Vector((0,0,seam_z)),plane_no=(matrix.to_3x3().transposed()@Vector((0,0,1))).normalized())
seam_edges=[e for e in bm.edges if all(abs((matrix@v.co).z-seam_z)<.0001 for v in e.verts) and (matrix@e.verts[0].co).y<hinge_y+.01]
bmesh.ops.split_edges(bm,edges=seam_edges)
seam=list({tuple(round(x,5) for x in v.co):v for v in bm.verts if abs((matrix@v.co).z-seam_z)<.0001 and (matrix@v.co).y<hinge_y+.01}.values())
lower_faces=[f for f in bm.faces if (matrix@f.calc_center_median()).y<hinge_y and (matrix@f.calc_center_median()).z<seam_z-.00001]
lower_vertices={v for f in lower_faces for v in f.verts}
count=0
for vertex in lower_vertices:
    p=matrix@vertex.co
    weight=max(0.,min(1.,(hinge_y-p.y)/max(.01,(hinge_y-front_y)*.65)))
    if weight>.001:
        weights=vertex[deform]
        for index in list(weights.keys()):weights[index]*=1-weight
        weights[group.index]=weight;count+=1
# Dark upper/lower palate caps expose a real mouth cavity as the seam opens.
mouth=bpy.data.materials.new('MouthInterior');mouth.diffuse_color=(.018,.028,.038,1)
mesh.data.materials.append(mouth);material_index=len(mesh.data.materials)-1
center_mouth=sum((matrix@v.co for v in seam),Vector())/max(1,len(seam))
ring=sorted(seam,key=lambda v:math.atan2((matrix@v.co).y-center_mouth.y,(matrix@v.co).x-center_mouth.x))
for jaw_side in (False,True):
    vertices=[]
    for v in ring:
        vcopy=bm.verts.new(v.co);vcopy[deform][group.index if jaw_side else face]=1
        vertices.append(vcopy)
    center_vertex=bm.verts.new(inverse@center_mouth);center_vertex[deform][group.index if jaw_side else face]=1
    for i in range(len(vertices)):
        tri=(center_vertex,vertices[i],vertices[(i+1)%len(vertices)])
        bm.faces.new(tri if jaw_side else tuple(reversed(tri))).material_index=material_index
# Tiny eyes make the original minimal head readable without changing the low-poly silhouette.
eye_width=max(abs(p.x) for p in head)*.91
for side in (-1,1):
    eye=inverse@Vector((side*eye_width,front_y+(back_y-front_y)*.55,seam_z+(top_z-seam_z)*.50))
    result=bmesh.ops.create_icosphere(bm,subdivisions=1,radius=.12,matrix=Matrix.Translation(eye))
    for v in result['verts']:
        v[deform][face]=1
        for f in v.link_faces:f.material_index=material_index
bm.to_mesh(mesh.data);bm.free();mesh.data.update()
print('HF_JAW_WEIGHTED_VERTICES',count)
assert count>=4
rig.pose.bones['Jaw'].rotation_mode='XYZ';rig.pose.bones['Jaw'].rotation_euler.x=math.radians(-25)
bpy.context.view_layer.update()
scene.render.filepath=str(output/'SharkJawOpen.png');bpy.ops.render.render(write_still=True)
rig.pose.bones['Jaw'].rotation_euler.x=0
scene.frame_set(0);bpy.context.view_layer.update()
source=project/'SourceArt/Ocean/Quaternius'
bpy.ops.wm.save_as_mainfile(filepath=str(source/'Shark_Jaw.blend'))
bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);mesh.select_set(True)
bpy.context.view_layer.objects.active=rig
bpy.ops.export_scene.fbx(filepath=str(source/'Shark_Jaw.fbx'),use_selection=True,
    object_types={'ARMATURE','MESH'},add_leaf_bones=False,axis_forward='-Z',axis_up='Y',
    bake_anim=True,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,
    bake_anim_use_all_bones=True,bake_anim_simplify_factor=0.0)
print('HF_SHARK_JAW_EXPORTED')
