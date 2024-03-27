#include "first_app.hpp"

#include <vulkan/vulkan_core.h>

#include <string>

#include "keyboard_movement_controller.hpp"
#include "lve_buffer.hpp"
#include "lve_camera.hpp"
#include "lve_frame_info.hpp"
#include "lve_image.hpp"
#include "lve_renderer.hpp"
#include "lve_swap_chain.hpp"
#include "systems/BRDF_LUT_system.hpp"
#include "systems/point_light_system.hpp"
#include "systems/ray_rendering_system.hpp"
#include "systems/simple_render_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
// #include <stdlib.h>

// #include <array>
// #include <cassert>
#include <chrono>
// #include <iostream>
// #include <stdexcept>
namespace lve {

FirstApp::FirstApp() {
    globalPool =
        LveDescriptorPool::Builder(lveDevice)
            .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();
    loadGameObjects();
}

FirstApp::~FirstApp() {}

void FirstApp::init() {
    VkCommandBuffer commandBuffer = lveDevice.beginSingleTimeCommands();
    BRDF_LUTSystem BRDFLUTsystem{lveDevice};
    BRDFLUTsystem.executeShader(commandBuffer);
    BRDF_LUTImage = BRDFLUTsystem.getBRDFLUTImage();
    BRDF_LUTImage->transitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandBuffer);
    
    lveDevice.endSingleTimeCommands(commandBuffer);

    
    
}

void FirstApp::run() {
    ImageCreateInfo imageCreateInfo{};
    imageCreateInfo.filename = "textures/truc.jpg";
    imageCreateInfo.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    LveImage image{&lveDevice, imageCreateInfo};

    PipelineRessourcesCreateInfo test{};
    test.colorAttachments = {VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R16G16B16A16_SNORM};
    test.hasDepthAttachement = true;
    test.numberOfImage = LveSwapChain::MAX_FRAMES_IN_FLIGHT;
    test.width = 1280;
    test.height = 720;

    // LvePipelineRessources t {test, lveDevice};

    std::vector<std::unique_ptr<LveBuffer>> uboBuffers(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++) {
        uboBuffers[i] = std::make_unique<LveBuffer>(
            lveDevice, sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        uboBuffers[i]->map();
    }

    auto globalSetLayout =
        LveDescriptorSetLayout::Builder(lveDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
            .build();

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = image.getVkImageView();
    imageInfo.sampler = image.getVkSampler();
    std::vector<VkDescriptorSet> globalDescriptorSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++) {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        LveDescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .writeImage(1, &imageInfo)
            .build(globalDescriptorSets[i]);
    }

    SimpleRenderSystem simpleRenderSystem{
        lveDevice, lveRenderer.getSwapchainPipeLineRessources(),
        globalSetLayout->getDescriptorSetLayout()};

    PointLightSystem pointLightSystem{
        lveDevice, lveRenderer.getSwapchainPipeLineRessources(),
        globalSetLayout->getDescriptorSetLayout()};

    RayRenderingSystem rayRenderingSystem{
        lveDevice, lveRenderer.getSwapchainPipeLineRessources(),
        globalSetLayout->getDescriptorSetLayout(), BRDF_LUTImage};
    LveCamera camera{};

    auto viewerObject = LveGameObject::createGameObject();
    viewerObject.transform.translation.z = -2.f;
    KeyboardMovementController cameraController{};

    auto currentTime = std::chrono::high_resolution_clock::now();
    float totalFrameTime = 0.f;
    int frameNumber1 = 0;
    int frameNumber2 = 0;
    while (!lveWindow.shouldClose()) {
        glfwPollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime =
            std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime)
                .count();
        currentTime = newTime;
        totalFrameTime += frameTime;

        if(cameraController.moveInPlaneXZ(lveWindow.getGLFWwindow(), frameTime, viewerObject)){
            frameNumber1 = 0;
            frameNumber2 = 0;
        }

        camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

        float aspect = lveRenderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);
        auto commandBuffer = lveRenderer.beginFrame();
        if (commandBuffer.renderCommandBuffer != nullptr && commandBuffer.preProcessommandBuffer != nullptr) {
            int frameIndex = lveRenderer.getFrameIndex();
            FrameInfo frameInfo{
                frameIndex,
                lveRenderer.getSwapChainImageIndex(),
                frameTime,

                lveRenderer.getSwapChainExtent(),
                commandBuffer,
                camera,
                globalDescriptorSets[frameIndex],
                gameObjects};

            // update
            GlobalUbo ubo{};
            ubo.projection = camera.getProjection();
            ubo.view = camera.getView();
            ubo.inverseView = camera.getInverseView();
            ubo.frameTime = totalFrameTime;
            if (frameIndex == 0) {
                ubo.frameNumber = frameNumber1;
                frameNumber1++;
            } else {
                ubo.frameNumber = frameNumber2;
                frameNumber2++;
            }
            pointLightSystem.update(frameInfo, ubo);

            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            //pre-process
            lveRenderer.startPreProcess();
            VkImageLayout t;
            t = BRDF_LUTImage->getActualImageLayout();

            BRDF_LUTImage->transitionImageLayout(t, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, frameInfo.commandBuffer.preProcessommandBuffer);
            BRDF_LUTImage->transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, t, frameInfo.commandBuffer.preProcessommandBuffer);
            
            lveRenderer.endPreProcess();

            // render
            lveRenderer.startRenderFrame();

            lveRenderer.beginSwapChainRenderPass(commandBuffer.renderCommandBuffer);

            // order here matters
            rayRenderingSystem.executeShader(frameInfo);
            // simpleRenderSystem.renderGameObjects(frameInfo);
            pointLightSystem.executeShader(frameInfo);

            lveRenderer.endSwapChainRenderPass(commandBuffer.renderCommandBuffer);
            LveImage::copyImage(
                lveDevice,
                *lveRenderer.getSwapchainPipeLineRessources()->getImageColorAttachment(
                    frameInfo.swapchainFrameIndex)[1],
                *rayRenderingSystem.getPreviousImage(frameIndex), commandBuffer.renderCommandBuffer);

            lveRenderer.endRenderFrame();
            lveRenderer.presentFrame();

        }
    }

    vkDeviceWaitIdle(lveDevice.device());
}

