import bpy
from math import *

from . import sync
from . import engine
import ctypes

class DemoRaytraceEngine(bpy.types.RenderEngine):
    bl_idname = "DEMO_RAYTRACE_ENGINE"
    bl_label = "Demo Raytrace Engine"
    bl_use_shading_nodes = True
    use_highlight_tiles = True

    def __init__(self):
        self.gideon = engine.load_gideon("/home/curtis/Projects/relatively-crazy/build/src/libraytrace.so")

    def render(self, scene):
        #load and compile all programs
        program = engine.create_program(self.gideon, "render")

        for source in scene.gideon.sources:
            print("Loading Object: ", source.name)
            engine.program_load_source(self.gideon, program, source.name)

        compiled = engine.program_compile(self.gideon, program)
        engine.destroy_program(self.gideon, program)
        
        #prepare blender scene data for gideon
        gd_scene = sync.GideonScene(self.gideon, compiled)

        sync.convert_scene(scene, gd_scene, compiled)
        gd_bvh = gd_scene.build_bvh()
        
        #call the renderer's entry point
        pixel_scale = scene.render.resolution_percentage * 0.01
        x_pixels = floor(pixel_scale * scene.render.resolution_x)
        y_pixels = floor(pixel_scale * scene.render.resolution_y)

        tile_x = scene.render.tile_x
        tile_y = scene.render.tile_y

        num_x_tiles = int(ceil(float(x_pixels) / tile_x))
        num_y_tiles = int(ceil(float(y_pixels) / tile_y))
        
        start_px = 0
        start_py = 0

        print("Num Tiles: ", num_x_tiles, num_y_tiles)
        print("Entry Point: ", scene.gideon.entry_point)

        for ty in range(num_y_tiles):
            end_py = min(start_py + tile_y, y_pixels)
            th = end_py - start_py

            for tx in range(num_x_tiles):
                end_px = min(start_px + tile_x, x_pixels)
                tw = end_px - start_px
                
                r = self.begin_result(start_px, start_py, tw, th)
                
                float4_ty = 4 * ctypes.c_float
                result = (tw * th * float4_ty)()
                engine.render_tile(self.gideon, gd_scene.scene, compiled, gd_bvh,
                                   scene.gideon.entry_point.encode('ascii'),
                                   start_px, start_py, tw, th,
                                   result)
                r.layers[0].rect = result

                
                self.end_result(r)
                start_px += tile_x
                
            start_px = 0
            start_py += tile_y

        #cleanup
        engine.destroy_compiled(self.gideon, compiled)
        engine.destroy_bvh(self.gideon, gd_bvh)
