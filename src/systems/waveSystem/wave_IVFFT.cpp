#include "wave_IVFFT.hpp"

#include "lve_swap_chain.hpp"
namespace lve {
WaveIVFFT::WaveIVFFT(
    LveDevice &device,
    std::vector<std::shared_ptr<LveImage>> buffer0,
    std::vector<std::shared_ptr<LveImage>> buffer1,
    std::shared_ptr<LveImage> preComputeData)
    : LveASystem(device, "wave_textureInverseVerticalFFT", nullptr),
      buffer0{buffer0},
      buffer1{buffer1},
      precomputeData{preComputeData} {
    height = 512;
    width = 512;

    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSet();
}
WaveIVFFT::~WaveIVFFT() { vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr); }

void WaveIVFFT::createDescriptorPool() {
    waveIVFFTPool =
        LveDescriptorPool::Builder(lveDevice)
            .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();
}

void WaveIVFFT::createDescriptorSetLayout() {
    waveIVFFTLayout =
        LveDescriptorSetLayout::Builder(lveDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)        
            .build();
}

void WaveIVFFT::createDescriptorSet() {
    waveIVFFTDescriptorSets.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo bufferDescriptorInfo0{};
        bufferDescriptorInfo0.imageView = buffer0[i]->getVkImageView();
        bufferDescriptorInfo0.imageLayout = buffer0[i]->getVkImageLayout();

        VkDescriptorImageInfo bufferDescriptorInfo1{};
        bufferDescriptorInfo1.imageView = buffer1[i]->getVkImageView();
        bufferDescriptorInfo1.imageLayout = buffer1[i]->getActualImageLayout();

        VkDescriptorImageInfo precomputeDataDescriptorInfo{};
        precomputeDataDescriptorInfo.imageView = precomputeData->getVkImageView();
        precomputeDataDescriptorInfo.imageLayout = precomputeData->getVkImageLayout();

        LveDescriptorWriter(*waveIVFFTLayout, *waveIVFFTPool)
            .writeImage(0, &bufferDescriptorInfo0)
            .writeImage(1, &bufferDescriptorInfo1)
            .writeImage(2, &precomputeDataDescriptorInfo)
            .build(waveIVFFTDescriptorSets[i]);
    }
}

}  // namespace lve