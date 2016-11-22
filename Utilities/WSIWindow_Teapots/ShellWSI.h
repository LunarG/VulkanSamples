#ifndef SHELL_WSI_H
#define SHELL_WSI_H

#include "Helpers.h"
#include "Shell.h"
#include <chrono>

#include "WSIWindow.h"
#include "Shell.h"

//-----------------------------High Resolution Timer-----------------------------
class CTimer{
    typedef std::chrono::high_resolution_clock clock;
    typedef std::chrono::duration<double> duration;
    clock::time_point start=clock::now();
public:
    void  reset(){ start=clock::now(); }    // reset the timer
    double span(){                          // returns seconds since last reset
        return (std::chrono::duration_cast<duration>(clock::now()-start)).count();
    }
};
//-------------------------------------------------------------------------------

//----------------------------Swapchain and context------------------------------
class ShellWSI : public Shell{
public:
    ShellWSI(Game &game, VkInstance instance, CSurface* surface);
    ~ShellWSI(){ }
    void resize_swapchain(uint32_t width_hint, uint32_t height_hint);
    void step();                         // render frame and show fps
    void quit();                         // destroy vulkan context
    CSurface* surface=0;                 // for CanPresent()

    CTimer timer;
    double fps=0;
protected:
    Game &game_;
    const Game::Settings &settings_;
private:
    std::vector<const char *> device_extensions_;

    //PFN_vkGetInstanceProcAddr load_vk(){}
    bool can_present(VkPhysicalDevice phy, uint32_t queue_family);

    void create_dev();
    void create_back_buffers();
    void destroy_back_buffers();
    void create_swapchain();
    void destroy_swapchain();
    void fake_present();

    const float game_tick_;
    float game_time_;

    bool has_all_device_extensions(VkPhysicalDevice phy) const;
    void init_physical_device(VkInstance instance, VkSurfaceKHR surface);

    void create_context();
    void destroy_context();
    void add_game_time(float time);
    void acquire_back_buffer();
    void present_back_buffer();
};
//-------------------------------------------------------------------------------

#endif // SHELL_WSI_H
