#include "ray_rendering_system.hpp"
#include <glm/fwd.hpp>
#include <vector>


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
    LveDevice& device, std::shared_ptr<LvePipelineRessources> pipelineRessources, VkDescriptorSetLayout globalSetLayout)
    : LveASystem(device, "ray_rendering", pipelineRessources, std::vector<VkDescriptorSetLayout>{globalSetLayout}){
    createPipelineLayout(std::vector<VkDescriptorSetLayout>{globalSetLayout});
    createPipeline();
}

RayRenderingSystem::~RayRenderingSystem() {
}



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

void RayRenderingSystem::update(FrameInfo& frameInfo, GlobalUbo& ubo) {
}

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
    vkCmdBindDescriptorSets(
        frameInfo.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout,
        0,
        1,
        &frameInfo.globalDescriptorSet,
        0,
        nullptr);

    vkCmdDraw(frameInfo.commandBuffer, 3, 1, 0, 0);
    
}

}  // namespace lve
