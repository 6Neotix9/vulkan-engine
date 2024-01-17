#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "lve_device.hpp"
#include "lve_image.hpp"

namespace lve {

struct PipelineRessourcesCreateInfo {
    std::vector<VkFormat> colorAttachments;
    bool hasDepthAttachement;
    uint32_t widht;
    uint32_t height;
    int numberOfImage;
};

class LvePipelineRessources {
   public:
    LvePipelineRessources(
        PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo, LveDevice &lveDevice);

    ~LvePipelineRessources();

   private:
    void createAttachementImage(PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo);
    void createRenderPass(PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo);
    void createFrameBuffer(PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo);

    std::shared_ptr<VkRenderPass> renderPass;
    std::vector<VkFramebuffer> frameBuffer;
    std::vector<std::shared_ptr<LveImage>> colorImage;
    std::shared_ptr<LveImage> depthImage;
    LveDevice &lveDevice;
};

}  // namespace lve