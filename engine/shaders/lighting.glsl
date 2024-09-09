
layout(binding = 3, set = 0, scalar) buffer Lights { Light l[]; } lights;

vec3 lighting(in vec3 global_position, in vec3 global_normal, in Material material)
{   
    const vec3 view_direction = -gl_WorldRayDirectionEXT;

	vec3 color = vec3(0.0);
    for (int i = 0; i < lights.l.length(); i++)
    {
        const Light light = lights.l[nonuniformEXT(i)];
        const vec3 light_direction = normalize(light.position - global_position);
        
        const vec3 ambient = 0.05 * light.color;
        const vec3 diffuse = max(dot(global_normal, light_direction), 0.0) * light.color;

        const vec3 halfway = normalize(light_direction + view_direction);
        const vec3 specular = pow(max(dot(global_normal, halfway), 0.0), material.shininess) * light.color;

         color += material.color.xyz * (diffuse + ambient)  + material.ks * specular;
     }
     return color;
}
