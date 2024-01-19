#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "lve_device.hpp"
#include "lve_image.hpp"

namespace lve {

struct ExternalImageAttachement {
    std::vector<VkImageView> imageView;
    VkFormat imageformat;
};

struct PipelineRessourcesCreateInfo {
    std::shared_ptr<VkRenderPass> renderPass;
    std::vector<VkFormat> colorAttachments;
    std::vector<ExternalImageAttachement> externalAttachments;
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
        LveDevice &lveDevice, PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo);

    ~LvePipelineRessources();

    //getters
    VkRenderPass getRenderPass() { return *renderPass; }
    VkFramebuffer getFrameBuffer(int index) { return frameBuffer[index]; }
    

   private:
    void createAttachementImage(PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo);
    void createRenderPass(PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo);
    void createFrameBuffer(PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo);

    std::shared_ptr<VkRenderPass> renderPass;
    VkRenderPass oldRenderPass;
    std::vector<VkFramebuffer> frameBuffer;
    std::vector<ImagesAttachment> imagesAttachments;

    LveDevice &lveDevice;
};

}  // namespace lve