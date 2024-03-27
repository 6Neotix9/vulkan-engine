#pragma once

#include <vulkan/vulkan_core.h>

#include "lve_c_pipeline.hpp"
#include "lve_descriptors.hpp"
#include "lve_device.hpp"
#include "lve_image.hpp"

// std
#include <memory>
#include <vector>

namespace lve {
class BRDF_LUTSystem {
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
    std::unique_ptr<LveCPipeline> lveCPipeline;
    VkDescriptorSet brdfLUTDescriptorSet;
    VkPipelineLayout pipelineLayout;
    LveDevice &lveDevice;
    void createBRDFLUTImage();
    void createDescriptorSet();
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayouts);
    void createPipeline();
};
}  // namespace lve
