#pragma once

#include <map>
#include <string>

#include "systems/lve_A_system.hpp"

#include <uv.h>
#include <thread>


namespace lve {
class LveHotReload {
   public:
    
    ~LveHotReload();

    static LveHotReload* getInstance();

    void addShader(std::string name, LveASystem *shader);
    void reloadShaders(std::string name);
    void on_file_change(uv_fs_event_t* handle, const char* filename, int events, int status);
   
   private:
    LveHotReload();
    inline static std::unique_ptr<LveHotReload> instance;
    std::map<std::string, LveASystem*> shaderMap;
    std::thread uv_thread;
    uv_fs_event_t fs_event;
    uv_loop_t loop;
};
}  // namespace lve