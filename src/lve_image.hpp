#pragma once
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "lve_device.hpp"

namespace lve {

struct ImageCreateInfo {
    int32_t width;
    int32_t height;
    VkFormat format;
    VkImageUsageFlags usageFlags;
    VkImageLayout imageLayout;
    std::string filename;
    void* pixels;
};

class LveImage {
   public:
    // Functions
    LveImage(LveDevice* device, ImageCreateInfo& imageCreateInfo);
    LveImage(LveDevice* device, VkImage image, VkImageView imageView, VkFormat format, VkExtent2D swapChainExtent);

    ~LveImage();
    void transitionImageLayout(
        VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuffer);
    void transitionImageLayout(
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        u_int32_t mipLevel,
        u_int32_t mipCount,
        VkCommandBuffer commandBuffer);

    // Getters
    VkImage getVkImage() const { return image; }
    VkImageView getVkImageView() const { return imageView; }
    VkSampler getVkSampler() const { return sampler; }
    VkImageLayout getVkImageLayout() const { return imageLayout; }
    VkImageLayout getActualImageLayout() const { return actualImageLayout; }
    int32_t getWidth() const { return width; }
    int32_t getHeight() const { return height; }
    VkFormat getFormat() const { return imageFormat; }

    // Static Functions
    static void copyImage(LveDevice& device, LveImage& srcImage, LveImage& dstImage, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

   private:
    int32_t width = 0;
    int32_t height = 0;
    uint32_t mipLevels = 1;
    LveDevice* lveDevice;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkFormat imageFormat;
    VkImageLayout imageLayout;
    VkImageLayout actualImageLayout;

    bool isSwapchainImage;

    void createImage(ImageCreateInfo& imageCreateInfo);
    void createImageView(ImageCreateInfo& imageCreateInfo);
    void createSampler(ImageCreateInfo& imageCreateInfo);

    void generateMipmaps();

    void loadImageFromFile(ImageCreateInfo& imageCreateInfo);
    void loadImageToGPU(ImageCreateInfo& imageCreateInfo);

    static unsigned int getPixelSizeFromFormat(VkFormat format);
    static VkAccessFlags getAccessFlagsFromLayout(VkImageLayout layout);
};
}  // namespace lve