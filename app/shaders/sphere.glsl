#define ADVANCE_RATIO 1.0
#define RED_ID 0

float sd_sphere(in vec3 position, in float radius)
{
    return length(position) - radius;
}

Hit map(in vec3 position)
{
    float d = sd_sphere(position, 0.5);
    //d -= 0.01 * sin(30 * (position.y * position.x));
    return Hit(d, RED_ID);
}
