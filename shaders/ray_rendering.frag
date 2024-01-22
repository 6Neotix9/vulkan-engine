#version 450


const uint XOR = 0x00000001u;
const uint UNION = 0x00000002u;
const uint INTER = 0x00000003u;
const uint SUB = 0x00000004u;
const uint Sphere = 0x00000005u;
const uint Plan = 0x00000006u;


layout(location = 0) in vec2 fragOffset;
layout(location = 0) out vec4 outColor;

struct PointLight {
    vec4 position;  // ignore w
    vec4 color;     // w is intensity
};

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 invView;
    vec4 ambientLightColor;  // w is intensity
    PointLight pointLights[10];
    int numLights;
}
ubo;

layout(set = 0, binding = 1) uniform sampler2D image;

layout(push_constant) uniform Push { vec2 resolution; }
push;

void createRay(in vec2 px, out vec3 ro, out vec3 rd) {
    // convert pixel to NDS
    vec2 pxNDS = (px / push.resolution) * 2.0 - 1.0;
    vec4 pointNDSH = vec4(pxNDS, 0.1f, 1.f);
    vec4 dirEye = inverse(ubo.projection) * pointNDSH;
    dirEye.w = 0.;
    vec3 dirWorld = (ubo.invView * dirEye).xyz;
    ro = ubo.invView[3].xyz;
    rd = normalize(dirWorld);
}

// struct Plan {
//     vec3 pos;
//     vec3 normal;
//     vec3 color;
// };

// struct Sphere {
//     vec3 pos;
//     vec3 color;
//     float size;
// };

struct Object {
    uint type;
    uint objectL;
    uint objectR;
    vec3 pos;
    vec3 color;
    vec3 normal;
    float size;
};

struct Ray{
    vec3 ro;
    vec3 rd;
};

struct Point{
    vec3 pos;
    vec3 color;
    vec3 normal;
    float dist;
};

Point intersect_sphere(in Object s, in  Ray ray) {
    float a = dot(ray.rd, ray.rd);
    float b = 2 * dot(ray.rd, ray.ro - s.pos);
    float k = dot(ray.ro - s.pos, ray.ro - s.pos) - s.size * s.size;
    float dd = b * b - 4 * a * k;
    float d = 1.0 / 0.0;
    if (dd > 0) {
        float t1 = -b + sqrt(b * b - (4 * a * k)) / (2 * a);
        float t2 = -b - sqrt(b * b - (4 * a * k)) / (2 * a);
        if (t1 < t2 && t1 > 0) {
            d = t1;
        } else if (t2 > 0){
            d = t2;
        }
    }
    return Point(ray.ro + ray.rd * d, s.color, normalize(ray.ro + ray.rd * d - s.pos), d);
}


Point intersect_plan(in Object plan, in Ray ray) {
    float d = dot(plan.normal, (plan.pos - ray.ro) / dot(plan.normal, ray.rd));
    return Point(ray.ro + ray.rd * d, plan.color, plan.normal, d); 
}


Point intersect(in Object obj, in Ray ray){
    switch (obj.type)
    {
    case Sphere:
        return intersect_sphere(obj, ray);
        break;
    case Plan:
        return intersect_plan(obj, ray);
        break;
    }
}





void main() {
    // Create objects
    Object plan = Object(Plan, 0, 0, vec3(0, 4, 0), vec3(1, 1, 1), vec3(0, 1, 0), 0);
    Object sphere = Object(Sphere, 0, 0, vec3(0, 0, 0), vec3(0.1, 0.8, 0.1), vec3(0, 0, 0), 2);
    Object objects[2] = {sphere, plan};

    // create variable
    vec3 ro;
    vec3 rd;
    float dist = 1.0 / 0.0;
    vec3 color = vec3(1, 1, 1);
    vec2 coord = gl_FragCoord.xy;
    vec3 sunDir = normalize(vec3(1, 1, 1));

    createRay(coord, ro, rd);

    // intersect
    
    
    Point p;
    Point finalP;
    
    for (int i = 0; i < 2; i++) {
        p = intersect(objects[i], Ray(ro, rd));
        if (p.dist < dist && p.dist > 0 && p.dist != 1.0 / 0.0) {
            
            dist = p.dist;
            finalP = p;
        }
    }

    
    
    if (dist == 1.0 / 0.0 || dist < 0) {
        discard;
    }else{
        color = finalP.color * (max(0, dot(normalize(finalP.normal), normalize(sunDir))));
        outColor = vec4(color, 0.9);
    }
    
}
