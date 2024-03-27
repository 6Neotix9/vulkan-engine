#include "wave_spectrum.hpp"
#include <vulkan/vulkan_core.h>

#include <fstream>
#include <sstream>

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

namespace lve {

std::vector<float> loadNoise() {
    const std::string filePath = "textures/noise.csv";

    // Open the file
    std::ifstream inputFile(ENGINE_DIR + filePath);
    // Check if the file is open
    if (!inputFile.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    // Vector to store the float values
    std::vector<float> dataVector;

    // Read each line from the file
    std::string line;
    while (std::getline(inputFile, line)) {
        // Use a stringstream to extract float values between quotes
        std::stringstream ss(line);
        std::string token;
        while (std::getline(ss, token, ',')) {
            // Convert the string to float and push it to the vector
            float value = std::stof(token);
            dataVector.push_back(value);
        }
    }

    // Close the file
    inputFile.close();
    return dataVector;
}

struct SimplePushConstantData {
    glm::vec2 resolution;
};

struct SpectrumParam {
    float scale;
    float angle;
    float spreadBlend;
    float swell;
    float alpha;
    float peakOmega;
    float gamma;
    float shortWavesFade;
};

struct waveGenData {
    SpectrumParam spectrums[2];
    float LengthScale;
    float CutoffLow;
    float CutoffHigh;
    uint Size;
};

WaveSpectrum::WaveSpectrum(
    LveDevice &device,
    std::shared_ptr<LveImage> waveTexture,
    std::shared_ptr<LveImage> waveDataTexture,
    float LengthScale,
    float CutoffLow,
    float CutoffHigh)
    : LveASystem(device, "wave_textureSpectrum", nullptr),
      waveTexture{waveTexture},
      waveDataTexture{waveDataTexture} {
    height = 512;
    width = 512;
    ImageCreateInfo imageCreateInfo{};
    imageCreateInfo.width = width;
    imageCreateInfo.height = height;
    imageCreateInfo.pixels = loadNoise().data();
    imageCreateInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageCreateInfo.usageFlags = VK_IMAGE_USAGE_STORAGE_BIT;
    imageCreateInfo.format = VK_FORMAT_R32G32_SFLOAT;

    noiseTexture = std::make_shared<LveImage>(&lveDevice, imageCreateInfo);
    createWaveDataBuffer();
    setData(LengthScale, CutoffLow, CutoffHigh);
    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSet();
}
WaveSpectrum::~WaveSpectrum() {
    vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
}

void WaveSpectrum::createDescriptorPool() {
    waveSpectrumPool = LveDescriptorPool::Builder(lveDevice)
                           .setMaxSets(1)
                           .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
                           .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1)
                           .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1)
                           .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1)
                           .build();
}

void WaveSpectrum::createWaveDataBuffer() {
    waveGenDataBuffers = std::make_unique<LveBuffer>(
        lveDevice, sizeof(waveGenData), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    waveGenDataBuffers->map();
}

void WaveSpectrum::createDescriptorSetLayout() {
    waveSpectrumLayout =
        LveDescriptorSetLayout::Builder(lveDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .build();
}

void WaveSpectrum::createDescriptorSet() {
    VkDescriptorImageInfo imageNoiseDescriptorInfo{};
    imageNoiseDescriptorInfo.imageView = noiseTexture->getVkImageView();
    imageNoiseDescriptorInfo.imageLayout = noiseTexture->getVkImageLayout();

    VkDescriptorImageInfo imageWaveDescriptorInfo{};
    imageWaveDescriptorInfo.imageView = waveTexture->getVkImageView();
    imageWaveDescriptorInfo.imageLayout = waveTexture->getVkImageLayout();

    VkDescriptorImageInfo imageWaveDataDescriptorInfo{};
    imageWaveDataDescriptorInfo.imageView = waveDataTexture->getVkImageView();
    imageWaveDataDescriptorInfo.imageLayout = waveDataTexture->getVkImageLayout();

    auto bufferInfo = waveGenDataBuffers->descriptorInfo();
    LveDescriptorWriter(*waveSpectrumLayout, *waveSpectrumPool)
        .writeBuffer(0, &bufferInfo)
        .writeImage(1, &imageNoiseDescriptorInfo)
        .writeImage(2, &imageWaveDescriptorInfo)
        .writeImage(3, &imageWaveDataDescriptorInfo)
        .build(waveSpectrumDescriptorSets);
}

float JonswapAlpha(float g, float fetch, float windSpeed) {
    return 0.076f * pow(g * fetch / windSpeed / windSpeed, -0.22f);
}

float JonswapPeakFrequency(float g, float fetch, float windSpeed) {
    return 22 * pow(windSpeed * fetch / g / g, -0.33f);
}

void WaveSpectrum::setData(float LengthScale, float CutoffLow, float CutoffHigh) {
    SpectrumParam spectrum1;
    SpectrumParam spectrum2;

    spectrum1.scale = 0.5;
    spectrum1.angle = -29.81 / 180.0 * M_PI;
    spectrum1.spreadBlend = 1;
    spectrum1.swell = glm::clamp(0.198f, 0.01f, 1.f);
    spectrum1.alpha = JonswapAlpha(9.81f, 100000.f, 0.5f);
    spectrum1.peakOmega = JonswapPeakFrequency(9.81f, 100000.f, 0.5f);
    spectrum1.gamma = 3.3;
    spectrum1.shortWavesFade = 0.01;

    spectrum2.scale = 0;
    spectrum2.angle = 0 / 180.0 * M_PI;
    spectrum2.spreadBlend = 1;
    spectrum2.swell = glm::clamp(1.f, 0.01f, 1.f);
    spectrum2.alpha = JonswapAlpha(9.81f, 300000.f, 1.f);
    spectrum2.peakOmega = JonswapPeakFrequency(9.81f, 300000.f, 1.f);
    spectrum2.gamma = 3.3;
    spectrum2.shortWavesFade = 0.01;

    waveGenData waveGenDataVar;
    waveGenDataVar.LengthScale = LengthScale;
    // waveGenDataVar.CutoffLow = 0.0001;
    // waveGenDataVar.CutoffHigh = 2.2f;
    waveGenDataVar.CutoffLow = CutoffLow;
    waveGenDataVar.CutoffHigh = CutoffHigh;
    waveGenDataVar.Size = 512;
    waveGenDataVar.spectrums[0] = spectrum1;
    waveGenDataVar.spectrums[1] = spectrum2;

    waveGenDataBuffers->writeToBuffer(&waveGenDataVar);
    waveGenDataBuffers->flush();
}

void WaveSpectrum::executeShader(FrameInfo &frameInfo) {
    VkCommandBuffer commandBuffer = frameInfo.commandBuffer.preProcessommandBuffer;

    lveCPipeline->bind(commandBuffer);
    SimplePushConstantData push{};
    push.resolution = glm::vec2(512, 512);
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(SimplePushConstantData), &push);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1,
                            &waveSpectrumDescriptorSets, 0, nullptr);

    vkCmdDispatch(commandBuffer, width / 32, height / 32, 1);
}

}  // namespace lve