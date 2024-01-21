#version 450

const vec2 OFFSETS[3] = vec2[](
  vec2(0, -4.5),
  vec2(-2, 2),
  vec2(2, 2)

);

layout (location = 0) out vec2 fragOffset;

struct PointLight {
  vec4 position; // ignore w
  vec4 color; // w is intensity
};

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  mat4 invView;
  vec4 ambientLightColor; // w is intensity
  PointLight pointLights[10];
  int numLights;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D image;


void main() {
  fragOffset = OFFSETS[gl_VertexIndex];


  gl_Position = vec4(fragOffset,0,1);
}