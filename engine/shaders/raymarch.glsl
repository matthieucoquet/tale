Hit raymarch(in Ray ray)
{
    float len = length(ray.direction);
    float t = gl_RayTminEXT;
    for (int i = 0; i < 256 && t < gl_RayTmaxEXT; i++)
    {
        Hit hit = map(ray.origin + t * ray.direction);
        if(hit.distance < 0.0001) {
            return Hit(t, hit.material_id);
        }
        t += ADVANCE_RATIO * hit.distance / len;
    }
    return Hit(-1.0, 0);
}

vec3 normal(in vec3 position)
{
    float len = length(gl_ObjectRayDirectionEXT);
    vec2 e = vec2(1.0, -1.0) * 0.5773;
    const float eps = 0.0025 * len;
    return normalize(
        e.xyy * map(position + e.xyy * eps).distance +
        e.yyx * map(position + e.yyx * eps).distance +
        e.yxy * map(position + e.yxy * eps).distance +
        e.xxx * map(position + e.xxx * eps).distance);
}