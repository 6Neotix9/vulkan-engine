#pragma once
#include <memory>
#include <mutex>


namespace lve {
class LveSync {
   public:
    
    ~LveSync();

    static LveSync* getInstance();

    //getters
    std::mutex& getWaitForQueueMutex() { return waitForQueueMutex; }
    std::mutex& getwaitForStopRenderingMutex() { return waitForStopRenderingMutex; }

    bool getStopRendering() { return stopRendering; }

    //setters
    void setStopRendering(bool stop) { 
        stopRendering = stop; }
    

   
   private:
    LveSync();
    bool stopRendering;
    inline static std::unique_ptr<LveSync> instance;

    std::mutex waitForQueueMutex;
    std::mutex waitForStopRenderingMutex;

};
}  // namespace lve