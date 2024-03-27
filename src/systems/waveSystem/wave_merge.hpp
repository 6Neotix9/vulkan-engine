#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>

#include "lve_descriptors.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_image.hpp"
#include "systems/lve_A_system.hpp"

namespace lve {
class WaveMerge : public LveASystem {
   public:
    WaveMerge(
        LveDevice &device,
        std::vector<std::shared_ptr<LveImage>> Dx_Dz,
        std::vector<std::shared_ptr<LveImage>> Dy_Dxz,
        std::vector<std::shared_ptr<LveImage>> Dyx_Dyz,
        std::vector<std::shared_ptr<LveImage>> Dxx_Dzz,
        std::vector<std::shared_ptr<LveImage>> Displacement,
        std::vector<std::shared_ptr<LveImage>> Derivatives,
        std::vector<std::shared_ptr<LveImage>> Turbulence);
    ~WaveMerge();

    void createPipeline() override;
    void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayouts) override;
    void executeShader(FrameInfo &frameInfo) override;

   private:
    void createDescriptorPool();
    void createDescriptorSetLayout();
    void createDescriptorSet();

    int width;
    int height;
    std::vector<std::shared_ptr<LveImage>> Dx_Dz;
    std::vector<std::shared_ptr<LveImage>> Dy_Dxz;
    std::vector<std::shared_ptr<LveImage>> Dyx_Dyz;
    std::vector<std::shared_ptr<LveImage>> Dxx_Dzz;
    std::vector<std::shared_ptr<LveImage>> Displacement;
    std::vector<std::shared_ptr<LveImage>> Derivatives;
    std::vector<std::shared_ptr<LveImage>> Turbulence;

    std::vector<VkDescriptorSet> waveMergeDescriptorSets;
    std::unique_ptr<LveDescriptorSetLayout> waveMergeLayout;
    std::unique_ptr<LveDescriptorPool> waveMergePool{};
};
}  // namespace lve