#pragma once

#include "lve_device.hpp"
#include "lve_image.hpp"
#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace lve {

struct PipelineRessourcesCreateInfo {
  std::vector<VkFormat> colorAttachments;
  std::vector<VkFormat> depthAttachements;
  uint32_t widht;
  uint32_t height;
  int numberOfImage;
};

class LvePipelineRessources {
public:
  LvePipelineRessources(
      PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo,
      LveDevice &lveDevice);

  ~LvePipelineRessources();

private:
  
  void createAttachementImage(PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo);
  void createRenderPass(PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo);
  void createFrameBuffer(PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo);

  std::shared_ptr<VkRenderPass> renderPass;
  std::vector<VkFramebuffer> frameBuffer;
  std::vector<LveImage> colorImage;
  std::vector<LveImage> depthAndStencilImage;

  LveDevice &lveDevice;
};

} // namespace lve