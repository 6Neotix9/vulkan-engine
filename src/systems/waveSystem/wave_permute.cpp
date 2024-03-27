#include "wave_permute.hpp"

#include "lve_swap_chain.hpp"
namespace lve {
WavePermute::WavePermute(LveDevice &device, std::vector<std::shared_ptr<LveImage>> buffer0)
    : LveASystem(device, "wave_texturePermute", nullptr), buffer0{buffer0} {
    height = 512;
    width = 512;

    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSet();
}
WavePermute::~WavePermute() {
    vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
}

void WavePermute::createDescriptorPool() {
    wavePermutePool =
        LveDescriptorPool::Builder(lveDevice)
            .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();
}

void WavePermute::createDescriptorSetLayout() {
    wavePermuteLayout =
        LveDescriptorSetLayout::Builder(lveDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .build();
}

void WavePermute::createDescriptorSet() {
    wavePermuteDescriptorSets.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo bufferDescriptorInfo0{};
        bufferDescriptorInfo0.imageView = buffer0[i]->getVkImageView();
        bufferDescriptorInfo0.imageLayout = buffer0[i]->getVkImageLayout();

        LveDescriptorWriter(*wavePermuteLayout, *wavePermutePool)
            .writeImage(0, &bufferDescriptorInfo0)
            .build(wavePermuteDescriptorSets[i]);
    }
}

}  // namespace lve