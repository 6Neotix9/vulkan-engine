#version 450

////////////////////////////////////////////////////////
//// Constant Variable /////////////////////////////////
////////////////////////////////////////////////////////

const uint XOR = 0x00000001u;
const uint UNION = 0x00000002u;
const uint INTER = 0x00000003u;
const uint SUB = 0x00000004u;
const uint Sphere = 0x00000005u;
const uint Plan = 0x00000006u;
const uint Box = 0x00000007u;
const uint Cylinder = 0x00000008u;

const float PI = 3.14159265359;

const uint ANTI_ALIASING_FACTOR = 8;
const uint DOM_LIGHT = 0;
const uint MAX_BOUNCE = 2;

////////////////////////////////////////////////////////
//// Struct ////////////////////////////////////////////
////////////////////////////////////////////////////////

struct PointLight {
    vec4 position;  // ignore w
    vec4 color;     // w is intensity
};

struct TransformComponent {
    vec3 translation;
    vec3 scale;
    vec3 rotation;
};

struct Material {
    vec3 albedo;
    float metallic;
    float roughness;
};

struct Object {
    uint type;
    uint objectL;
    uint objectR;
    mat4 modelMatrix;
    mat3 normalMatrix;
    Material material;
    // vec3 size;
};

struct Ray {
    vec3 ro;
    vec3 rd;
};

struct HitPoint {
    vec3 pos;
    Material material;
    vec3 normal;
    float dist;
};

////////////////////////////////////////////////////////
//// Ressource Layout //////////////////////////////////
////////////////////////////////////////////////////////

layout(location = 0) in vec2 fragOffset;
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outColor2;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 invView;
    vec4 ambientLightColor;  // w is intensity
    PointLight HitPointLights[10];
    int numLights;
    float frameTime;
    int frameCount;
}
ubo;

layout(set = 0, binding = 1) uniform sampler2D image;
layout(set = 1, binding = 0) uniform sampler2D randomImage;
layout(set = 1, binding = 1) uniform sampler2D BRDFLUT;
layout(set = 2, binding = 0) uniform sampler2D previousImage;

layout(push_constant) uniform Push { vec2 resolution; }
push;

////////////////////////////////////////////////////////
//// Utils Function ////////////////////////////////////
////////////////////////////////////////////////////////

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

float distanceToZBufferValue(float dist, float near, float far) {
    // Convert distance from world space to normalized device coordinates (NDC)
    dist = min(99, dist);
    float z_n = ((2 * 0.1 * 100 / dist) - 100 - 0.1) / (100 - 0.1);

    // Convert from NDC to z-buffer range (0 to 1)
    float z_buffer_value = (z_n + 1.0) / 2.0;

    return 1 - z_buffer_value;
}

vec3 Point(Ray ray, float t) { return ray.ro + t * ray.rd; }

////////////////////////////////////////////////////////
//// Creation Function /////////////////////////////////
////////////////////////////////////////////////////////

Ray createRay(in vec2 px) {
    // convert pixel to NDS
    vec2 pxNDS = (px / (push.resolution)) * 2. - 1.;

    // choose an arbitrary HitPoint in the viewing volume
    // z = -1 equals a HitPoint on the near plane, i.e. the screen
    vec3 HitPointNDS = vec3(pxNDS, 0.1);

    // as this is in homogenous space, add the last homogenous coordinate
    vec4 HitPointNDSH = vec4(HitPointNDS, 1.0);
    // transform by inverse projection to get the HitPoint in view space
    vec4 dirEye = inverse(ubo.projection) * HitPointNDSH;

    // since the camera is at the origin in view space by definition,
    // the current HitPoint is already the correct direction
    // (dir(0,P) = P - 0 = P as a direction, an infinite HitPoint,
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
            t.scale.x * (c1 * c3 + s1 * s2 * s3), t.scale.x * (c2 * s3),
            t.scale.x * (c1 * s2 * s3 - c3 * s1), 0.),
        vec4(
            t.scale.y * (c3 * s1 * s2 - c1 * s3), t.scale.y * (c2 * c3),
            t.scale.y * (c1 * c3 * s2 + s1 * s3), 0.),
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
            invScale.x * (c1 * c3 + s1 * s2 * s3), invScale.x * (c2 * s3),
            invScale.x * (c1 * s2 * s3 - c3 * s1)),
        vec3(
            invScale.y * (c3 * s1 * s2 - c1 * s3), invScale.y * (c2 * c3),
            invScale.y * (c1 * c3 * s2 + s1 * s3)),
        vec3(invScale.z * (c2 * s1), invScale.z * (-s2), invScale.z * (c1 * c2)));
    return transpose(normalMatrix);
}

