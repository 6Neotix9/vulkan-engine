#include "wave_IHFFT.hpp"

#include "lve_swap_chain.hpp"
namespace lve {
WaveIHFFT::WaveIHFFT(
    LveDevice &device,
    std::vector<std::shared_ptr<LveImage>> buffer0,
    std::vector<std::shared_ptr<LveImage>> buffer1,
    std::shared_ptr<LveImage> preComputeData)
    : LveASystem(device, "wave_textureInverseHorizontalFFT", nullptr),
      buffer0{buffer0},
      buffer1{buffer1},
      precomputeData{preComputeData} {
    height = 512;
    width = 512;

    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSet();
}
WaveIHFFT::~WaveIHFFT() { vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr); }

void WaveIHFFT::createDescriptorPool() {
    waveIHFFTPool =
        LveDescriptorPool::Builder(lveDevice)
            .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();
}

void WaveIHFFT::createDescriptorSetLayout() {
    waveIHFFTLayout =
        LveDescriptorSetLayout::Builder(lveDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .build();
}

void WaveIHFFT::createDescriptorSet() {
    waveIHFFTDescriptorSets.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);

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

        LveDescriptorWriter(*waveIHFFTLayout, *waveIHFFTPool)
            .writeImage(0, &bufferDescriptorInfo0)
            .writeImage(1, &bufferDescriptorInfo1)
            .writeImage(2, &precomputeDataDescriptorInfo)
            .build(waveIHFFTDescriptorSets[i]);
    }
}

}  // namespace lve