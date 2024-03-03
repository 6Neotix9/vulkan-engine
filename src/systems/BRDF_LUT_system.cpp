#include "BRDF_LUT_system.hpp"

#include <vulkan/vulkan_core.h>

#include <iostream>
#include <thread>
#include <vector>

#include "lve_c_pipeline.hpp"
#include "lve_descriptors.hpp"
#include "lve_device.hpp"
#include "lve_image.hpp"
#include "lve_pipeline_ressources.hpp"
#include "systems/lve_A_system.hpp"


// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <stdexcept>

namespace lve {

struct SimplePushConstantData {
    glm::mat4 modelMatrix{1.f};
    glm::mat4 normalMatrix{1.f};
};

BRDF_LUTSystem::BRDF_LUTSystem(LveDevice& device) : LveASystem(device, "gendbrdfLut", nullptr) {
    createBRDFLUTImage();
    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSet();
    createPipelineLayout({brdfLUTDescriptorLayout->getDescriptorSetLayout()});
    createPipeline();
}

BRDF_LUTSystem::~BRDF_LUTSystem() {}

void BRDF_LUTSystem::createBRDFLUTImage() {
    ImageCreateInfo imageCreateInfo{};
    imageCreateInfo.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    imageCreateInfo.format = VK_FORMAT_R16G16_SFLOAT;
    imageCreateInfo.width = 512;
    imageCreateInfo.height = 512;
    imageCreateInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    brdfLUTImage = std::make_shared<LveImage>(&lveDevice, imageCreateInfo);
}

void BRDF_LUTSystem::createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayouts) {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

    if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void BRDF_LUTSystem::createDescriptorPool(){
    brdfLUTDescriptorPool = LveDescriptorPool::Builder(lveDevice)
                                    .setMaxSets(1)
                                    .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1)
                                    .build();
}


void BRDF_LUTSystem::createDescriptorSetLayout(){
    brdfLUTDescriptorLayout = LveDescriptorSetLayout::Builder(lveDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .build();
}

void BRDF_LUTSystem::createDescriptorSet(){
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo.imageView = brdfLUTImage->getVkImageView();

    LveDescriptorWriter(*brdfLUTDescriptorLayout, *brdfLUTDescriptorPool)
        .writeImage(0, &imageInfo)
        .build(brdfLUTDescriptorSet);
}

void BRDF_LUTSystem::createPipeline() {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    ComputePipelineConfigInfo pipelineConfig{};
    pipelineConfig.computePipelineLayout = pipelineLayout;
    lveCPipeline = std::make_unique<LveCPipeline>(
        lveDevice, "shaders/compiled/gendbrdfLut.comp.spv",
         pipelineConfig);
}

void BRDF_LUTSystem::executeShader(VkCommandBuffer preRenderCommandBuffer) {
    lveCPipeline->bind(preRenderCommandBuffer);
    vkCmdBindDescriptorSets(
        preRenderCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &brdfLUTDescriptorSet,
        0, nullptr);

    int groupCountX = 512 / 32;
    if (512 % 32 != 0) {
        groupCountX++;
    }

    vkCmdDispatch(preRenderCommandBuffer, groupCountX, groupCountX, 1);
}
}  // namespace lve