Object createObject(
    in uint type, in uint objectL, in uint objectR, in TransformComponent t, in Material material) {
    mat4 modelMatrix = createModelMatrix(t);
    mat3 normalMatrix = createNormalMatrix(t);
    return Object(type, objectL, objectR, modelMatrix, normalMatrix, material);
}

////////////////////////////////////////////////////////
//// Intersection Function /////////////////////////////
////////////////////////////////////////////////////////

HitPoint intersect_sphere(in Object s, in Ray ray) {
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
    normal = normalize((normal * s.normalMatrix).xyz);
    return HitPoint(hitCoord, s.material, normal, d);
}

HitPoint intersect_plan(in Object plan, in Ray ray) {
    vec3 ro = (vec4(ray.ro, 1.0) * inverse(plan.modelMatrix)).xyz;
    vec3 rd = (vec4(ray.rd, 0.0) * inverse(plan.modelMatrix)).xyz;

    vec3 pos = {plan.modelMatrix[0].w, plan.modelMatrix[1].w, plan.modelMatrix[2].w};

    float d = dot(vec3(0, -1, 0), vec3(-ro)) / dot(vec3(0, -1, 0), rd);

    vec3 hitCoord = ray.ro + ray.rd * d;
    vec3 normal = vec3(0, -1, 0);
    vec2 hitCoord2d = mod(hitCoord.xz, 4.0);
    Material color;
    if ((hitCoord2d.x > 2 && hitCoord2d.y > 2) || (hitCoord2d.x < 2 && hitCoord2d.y < 2)) {
        color = plan.material;
    } else {
        color = plan.material;
    }

    return HitPoint(hitCoord, plan.material, normalize(normal), d);
}

// Source : Inigo Quilez - https://iquilezles.org/articles/intersectors/
HitPoint intersect_box(in Object box, in Ray ray) {
    vec3 ro = (vec4(ray.ro, 1.0) * inverse(box.modelMatrix)).xyz;
    vec3 rd = (vec4(ray.rd, 0.0) * inverse(box.modelMatrix)).xyz;
    float d = 1.0 / 0.0;
    vec3 m = 1.0 / rd;  // can precompute if traversing a set of aligned boxes
    vec3 n = m * ro;    // can precompute if traversing a set of aligned boxes
    vec3 k = abs(m);
    vec3 t1 = -n - k;
    vec3 t2 = -n + k;
    float tN = max(max(t1.x, t1.y), t1.z);
    float tF = min(min(t2.x, t2.y), t2.z);
    Material mat;
    if (tN > tF || tF < 0.0) {
        return HitPoint(vec3(0, 0, 0), mat, vec3(0, 0, 0),
                        1.0 / 0.0);  // no intersection
    }

    if (tN < tF && tN > 0) {
        d = tN;
    } else if (tF > 0) {
        d = tF;
    } else if (tN > 0) {
        d = tN;
    }

    vec3 outNormal = (tN > 0.0) ? step(vec3(tN), t1) : step(t2, vec3(tF));
    outNormal *= -sign(rd);

    vec3 hitCoord = ray.ro + ray.rd * d;
    outNormal = normalize((outNormal * box.normalMatrix).xyz);

    return HitPoint(hitCoord, box.material, outNormal, d);
}

// Source : cdyk - https://www.shadertoy.com/view/ttXSzl
HitPoint interest_cylinder2(in Object cyl, in Ray ray) {
    vec3 ro = (vec4(ray.ro, 1.0) * inverse(cyl.modelMatrix)).xyz;
    vec3 rd = (vec4(ray.rd, 0.0) * inverse(cyl.modelMatrix)).xyz;
    vec3 pos = vec3(0, 0, 0);
    Material mat;
    HitPoint res = HitPoint(vec3(0, 0, 0), cyl.material, vec3(0, 0, 0), 1.0 / 0.0);
    float radius = 0.2;
    float height = 1.0;

    vec2 g = ro.xy - pos.xy;

    //<g + t*v.d, g + t*v.d> = r^2
    //<g,g> - r^2 + 2*t*<g,v.d> + t^2 <v.d,v.d> = 0

    float a = dot(rd.xy, rd.xy);
    float b = 2.0 * dot(g.xy, rd.xy);
    float c = dot(g.xy, g.xy) - radius;

    float disc = b * b - 4.0 * a * c;
    if (disc < 0.0) return res;

    float d = sqrt(disc);
    float t0 = (-b - d) / (2.0 * a);
    float t1 = (-b + d) / (2.0 * a);

    float rcp = 1.0 / rd.z;
    float aa = rcp * (pos - ro).z;
    float ta = aa - abs(rcp) * height;
    float tb = aa + abs(rcp) * height;

    // cylinder is between near and far cap
    if (ta <= t0 && t0 <= tb) {
        vec2 w = g + t0 * rd.xy;
        res.normal = normalize((normalize(vec3(w, 0)) * cyl.normalMatrix).xyz);
        res.pos = ray.ro + t0 * ray.rd;
        res.dist = t0;
        return res;
    }

    // near cap is inside infinite cylinder
    if (t0 < ta && ta < t1) {
        res.normal = normalize((vec3(0, 0, -sign(rd.z)) * cyl.normalMatrix).xyz);
        res.pos = ray.ro + ta * ray.rd;
        res.dist = ta;
        return res;
    }

    return res;
}

