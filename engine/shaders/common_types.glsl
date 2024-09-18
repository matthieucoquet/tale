struct Pose
{
    vec4 rotation;
    vec3 position;
};

struct Fov
{
    float left;
    float right;
    float up;
    float down;
};

struct Material
{
    vec4 color;
    float ks;
    float shininess;
    float f0;
};

struct Light
{
    vec3 position;
    vec3 color;
};

struct Hit
{
    float distance;
    uint material_id;
};

layout(push_constant, scalar) uniform Eye {
    Pose pose;
    Fov fov;
} eye;

float get_pixel_radius()
{
    vec2 pixel_area = vec2(tan(eye.fov.right) - tan(eye.fov.left),
        tan(eye.fov.up) - tan(eye.fov.down));
    pixel_area = pixel_area / vec2(gl_LaunchSizeEXT.xy);
    return min(pixel_area.x, pixel_area.y) * 0.5;
}