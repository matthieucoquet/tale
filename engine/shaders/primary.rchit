#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#include "common_types.glsl"
#include "model_map_function"
#include "raymarch.glsl"

layout(location = 0) rayPayloadInEXT vec3 hit_value;

void main()
{
    vec3 local_position = gl_ObjectRayOriginEXT + gl_ObjectRayDirectionEXT * gl_HitTEXT;
    vec3 local_normal = normal(local_position);
    vec3 global_normal = normalize(vec3(gl_ObjectToWorldEXT * vec4(local_normal, 0.0f)));
    
    vec3 color = vec3(0.2, 0.2, 0.6);
    vec3 light_dir = normalize(vec3(-0.5, 0.5, 0.2));  
    vec3 light_color = vec3(1.0);          
    vec3 diffuse = max(dot(global_normal, light_dir), 0.0) * light_color;
    vec3 ambient = 0.1 * light_color; 

    hit_value = color * (diffuse + ambient);
}
