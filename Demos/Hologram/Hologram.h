#ifndef HOLOGRAM_H
#define HOLOGRAM_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "Simulation.h"
#include "Game.h"

class Meshes;

class Hologram : public Game {
public:
    Hologram(const std::vector<std::string> &args);
    ~Hologram();

    void attach_shell(Shell &sh);
    void detach_shell();

    void attach_swapchain();
    void detach_swapchain();

    void on_key(Key key);
    void on_tick();

    void on_frame(float frame_pred);

private:
    class Worker {
    public:
        Worker(Hologram &hologram, int object_begin, int object_end);

        void start();
        void stop();
        void update_simulation();
        void draw_objects(VkFramebuffer fb);
        void wait_idle();

        Hologram &hologram_;

        const int object_begin_;
        const int object_end_;

        const float tick_interval_;

        VkCommandBuffer cmd_;

        VkFramebuffer fb_;

    private:
        enum State {
            INIT,
            IDLE,
            STEP,
            DRAW,
        };

        void update_loop();

        static void thread_loop(Worker *worker) { worker->update_loop(); }

        std::thread thread_;
        std::mutex mutex_;
        std::condition_variable state_cv_;
        State state_;
    };

    struct FrameData {
        VkBuffer buf;
        VkDeviceMemory mem;
        uint8_t *base;

        VkPushConstantRange push_const_range;
    };

    // called by the constructor
    void init_workers();

    bool multithread_;
    bool use_push_constants_;

    bool sim_paused_;
    Simulation sim_;

    std::vector<std::unique_ptr<Worker>> workers_;

    // called by attach_shell
    void create_frame_data();
    void create_descriptor_set();
    void create_render_pass();
    void create_shader_modules();
    void create_pipeline_layout();
    void create_pipeline();
    void create_primary_cmd();
    void start_workers();
    void stop_workers();

    VkPhysicalDevice physical_dev_;
    VkDevice dev_;
    VkQueue queue_;
    uint32_t queue_family_;
    VkFormat format_;

    VkPhysicalDeviceProperties physical_dev_props_;
    std::vector<VkMemoryPropertyFlags> mem_flags_;

    const Meshes *meshes_;

    FrameData frame_data_;

    VkDescriptorPool desc_pool_;
    VkDescriptorSetLayout desc_set_layout_;
    VkDescriptorSet desc_set_;

    VkRenderPass render_pass_;
    VkClearValue render_pass_clear_value_;
    VkRenderPassBeginInfo render_pass_begin_info_;

    VkShaderModule vs_;
    VkShaderModule fs_;
    VkPipelineLayout pipeline_layout_;
    VkPipeline pipeline_;

    VkCommandPool primary_cmd_pool_;
    VkCommandBuffer primary_cmd_;
    VkFence primary_cmd_fence_;
    VkCommandBufferBeginInfo primary_cmd_begin_info_;
    VkPipelineStageFlags primary_cmd_submit_wait_stages_;
    VkSubmitInfo primary_cmd_submit_info_;

    std::vector<VkCommandPool> worker_cmd_pools_;
    std::vector<VkCommandBuffer> worker_cmds_;

    // called by attach_swapchain
    void prepare_viewport(const VkExtent2D &extent);
    void prepare_framebuffers(VkSwapchainKHR swapchain);

    VkExtent2D extent_;
    VkViewport viewport_;
    VkRect2D scissor_;

    std::vector<VkImage> images_;
    std::vector<VkImageView> image_views_;
    std::vector<VkFramebuffer> framebuffers_;

    // called mostly by on_key
    void update_projection();

    glm::vec3 eye_pos_;
    glm::mat4 view_projection_;

    // called by workers
    void update_simulation(const Worker &worker);
    void draw_object(const Simulation::Object &obj, FrameData &data, VkCommandBuffer cmd) const;
    void draw_objects(Worker &worker);
};

#endif // HOLOGRAM_H
