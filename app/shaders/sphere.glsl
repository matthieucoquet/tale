#define ADVANCE_RATIO 0.99

float sd_sphere(in vec3 position, in float radius)
{
    return length(position) - radius;
}

float map(in vec3 position)
{
    float d = sd_sphere(position, 0.49);
    //d -= 0.01 * sin(10 * (2*scene_global.time + position.x + position.y));
    return d;
}
