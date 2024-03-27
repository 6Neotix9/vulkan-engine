#pragma once

#include "lve_camera.hpp"
#include "lve_game_object.hpp"

// lib
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace lve {

#define MAX_LIGHTS 10

struct PointLight {
  glm::vec4 position{};  // ignore w
  glm::vec4 color{};     // w is intensity
};

struct GlobalUbo {
  glm::mat4 projection{1.f};
  glm::mat4 view{1.f};
  glm::mat4 inverseView{1.f};
  glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .02f};  // w is intensity
  PointLight pointLights[MAX_LIGHTS];
  int numLights;
  float frameTime;
  int frameNumber;
};

struct frameCommandBuffers{
  VkCommandBuffer renderCommandBuffer;
  VkCommandBuffer preProcessommandBuffer;
};

struct FrameInfo {
  int frameIndex;
  uint swapchainFrameIndex;
  float frameTime;
  VkExtent2D extent;
  frameCommandBuffers commandBuffer;
  
  LveCamera &camera;
  VkDescriptorSet globalDescriptorSet;
  LveGameObject::Map &gameObjects;
};
}  // namespace lve
