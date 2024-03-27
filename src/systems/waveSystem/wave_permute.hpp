#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>

#include "lve_descriptors.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_image.hpp"
#include "systems/lve_A_system.hpp"

namespace lve {
class WavePermute : public LveASystem {
   public:
    WavePermute(
        LveDevice &device,
        std::vector<std::shared_ptr<LveImage>> buffer0);
    ~WavePermute();

    void createPipeline() override;
    void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayouts) override;
    void executeShader(FrameInfo &frameInfo) override;

   private:
    void createDescriptorPool();
    void createDescriptorSetLayout();
    void createDescriptorSet();

    int width;
    int height;
    std::vector<std::shared_ptr<LveImage>> buffer0;
    std::vector<VkDescriptorSet> wavePermuteDescriptorSets;
    std::unique_ptr<LveDescriptorSetLayout> wavePermuteLayout;
    std::unique_ptr<LveDescriptorPool> wavePermutePool{};
};
}  // namespace lve