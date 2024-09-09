struct Ray
{
    vec3 origin;
    vec3 direction;
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
