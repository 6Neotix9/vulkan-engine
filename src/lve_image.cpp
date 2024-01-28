#include "lve_image.hpp"

#include <sys/types.h>
#include <vulkan/vulkan_core.h>

#include <cmath>
#include <cstdint>
#include <stdexcept>

#include "lve_buffer.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

namespace lve {

LveImage::LveImage(LveDevice* device, ImageCreateInfo& imageCreateInfo) : lveDevice{device} {
    if (!imageCreateInfo.filename.empty()) {
        loadImageFromFile(imageCreateInfo);
    }
    this->imageFormat = imageCreateInfo.format;
    this->width = imageCreateInfo.width;
    this->height = imageCreateInfo.height;
    createImage(imageCreateInfo);
    if (imageCreateInfo.pixels != 0) {
        loadImageToGPU(imageCreateInfo);
    }
    createImageView(imageCreateInfo);
    createSampler(imageCreateInfo);
}

LveImage::~LveImage() {
    if (sampler != VK_NULL_HANDLE) vkDestroySampler(lveDevice->device(), sampler, nullptr);

    if (imageView != VK_NULL_HANDLE) vkDestroyImageView(lveDevice->device(), imageView, nullptr);

    if (image != VK_NULL_HANDLE) vkDestroyImage(lveDevice->device(), image, nullptr);
    vkFreeMemory(lveDevice->device(), imageMemory, nullptr);
}

void LveImage::createImage(ImageCreateInfo& imageCreateInfo) {
    if (imageCreateInfo.usageFlags & VK_IMAGE_USAGE_SAMPLED_BIT) {
        mipLevels = static_cast<uint32_t>(std::floor(
                        std::log2(std::max(imageCreateInfo.width, imageCreateInfo.width)))) +
                    1;
        imageCreateInfo.usageFlags |=
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (imageCreateInfo.usageFlags & VK_IMAGE_USAGE_STORAGE_BIT) {
        imageCreateInfo.usageFlags |=
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = imageCreateInfo.format;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = imageCreateInfo.usageFlags;

    imageInfo.extent = {(uint32_t)imageCreateInfo.width, (uint32_t)imageCreateInfo.height, 1};

    lveDevice
        ->createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);
    actualImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

void LveImage::createImageView(ImageCreateInfo& imageCreateInfo) {
    VkImageViewCreateInfo imageViewInfo{};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;  // VK_IMAGE_VIEW_TYPE_2D
    imageViewInfo.format = imageFormat;
    // imageViewInfo.components = {
    //     VK_COMPONENT_SWIZZLE_R,
    //     VK_COMPONENT_SWIZZLE_G,
    //     VK_COMPONENT_SWIZZLE_B,
    //     VK_COMPONENT_SWIZZLE_A};  // VK_COMPONENT_SWIZZLE_IDENTITY
    imageViewInfo.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_COLOR_BIT;  // VK_IMAGE_ASPECT_COLOR_BIT
    if (imageCreateInfo.usageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;
    imageViewInfo.subresourceRange.levelCount = mipLevels;
    imageViewInfo.image = image;
    vkCreateImageView(lveDevice->device(), &imageViewInfo, nullptr, &imageView);
}

void LveImage::createSampler(ImageCreateInfo& imageCreateInfo) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;  // VK_FILTER_LINEAR
    samplerInfo.minFilter = VK_FILTER_LINEAR;  // VK_FILTER_LINEAR
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // VK_SAMPLER_ADDRESS_MODE_REPEAT
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // VK_SAMPLER_ADDRESS_MODE_REPEAT
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // VK_SAMPLER_ADDRESS_MODE_REPEAT
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;  // VK_COMPARE_OP_NEVER
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipLevels);
    samplerInfo.maxAnisotropy = 4.0f;
    samplerInfo.anisotropyEnable = VK_TRUE;                      // VK_FALSE
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;  // VK_BORDER_COLOR_INT_OPAQUE_BLACK
    vkCreateSampler(lveDevice->device(), &samplerInfo, nullptr, &sampler);
}

void LveImage::loadImageFromFile(ImageCreateInfo& imageCreateInfo) {
    stbi_set_flip_vertically_on_load(true);
    int byPerPixel;
    int width, height;
    imageCreateInfo.pixels =
        stbi_load((ENGINE_DIR + imageCreateInfo.filename).c_str(), &width, &height, &byPerPixel, 4);
    imageCreateInfo.width = width;
    imageCreateInfo.height = height;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
}

void LveImage::loadImageToGPU(ImageCreateInfo& imageCreateInfo) {
    VkDeviceSize imageSize = width * height * getPixelSizeFromFormat(imageCreateInfo.format);
    LveBuffer stagingBuffer{
        *lveDevice,
        getPixelSizeFromFormat(imageCreateInfo.format),
        static_cast<u_int32_t>(width * height),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

    stagingBuffer.map();
    stagingBuffer.writeToBuffer(imageCreateInfo.pixels);

    transitionImageLayout(actualImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, nullptr);

    lveDevice->copyBufferToImage(
        stagingBuffer.getBuffer(),
        image,
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        1);

    // transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageLayout, nullptr);
    if (!imageCreateInfo.filename.empty()) stbi_image_free(imageCreateInfo.pixels);
}

void LveImage::generateMipmaps() {
    VkCommandBuffer commandBuffer = lveDevice->beginSingleTimeCommands();

    transitionImageLayout(
        actualImageLayout,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        0,
        1,
        commandBuffer);

    for (uint32_t i = 1; i < mipLevels; i++) {
        VkImageBlit imageBlit{};

        // Source
        imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.srcSubresource.layerCount = 1;
        imageBlit.srcSubresource.mipLevel = i - 1;
        imageBlit.srcOffsets[1].x = int32_t(this->width >> (i - 1));
        imageBlit.srcOffsets[1].y = int32_t(this->height >> (i - 1));
        imageBlit.srcOffsets[1].z = 1;

        // Destination
        imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.dstSubresource.layerCount = 1;
        imageBlit.dstSubresource.mipLevel = i;
        imageBlit.dstOffsets[1].x = int32_t(this->width >> i);
        imageBlit.dstOffsets[1].y = int32_t(this->height >> i);
        imageBlit.dstOffsets[1].z = 1;

        VkImageSubresourceRange mipSubRange = {};
        mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        mipSubRange.baseMipLevel = i;
        mipSubRange.levelCount = 1;
        mipSubRange.layerCount = 1;

        // Prepare current mip level as image blit destination
        transitionImageLayout(
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            i,
            1,
            commandBuffer);

        // Blit from previous level
        vkCmdBlitImage(
            commandBuffer,
            this->image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            this->image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &imageBlit,
            VK_FILTER_LINEAR);

        // // Prepare current mip level as image blit source for next level
        transitionImageLayout(
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            i,
            1,
            commandBuffer);
    }

    transitionImageLayout(
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        commandBuffer);
    lveDevice->endSingleTimeCommands(commandBuffer);
}

void LveImage::transitionImageLayout(
    VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer externalCommandBuffer) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;                          // VK_IMAGE_LAYOUT_UNDEFINED
    barrier.newLayout = newLayout;                          // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // VK_QUEUE_FAMILY_IGNORED
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // VK_QUEUE_FAMILY_IGNORED
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // VK_IMAGE_ASPECT_COLOR_BIT
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = this->mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    barrier.srcAccessMask = getAccessFlagsFromLayout(oldLayout);
    barrier.dstAccessMask = getAccessFlagsFromLayout(newLayout);

    // TODO : GET CORRECT STATE FLAGS
    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    if (externalCommandBuffer == nullptr) {
        VkCommandBuffer commandBuffer = lveDevice->beginSingleTimeCommands();
        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage,
            destinationStage,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);
        lveDevice->endSingleTimeCommands(commandBuffer);
    } else {
        vkCmdPipelineBarrier(
            externalCommandBuffer,
            sourceStage,
            destinationStage,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);
    }
    actualImageLayout = newLayout;
}

void LveImage::transitionImageLayout(
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    u_int32_t mipLevel,
    u_int32_t mipCount,
    VkCommandBuffer externalCommandBuffer) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;                          // VK_IMAGE_LAYOUT_UNDEFINED
    barrier.newLayout = newLayout;                          // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // VK_QUEUE_FAMILY_IGNORED
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // VK_QUEUE_FAMILY_IGNORED
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // VK_IMAGE_ASPECT_COLOR_BIT
    barrier.subresourceRange.baseMipLevel = mipLevel;
    barrier.subresourceRange.levelCount = mipCount;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    barrier.srcAccessMask = getAccessFlagsFromLayout(oldLayout);
    barrier.dstAccessMask = getAccessFlagsFromLayout(newLayout);

