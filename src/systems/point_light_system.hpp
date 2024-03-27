#pragma once


#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_A_system.hpp"
#include "lve_pipeline_ressources.hpp"


// std
#include <memory>
#include <vector>

namespace lve {
class PointLightSystem : public LveASystem{
 public:
  PointLightSystem(
      LveDevice &device, std::shared_ptr<LvePipelineRessources> renderPass, VkDescriptorSetLayout globalSetLayout);
  ~PointLightSystem();

  PointLightSystem(const PointLightSystem &) = delete;
  PointLightSystem &operator=(const PointLightSystem &) = delete;

  void update(FrameInfo &frameInfo, GlobalUbo &ubo);
  void executeShader(FrameInfo &frameInfo) override;


 private:
  void createPipelineLayout(std::vector<VkDescriptorSetLayout> globalSetLayout) override;
  void createPipeline() override;
};
}  // namespace lve
