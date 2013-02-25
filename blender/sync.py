import bpy, ctypes
from . import scene, mesh

#Converts all scene objects into a usable format for the renderer.
def SyncScene(bl_scene, is_preview):
    s = scene.Scene()
    s.set_camera(bl_scene)
    
    for obj in bl_scene.objects:
        try:
            #try to load this object as a mesh first
            m = mesh.LoadMeshObject(bl_scene, obj, is_preview)
            s.add_mesh_object(m)
        except RuntimeError:
            if obj.type == 'LAMP':
                print("Loading Lamp:", obj.name)
                lamp = obj.data
                s.add_lamp(obj.location, lamp.energy, lamp.color, lamp.shadow_soft_size)
        
    return s