    // TODO : GET CORRECT STATE FLAGS
    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    if (externalCommandBuffer == nullptr) {
        VkCommandBuffer commandBuffer = lveDevice->beginSingleTimeCommands();
        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage,
            destinationStage,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);
        lveDevice->endSingleTimeCommands(commandBuffer);
    } else {
        vkCmdPipelineBarrier(
            externalCommandBuffer,
            sourceStage,
            destinationStage,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);
    }
}

unsigned int LveImage::getPixelSizeFromFormat(VkFormat format) {
    switch (format) {
        case VK_FORMAT_UNDEFINED:
            throw std::runtime_error("Undefined image format");
            break;

        case VK_FORMAT_R4G4_UNORM_PACK8:
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_USCALED:
        case VK_FORMAT_R8_SSCALED:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_SRGB:
            return 1;
            break;

        case VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR:
        case VK_FORMAT_R10X6_UNORM_PACK16:
        case VK_FORMAT_R12X4_UNORM_PACK16:
        case VK_FORMAT_A4R4G4B4_UNORM_PACK16:
        case VK_FORMAT_A4B4G4R4_UNORM_PACK16:
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
        case VK_FORMAT_B5G6R5_UNORM_PACK16:
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
        case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_USCALED:
        case VK_FORMAT_R8G8_SSCALED:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8_SRGB:
        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_USCALED:
        case VK_FORMAT_R16_SSCALED:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_SFLOAT:
            return 2;
            break;

        case VK_FORMAT_A8_UNORM_KHR:
            return 1;
            break;

        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_USCALED:
        case VK_FORMAT_R8G8B8_SSCALED:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_UNORM:
        case VK_FORMAT_B8G8R8_SNORM:
        case VK_FORMAT_B8G8R8_USCALED:
        case VK_FORMAT_B8G8R8_SSCALED:
        case VK_FORMAT_B8G8R8_UINT:
        case VK_FORMAT_B8G8R8_SINT:
        case VK_FORMAT_B8G8R8_SRGB:
            return 3;
            break;

        case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
        case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
        case VK_FORMAT_R16G16_S10_5_NV:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_USCALED:
        case VK_FORMAT_R8G8B8A8_SSCALED:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SNORM:
        case VK_FORMAT_B8G8R8A8_USCALED:
        case VK_FORMAT_B8G8R8A8_SSCALED:
        case VK_FORMAT_B8G8R8A8_UINT:
        case VK_FORMAT_B8G8R8A8_SINT:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:
        case VK_FORMAT_A2B10G10R10_SINT_PACK32:
        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_USCALED:
        case VK_FORMAT_R16G16_SSCALED:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_SFLOAT:
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
            return 4;
            break;

        case VK_FORMAT_R16G16B16_UNORM:
        case VK_FORMAT_R16G16B16_SNORM:
        case VK_FORMAT_R16G16B16_USCALED:
        case VK_FORMAT_R16G16B16_SSCALED:
        case VK_FORMAT_R16G16B16_UINT:
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16_SFLOAT:
            return 6;
            break;

        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_USCALED:
        case VK_FORMAT_R16G16B16A16_SSCALED:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
        case VK_FORMAT_R64_UINT:
        case VK_FORMAT_R64_SINT:
        case VK_FORMAT_R64_SFLOAT:
            return 8;
            break;

        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
            return 12;
            break;

        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        case VK_FORMAT_R64G64_UINT:
        case VK_FORMAT_R64G64_SINT:
        case VK_FORMAT_R64G64_SFLOAT:
            return 16;
            break;

        case VK_FORMAT_R64G64B64_UINT:
        case VK_FORMAT_R64G64B64_SINT:
        case VK_FORMAT_R64G64B64_SFLOAT:
            return 24;
            break;

        case VK_FORMAT_R64G64B64A64_UINT:
        case VK_FORMAT_R64G64B64A64_SINT:
        case VK_FORMAT_R64G64B64A64_SFLOAT:
            return 32;
            break;

        default:
            throw std::runtime_error("Unsupported image format");
            break;
    }
}

