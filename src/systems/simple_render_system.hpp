#pragma once

#include <vulkan/vulkan_core.h>
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_pipeline.hpp"
#include "systems/lve_I_system.hpp"

// std
#include <memory>
#include <vector>
#include <mutex>

namespace lve {
class SimpleRenderSystem : public LveISystem {
   public:
    SimpleRenderSystem(
        LveDevice &device, std::shared_ptr<LvePipelineRessources> pipelineRessources, VkDescriptorSetLayout globalSetLayout);
    ~SimpleRenderSystem();

    SimpleRenderSystem(const SimpleRenderSystem &) = delete;
    SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

    void renderGameObjects(FrameInfo &frameInfo);

    void reloadShaders() override;

   private:
    void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createPipeline();

    std::mutex shaderReloadMutex = std::mutex();
    LveDevice &lveDevice;
    std::shared_ptr<LvePipelineRessources> pipelineRessources;
    std::unique_ptr<LvePipeline> lvePipeline;
    VkPipelineLayout pipelineLayout;
};
}  // namespace lve
