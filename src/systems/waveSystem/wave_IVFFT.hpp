#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>

#include "lve_descriptors.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_image.hpp"
#include "systems/lve_A_system.hpp"

namespace lve {
class WaveIVFFT : public LveASystem {
   public:
    WaveIVFFT(
        LveDevice &device,
        std::vector<std::shared_ptr<LveImage>> buffer0,
                std::vector<std::shared_ptr<LveImage>> buffer1, std::shared_ptr<LveImage> preComputeData);
    ~WaveIVFFT();

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
    std::vector<std::shared_ptr<LveImage>> buffer1;
    std::shared_ptr<LveImage> precomputeData;

    std::vector<VkDescriptorSet> waveIVFFTDescriptorSets;
    std::unique_ptr<LveDescriptorSetLayout> waveIVFFTLayout;
    std::unique_ptr<LveDescriptorPool> waveIVFFTPool{};
};
}  // namespace lve