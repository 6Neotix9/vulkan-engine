#include "lve_A_system.hpp"
#include "../lve_hot_reload.hpp"

namespace lve {

LveASystem::LveASystem(
    LveDevice &lveDevice,
    const char *shaderName,
    std::shared_ptr<LvePipelineRessources> pipelineRessources
    )
    : lveDevice{lveDevice}, pipelineRessources{pipelineRessources} {
    
    LveHotReload::getInstance()->addShader(shaderName, this);
}

LveASystem::~LveASystem() {
    vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
}

void LveASystem::reloadShaders() {
    lveDevice.stopRendering();
    this->createPipeline();
    lveDevice.startRendering();
}

}  // namespace lve