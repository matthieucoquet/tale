#define ADVANCE_RATIO 0.99
#define BLUE_ID 1

float sd_box(in vec3 position, in vec3 half_sides)
{
  vec3 q = abs(position) - half_sides;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

Hit map(in vec3 position)
{
    float scale = 100.0;
    position = position * scale;
    position.z += 0.5;
    position.xy -= 0.5;
    position.xy = position.xy - clamp(round(position.xy), -50, 49);
    float distance = sd_box(position, vec3(0.45, 0.45, 0.45)) - 0.05;
    return Hit(distance / scale, BLUE_ID);
}
