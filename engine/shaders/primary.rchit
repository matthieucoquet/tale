#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#include "common_types.glsl"
#include "model_map_function"

layout(binding = 0, set = 0) uniform accelerationStructureEXT acceleration_structure; 
layout(location = 0) rayPayloadInEXT vec3 hit_value;
layout(location = 1) rayPayloadEXT float shadow_payload;
layout(binding = 2, set = 0, scalar) buffer Materials { Material m[]; } materials;
layout(binding = 3, set = 0, scalar) buffer Lights { Light l[]; } lights;

vec3 normal(in vec3 position)
{
    const float pixel_radius = get_pixel_radius();
    const float len = length(gl_ObjectRayDirectionEXT);
    vec2 e = vec2(1.0, -1.0) * 0.5773;
    const float eps = pixel_radius * gl_HitTEXT * len;
    return normalize(
        e.xyy * map(position + e.xyy * eps).distance +
        e.yyx * map(position + e.yyx * eps).distance +
        e.yxy * map(position + e.yxy * eps).distance +
        e.xxx * map(position + e.xxx * eps).distance);
}

vec3 lighting(in vec3 global_position, in vec3 global_normal, in Material material)
{   
    shadow_payload = 1.0;    
    traceRayEXT(acceleration_structure,
        gl_RayFlagsSkipClosestHitShaderEXT,
        0xFF,
        2,  // sbtRecordOffset
        0,  // sbtRecordStride
        1,  // missIndex
        global_position,
        0.01,
        global_normal,
        0.5,
        1  // payload (location = 1)
        );
    const float ao = shadow_payload;

    const vec3 view_direction = -gl_WorldRayDirectionEXT;

	vec3 color = vec3(0.0);
    for (int i = 0; i < lights.l.length(); i++)
    {
        const Light light = lights.l[nonuniformEXT(i)];
        vec3 light_direction = light.position - global_position;
        const float light_distance = length(light_direction);
        light_direction = normalize(light_direction);
        
        shadow_payload = 0.0;
        if (dot(global_normal, light_direction) > 0)
        {
            shadow_payload = 1.0;
            traceRayEXT(acceleration_structure,
                        gl_RayFlagsSkipClosestHitShaderEXT,
                        0xFF,
                        1,  // sbtRecordOffset
                        0,  // sbtRecordStride
                        1,  // missIndex
                        global_position,
                        0.01,
                        light_direction,   
                        light_distance,
                        1   // payload (location = 1)
                        );
        }

        const vec3 ambient = 0.03 * light.color;
        const vec3 diffuse = max(dot(global_normal, light_direction), 0.0) * light.color;

        const vec3 halfway = normalize(light_direction + view_direction);
        const vec3 specular = pow(max(dot(global_normal, halfway), 0.0), material.shininess) * light.color;

        color += ao * (material.color.xyz * (ambient + shadow_payload * diffuse) + shadow_payload * material.ks * specular);
     }
     return color;
}

void main()
{
    const vec3 local_position = gl_ObjectRayOriginEXT + gl_ObjectRayDirectionEXT * gl_HitTEXT;
    const vec3 global_position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    const vec3 local_normal = normal(local_position);
    const vec3 global_normal = normalize(vec3(gl_ObjectToWorldEXT * vec4(local_normal, 0.0f)));
    
    const Material material = materials.m[nonuniformEXT(gl_HitKindEXT)];
    
    hit_value = lighting(global_position, global_normal, material);
}
