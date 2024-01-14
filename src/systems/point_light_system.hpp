#pragma once


#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_pipeline.hpp"
#include "lve_I_system.hpp"


// std
#include <memory>
#include <vector>

namespace lve {
class PointLightSystem : public LveISystem{
 public:
  PointLightSystem(
      LveDevice &device, std::shared_ptr<VkRenderPass> renderPass, VkDescriptorSetLayout globalSetLayout);
  ~PointLightSystem();

  PointLightSystem(const PointLightSystem &) = delete;
  PointLightSystem &operator=(const PointLightSystem &) = delete;

  void update(FrameInfo &frameInfo, GlobalUbo &ubo);
  void render(FrameInfo &frameInfo);

  void reloadShaders() override;

 private:
  void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
  void createPipeline(VkRenderPass renderPass);

  LveDevice &lveDevice;
  std::shared_ptr<VkRenderPass> renderPass;
  std::unique_ptr<LvePipeline> lvePipeline;
  VkPipelineLayout pipelineLayout;
};
}  // namespace lve
