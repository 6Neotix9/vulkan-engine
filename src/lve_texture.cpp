#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "lve_texture.hpp"
#include "lve_buffer.hpp"
#include "lve_swap_chain.hpp"

#include <cassert>
#include <cstring>
#include <unordered_map>
#include <stdexcept>
#include <iostream>
#include <filesystem>

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

namespace lve
{

    LveTexture::LveTexture(LveDevice &device, const std::string &filepath, bool isComputeTexture) : lveDevice{device}
    {
        if (isComputeTexture)
        {
            computeTextureConstructor(filepath);
        }
        else
        {
            objectTextureConstructor(filepath);
        }
    }

    LveTexture::LveTexture(LveDevice &device, int width , int height) : lveDevice{device}
    {
        postprocessingTextureConstructor(width, height);
    }

    void LveTexture::postprocessingTextureConstructor(int width, int height){

        LveBuffer stagingBuffer{lveDevice, 4,
                                static_cast<u_int32_t>(width * height),
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

        imageFormat = VK_FORMAT_R8G8B8A8_UNORM;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = imageFormat;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

        lveDevice.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage,
            textureImageMemory);

        

        transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        // lveDevice.copyBufferToImage(stagingBuffer.getBuffer(), textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1, imageLayout);


        

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST; // VK_FILTER_LINEAR
        samplerInfo.minFilter = VK_FILTER_NEAREST; // VK_FILTER_LINEAR
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // VK_SAMPLER_ADDRESS_MODE_REPEAT
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT; // VK_SAMPLER_ADDRESS_MODE_REPEAT
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT; // VK_SAMPLER_ADDRESS_MODE_REPEAT
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.compareOp = VK_COMPARE_OP_NEVER; // VK_COMPARE_OP_NEVER
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;
        samplerInfo.maxAnisotropy = 4.0f;
        samplerInfo.anisotropyEnable = VK_TRUE;                     // VK_FALSE
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE; // VK_BORDER_COLOR_INT_OPAQUE_BLACK


        vkCreateSampler(lveDevice.device(), &samplerInfo, nullptr, &sampler);

        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // VK_IMAGE_VIEW_TYPE_2D
        imageViewInfo.format = imageFormat;
        imageViewInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A}; // VK_COMPONENT_SWIZZLE_IDENTITY
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;                                                       // VK_IMAGE_ASPECT_COLOR_BIT
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.image = textureImage;


        vkCreateImageView(lveDevice.device(), &imageViewInfo, nullptr, &imageView);


    }

    void LveTexture::objectTextureConstructor(const std::string &filepath)
    {
        int width, height, channels;
        int byPerPixel;

        stbi_uc *pixels = stbi_load((ENGINE_DIR + filepath).c_str(), &width, &height, &byPerPixel, 4);
        LveBuffer stagingBuffer{lveDevice, 4,
                                static_cast<u_int32_t>(width * height),
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
        stagingBuffer.map();
        stagingBuffer.writeToBuffer(pixels);
        imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = imageFormat;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        lveDevice.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage,
            textureImageMemory);

        transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        lveDevice.copyBufferToImage(stagingBuffer.getBuffer(), textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1);

        transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST; // VK_FILTER_LINEAR
        samplerInfo.minFilter = VK_FILTER_NEAREST; // VK_FILTER_LINEAR
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // VK_SAMPLER_ADDRESS_MODE_REPEAT
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT; // VK_SAMPLER_ADDRESS_MODE_REPEAT
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT; // VK_SAMPLER_ADDRESS_MODE_REPEAT
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.compareOp = VK_COMPARE_OP_NEVER; // VK_COMPARE_OP_NEVER
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;
        samplerInfo.maxAnisotropy = 4.0f;
        samplerInfo.anisotropyEnable = VK_TRUE;                     // VK_FALSE
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE; // VK_BORDER_COLOR_INT_OPAQUE_BLACK

        vkCreateSampler(lveDevice.device(), &samplerInfo, nullptr, &sampler);

        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // VK_IMAGE_VIEW_TYPE_2D
        imageViewInfo.format = imageFormat;
        imageViewInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A}; // VK_COMPONENT_SWIZZLE_IDENTITY
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;                                                       // VK_IMAGE_ASPECT_COLOR_BIT
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.image = textureImage;

        vkCreateImageView(lveDevice.device(), &imageViewInfo, nullptr, &imageView);

        stbi_image_free(pixels);
    }

    void LveTexture::computeTextureConstructor(const std::string &filepath)
    {
        int width, height, channels;
        int byPerPixel;


        stbi_uc *pixels = stbi_load((ENGINE_DIR + filepath).c_str(), &width, &height, &byPerPixel, 4);
        LveBuffer stagingBuffer{lveDevice, 4,
                                static_cast<u_int32_t>(width * height),
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
        stagingBuffer.map();
        stagingBuffer.writeToBuffer(pixels);
        imageFormat = VK_FORMAT_R8G8B8A8_UNORM;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = imageFormat;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        lveDevice.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage,
            textureImageMemory);



        transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        lveDevice.copyBufferToImage(stagingBuffer.getBuffer(), textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1, imageLayout);


        

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST; // VK_FILTER_LINEAR
        samplerInfo.minFilter = VK_FILTER_NEAREST; // VK_FILTER_LINEAR
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // VK_SAMPLER_ADDRESS_MODE_REPEAT
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT; // VK_SAMPLER_ADDRESS_MODE_REPEAT
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT; // VK_SAMPLER_ADDRESS_MODE_REPEAT
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.compareOp = VK_COMPARE_OP_NEVER; // VK_COMPARE_OP_NEVER
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;
        samplerInfo.maxAnisotropy = 4.0f;
        samplerInfo.anisotropyEnable = VK_TRUE;                     // VK_FALSE
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE; // VK_BORDER_COLOR_INT_OPAQUE_BLACK


        vkCreateSampler(lveDevice.device(), &samplerInfo, nullptr, &sampler);

        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // VK_IMAGE_VIEW_TYPE_2D
        imageViewInfo.format = imageFormat;
        imageViewInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A}; // VK_COMPONENT_SWIZZLE_IDENTITY
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;                                                       // VK_IMAGE_ASPECT_COLOR_BIT
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.image = textureImage;


        vkCreateImageView(lveDevice.device(), &imageViewInfo, nullptr, &imageView);

        stbi_image_free(pixels);
    }

    void LveTexture::transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkCommandBuffer commandBuffer = lveDevice.beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;                         // VK_IMAGE_LAYOUT_UNDEFINED
        barrier.newLayout = newLayout;                         // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // VK_QUEUE_FAMILY_IGNORED
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // VK_QUEUE_FAMILY_IGNORED
        barrier.image = textureImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // VK_IMAGE_ASPECT_COLOR_BIT
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;                            // VK_ACCESS_TRANSFER_WRITE_BIT
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // 0

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;   // VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT; // VK_PIPELINE_STAGE_TRANSFER_BIT
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // 0
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;    // 0

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;             // VK_PIPELINE_STAGE_TRANSFER_BIT
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        }
        else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        lveDevice.endSingleTimeCommands(commandBuffer);
    }

    LveTexture::~LveTexture()
    {
        vkDestroyImage(lveDevice.device(), textureImage, nullptr);
        vkFreeMemory(lveDevice.device(), textureImageMemory, nullptr);
        vkDestroyImageView(lveDevice.device(), imageView, nullptr);
        vkDestroySampler(lveDevice.device(), sampler, nullptr);
    }

}