void FirstApp::loadGameObjects() {
    std::shared_ptr<LveModel> lveModel =
        LveModel::createModelFromFile(lveDevice, "models/flat_vase.obj");
    auto flatVase = LveGameObject::createGameObject();
    flatVase.model = lveModel;
    flatVase.transform.translation = {-.5f, .5f, 0.f};
    flatVase.transform.scale = {3.f, 1.5f, 3.f};
    gameObjects.emplace(flatVase.getId(), std::move(flatVase));

    lveModel = LveModel::createModelFromFile(lveDevice, "models/smooth_vase.obj");
    auto smoothVase = LveGameObject::createGameObject();
    smoothVase.model = lveModel;
    smoothVase.transform.translation = {.5f, .5f, 0.f};
    smoothVase.transform.scale = {3.f, 1.5f, 3.f};
    gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

    lveModel = LveModel::createModelFromFile(lveDevice, "models/quad.obj");
    auto floor = LveGameObject::createGameObject();
    floor.model = lveModel;
    floor.transform.translation = {0.f, .5f, 0.f};
    floor.transform.scale = {3.f, 1.f, 3.f};
    gameObjects.emplace(floor.getId(), std::move(floor));

    std::vector<glm::vec3> lightColors{
        {1.f, .1f, .1f}, {.1f, .1f, 1.f}, {.1f, 1.f, .1f},
        {1.f, 1.f, .1f}, {.1f, 1.f, 1.f}, {1.f, 1.f, 1.f}  //
    };

    for (int i = 0; i < lightColors.size(); i++) {
        auto pointLight = LveGameObject::makePointLight(0.2f);
        pointLight.color = lightColors[i];
        auto rotateLight = glm::rotate(
            glm::mat4(1.f), (i * glm::two_pi<float>()) / lightColors.size(), {0.f, -1.f, 0.f});
        pointLight.transform.translation =
            glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
        gameObjects.emplace(pointLight.getId(), std::move(pointLight));
    }
}

}  // namespace lve
