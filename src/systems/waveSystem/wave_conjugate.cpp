#include "wave_conjugate.hpp"
#include "lve_swap_chain.hpp"
namespace lve {
WaveConjugate::WaveConjugate(
    LveDevice &device,
    std::shared_ptr<LveImage> spectrumTexture,
    std::shared_ptr<LveImage> spectrumConjugateTexture)
    : LveASystem(device, "wave_texture_spectrumConjugated", nullptr),
      spectrumTexture{spectrumTexture},
      spectrumConjugateTexture{spectrumConjugateTexture} {
    height = 512;
    width = 512;

    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSet();
}
WaveConjugate::~WaveConjugate() { vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr); }

void WaveConjugate::createDescriptorPool() {
    wavePool = LveDescriptorPool::Builder(lveDevice)
                   .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                   .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                   .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                   .build();
}

void WaveConjugate::createDescriptorSetLayout() {
    waveGenSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
                           .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
                           .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
                           .build();
}

void WaveConjugate::createDescriptorSet() {
    VkDescriptorImageInfo imageSpectrumDescriptorInfo{};
    imageSpectrumDescriptorInfo.imageView = spectrumTexture->getVkImageView();
    imageSpectrumDescriptorInfo.imageLayout = spectrumTexture->getVkImageLayout();

    VkDescriptorImageInfo imageSpectrumConjDescriptorInfo1{};
    imageSpectrumConjDescriptorInfo1.imageView = spectrumConjugateTexture->getVkImageView();
    imageSpectrumConjDescriptorInfo1.imageLayout = spectrumConjugateTexture->getVkImageLayout();

    LveDescriptorWriter(*waveGenSetLayout, *wavePool)
        .writeImage(0, &imageSpectrumDescriptorInfo)
        .writeImage(1, &imageSpectrumConjDescriptorInfo1)
        .build(waveConjugateDescriptorSets);
}

}  // namespace lve