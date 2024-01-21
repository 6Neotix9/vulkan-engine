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

layout(push_constant) uniform Push {
  vec2 resolution;

} push;


vec3 createRay(vec2 px){
    //convert pixel to NDS
    vec2 pxNDS = (px / push.resolution) * 2.0 - 1.0;
    vec4 pointNDSH = vec4(pxNDS, 0.1f,1.f);
    vec4 dirEye = inverse(ubo.projection) * pointNDSH;
    dirEye.w = 0.;
    vec3 dirWorld = (ubo.invView * dirEye).xyz;
    return normalize(dirWorld); 
}



void main() {
    vec2 coord = gl_FragCoord.xy;
    vec3 color = createRay(coord);

    
    outColor = vec4(color, 1);
    

    
}
