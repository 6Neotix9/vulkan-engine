
#include "lve_pipeline_ressources.hpp"

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

#include "lve_device.hpp"
#include "lve_image.hpp"

namespace lve {

LvePipelineRessources::LvePipelineRessources(
    LveDevice *lveDevice, PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo)
    : lveDevice(lveDevice),
      externalAttachments{pipelineRessourcesCreateInfo.externalAttachments} {
    createAttachementImage(pipelineRessourcesCreateInfo);
    createRenderPass(pipelineRessourcesCreateInfo);
    createFrameBuffer(pipelineRessourcesCreateInfo);
}
LvePipelineRessources::LvePipelineRessources(
        ){}


LvePipelineRessources& LvePipelineRessources::operator=(LvePipelineRessources&& other){
    LveDevice* tempLvedevice = this->lveDevice;
    std::vector<ExternalImageAttachement> tempExternalAttachments = this->externalAttachments;
    std::vector<VkFramebuffer> tempFrameBuffer = this->frameBuffer;
    std::vector<ImagesAttachment> tempImagesAttachments = this->imagesAttachments;
    VkRenderPass tempRenderPass = this->renderPass;

    
    this->lveDevice = other.lveDevice;
    this->externalAttachments = other.externalAttachments;
    this->frameBuffer = other.frameBuffer;
    this->imagesAttachments = other.imagesAttachments;
    this->renderPass = other.renderPass;


    other.lveDevice = tempLvedevice;
    other.externalAttachments = tempExternalAttachments;
    other.frameBuffer = tempFrameBuffer;
    other.imagesAttachments = tempImagesAttachments;
    other.renderPass = tempRenderPass;
    return *this;
}

LvePipelineRessources::~LvePipelineRessources() {
    for (auto framebuffer : frameBuffer) {
        vkDestroyFramebuffer(lveDevice->device(), framebuffer, nullptr);
    }
    if (renderPass != VK_NULL_HANDLE)
        vkDestroyRenderPass(lveDevice->device(), renderPass, nullptr);
}

void LvePipelineRessources::createAttachementImage(
    PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo) {
    for (int i = 0; i < pipelineRessourcesCreateInfo.numberOfImage; i++) {
        std::vector<std::shared_ptr<LveImage>> colorImages;
        for (auto imageformat : pipelineRessourcesCreateInfo.colorAttachments) {
            ImageCreateInfo imageCreateInfo{};
            imageCreateInfo.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            imageCreateInfo.format = imageformat;
            imageCreateInfo.width = pipelineRessourcesCreateInfo.width;
            imageCreateInfo.height = pipelineRessourcesCreateInfo.height;
            colorImages.push_back(std::make_shared<LveImage>(lveDevice, imageCreateInfo));
        }
        ImagesAttachment imagesAttachment{colorImages, nullptr};
        if (pipelineRessourcesCreateInfo.hasDepthAttachement) {
            ImageCreateInfo imageCreateInfo{};
            imageCreateInfo.usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            imageCreateInfo.format = lveDevice->findSupportedFormat(
                {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
            ;
            imageCreateInfo.width = pipelineRessourcesCreateInfo.width;
            imageCreateInfo.height = pipelineRessourcesCreateInfo.height;

            imagesAttachment.depthImage = std::make_shared<LveImage>(lveDevice, imageCreateInfo);
        }

        imagesAttachments.push_back(imagesAttachment);
    }
}

void LvePipelineRessources::createRenderPass(
    PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo) {
    std::vector<VkAttachmentDescription> attachments;

    for (auto externalAttachement : pipelineRessourcesCreateInfo.externalAttachments) {
        VkAttachmentDescription externalAttachment = {};
        externalAttachment.format = externalAttachement.imageformat;
        externalAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        externalAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        externalAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        externalAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        externalAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        externalAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        externalAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachments.push_back(externalAttachment);
    }

    for (auto colorAttachement : pipelineRessourcesCreateInfo.colorAttachments) {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = colorAttachement;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments.push_back(colorAttachment);
    }

    if (pipelineRessourcesCreateInfo.hasDepthAttachement) {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = imagesAttachments[0].depthImage->getFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.push_back(depthAttachment);
    }

    std::vector<VkAttachmentReference> colorAttachmentRef;
    // for (uint32_t i = 0; i < pipelineRessourcesCreateInfo.externalAttachments.size(); i++) {
    //     colorAttachmentRef.push_back({i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    // }

    for (uint32_t i = 0; i < attachments.size() - 1; i++) {
        colorAttachmentRef.push_back({i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    }

    VkAttachmentReference depthAttachmentRef{};
    if (pipelineRessourcesCreateInfo.hasDepthAttachement) {
        depthAttachmentRef.attachment = attachments.size() - 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = colorAttachmentRef.size();
    subpass.pColorAttachments = colorAttachmentRef.data();
    if (pipelineRessourcesCreateInfo.hasDepthAttachement) {
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
    }

    // TODO : Se renseigner sur le paramétrage des subpass
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(lveDevice->device(), &renderPassInfo, nullptr, &renderPass) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }

}
void LvePipelineRessources::createFrameBuffer(
    PipelineRessourcesCreateInfo pipelineRessourcesCreateInfo) {
    frameBuffer.resize(pipelineRessourcesCreateInfo.numberOfImage);

    for (size_t i = 0; i < pipelineRessourcesCreateInfo.numberOfImage; i++) {
        std::vector<VkImageView> attachments{};

        for (auto externalAttachement : pipelineRessourcesCreateInfo.externalAttachments) {
            attachments.push_back(externalAttachement.imageView[i]);
        }

        for (auto colorAttachment : imagesAttachments[i].colorImage) {
            attachments.push_back(colorAttachment->getVkImageView());
        }

        if (pipelineRessourcesCreateInfo.hasDepthAttachement) {
            attachments.push_back(imagesAttachments[i].depthImage->getVkImageView());
        }

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = pipelineRessourcesCreateInfo.width;
        framebufferInfo.height = pipelineRessourcesCreateInfo.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(lveDevice->device(), &framebufferInfo, nullptr, &frameBuffer[i]) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

}  // namespace lve