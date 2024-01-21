#pragma once

#include <vector>
#include "lve_device.hpp"
#include "lve_pipeline_ressources.hpp"
#include "lve_pipeline.hpp"

namespace lve {
class LveASystem {
   public:
    LveASystem(LveDevice &lveDevice, const char *shaderName, std::shared_ptr<LvePipelineRessources> pipelineRessources, std::vector<VkDescriptorSetLayout> descriptorSetLayouts);
    virtual ~LveASystem();
    void reloadShaders();
    virtual void createPipeline() = 0;
    virtual void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayouts) = 0;
   protected:
    LveDevice &lveDevice;
    std::shared_ptr<LvePipelineRessources> pipelineRessources;
    std::unique_ptr<LvePipeline> lvePipeline;
    VkPipelineLayout pipelineLayout;
   private:
};
}  // namespace lve