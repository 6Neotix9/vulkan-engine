#pragma once

#include "lve_device.hpp"
#include "lve_image.hpp"
#include "lve_pipeline_ressources.hpp"

// vulkan headers
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

// std lib headers
#include <memory>
#include <vector>

namespace lve {

class LveSwapChain {
   public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    LveSwapChain(
        LveDevice *deviceRef,
        VkExtent2D windowExtent,
        std::shared_ptr<LvePipelineRessources> &renderPass);
    LveSwapChain(
        LveDevice *deviceRef,
        VkExtent2D windowExtent,
        std::shared_ptr<LveSwapChain> previous,
        std::shared_ptr<LvePipelineRessources> &renderPass);

    ~LveSwapChain();

    LveSwapChain(const LveSwapChain &) = delete;
    LveSwapChain &operator=(const LveSwapChain &) = delete;

    VkFramebuffer getFrameBuffer(int index) { return pipelineRessources->getFrameBuffer(index); }
    VkImageView getImageView(int index) { return swapChainImageViews[index]; }
    size_t imageCount() { return swapChainImages.size(); }
    VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
    VkExtent2D getSwapChainExtent() { return swapChainExtent; }
    uint32_t width() { return swapChainExtent.width; }
    uint32_t height() { return swapChainExtent.height; }
    std::shared_ptr<LvePipelineRessources> getPipelineRessources() { return pipelineRessources; }
    
    int getFrameIndex() { return currentFrame; }

    float extentAspectRatio() {
        return static_cast<float>(swapChainExtent.width) /
               static_cast<float>(swapChainExtent.height);
    }
    VkFormat findDepthFormat();

    VkResult acquireNextImage(uint32_t *imageIndex);
    VkResult submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex);
    VkResult presentFrame(uint32_t *imageIndex);

    bool compareSwapFormats(const LveSwapChain &swapChain) const {
        return swapChain.swapChainDepthFormat == swapChainDepthFormat &&
               swapChain.swapChainImageFormat == swapChainImageFormat;
    }

   private:
    void init();
    void createSwapChain();
    void createImageViews();
    void createSyncObjects();
    void createLveImages();

    // Helper functions
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

    

    VkFormat swapChainImageFormat;
    VkFormat swapChainDepthFormat;
    VkExtent2D swapChainExtent;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    LveDevice *device;
    VkExtent2D windowExtent;

    VkSwapchainKHR swapChain;
    std::shared_ptr<LveSwapChain> oldSwapChain;

    std::vector<std::shared_ptr<LveImage>> LveSwapChainImages;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;

    std::shared_ptr<LvePipelineRessources> pipelineRessources;
};

}  // namespace lve