VkAccessFlags LveImage::getAccessFlagsFromLayout(VkImageLayout layout) {
    switch (layout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            return 0;
            break;
        case VK_IMAGE_LAYOUT_GENERAL:
            return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                   VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_ACCESS_SHADER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_ACCESS_TRANSFER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return VK_ACCESS_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            return VK_ACCESS_HOST_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL:
            return VK_ACCESS_SHADER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR:
            return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        default:
            throw std::runtime_error("Unsupported image layout");
            break;
    }
}

void LveImage::copyImage(LveDevice& device, LveImage& srcImage, LveImage& dstImage) {
    VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

    VkImageLayout srcImageLayout = srcImage.getVkImageLayout();
    VkImageLayout dstImageLayout = dstImage.getVkImageLayout();

    srcImage.transitionImageLayout(
        srcImageLayout,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        commandBuffer);
    dstImage.transitionImageLayout(
        dstImageLayout,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        commandBuffer);

    VkImageBlit blit{};
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = {srcImage.getWidth(), srcImage.getHeight(), 1};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = 0;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1] = {srcImage.getWidth(), srcImage.getHeight(), 1};
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = 0;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    vkCmdBlitImage(
        commandBuffer,
        srcImage.getVkImage(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstImage.getVkImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &blit,
        VK_FILTER_LINEAR);

    srcImage.transitionImageLayout(
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        srcImageLayout,
        commandBuffer);
    dstImage.transitionImageLayout(
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        dstImageLayout,
        commandBuffer);

    device.endSingleTimeCommands(commandBuffer);
}
}  // namespace lve