////////////////////////////////////////////////////////
//// Launch Ray Function ///////////////////////////////
////////////////////////////////////////////////////////

HitPoint intersect(in Ray ray, in Object[100] objects, in uint numberOfobjects, out float d) {
    HitPoint p;
    HitPoint finalP;
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
                p = intersect_box(obj, ray);
                break;
            case Cylinder:
                p = interest_cylinder2(obj, ray);
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
    in vec3 sunDir,
    in HitPoint hitHitPoint,
    in Object[100] objects,
    in uint numberOfobjects,
    in float distanceMin) {
    float d;
    Ray r = Ray(hitHitPoint.pos, sunDir);
    HitPoint p = intersect(r, objects, numberOfobjects, d);
    if (d < 0 || d == 1.0 / 0.0 || d >= distanceMin)
        return false;
    else
        return true;
}

////////////////////////////////////////////////////////
//// PBR Function (from learnopengl) ///////////////////
////////////////////////////////////////////////////////

float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a2 = roughness * roughness * roughness * roughness;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    return geometrySchlickGGX(max(dot(N, L), 0.0), roughness) *
           geometrySchlickGGX(max(dot(N, V), 0.0), roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) { return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); }

// Specular BRDF composition --------------------------------------------

vec3 PBR(
    vec3 L,
    vec3 V,
    vec3 N,
    vec3 lightColor,
    float metallic,
    float roughness,
    vec3 albedo,
    float attenuation) {
    vec3 H = normalize(V + L);
    vec3 F0 = mix(vec3(0.04), pow(albedo, vec3(2.2)), metallic);
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    vec3 kD = vec3(1.0) - F;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular = numerator / max(denominator, 0.001);

    float NdotL = max(dot(N, L), 0.0);
    vec3 color =
        lightColor * attenuation * (kD * pow(albedo, vec3(2.2)) / PI + specular) * (NdotL);

    return color;
}

////////////////////////////////////////////////////////
//// Light /////////////////////////////////////////////
////////////////////////////////////////////////////////

vec3 sunLight(
    Ray r, vec3 sunDir, vec3 sunColor, HitPoint finalP, Object[100] objects, uint nbOfObjects) {
    if (checkIfShadow(sunDir, finalP, objects, nbOfObjects, 1 / 0.0)) {
        return vec3(0);
    } else {
        return PBR(
            sunDir, -r.rd, finalP.normal, sunColor, finalP.material.metallic,
            finalP.material.roughness, finalP.material.albedo, 1.0);
    }
}

vec3 pointLight(Ray r, PointLight pl, HitPoint finalP, Object[100] objects, uint nbOfObjects) {
    vec3 lightdir = normalize(pl.position.xyz - finalP.pos);
    float d = length(pl.position.xyz - finalP.pos);
    if (checkIfShadow(lightdir, finalP, objects, nbOfObjects, d)) {
        return vec3(0);
    } else {
        float attenuation = 1.0 / (d * d);
        return PBR(
            lightdir, -r.rd, finalP.normal, pl.color.xyz, finalP.material.metallic,
            finalP.material.roughness, finalP.material.albedo, attenuation);
    }
}

vec3 skyLight() {
    // color = vec3(finalP.normal.x, -finalP.normal.y, finalP.normal.z)/10;

    // if (checkIfShadow(sunDir, finalP, objects, nbOfObjects)) {
    //     color = color * 0.;
    // }

    // if (DOM_LIGHT > 0) {
    //     int raylaunched = 0;
    //     vec3 lighDomeColor = vec3(0, 0, 0);
    //     for (int j = 0; j < DOM_LIGHT; j++) {
    //         vec2 randomNum = vec2(random(rseed), random(rseed));
    //         float r1 = randomNum.x;
    //         float r2 = randomNum.y;
    //         float theta = r1 * PI;
    //         float phi = r2 * 2 * PI;

    //         vec3 d = vec3(sin(phi) * sin(theta), cos(phi) * sin(theta), cos(theta));
    //         if (dot(d, finalP.normal) > 0) {
    //             raylaunched++;
    //             vec3 temp =
    //                 PBR(normalize(d), V, N, sunColor, metallic, roughness, finalP.color);
    //             if (checkIfShadow(d, finalP, objects, nbOfObjects)) {
    //                 temp = color * 0.;
    //             }
    //             lighDomeColor += temp;
    //         }
    //     }
    //     lighDomeColor =
    //         (raylaunched > 0) ? lighDomeColor / float(raylaunched) : vec3(0, 0, 0);
    //     color = color + lighDomeColor * 0.5;
    // }

    return vec3(1, 1, 1);
}

