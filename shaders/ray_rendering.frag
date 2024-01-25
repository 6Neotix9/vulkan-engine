#version 450

const uint XOR = 0x00000001u;
const uint UNION = 0x00000002u;
const uint INTER = 0x00000003u;
const uint SUB = 0x00000004u;
const uint Sphere = 0x00000005u;
const uint Plan = 0x00000006u;

const float PI = 3.14159265359;

const uint ANTI_ALIASING_FACTOR = 256;

layout(location = 0) in vec2 fragOffset;
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outColor2;
layout(location = 2) out vec4 outColor3;

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
layout(set = 1, binding = 0) uniform sampler2D randomImage;

layout(push_constant) uniform Push { vec2 resolution; }
push;

struct Object {
    uint type;
    uint objectL;
    uint objectR;
    vec3 pos;
    vec3 color;
    vec3 normal;
    float size;
};

struct Ray {
    vec3 ro;
    vec3 rd;
};

struct Point {
    vec3 pos;
    vec3 color;
    vec3 normal;
    float dist;
};

float random(vec2 seed, inout int iteration) {
    float value = sin(dot(seed, vec2(12.9898, 78.233)) * 43758.5453 + float(iteration));
    iteration++;
    return fract(value);
}

Ray createRay(in vec2 px) {
    // convert pixel to NDS
    vec2 pxNDS = (px / (push.resolution)) * 2. - 1.;

    // choose an arbitrary point in the viewing volume
    // z = -1 equals a point on the near plane, i.e. the screen
    vec3 pointNDS = vec3(pxNDS, 0.1);

    // as this is in homogenous space, add the last homogenous coordinate
    vec4 pointNDSH = vec4(pointNDS, 1.0);
    // transform by inverse projection to get the point in view space
    vec4 dirEye = inverse(ubo.projection) * pointNDSH;

    // since the camera is at the origin in view space by definition,
    // the current point is already the correct direction
    // (dir(0,P) = P - 0 = P as a direction, an infinite point,
    // the homogenous component becomes 0 the scaling done by the
    // w-division is not of interest, as the direction in xyz will
    // stay the same and we can just normalize it later
    dirEye.w = 0.;
    vec3 ro = ubo.invView[3].xyz;
    // compute world ray direction by multiplying the inverse view matrix
    vec3 rd = (ubo.invView * dirEye).xyz;
    return Ray(ro, rd);
}

Point intersect_sphere(in Object s, in Ray ray) {
    float a = dot(ray.rd, ray.rd);

    float b = 2 * dot(ray.rd, ray.ro - s.pos);
    float k = dot(ray.ro - s.pos, ray.ro - s.pos) - s.size * s.size;
    float dd = b * b - 4 * a * k;
    float d = 1.0 / 0.0;
    if (dd > 0) {
        float t1 = (-b + sqrt(b * b - (4 * a * k))) / (2 * a);
        float t2 = (-b - sqrt(b * b - (4 * a * k))) / (2 * a);

        if (t1 < t2 && t1 > 0) {
            d = t1;
        } else if (t2 > 0) {
            d = t2;
        } else if (t1 > 0) {
            d = t1;
        }
    }
    return Point(ray.ro + ray.rd * d, s.color, normalize(ray.ro + ray.rd * d - s.pos), d);
}

Point intersect_plan(in Object plan, in Ray ray) {
    float d = dot(plan.normal, (plan.pos - ray.ro) / dot(plan.normal, ray.rd));

    vec3 hitCoord = ray.ro + ray.rd * d;
    vec2 hitCoord2d = mod(hitCoord.xz, 4.0);
    vec3 color;
    if ((hitCoord2d.x > 2 && hitCoord2d.y > 2) || (hitCoord2d.x < 2 && hitCoord2d.y < 2)) {
        color = plan.color;
    } else {
        color = vec3(0., 0., 0.);
    }
    return Point(hitCoord, color, plan.normal, d);
}

Point intersect(in Ray ray, in Object[100] objects, in uint numberOfobjects, out float d) {
    Point p;
    Point finalP;
    Object obj;
    float dist = 1.0 / 0.0;

    for (int i = 0; i < numberOfobjects; i++) {
        obj = objects[i];
        switch (obj.type) {
            case Sphere:
                p = intersect_sphere(obj, ray);
                break;
            case Plan:
                p = intersect_plan(obj, ray);
                break;
        }
        if (p.dist < dist && p.dist > 0 && p.dist != 1.0 / 0.0) {
            dist = p.dist;
            finalP = p;
        }
    }
    d = dist;
    return finalP;
}

bool checkIfShadow(
    in vec3 sunDir, in Point hitPoint, in Object[100] objects, in uint numberOfobjects) {
    float d;
    Ray r = Ray(hitPoint.pos, sunDir);
    Point p = intersect(r, objects, numberOfobjects, d);
    if (d < 0 || d == 1.0 / 0.0)
        return false;
    else
        return true;
}

void main() {
    // Create objects
    Object plan = Object(Plan, 0, 0, vec3(0, 4, 0), vec3(1, 1, 1), vec3(0, -1, 0), 0);
    Object sphere = Object(Sphere, 0, 0, vec3(0, -2, 0), vec3(0.1, 0.2, 0.8), vec3(0, 0, 0), 2);
    Object spher2 = Object(Sphere, 0, 0, vec3(-0, -40, -0), vec3(1, 0.2, 0.8), vec3(0, 0, 0), 13);
    Object objects[100];
    objects[0] = plan;
    objects[1] = sphere;
    objects[2] = spher2;
    uint nbOfObjects = 3;

    // create variable
    int randomIteration = 0;
    vec3 ro;
    vec3 rd;
    float dist;
    vec3 color = vec3(1, 1, 1);
    vec2 coord = gl_FragCoord.xy;
    vec2 randomSeed = texture(randomImage, coord / vec2(3840, 2160)).rg;
    vec3 sunDir = normalize(vec3(1, -1, 1));
    vec3 ImageColor = {0,0,0};
    for (int i = 0; i < ANTI_ALIASING_FACTOR; i++) {
        Ray ray = createRay(vec2(coord.x + random(randomSeed, randomIteration), coord.y + random(randomSeed, randomIteration)));

        // intersect

        Point finalP = intersect(ray, objects, nbOfObjects, dist);
        if (!(dist == 1.0 / 0.0 || dist < 0)) {
            color = finalP.color * (max(0, dot(normalize(finalP.normal), normalize(sunDir))));
            finalP.pos = finalP.pos + 0.001 * finalP.normal;
            if (checkIfShadow(sunDir, finalP, objects, nbOfObjects)) {
                color = color * 0.;
            }
            ImageColor += color;
        }
    }
    vec3 finalColor = ImageColor / ANTI_ALIASING_FACTOR ;
    outColor = vec4(vec3(finalColor), 1);
    
}
