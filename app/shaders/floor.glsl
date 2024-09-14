#define ADVANCE_RATIO 1.0
#define WHITE_ID 2
#define GRAY_ID 3

float sd_box(in vec3 position, in vec3 half_sides)
{
  vec3 q = abs(position) - half_sides;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

Hit map(in vec3 position)
{
    position.z += 0.5;
    position.xy -= 0.5;
    ivec2 id = ivec2(clamp(round(position.xy), -50, 49));
    position.xy = position.xy - id;
    float distance = sd_box(position, vec3(0.46, 0.46, 0.46)) - 0.04;
    uint color_id = (id.x + id.y) % 2 == 0 ? WHITE_ID : GRAY_ID;
    return Hit(distance, color_id);
}
