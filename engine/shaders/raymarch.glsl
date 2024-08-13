float raymarch(in Ray ray)
{
    float t = gl_RayTminEXT;
    for (int i = 0; i < 128 && t < gl_RayTmaxEXT; i++)
    {
        float distance = map(ray.origin + t * ray.direction);
        if(distance < 0.0001) {
            return t;
        }
        t += ADVANCE_RATIO * distance;
    }
    return -1.0;
}