#include "ray_rendering_system.hpp"

#include <vulkan/vulkan_core.h>

#include <glm/fwd.hpp>
#include <memory>
#include <vector>

#include "lve_image.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <map>
#include <stdexcept>

namespace lve {

struct ResolutionPushConstant {
    glm::vec2 resolution;
};

RayRenderingSystem::RayRenderingSystem(
    LveDevice& device,
    std::shared_ptr<LvePipelineRessources> pipelineRessources,
    VkDescriptorSetLayout globalSetLayout)
    : LveASystem(
          device,
          "ray_rendering",
          pipelineRessources,
          std::vector<VkDescriptorSetLayout>{globalSetLayout}) {
    createRandomImage();
    createDescriptorSet();
    createPipelineLayout(std::vector<VkDescriptorSetLayout>{globalSetLayout, randomImageDescriptorLayout->getDescriptorSetLayout()});
    createPipeline();
    
}

RayRenderingSystem::~RayRenderingSystem() {}

void RayRenderingSystem::createPipelineLayout(std::vector<VkDescriptorSetLayout> globalSetLayout) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(ResolutionPushConstant);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void RayRenderingSystem::createPipeline() {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    pipelineConfig.pipelineRessource = pipelineRessources;
    LvePipeline::defaultPipelineConfigInfo(pipelineConfig);
    LvePipeline::enableAlphaBlending(pipelineConfig);
    pipelineConfig.attributeDescriptions.clear();
    pipelineConfig.bindingDescriptions.clear();
    pipelineConfig.renderPass = pipelineRessources->getRenderPass();
    pipelineConfig.pipelineLayout = pipelineLayout;
    lvePipeline = std::make_unique<LvePipeline>(
        lveDevice,
        "shaders/ray_rendering.vert.spv",
        "shaders/ray_rendering.frag.spv",
        pipelineConfig);
}

void RayRenderingSystem::createDescriptorSet() {
    randomImageDescriptorPool = LveDescriptorPool::Builder(lveDevice)
                                    .setMaxSets(1)
                                    .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
                                    .build();
    randomImageDescriptorLayout =
        LveDescriptorSetLayout::Builder(lveDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
            .build();
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = randomImage->getVkImageView();
    imageInfo.sampler = randomImage->getVkSampler();
    LveDescriptorWriter(*randomImageDescriptorLayout, *randomImageDescriptorPool)
            .writeImage(0, &imageInfo)
            .build(randomImageDescriptorSet);
}

void RayRenderingSystem::createRandomImage() {
    srand(static_cast<unsigned>(time(0)));
    std::vector<float> randomValues;
    for (int i = 0; i < randomValues.size(); i++) {
        randomValues.push_back(static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
    }
    ImageCreateInfo imageCreateInfo{};
    imageCreateInfo.pixels = randomValues.data();
    imageCreateInfo.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.format = VK_FORMAT_R32G32_SFLOAT;
    imageCreateInfo.width = 3840;
    imageCreateInfo.height = 2160;
    randomImage = std::make_unique<LveImage>(&lveDevice, imageCreateInfo);
}

void RayRenderingSystem::update(FrameInfo& frameInfo, GlobalUbo& ubo) {}

void RayRenderingSystem::render(FrameInfo& frameInfo) {
    lvePipeline->bind(frameInfo.commandBuffer);

    ResolutionPushConstant pushConstant{};
    pushConstant.resolution = glm::vec2(frameInfo.extent.width, frameInfo.extent.height);
    vkCmdPushConstants(
        frameInfo.commandBuffer,
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(ResolutionPushConstant),
        &pushConstant);
        std::vector<VkDescriptorSet> descriptorSet = {frameInfo.globalDescriptorSet, randomImageDescriptorSet};
    vkCmdBindDescriptorSets(
        frameInfo.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout,
        0,
        2,
        descriptorSet.data(),
        0,
        nullptr);

    vkCmdDraw(frameInfo.commandBuffer, 3, 1, 0, 0);
}

}  // namespace lve
