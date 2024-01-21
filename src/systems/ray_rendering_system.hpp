#pragma once


#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_A_system.hpp"
#include "lve_pipeline_ressources.hpp"


// std
#include <memory>
#include <vector>

namespace lve {
class RayRenderingSystem : public LveASystem{
 public:
  RayRenderingSystem(
      LveDevice &device, std::shared_ptr<LvePipelineRessources> renderPass, VkDescriptorSetLayout globalSetLayout);
  ~RayRenderingSystem();

  RayRenderingSystem(const RayRenderingSystem &) = delete;
  RayRenderingSystem &operator=(const RayRenderingSystem &) = delete;

  void update(FrameInfo &frameInfo, GlobalUbo &ubo);
  void render(FrameInfo &frameInfo);


 private:
  void createPipelineLayout(std::vector<VkDescriptorSetLayout> globalSetLayout) override;
  void createPipeline() override;
};
}  // namespace lve
