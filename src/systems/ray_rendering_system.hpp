#pragma once

#include "lve_A_system.hpp"
#include "lve_descriptors.hpp"
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_image.hpp"
#include "lve_pipeline_ressources.hpp"

// std
#include <memory>
#include <vector>

namespace lve {
class RayRenderingSystem : public LveASystem {
   public:
    RayRenderingSystem(
        LveDevice &device,
        std::shared_ptr<LvePipelineRessources> renderPass,
        VkDescriptorSetLayout globalSetLayout, std::shared_ptr<LveImage> BRDF_LUTImage);
    ~RayRenderingSystem();

    RayRenderingSystem(const RayRenderingSystem &) = delete;
    RayRenderingSystem &operator=(const RayRenderingSystem &) = delete;

    void update(FrameInfo &frameInfo, GlobalUbo &ubo);
    void executeShader(FrameInfo &frameInfo) override;
    std::shared_ptr<LveImage> getPreviousImage(uint i) const {return previousImages[i];}
    
   private:
    void createRandomImage();
    void createDescriptorSet();
    void createPipelineLayout(std::vector<VkDescriptorSetLayout> globalSetLayout) override;
    void createPipeline() override;

    std::vector<std::shared_ptr<LveImage>> previousImages;
    std::unique_ptr<LveDescriptorPool> randomImageDescriptorPool;
    std::unique_ptr<LveDescriptorSetLayout> randomImageDescriptorLayout;
    VkDescriptorSet randomImageDescriptorSet;
    std::unique_ptr<LveDescriptorSetLayout> previousImageDescriptorLayout;
    std::vector<VkDescriptorSet> previousImageDescriptorSets;
    

    std::shared_ptr<LveImage> BRDF_LUTImage;
    std::unique_ptr<LveImage> randomImage;
};
}  // namespace lve
