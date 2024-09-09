#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#include "common_types.glsl"
#include "model_map_function"
#include "raymarch.glsl"
#include "lighting.glsl"

layout(location = 0) rayPayloadInEXT vec3 hit_value;
layout(binding = 2, set = 0, scalar) buffer Materials { Material m[]; } materials;

void main()
{
    const vec3 local_position = gl_ObjectRayOriginEXT + gl_ObjectRayDirectionEXT * gl_HitTEXT;
    const vec3 global_position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    const vec3 local_normal = normal(local_position);
    const vec3 global_normal = normalize(vec3(gl_ObjectToWorldEXT * vec4(local_normal, 0.0f)));
    
    const Material material = materials.m[nonuniformEXT(gl_HitKindEXT)];
    
    hit_value = lighting(global_position, global_normal, material);
}
