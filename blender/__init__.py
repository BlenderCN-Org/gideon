import bpy

bl_info = {
    "name" : "Extendible Raytracer",
    "description" : "Fully functional raytracer easily extendible for experimenting with new rendering features.",
    "author" : "Curtis Andrus",
    "version": (1, 0),  
    "blender": (2, 6, 5),  
    "category": "Render"
}

from . import render, ui

def register():
    ui.register()
    bpy.utils.register_module(__name__)


def unregister():
    ui.unregister()
    bpy.utils.unregister_module(__name__)
