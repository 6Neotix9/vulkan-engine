#pragma once

#include <vulkan/vulkan_core.h>

#include "lve_descriptors.hpp"
#include "lve_device.hpp"
#include "lve_image.hpp"
#include "systems/lve_A_system.hpp"

// std
#include <memory>
#include <mutex>
#include <vector>

namespace lve {
class BRDF_LUTSystem : public LveASystem {
   public:
    BRDF_LUTSystem(LveDevice &device);
    ~BRDF_LUTSystem();

    BRDF_LUTSystem(const BRDF_LUTSystem &) = delete;
    BRDF_LUTSystem &operator=(const BRDF_LUTSystem &) = delete;

    void executeShader(VkCommandBuffer preRenderCommandBuffer);
    std::shared_ptr<LveImage> getBRDFLUTImage() const { return brdfLUTImage; }

   private:
    std::shared_ptr<LveImage> brdfLUTImage;
    std::unique_ptr<LveDescriptorPool> brdfLUTDescriptorPool;
    std::unique_ptr<LveDescriptorSetLayout> brdfLUTDescriptorLayout;
    VkDescriptorSet brdfLUTDescriptorSet;

    void createBRDFLUTImage();
    void createDescriptorSet();
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayouts) override;
    void createPipeline() override;
};
}  // namespace lve
