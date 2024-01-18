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
    uint32_t width;
    uint32_t height;
    int numberOfImage;
};

struct ImagesAttachment {
    std::vector<std::shared_ptr<LveImage>> colorImage;
    std::shared_ptr<LveImage> depthImage;
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

    VkRenderPass renderPass;
    VkRenderPass oldRenderPass;
    std::vector<VkFramebuffer> frameBuffer;
    std::vector<ImagesAttachment> imagesAttachments;

    LveDevice &lveDevice;
};

}  // namespace lve