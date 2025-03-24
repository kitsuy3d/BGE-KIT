bl_info = {
    "name": "TerrainLod Tools",
    "author": "Rafael Tavares(ESG)",
    "version": (1, 0, 0),
    "blender": (0, 2, 5),
    "location": "View3D > Tools",
    "description": "Create a lod terrain for upbge games.",
    "warning": "",
    "wiki_url": "https://discord.com/invite/NcfWjCD",
    "category": "Game Engine",
    }
    
import bpy, bmesh
from bpy.types import (Operator,Panel)
from  mathutils import Vector


class makeSlice(Operator):
    bl_idname = "make.slice"
    bl_label = "Slice the Terrain"
    bl_description = ("Make Chunks")
                      
    def execute(self, context):
        bm = bmesh.new()
        ob = context.object
        ob.name += "_Chunk"
        me = ob.data
        bm.from_mesh(me)
        o, x, y, z = bbox_axes(ob)        
        slice(bm, o, x, context.scene.lodtools_xslice)
        slice(bm, o, y, context.scene.lodtools_yslice)
        slice(bm, o, z, 1)    
        bm.to_mesh(me)
        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.mesh.separate(type='LOOSE')
        bpy.ops.object.mode_set()
        
        return {'FINISHED'}
        
class makeDecimate(Operator):
    bl_idname = "make.decimate"
    bl_label = "Make Decimate in Terrain Chunks"
    bl_description = ("Make Lod Chunks")
    def execute(self, context):
        import random
        
        object = bpy.context.active_object
        
        mesh = object.data
        ratio = context.scene.lodtools_decimatelod
        lodGenerateRate = context.scene.lodtools_lodgen
        vert_group = "Lod"
        if lodGenerateRate < 1:
            lodGenerateRate = 1
        
        Chunks = bpy.context.selected_objects
        
        
        for object in Chunks:
            if object.type == 'MESH':
               bpy.ops.object.select_all(action='DESELECT')
               bpy.context.scene.objects.active = object
               object.select = True

               bpy.ops.object.lod_generate(count=1+lodGenerateRate, target=0.5, package=False)
               
        for object in Chunks: 
           Chunk = object
           for lod in range(1,lodGenerateRate+1):
                bpy.context.scene.objects.active = bpy.data.objects[Chunk.name+"lod{}".format(lod)]
                bpy.ops.object.modifier_remove(modifier="lod_decimate")
                object = bpy.context.scene.objects.active # lod chunk
                object.matrix_world.translation = Chunk.location.x,Chunk.location.y,Chunk.location.z + -5*lod
           
                bpy.ops.object.mode_set( mode = 'EDIT' )
                bpy.ops.mesh.select_mode( type = 'VERT' )
                bpy.ops.mesh.select_all( action = 'DESELECT' )
                bpy.ops.mesh.select_non_manifold()
                bpy.ops.mesh.select_all( action = 'INVERT' )
                   
                if bpy.context.scene.objects.active.vertex_groups.active:
                    bpy.ops.object.vertex_group_remove(all=True)


                bpy.ops.object.vertex_group_add()
                   

                bpy.ops.object.vertex_group_assign()
                object.vertex_groups.active.name = vert_group
                bpy.ops.object.mode_set( mode = 'OBJECT' )


                if bpy.context.scene.objects.active.modifiers.find('Decimate') == 1:
                    bpy.ops.object.modifier_remove( modifier='Decimate' )       
                   
                modi = object.modifiers.new("Decimate", "DECIMATE")
                modi.ratio = 1-ratio*lod
                modi.vertex_group = vert_group
                   
                bpy.ops.object.modifier_apply(apply_as='DATA', modifier="Decimate")
                
        for chunk in Chunks:
            chunk.select = True
        return {'FINISHED'}
    
class SetChunkDistance(Operator):
    bl_idname = "chunk.distance"
    bl_label = "Set Distance Factor of the Chunk"
    bl_description = ("Set Distance Factor for all the chunks")
    def execute(self, context):
        for object in bpy.context.selected_objects:
            object.lod_factor = context.scene.lodtools_lod_distance
        return {'FINISHED'}        
    
