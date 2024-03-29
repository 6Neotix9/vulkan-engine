#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "lve_c_pipeline.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_texture.hpp"

namespace lve {
class WaveScale {
   public:
    struct WaveCreationStriuct {
        // waveGeneration
        LveTexture waveTexture;
        LveTexture waveTextureConj;
        LveTexture waveData;
        // For IFFT
        LveTexture Dx_Dz;
        LveTexture Dy_Dxz;
        LveTexture Dyx_Dyz;
        LveTexture Dxx_Dzz;
        LveTexture precomputedData;
        LveTexture buffer[4];
        // Result
        LveTexture Displacement;
        LveTexture Derivatives;
        LveTexture Turbulence;
    };

    WaveScale(LveDevice &device, int height, int width, std::vector<std::shared_ptr<LveTexture>> buffer0);
    ~WaveScale();

    void executePreCpS(FrameInfo FrameInfo);

   private:
    void createDescriptorPool();
    void createDescriptorSetLayout();
    void createDescriptorSet();

    LveDevice &lveDevice;

    int width;
    int height;
    std::vector<std::shared_ptr<LveTexture>> buffer0;

    std::vector<VkDescriptorSet> waveConjugateDescriptorSets;
    std::unique_ptr<LveDescriptorSetLayout> waveGenSetLayout;

    std::unique_ptr<LveDescriptorPool> wavePool{};
    std::unique_ptr<LveCPipeline> lveCPipeline;
    VkPipelineLayout pipelineLayout;
};
}  // namespace lve