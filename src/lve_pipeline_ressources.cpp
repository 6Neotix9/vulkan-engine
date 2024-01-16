
#include "lve_pipeline_ressources.hpp"
#include "lve_device.hpp"
#include "lve_image.hpp"
#include <vulkan/vulkan_core.h>

namespace lve {

LvePipelineRessources::LvePipelineRessources(
    PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo,
    LveDevice &lveDevice)
    : lveDevice(lveDevice) {
  createAttachementImage(pipelineRessourcesCreateInfo);
  //createRenderPass(pipelineRessourcesCreateInfo);
  //createFrameBuffer(pipelineRessourcesCreateInfo);
}

LvePipelineRessources::~LvePipelineRessources() {}

void LvePipelineRessources::createAttachementImage(
    PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo) {
  for (auto imageformat : pipelineRessourcesCreateInfo.colorAttachments) {

    ImageCreateInfo imageCreateInfo{};
    imageCreateInfo.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageCreateInfo.format = imageformat;
    imageCreateInfo.width = pipelineRessourcesCreateInfo.widht;
    imageCreateInfo.height = pipelineRessourcesCreateInfo.height;

    LveImage colorAttachement{lveDevice, imageCreateInfo};
    colorImage.push_back(colorAttachement);
  }

  for (auto imageformat : pipelineRessourcesCreateInfo.depthAttachements) {

    ImageCreateInfo imageCreateInfo{};
    imageCreateInfo.usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageCreateInfo.format = imageformat;
    imageCreateInfo.width = pipelineRessourcesCreateInfo.widht;
    imageCreateInfo.height = pipelineRessourcesCreateInfo.height;

    LveImage colorAttachement{lveDevice, imageCreateInfo};
    colorImage.push_back(colorAttachement);
  }
}

void LvePipelineRessources::createRenderPass(
    PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo) {}
void LvePipelineRessources::createFrameBuffer(
    PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo) {}

} // namespace lve