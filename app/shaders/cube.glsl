#define ADVANCE_RATIO 1.0
#define BLUE_ID 1

float sd_box(in vec3 position, in vec3 half_sides)
{
  vec3 q = abs(position) - half_sides;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

Hit map(in vec3 position)
{
    float distance = sd_box(position, vec3(0.45));
    return Hit(distance - 0.05, BLUE_ID);
}