////////////////////////////////////////////////////////
//// Main Function /////////////////////////////////////
////////////////////////////////////////////////////////

void main() {
    Object plan = createObject(
        Plan, 0, 0, TransformComponent(vec3(0, 2, 0), vec3(1, 1, 1), vec3(0, 0, 0)),
        Material(vec3(1, 1, 1), 0., 1.));

    Object block1 = createObject(
        Sphere, 0, 0, TransformComponent(vec3(-1.5, 1, 0), vec3(1, 1, 1), vec3(0, 0, 0)),
        Material(vec3(1, 0, 0), 1, 0.9));

    Object block2 = createObject(
        Box, 0, 0, TransformComponent(vec3(1.5, 1, 0), vec3(1, 0.5, 1), vec3(0, 0, 0)),
        Material(vec3(0, 0, 1), 0, 0.5));

    Object objects[100];
    objects[0] = plan;
    objects[1] = block1;
    objects[2] = block2;
    uint nbOfObjects = 3;

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
    float depthDist = 0;

    for (int i = 0; i < ANTI_ALIASING_FACTOR; i++) {
        vec2 randomRayDir = vec2(random(rseed), random(rseed));
        Ray ray = createRay(coord + randomRayDir - vec2(0.5, 0.5));

        HitPoint finalP = intersect(ray, objects, nbOfObjects, dist);
        depthDist += dist;

        vec3 mask = vec3(1.0f);
        int bounce = 0;
        float c_refl = 1.0f;
        vec3 color = vec3(0);

        while (!(dist == 1.0 / 0.0 || dist < 0) && bounce < 2) {
            finalP.pos += 0.0001 * finalP.normal;

            color += sunLight(ray, sunDir, sunColor, finalP, objects, nbOfObjects) * mask;

            // for (int w = 0; w < ubo.numLights; w++) {
            //     color +=
            //         pointLight(ray, ubo.HitPointLights[w], finalP, objects, nbOfObjects) * mask;
            // }
            vec3 Ve = normalize(ray.ro - finalP.pos);
            vec3 H = reflect(ray.rd, finalP.normal);
            c_refl = length(fresnelSchlick(
                max(dot(Ve  , finalP.normal), 0.0),
                mix(vec3(0.04), pow(finalP.material.albedo, vec3(2.2)), finalP.material.metallic)));
            
            mask = mask  * max(c_refl-finalP.material.roughness, 0.0);
            // color = vec3(mask);
            ray.ro = finalP.pos;
            ray.rd = reflect(ray.rd, finalP.normal);
            finalP = intersect(ray, objects, nbOfObjects, dist);
            bounce++;
        }
        ImageColor += color;
        if((dist == 1.0 / 0.0 || dist < 0)){
            ImageColor += pow(vec3(0.502, 0.869, 1), vec3(2.2)) * ImageColor * mask * 0.001;
        }
        // 

        if (bounce == 0) {
            ImageColor += pow(vec3(0.502, 0.869, 1), vec3(2.2));
            depthDist = 1.0 / 0.0;
        }
    }

    vec3 finalColor = ImageColor / ANTI_ALIASING_FACTOR;
    finalColor = finalColor / (finalColor + vec3(1.0));
    finalColor = pow(finalColor, vec3(1.0f / 2.2));

    // // finalColor = finalColor * 0.5 + texture(previousImage, coord / vec2(3840, 2160)).rgb *
    // 0.5; if (ubo.frameCount > 0) {
    //     float merginFactor = 1.0 / float(ubo.frameCount);
    //     finalColor = finalColor * merginFactor +
    //                  texture(previousImage, coord / vec2(3840, 2160)).rgb * (1.0 - merginFactor);
    // }
    gl_FragDepth = distanceToZBufferValue(depthDist / ANTI_ALIASING_FACTOR, 0.1, 100.0);
    outColor2 = vec4(finalColor, 1);
    outColor = vec4(finalColor, 1);
}
