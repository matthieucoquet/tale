#define ADVANCE_RATIO 1.0

float sd_sphere(in vec3 position, in float radius)
{
    return length(position) - radius;
}

float map(in vec3 position)
{
    float d = sd_sphere(position, 0.5);
    //d -= 0.01 * sin(10 * (2*scene_global.time + position.x + position.y));
    return d;
}
