#pragma once

#include <vulkan/vulkan_core.h>
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "systems/lve_A_system.hpp"

// std
#include <memory>
#include <vector>

namespace lve {
class SimpleRenderSystem : public LveASystem {
   public:
    SimpleRenderSystem(
        LveDevice &device, std::shared_ptr<LvePipelineRessources> pipelineRessources, VkDescriptorSetLayout globalSetLayout);
    ~SimpleRenderSystem();

    SimpleRenderSystem(const SimpleRenderSystem &) = delete;
    SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

    void executeShader(FrameInfo &frameInfo) override;



   private:
    void createPipelineLayout(std::vector<VkDescriptorSetLayout> globalSetLayout) override;
    void createPipeline() override;
};
}  // namespace lve
