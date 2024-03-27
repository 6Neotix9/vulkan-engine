#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>

#include "lve_descriptors.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_image.hpp"
#include "systems/lve_A_system.hpp"

namespace lve {
class WaveConjugate : public LveASystem {
   public:
    WaveConjugate(
        LveDevice &device,
        std::shared_ptr<LveImage> spectrumTexture,
        std::shared_ptr<LveImage> spectrumConjugateTexture);
    ~WaveConjugate();

    void createPipeline() override;
    void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayouts) override;
    void executeShader(FrameInfo &frameInfo) override;

   private:
    void createDescriptorPool();
    void createDescriptorSetLayout();
    void createDescriptorSet();

    int width;
    int height;
    std::shared_ptr<LveImage> spectrumTexture;
    std::shared_ptr<LveImage> spectrumConjugateTexture;
    VkDescriptorSet waveConjugateDescriptorSets;
    std::unique_ptr<LveDescriptorSetLayout> waveGenSetLayout;
    std::unique_ptr<LveDescriptorPool> wavePool{};
};
}  // namespace lve