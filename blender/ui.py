import bpy

class CustomButtonsPanel():
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "render"

    @classmethod
    def poll(cls, context):
        rd = context.scene.render
        return rd.engine == 'DEMO_RAYTRACE_ENGINE'


class CustomLampPanel(CustomButtonsPanel, bpy.types.Panel):
    bl_label = "Lamp"
    bl_context = "data"

    @classmethod
    def poll(cls, context):
        return context.lamp and CustomButtonsPanel.poll(context)

    def draw(self, context):
        layout = self.layout
        lamp = context.lamp
        
        layout.prop(lamp, "energy", text="Energy")
        layout.prop(lamp, "color", text="Color")
        layout.prop(lamp, "shadow_soft_size", text="Size")
        

def get_panels():
    return (
        bpy.types.RENDER_PT_render,
        bpy.types.RENDER_PT_output,
        bpy.types.RENDER_PT_encoding,
        bpy.types.RENDER_PT_dimensions,
        bpy.types.RENDER_PT_stamp,
        bpy.types.SCENE_PT_scene,
        bpy.types.SCENE_PT_audio,
        bpy.types.SCENE_PT_unit,
        bpy.types.SCENE_PT_keying_sets,
        bpy.types.SCENE_PT_keying_set_paths,
        bpy.types.SCENE_PT_physics,
        bpy.types.WORLD_PT_context_world,
        bpy.types.DATA_PT_context_mesh,
        bpy.types.DATA_PT_context_camera,
        bpy.types.DATA_PT_context_lamp,
        bpy.types.DATA_PT_context_speaker,
        bpy.types.DATA_PT_texture_space,
        bpy.types.DATA_PT_curve_texture_space,
        bpy.types.DATA_PT_mball_texture_space,
        bpy.types.DATA_PT_vertex_groups,
        bpy.types.DATA_PT_shape_keys,
        bpy.types.DATA_PT_uv_texture,
        bpy.types.DATA_PT_vertex_colors,
        bpy.types.DATA_PT_camera,
        bpy.types.DATA_PT_camera_display,
        bpy.types.DATA_PT_lens,
        bpy.types.DATA_PT_speaker,
        bpy.types.DATA_PT_distance,
        bpy.types.DATA_PT_cone,
        bpy.types.DATA_PT_customdata,
        bpy.types.DATA_PT_custom_props_mesh,
        bpy.types.DATA_PT_custom_props_camera,
        bpy.types.DATA_PT_custom_props_lamp,
        bpy.types.DATA_PT_custom_props_speaker,
        bpy.types.TEXTURE_PT_clouds,
        bpy.types.TEXTURE_PT_wood,
        bpy.types.TEXTURE_PT_marble,
        bpy.types.TEXTURE_PT_magic,
        bpy.types.TEXTURE_PT_blend,
        bpy.types.TEXTURE_PT_stucci,
        bpy.types.TEXTURE_PT_image,
        bpy.types.TEXTURE_PT_image_sampling,
        bpy.types.TEXTURE_PT_image_mapping,
        bpy.types.TEXTURE_PT_musgrave,
        bpy.types.TEXTURE_PT_voronoi,
        bpy.types.TEXTURE_PT_distortednoise,
        bpy.types.TEXTURE_PT_voxeldata,
        bpy.types.TEXTURE_PT_pointdensity,
        bpy.types.TEXTURE_PT_pointdensity_turbulence,
        bpy.types.TEXTURE_PT_mapping,
        bpy.types.TEXTURE_PT_influence,
        bpy.types.PARTICLE_PT_context_particles,
        bpy.types.PARTICLE_PT_emission,
        bpy.types.PARTICLE_PT_hair_dynamics,
        bpy.types.PARTICLE_PT_cache,
        bpy.types.PARTICLE_PT_velocity,
        bpy.types.PARTICLE_PT_rotation,
        bpy.types.PARTICLE_PT_physics,
        bpy.types.PARTICLE_PT_boidbrain,
        bpy.types.PARTICLE_PT_render,
        bpy.types.PARTICLE_PT_draw,
        bpy.types.PARTICLE_PT_children,
        bpy.types.PARTICLE_PT_field_weights,
        bpy.types.PARTICLE_PT_force_fields,
        bpy.types.PARTICLE_PT_vertexgroups,
        bpy.types.PARTICLE_PT_custom_props,
        )

def register():
    for panel in get_panels():
        panel.COMPAT_ENGINES.add('DEMO_RAYTRACE_ENGINE')
    
def unregister():
    for panel in get_panels():
        panel.COMPAT_ENGINES.remove('DEMO_RAYTRACE_ENGINE')
