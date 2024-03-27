#include "wave_merge.hpp"

#include "lve_swap_chain.hpp"
namespace lve {
WaveMerge::WaveMerge(
    LveDevice &device,
    std::vector<std::shared_ptr<LveImage>> Dx_Dz,
    std::vector<std::shared_ptr<LveImage>> Dy_Dxz,
    std::vector<std::shared_ptr<LveImage>> Dyx_Dyz,
    std::vector<std::shared_ptr<LveImage>> Dxx_Dzz,
    std::vector<std::shared_ptr<LveImage>> Displacement,
    std::vector<std::shared_ptr<LveImage>> Derivatives,
    std::vector<std::shared_ptr<LveImage>> Turbulence)
    : LveASystem(device, "wave_texture_merge", nullptr),
      Dx_Dz{Dx_Dz},
      Dy_Dxz{Dy_Dxz},
      Dyx_Dyz{Dyx_Dyz},
      Dxx_Dzz{Dxx_Dzz},
      Displacement{Displacement},
      Derivatives{Derivatives},
      Turbulence{Turbulence} {
    height = 512;
    width = 512;

    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSet();
}
WaveMerge::~WaveMerge() { vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr); }

void WaveMerge::createDescriptorPool() {
    waveMergePool =
        LveDescriptorPool::Builder(lveDevice)
            .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();
}

void WaveMerge::createDescriptorSetLayout() {
    waveMergeLayout =
        LveDescriptorSetLayout::Builder(lveDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(5, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(6, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .build();
}

void WaveMerge::createDescriptorSet() {
    waveMergeDescriptorSets.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);


    for (size_t i = 0; i < LveSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo Dx_DzDesc{};
        Dx_DzDesc.imageView = Dx_Dz[i]->getVkImageView();
        Dx_DzDesc.imageLayout = Dx_Dz[i]->getVkImageLayout();

        VkDescriptorImageInfo Dy_DxzDesc{};
        Dy_DxzDesc.imageView = Dy_Dxz[i]->getVkImageView();
        Dy_DxzDesc.imageLayout = Dy_Dxz[i]->getVkImageLayout();

        VkDescriptorImageInfo Dyx_DyzDesc{};
        Dyx_DyzDesc.imageView = Dyx_Dyz[i]->getVkImageView();
        Dyx_DyzDesc.imageLayout = Dyx_Dyz[i]->getVkImageLayout();

        VkDescriptorImageInfo Dxx_DzzDesc{};
        Dxx_DzzDesc.imageView = Dxx_Dzz[i]->getVkImageView();
        Dxx_DzzDesc.imageLayout = Dxx_Dzz[i]->getVkImageLayout();

        VkDescriptorImageInfo DisplacementorDesv{};
        DisplacementorDesv.imageView = Displacement[i]->getVkImageView();
        DisplacementorDesv.imageLayout = Displacement[i]->getVkImageLayout();

        VkDescriptorImageInfo DerivativesDesv{};
        DerivativesDesv.imageView = Derivatives[i]->getVkImageView();
        DerivativesDesv.imageLayout = Derivatives[i]->getVkImageLayout();

        VkDescriptorImageInfo TurbulenceDesc{};
        TurbulenceDesc.imageView = Turbulence[i]->getVkImageView();
        TurbulenceDesc.imageLayout = Turbulence[i]->getVkImageLayout();

        LveDescriptorWriter(*waveMergeLayout, *waveMergePool)
            .writeImage(0, &Dx_DzDesc)
            .writeImage(1, &Dy_DxzDesc)
            .writeImage(2, &Dyx_DyzDesc)
            .writeImage(3, &Dxx_DzzDesc)
            .writeImage(4, &DisplacementorDesv)
            .writeImage(5, &DerivativesDesv)
            .writeImage(6, &TurbulenceDesc)
            .build(waveMergeDescriptorSets[i]);
    }
}

}  // namespace lve