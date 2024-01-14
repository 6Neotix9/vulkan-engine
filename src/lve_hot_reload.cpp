#include "lve_hot_reload.hpp"

#include <iostream>

namespace lve {

static void on_file_change_static(
    uv_fs_event_t* handle, const char* filename, int events, int status) {
    LveHotReload::getInstance()->on_file_change(handle, filename, events, status);
}

LveHotReload::LveHotReload() {
    shaderMap = std::map<std::string, LveISystem*>();
    uv_loop_init(&loop);

    uv_fs_event_init(&loop, &fs_event);

    // Spécifiez le dossier à surveiller et la fonction de rappel
    uv_fs_event_start(&fs_event, on_file_change_static, "../shaders", UV_FS_EVENT_RECURSIVE);

    uv_thread = std::thread([this]() { uv_run(&loop, UV_RUN_DEFAULT); });
}

LveHotReload* LveHotReload::getInstance() {
    if (instance == nullptr) {
        instance = std::unique_ptr<LveHotReload>(new LveHotReload());
    }
    return instance.get();
}

LveHotReload::~LveHotReload() {
    uv_stop(&loop);
    uv_thread.join();  // Attendre la fin du thread

    // Libérer les ressources
    uv_fs_event_stop(&fs_event);
    uv_loop_close(&loop);
}

void LveHotReload::addShader(std::string name, LveISystem* shader) {
    shaderMap.insert({name, shader});
}

void LveHotReload::reloadShaders(std::string name) { 
    shaderMap[name]->reloadShaders(); 
    }

inline bool ends_with(std::string const& value, std::string const& ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void LveHotReload::on_file_change(
    uv_fs_event_t* handle, const char* filename, int events, int status) {
    if (status == 0) {
        std::string filenameStr = std::string(filename);
        if (ends_with(filenameStr, ".vert") || ends_with(filenameStr, ".frag") ||
            ends_with(filenameStr, ".comp")) {
            // create command to compile shader
            std::string command =
                "glslc ../shaders/" + filenameStr + " -o ../shaders/" + filenameStr + ".spv";
            system(command.c_str());
        }
        else if (ends_with(filenameStr, ".spv")) {
            // get shader name from filename
            std::string shaderName = filenameStr.substr(
                0,
                filenameStr.substr(0, filenameStr.find_last_of(".")).find_last_of("."));
            std::cout << "Reloading shader: " << shaderName << std::endl;
            reloadShaders(shaderName);
        }

    } else {
        fprintf(stderr, "Erreur lors de la surveillance du fichier: %s\n", uv_strerror(status));
    }
}

}  // namespace lve