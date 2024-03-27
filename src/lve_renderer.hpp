#pragma once

#include <sys/types.h>
#include <vulkan/vulkan_core.h>
#include "lve_device.hpp"
#include "lve_frame_info.hpp"
#include "lve_pipeline_ressources.hpp"
#include "lve_swap_chain.hpp"
#include "lve_window.hpp"

// std
#include <cassert>
#include <memory>
#include <vector>

namespace lve {
class LveRenderer {
 public:
  LveRenderer(LveWindow &window, LveDevice *device);
  ~LveRenderer();

  LveRenderer(const LveRenderer &) = delete;
  LveRenderer &operator=(const LveRenderer &) = delete;

  std::shared_ptr<LvePipelineRessources> getSwapchainPipeLineRessources(){return pipelineRessources;}
  float getAspectRatio() const { return lveSwapChain->extentAspectRatio(); }
  bool isFrameInProgress() const { return isFrameStarted; }
VkExtent2D getSwapChainExtent() const { return lveSwapChain->getSwapChainExtent(); }
  VkCommandBuffer getCurrentCommandBuffer() const {
    assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
    return commandBuffers[currentFrameIndex];
  }
  VkCommandBuffer getCurrentPreProcessCommandBuffer() const {
    assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
    return preProcessCommandBuffers[currentFrameIndex];
  }

  int getFrameIndex() const {
    assert(isFrameStarted && "Cannot get frame index when frame not in progress");
    return currentFrameIndex;
  }
  uint getSwapChainImageIndex () const {
    assert(isFrameStarted && "Cannot get swap chain image index when frame not in progress");
    return currentImageIndex;
  }

  frameCommandBuffers beginFrame();

  void startRenderFrame();

  void endRenderFrame();

  void startPreProcess();
  void endPreProcess();

  void presentFrame();

  void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
  void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

  

 private:
  void createCommandBuffers();
  void freeCommandBuffers();
  void recreateSwapChain();
  void createSemaphore();

  LveWindow &lveWindow;
  LveDevice *lveDevice;
  std::unique_ptr<LveSwapChain> lveSwapChain;
  std::vector<VkCommandBuffer> commandBuffers;

  std::vector<VkCommandBuffer> preProcessCommandBuffers;
  std::vector<VkSemaphore> preComputeFinishedSemaphores;



  std::shared_ptr<LvePipelineRessources> pipelineRessources;

  uint32_t currentImageIndex;
  int currentFrameIndex{0};
  bool isFrameStarted{false};
};
}  // namespace lve
