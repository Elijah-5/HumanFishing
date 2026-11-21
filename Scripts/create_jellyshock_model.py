"""Author the Jellyshock mesh set. Run with Blender 4.5 --background --python."""
import bpy, math, json
from mathutils import Vector
from pathlib import Path

root=Path(__file__).resolve().parents[1]
output=root/'SourceArt/Equipment/Jellyshock';output.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
def material(name,color,emission=0,alpha=1):
    m=bpy.data.materials.new(name);m.diffuse_color=(*color,alpha);m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,alpha)
    p.inputs['Emission Color'].default_value=(*color,1);p.inputs['Emission Strength'].default_value=emission
    p.inputs['Metallic'].default_value=.05;p.inputs['Roughness'].default_value=.26
    p.inputs['Alpha'].default_value=alpha
    if alpha<1:m.surface_render_method='DITHERED'
    return m
cyan=material('Jelly_Cyan',(.035,.65,.58),.65,.22)
shell=material('Jelly_Glass',(.1,.85,.72),.4,.48)
gold=material('Jelly_Gold',(1,.58,.12),2)
rim=material('Jelly_Rim',(.48,1,.83),1.2)
def mesh(name,vertices,faces,mat):
    data=bpy.data.meshes.new(name);data.from_pydata(vertices,[],faces);data.update()
    obj=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(obj);data.materials.append(mat)
    color=data.color_attributes.new(name='Color',type='BYTE_COLOR',domain='POINT')
    for i,v in enumerate(data.vertices):color.data[i].color=(max(0,min(1,-v.co.z/.72)),0,0,1)
    return obj
def tube(name,points,radii,mat,sides=5):
    vertices=[];faces=[]
    for i,p in enumerate(points):
        tangent=Vector(points[min(i+1,len(points)-1)])-Vector(points[max(0,i-1)])
        tangent.normalize();right=tangent.cross(Vector((0,1,0))).normalized();up=tangent.cross(right).normalized()
        for j in range(sides):
            a=j*2*math.pi/sides;vertices.append(Vector(p)+radii[i]*(right*math.cos(a)+up*math.sin(a)))
    for i in range(len(points)-1):
        for j in range(sides):
            a=i*sides+j;b=i*sides+(j+1)%sides;c=b+sides;d=a+sides
            faces.append((a,b,c,d))
    faces.extend([tuple(reversed(range(sides))),tuple((len(points)-1)*sides+j for j in range(sides))])
    return mesh(name,vertices,faces,mat)
def dome(name,radius,height,mat):
    vertices=[];faces=[];segments=16
    for row in range(6):
        theta=.04+row/5*math.pi*.49
        for j in range(segments):
            a=2*math.pi*j/segments
            z=.04+math.cos(theta)*height+(math.cos(a*8)*.018 if row==5 else 0)
            vertices.append((math.cos(a)*math.sin(theta)*radius,math.sin(a)*math.sin(theta)*radius,z))
    faces.append(tuple(reversed(range(segments))))
    for row in range(5):
        for j in range(segments):faces.append((row*segments+j,row*segments+(j+1)%segments,(row+1)*segments+(j+1)%segments,(row+1)*segments+j))
    return mesh(name,vertices,faces,mat)
bell=dome('SM_JellyBell',.30,.30,shell)
inner=dome('SM_JellyInner',.245,.235,cyan)
points=[]
for i in range(23):
    t=i/22
    if t<.68:p=Vector((.035*math.sin(t*6),0,-t*.9))
    else:
        a=(t-.68)/.32*math.pi*1.65;p=Vector((.10*(1-math.cos(a)),0,-.612-.1*math.sin(a)))
    points.append(p)
tentacle=tube('SM_JellyTentacle',points,[.031*(1-i/25) for i in range(23)],rim)
bolt=[(-.015,.115),(.11,.26),(.055,.14),(.15,.17),(-.035,-.015),(.015,.105)]
vertices=[(x,y,z+.04) for y in (-.018,.018) for x,z in bolt]
n=len(bolt);faces=[tuple(reversed(range(n))),tuple(range(n,2*n))]
faces += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
core=mesh('SM_JellyCore',vertices,faces,gold)
vertices=[];faces=[]
for j in range(48):
    a=j*2*math.pi/48
    for radius in (.982,1.,1.018):vertices.append((math.cos(a)*radius,math.sin(a)*radius,0))
for j in range(48):
    for k in range(2):faces.append((j*3+k,j*3+k+1,((j+1)%48)*3+k+1,((j+1)%48)*3+k))
ring=mesh('SM_JellyRing',vertices,faces,rim)
for i,value in enumerate(ring.data.color_attributes['Color'].data):value.color=(1,1,1,1 if i%3==1 else 0)
arc=tube('SM_JellyArc',[(0,0,0),(.18,0,.10),(.32,0,-.08),(.5,0,.12),(.65,0,-.05),(.83,0,.065),(1,0,0)],[.016]*7,rim,3)
bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1,radius=1)
bubble=bpy.context.object;bubble.name='SM_JellyBubble';bubble.data.materials.append(rim)
assets=[bell,inner,tentacle,core,ring,arc,bubble]
report={}
for obj in assets:
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.ops.export_scene.fbx(filepath=str(output/(obj.name+'.fbx')),use_selection=True,object_types={'MESH'},
        bake_anim=False,add_leaf_bones=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True)
    report[obj.name]=sum(len(p.vertices)-2 for p in obj.data.polygons)
    obj.hide_render=obj in (ring,arc,bubble)
for i,x in enumerate((-.16,0,.16)):
    obj=tentacle if i==0 else tentacle.copy()
    if i:obj.data=tentacle.data;bpy.context.collection.objects.link(obj)
    obj.location=(x,0,.055);obj.rotation_euler.z=(i-1)*.55
    obj.scale=(1,1,.88 if i!=1 else 1.12)
second=core.copy();bpy.context.collection.objects.link(second);second.rotation_euler.z=math.pi/2
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=32
scene.world.color=(.025,.045,.055)
bpy.ops.object.camera_add(location=(1.6,-3.4,1.15));camera=bpy.context.object
camera.rotation_euler=(Vector((0,0,-.16))-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.type='ORTHO';camera.data.ortho_scale=1.4;scene.camera=camera
bpy.ops.object.light_add(type='AREA',location=(1,-2,3));bpy.context.object.data.energy=250;bpy.context.object.data.shape='DISK';bpy.context.object.data.size=4
scene.render.resolution_x=900;scene.render.resolution_y=900;scene.render.resolution_percentage=100
scene.view_settings.view_transform='Standard'
scene.render.filepath=str(root/'Saved/Validation/Images/JellyshockModel.png')
Path(scene.render.filepath).parent.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(output/'Jellyshock.blend'))
bpy.ops.render.render(write_still=True)
(output/'mesh-budget.json').write_text(json.dumps(report,indent=2)+'\n')
print('HF_JELLY_MODEL',report,'PROJECTILE_TRIANGLES',report[bell.name]+report[inner.name]+3*report[tentacle.name]+2*report[core.name])