class ChunkAdjust(Operator):
    bl_idname = "chunk.adjust"
    bl_label = "Make Decimate in Terrain Chunks"
    bl_description = ("Adjust Chunks origins")
    def execute(self, context):
        for object in bpy.context.selected_objects:
            bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='MEDIAN')
        return {'FINISHED'}


class LodTools(Panel):
    bl_label = "TerrainLod Tools"
    bl_idname = "TerrainLod_Tools"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'TOOLS'
    bl_context = "objectmode"
    bl_category = "Tools"

    def draw(self, context):
        layout = self.layout
        layout.label("Make Chunks")
        
        col = layout.column(align=True)
        col.prop(context.scene, "lodtools_xslice", text="Chunks X")
        col.prop(context.scene, "lodtools_yslice", text="Chunks Y")
        layout.operator("make.slice", text="Make Chunks", icon='UV_FACESEL')
        layout.operator("chunk.adjust", text="Adjust Chunks", icon='UV_VERTEXSEL')
        col = layout.column(align=True)
        col.label("Optimize Chunks")
        col.prop(context.scene, "lodtools_lodgen", text="Lod Generate")
        col.prop(context.scene, "lodtools_lod_offset", text="Lod Offset")
        col.prop(context.scene, "lodtools_decimatelod", text="Lod Decimate")
        layout.operator("make.decimate", text="Create Lod Chunks", icon='UV_ISLANDSEL')
        col = layout.column(align=True)
        col.label("Chunks Lod Factor")
        col.prop(context.scene, "lodtools_lod_distance", text="Chunk Lod Factor")
        layout.operator("chunk.distance", text="Set Lod Factor", icon='UV_ISLANDSEL')
        
# Slice 


def slice(bm, start, end, segments):
    if segments == 1:
        return
    def geom(bm):
        return bm.verts[:] + bm.edges[:] + bm.faces[:]
    planes = [start.lerp(end, f / segments) for f in range(1, segments)]
    #p0 = start
    plane_no = (end - start).normalized() 
    while(planes): 
        p0 = planes.pop(0)                 
        ret = bmesh.ops.bisect_plane(bm, 
                geom=geom(bm),
                plane_co=p0, 
                plane_no=plane_no)
        bmesh.ops.split_edges(bm, 
                edges=[e for e in ret['geom_cut'] 
                if isinstance(e, bmesh.types.BMEdge)])
# Slice

def bbox(ob):
    return (Vector(b) for b in ob.bound_box)

def bbox_center(ob):
    return sum(bbox(ob), Vector()) / 8

def bbox_axes(ob):
    bb = list(bbox(ob))
    return tuple(bb[i] for i in (0, 4, 3, 1))

def register():
    bpy.utils.register_class(LodTools)
    bpy.utils.register_class(makeSlice)
    bpy.utils.register_class(makeDecimate)
    bpy.utils.register_class(ChunkAdjust)
    bpy.utils.register_class(SetChunkDistance)
    
    bpy.types.Scene.lodtools_xslice = bpy.props.IntProperty(name="", default=4)
    bpy.types.Scene.lodtools_yslice = bpy.props.IntProperty(name="", default=4)
    bpy.types.Scene.lodtools_decimatelod = bpy.props.FloatProperty(name="", default=0.5)
    bpy.types.Scene.lodtools_lodgen = bpy.props.IntProperty(name="", default=2)
    bpy.types.Scene.lodtools_lod_offset = bpy.props.FloatProperty(name="", default=5)
    bpy.types.Scene.lodtools_lod_distance = bpy.props.FloatProperty(name="", default=0.5)

def unregister():
    bpy.utils.unregister_class(LodTools)
    bpy.utils.unregister_class(makeSlice)
    bpy.utils.unregister_class(makeDecimate)
    bpy.utils.unregister_class(ChunkAdjust)
    bpy.utils.unregister_class(SetChunkDistance)
    
    del bpy.types.Scene.lodtools_xslice
    del bpy.types.Scene.lodtools_yslice
    del bpy.types.Scene.lodtools_decimatelod
    del bpy.types.Scene.lodtools_lodgen
    del bpy.types.Scene.lodtools_lod_offset


if __name__ == "__main__":
    register()