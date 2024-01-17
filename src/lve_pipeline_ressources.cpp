
#include "lve_pipeline_ressources.hpp"

#include <vulkan/vulkan_core.h>

#include <memory>

#include "lve_device.hpp"
#include "lve_image.hpp"

namespace lve {

LvePipelineRessources::LvePipelineRessources(
    PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo, LveDevice &lveDevice)
    : lveDevice(lveDevice) {
    createAttachementImage(pipelineRessourcesCreateInfo);
    // createRenderPass(pipelineRessourcesCreateInfo);
    // createFrameBuffer(pipelineRessourcesCreateInfo);
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

        std::shared_ptr<LveImage> colorAttachement =
            std::make_shared<LveImage>(lveDevice, imageCreateInfo);
        colorImage.push_back(colorAttachement);
    }
    if (pipelineRessourcesCreateInfo.hasDepthAttachement) {
        ImageCreateInfo imageCreateInfo{};
        imageCreateInfo.usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageCreateInfo.format = lveDevice.findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        ;
        imageCreateInfo.width = pipelineRessourcesCreateInfo.widht;
        imageCreateInfo.height = pipelineRessourcesCreateInfo.height;

        depthImage = std::make_shared<LveImage>(lveDevice, imageCreateInfo);
    }
}

void LvePipelineRessources::createRenderPass(
    PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo) {
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthImage->getFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = getSwapChainImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.dstSubpass = 0;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &oldRenderPass) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}
void LvePipelineRessources::createFrameBuffer(
    PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo) {}

}  // namespace lve