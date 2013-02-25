import bpy
from math import *

from . import sync

class DemoRaytraceEngine(bpy.types.RenderEngine):
    bl_idname = "DEMO_RAYTRACE_ENGINE"
    bl_label = "Demo Raytrace Engine"
    bl_use_shading_nodes = True

    def __init__(self):
        pass

    def render(self, scene):
        print("Scene: ", dir(scene))
        s = sync.SyncScene(scene, False)
        print(s)
        s.stats()

        pixel_scale = scene.render.resolution_percentage * 0.01
        x_pixels = floor(pixel_scale * scene.render.resolution_x)
        y_pixels = floor(pixel_scale * scene.render.resolution_y)

        r = self.begin_result(0, 0, x_pixels, y_pixels)
        layer = r.layers[0]
        print("Layer:", layer)
        print("Rect:", layer.rect)

        s.render_depth(x_pixels, y_pixels, layer)
        self.end_result(r)
        
