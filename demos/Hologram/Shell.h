#ifndef SHELL_H
#define SHELL_H

#include <vector>
#include <stdexcept>
#include <vulkan/vulkan.h>

#include "Game.h"

class Game;

class Shell {
public:
    Shell(const Shell &sh) = delete;
    Shell &operator=(const Shell &sh) = delete;

    struct Context {
        VkInstance instance;
        VkSurfaceKHR surface;

        VkPhysicalDevice physical_dev;
        uint32_t game_queue_family;
        uint32_t present_queue_family;

        VkDevice dev;
        VkQueue game_queue;
        VkQueue present_queue;

        VkSurfaceFormatKHR format;

        VkSwapchainKHR swapchain;
        VkExtent2D extent;
    };
    const Context &context() const { return ctx_; }

    virtual void run() = 0;

protected:
    Shell(Game &game) : game_(game), settings_(game.settings()), ctx_() {}

    void init_vk();
    void cleanup_vk();

    void resize_swapchain(int32_t width_hint, int32_t height_hint);
    void present(float frame_time);

    Game &game_;
    const Game::Settings &settings_;

    std::vector<const char *> global_extensions_;
    std::vector<const char *> device_extensions_;

private:
    void assert_all_global_extensions() const;
    bool has_all_device_extensions(VkPhysicalDevice phy) const;

    virtual PFN_vkGetInstanceProcAddr load_vk() = 0;
    virtual VkSurfaceKHR create_surface(VkInstance instance) = 0;
    VkInstance create_instance();

    void init_physical_dev();
    void init_dev();
    void init_swapchain();

    Context ctx_;
};

#endif // SHELL_H
