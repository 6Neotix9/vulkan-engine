#version 450

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

struct Plan {
    vec3 pos;
    vec3 normal;
    vec3 color;
};

struct Sphere {
    vec3 pos;
    vec3 color;
    float size;
};

float intersect_plan(in Plan plan, in vec3 ro, in vec3 rd) {
    return dot(plan.normal, (ro - plan.pos) / dot(plan.normal, rd));
}

float intersect_sphere(in Sphere s, in vec3 ro, in vec3 rd) {
    float a = dot(rd, rd);
    float b = 2 * dot(rd, s.pos - ro);
    float k = dot(s.pos - ro, s.pos - ro) - s.size * s.size;
    float dd = b * b - 4 * a * k;
    if (dd > 0) {
        float t1 = -b + sqrt(b * b - (4 * a * k)) / (2 * a);
        float t2 = -b - sqrt(b * b - (4 * a * k)) / (2 * a);
        if (t1 < t2) {
            return t2;
        } else {
            return t1;
        }
    }
    return - 1.0 / 0.0;
}

void main() {
    // Create objects
    Plan plan1 = Plan(vec3(0, 1, 0), vec3(0, -1, 0), vec3(0.1, 0.1, 0.5));
    Sphere s = Sphere(vec3(0, 1, 0), vec3(0, 1, 1), 4 );

    // create variable
    vec3 ro;
    vec3 rd;
    float dist = - 1.0 / 0.0;
    vec3 color = vec3(1, 1, 1);
    vec2 coord = gl_FragCoord.xy;

   
    createRay(coord, ro, rd);

    float test = intersect_plan(plan1, ro, rd);
    float test2 = intersect_sphere(s, ro, rd);
    // if(test2 <= 0 && test2 > dist){
    //     dist = test2;
    //     color = s.color;
    // }
    if(test < 0 ){
        dist = test;
        color = plan1.color;
    }

    outColor = vec4(test,test,test, 0.8);
}
