#version 450

const uint XOR = 0x00000001u;
const uint UNION = 0x00000002u;
const uint INTER = 0x00000003u;
const uint SUB = 0x00000004u;
const uint Sphere = 0x00000005u;
const uint Plan = 0x00000006u;
const uint Box = 0x00000007u;

const float PI = 3.14159265359;

const uint ANTI_ALIASING_FACTOR = 32;
const uint DOM_LIGHT = 8;

layout(location = 0) in vec2 fragOffset;
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outColor2;

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
    float frameTime;
}
ubo;

layout(set = 0, binding = 1) uniform sampler2D image;
layout(set = 1, binding = 0) uniform sampler2D randomImage;
layout(set = 2, binding = 0) uniform sampler2D previousImage;

layout(push_constant) uniform Push { vec2 resolution; }
push;

struct TransformComponent {
    vec3 translation;
    vec3 scale;
    vec3 rotation;
};

struct Object {
    uint type;
    uint objectL;
    uint objectR;
    mat4 modelMatrix;
    mat3 normalMatrix;
    vec3 color;
    // vec3 size;
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

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value
// below 1.0.
float floatConstruct(uint m) {
    const uint ieeeMantissa = 0x007FFFFFu;  // binary32 mantissa bitmask
    const uint ieeeOne = 0x3F800000u;       // 1.0 in IEEE binary32

    m &= ieeeMantissa;  // Keep only mantissa bits (fractional part)
    m |= ieeeOne;       // Add fractional part to 1.0

    float f = uintBitsToFloat(m);  // Range [1:2]
    return f - 1.0;                // Range [0:1]
}

float random(inout uint seed) {
    uint x = seed;
    x ^= x >> 16;
    x *= 0x21f0aaad;
    x ^= x >> 15;
    x *= 0xd35a2d97;
    x ^= x >> 15;
    seed++;
    return floatConstruct(x);
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

mat4 createModelMatrix(in TransformComponent t) {
    const float c3 = cos(t.rotation.z);
    const float s3 = sin(t.rotation.z);
    const float c2 = cos(t.rotation.x);
    const float s2 = sin(t.rotation.x);
    const float c1 = cos(t.rotation.y);
    const float s1 = sin(t.rotation.y);
    mat4 matrix = mat4(
        vec4(
            t.scale.x * (c1 * c3 + s1 * s2 * s3),
            t.scale.x * (c2 * s3),
            t.scale.x * (c1 * s2 * s3 - c3 * s1),
            0.),
        vec4(
            t.scale.y * (c3 * s1 * s2 - c1 * s3),
            t.scale.y * (c2 * c3),
            t.scale.y * (c1 * c3 * s2 + s1 * s3),
            0.),
        vec4(t.scale.z * (c2 * s1), t.scale.z * (-s2), t.scale.z * (c1 * c2), 0.),
        vec4(t.translation.x, t.translation.y, t.translation.z, 1.0));
    return transpose(matrix);
}

mat3 createNormalMatrix(in TransformComponent t) {
    const float c3 = cos(t.rotation.z);
    const float s3 = sin(t.rotation.z);
    const float c2 = cos(t.rotation.x);
    const float s2 = sin(t.rotation.x);
    const float c1 = cos(t.rotation.y);
    const float s1 = sin(t.rotation.y);
    const vec3 invScale = 1.0f / t.scale;
    mat3 normalMatrix = mat3(
        vec3(
            invScale.x * (c1 * c3 + s1 * s2 * s3),
            invScale.x * (c2 * s3),
            invScale.x * (c1 * s2 * s3 - c3 * s1)),
        vec3(
            invScale.y * (c3 * s1 * s2 - c1 * s3),
            invScale.y * (c2 * c3),
            invScale.y * (c1 * c3 * s2 + s1 * s3)),
        vec3(invScale.z * (c2 * s1), invScale.z * (-s2), invScale.z * (c1 * c2)));
    return transpose(normalMatrix);
}

Object createObject(
    in uint type, in uint objectL, in uint objectR, in TransformComponent t, in vec3 color) {
    mat4 modelMatrix = createModelMatrix(t);
    mat3 normalMatrix = createNormalMatrix(t);
    return Object(type, objectL, objectR, modelMatrix, normalMatrix, color);
}

Point intersect_sphere(in Object s, in Ray ray) {
    vec3 ro = (vec4(ray.ro, 1.0) * inverse(s.modelMatrix)).xyz;
    vec3 rd = (vec4(ray.rd, .0) * inverse(s.modelMatrix)).xyz;
    vec3 pos = {s.modelMatrix[0].w, s.modelMatrix[1].w, s.modelMatrix[2].w};
    float a = dot(rd, rd);
    float b = 2 * dot(rd, ro);
    float k = dot(ro, ro) - 1 * 1;
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
    vec3 hitCoord = ray.ro + ray.rd * d;
    vec3 normal = normalize((ro + rd * d));
    normal = (normal * s.normalMatrix ).xyz;
    return Point(hitCoord, s.color, normal, d);
}

Point intersect_plan(in Object plan, in Ray ray) {
    vec3 ro = (vec4(ray.ro, 1.0) * inverse(plan.modelMatrix)).xyz;
    vec3 rd = (vec4(ray.rd, 0.0) * inverse(plan.modelMatrix)).xyz;

    vec3 pos = {plan.modelMatrix[0].w, plan.modelMatrix[1].w, plan.modelMatrix[2].w};

    float d = dot(vec3(0, -1, 0), vec3(-ro)) / dot(vec3(0, -1, 0), rd);

    vec3 hitCoord = ray.ro + ray.rd * d;
    vec3 normal = vec3(0, -1, 0);
    vec2 hitCoord2d = mod(hitCoord.xz, 4.0);
    vec3 color;
    if ((hitCoord2d.x > 2 && hitCoord2d.y > 2) || (hitCoord2d.x < 2 && hitCoord2d.y < 2)) {
        color = plan.color;
    } else {
        color = vec3(0., 0., 0.);
    }

    return Point(hitCoord, plan.color, normal, d);
}

// // Source : Inigo Quilez sur shaderToy
// Point intersect_box(in Object box, in Ray ray) {
//   float d = 1.0 / 0.0;
//   vec3 m = 1.0 / ray.rd; // can precompute if traversing a set of aligned
//   boxes vec3 n = m * ray.ro;   // can precompute if traversing a set of
//   aligned boxes vec3 k = abs(m) * box.size; vec3 t1 = -n - k; vec3 t2 = -n +
//   k; float tN = max(max(t1.x, t1.y), t1.z); float tF = min(min(t2.x, t2.y),
//   t2.z); if (tN > tF || tF < 0.0) {
//     return Point(vec3(0, 0, 0), vec3(0, 0, 0), vec3(0, 0, 0),
//                  1.0 / 0.0); // no intersection
//   }

//   if (tN < tF && tN > 0) {
//     d = tN;
//   } else if (tF > 0) {
//     d = tF;
//   } else if (tN > 0) {
//     d = tN;
//   }

//   vec3 outNormal = (tN > 0.0) ? step(vec3(tN), t1) : step(t2, vec3(tF));
//   outNormal *= -sign(ray.rd);

//   return Point(ray.ro + ray.rd * d, box.color, outNormal, d);
// }

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
            case Box:
                // p = intersect_box(obj, ray);
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
    Object plan = createObject(
        Plan,
        0,
        0,
        TransformComponent(vec3(0, 2, 0), vec3(1, 1, 1), vec3(0, 0, 0)),
        vec3(1, 1, 1));
    // Object sphere = createObject(
    //     Sphere,
    //     0,
    //     0,
    //     TransformComponent(vec3(0, 0, 0), vec3(2, 40, 2), vec3(0, 0, 0)),
    //     vec3(1, 0, 0));


    //plane made with sphere

    vec3 generalPosition = vec3(0*15, -45, 0);

    Object mainPart = createObject(
        Sphere,
        0,
        0,
        TransformComponent(vec3(0, 0, 0) + generalPosition, vec3(4, 1, 1), vec3(0, 0, 0)),
        vec3(0.2, 0.4, 0.4));
    Object rightWing = createObject(
        Sphere,
        0,
        0,
        TransformComponent(vec3(0, 0, -1.5) + generalPosition, vec3(1, 0.25, 2.2), vec3(0, -0.75, 0)),
        vec3(0.2, 0.4, 0.4));

    Object leftWing = createObject(
        Sphere,
        0,
        0,
        TransformComponent(vec3(0, 0, 1.5) + generalPosition, vec3(1, 0.25, 2.2), vec3(0, 0.75, 0)),
        vec3(0.2, 0.4, 0.4));

    Object horizontalStab = createObject(
        Sphere,
        0,
        0,
        TransformComponent(vec3(3.3, 0, 0) + generalPosition, vec3(0.5, 0.1, 2), vec3(0, 0., 0)),
        vec3(0.2, 0.4, 0.4));
    
    Object verticalStab = createObject(
        Sphere,
        0,
        0,
        TransformComponent(vec3(4, -.6, 0) + generalPosition, vec3(0.35, 1, 0.1), vec3(0, 0., 0.5)),
        vec3(0.2, 0.4, 0.4));

    Object objects[100];
    objects[0] = plan;

    objects[3] = mainPart;
    objects[4] = rightWing;
    objects[5] = leftWing;
    objects[6] = horizontalStab;
    objects[7] = verticalStab;
    uint nbOfObjects = 8;

    // create variable

    vec3 ro;
    vec3 rd;
    float dist;
    vec3 color = vec3(0, 0, 0);
    vec2 coord = gl_FragCoord.xy;
    vec2 randomSeed = texture(randomImage, coord / vec2(3840, 2160)).rg;
    uint rseed = uint((randomSeed.r + randomSeed.g) * 10 * ubo.frameTime * 10);
    vec3 sunDir = normalize(vec3(0.5, -1, -0.5));
    vec3 sunColor = vec3(1, 1, 1);
    vec3 ImageColor = {0, 0, 0};
    for (int i = 0; i < ANTI_ALIASING_FACTOR; i++) {
        vec2 randomRayDir = vec2(random(rseed), random(rseed));

        Ray ray = createRay(coord + randomRayDir - vec2(0.5, 0.5));

        // intersect

        Point finalP = intersect(ray, objects, nbOfObjects, dist);
        if (!(dist == 1.0 / 0.0 || dist < 0)) {
            color = finalP.color * (max(0, dot(normalize(finalP.normal), normalize(sunDir)))) *
                    sunColor;
            finalP.pos = finalP.pos + 0.001 * finalP.normal;
            if (checkIfShadow(sunDir, finalP, objects, nbOfObjects)) {
                color = color * 0.;
            }
            if (DOM_LIGHT > 0) {
                vec3 lighDomeColor = vec3(0, 0, 0);
                for (int j = 0; j < DOM_LIGHT; j++) {
                    vec2 randomNum = vec2(random(rseed), random(rseed));
                    float r1 = randomNum.x;
                    float r2 = randomNum.y;
                    float theta = r1 * PI;
                    float phi = r2 * 2 * PI;

                    vec3 d = vec3(sin(phi) * sin(theta), cos(phi) * sin(theta), cos(theta));
                    if (dot(d, finalP.normal) > 0) {
                        vec3 temp =
                            finalP.color * (max(0, dot(normalize(finalP.normal), d))) * sunColor;
                        if (checkIfShadow(d, finalP, objects, nbOfObjects)) {
                            temp = color * 0.;
                        }
                        lighDomeColor += temp;
                    }
                }
                color = color * 0.5 + (lighDomeColor / float(DOM_LIGHT)) * 0.5;
            }

            ImageColor += color;
        } else {
            ImageColor += vec3(0.502, 0.869, 1);
        }
    }

    vec3 finalColor = ImageColor / ANTI_ALIASING_FACTOR;
    finalColor = finalColor * 0.5 + texture(previousImage, coord / vec2(3840, 2160)).rgb * 0.5;

    outColor2 = vec4(finalColor, 1);
    outColor = vec4(finalColor, 1);
    // outColor = vec4(random(rseed), random(rseed), random(rseed), 1);
}
