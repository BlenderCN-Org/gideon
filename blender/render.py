import bpy
from math import *

from . import sync
from . import engine
import ctypes

class DemoRaytraceEngine(bpy.types.RenderEngine):
    bl_idname = "DEMO_RAYTRACE_ENGINE"
    bl_label = "Gideon"
    bl_use_shading_nodes = True
    use_highlight_tiles = True

    def __init__(self):
        self.gideon = engine.load_gideon("/home/curtis/Projects/relatively-crazy/build/src/libraytrace.so")
        self.context = engine.create_context(self.gideon)

    def __del__(self):
        engine.destroy_context(self.gideon, self.context)
    
    def update(self, data, scene):
        self.update_stats("", "Compiling render kernel")
        kernel = self.rebuild_kernel(scene)

        #sync this scene with gideon
        self.update_stats("", "Syncing scene data")
        gd_scene = sync.GideonScene(self.gideon, kernel)
        sync.convert_scene(scene, gd_scene)
        engine.context_set_scene(self.gideon, self.context, gd_scene.scene)

        #build the BVH
        self.update_stats("", "Building BVH")
        engine.context_build_bvh(self.gideon, self.context)
    
    #Compiles the render kernel.
    def rebuild_kernel(self, scene):
        #load and compile all programs
        program = engine.create_program(self.gideon, "render")

        for source in scene.gideon.sources:
            print("Loading Object: ", source.name)
            engine.program_load_source(self.gideon, program, source.name)

        kernel = engine.program_compile(self.gideon, program)
        engine.context_set_kernel(self.gideon, self.context, kernel)
        
        engine.destroy_program(self.gideon, program)
        return kernel

    def render(self, scene):
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

        total_tiles = num_x_tiles * num_y_tiles
        completed_tiles = 0

        for ty in range(num_y_tiles):
            end_py = min(start_py + tile_y, y_pixels)
            th = end_py - start_py

            for tx in range(num_x_tiles):
                end_px = min(start_px + tile_x, x_pixels)
                tw = end_px - start_px
                
                r = self.begin_result(start_px, start_py, tw, th)
                
                float4_ty = 4 * ctypes.c_float
                result = (tw * th * float4_ty)()
                engine.render_tile(self.gideon, self.context,
                                   scene.gideon.entry_point.encode('ascii'),
                                   start_px, start_py, tw, th,
                                   result)
                r.layers[0].rect = result
                
                self.end_result(r)

                completed_tiles += 1
                self.update_stats("", str.format("Completed {0}/{1} tiles", completed_tiles, total_tiles))
                start_px += tile_x
                
            start_px = 0
            start_py += tile_y
    
