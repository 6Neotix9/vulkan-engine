#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>

#include "lve_descriptors.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_image.hpp"
#include "systems/lve_A_system.hpp"

namespace lve {
class WaveSpectrum : public LveASystem {
   public:
    WaveSpectrum(
        LveDevice &device,
        std::shared_ptr<LveImage> waveTexture,
        std::shared_ptr<LveImage> waveDataTexture,
        float LengthScale,
        float CutoffLow,
        float CutoffHigh);
    ~WaveSpectrum();

    void createPipeline() override;
    void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayouts) override;
    void executeShader(FrameInfo &frameInfo) override;

   private:
    void createWaveDataBuffer();
    void createDescriptorPool();
    void createDescriptorSetLayout();
    void createDescriptorSet();
    void setData(float LengthScale, float CutoffLow, float CutoffHigh);

    int width;
    int height;
    std::shared_ptr<LveImage> noiseTexture;
    std::shared_ptr<LveImage> waveTexture;
    std::shared_ptr<LveImage> waveDataTexture;
    std::unique_ptr<LveBuffer> waveGenDataBuffers;

    VkDescriptorSet waveSpectrumDescriptorSets;
    std::unique_ptr<LveDescriptorSetLayout> waveSpectrumLayout;
    std::unique_ptr<LveDescriptorPool> waveSpectrumPool{};
};
}  // namespace lve