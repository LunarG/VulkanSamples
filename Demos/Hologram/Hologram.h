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

#include "Path.h"
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

    void on_frame(float frame_pred, int fb);

private:
    class Worker {
    public:
        Worker(Hologram &hologram, int index, int object_begin, int object_end);

        void start();
        void stop();
        void step_objects();
        void draw_objects(VkFramebuffer fb);
        void wait_idle();

        Hologram &hologram_;

        const int index_;
        const int object_begin_;
        const int object_end_;

        VkCommandBuffer cmd_;

        float object_time_;
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

        const float tick_interval_;
    };

    struct Object {
        int mesh;
        Path path;

        glm::mat4 model;
    };

    struct FrameData {
        // signaled when this struct is ready for reuse
        VkFence fence;

        VkCommandBuffer primary_cmd;
        std::vector<VkCommandBuffer> worker_cmds;

        VkBuffer buf;
        uint8_t *base;
        VkDescriptorSet desc_set;
    };

    // called by the constructor
    void init_workers();
    void init_objects();

    std::random_device random_dev_;

    bool multithread_;
    bool use_push_constants_;

    // called mostly by on_key
    void update_camera();

    bool sim_paused_;
    Simulation sim_;
    Camera camera_;

    std::vector<std::unique_ptr<Worker>> workers_;

    // called by attach_shell
    void create_render_pass();
    void create_shader_modules();
    void create_descriptor_set_layout();
    void create_pipeline_layout();
    void create_pipeline();

    void create_frame_data(int count);
    void destroy_frame_data();
    void create_fences();
    void create_command_buffers();
    void create_buffers();
    void create_buffer_memory();
    void create_descriptor_sets();

    VkPhysicalDevice physical_dev_;
    VkDevice dev_;
    VkQueue queue_;
    uint32_t queue_family_;
    VkFormat format_;

    VkPhysicalDeviceProperties physical_dev_props_;
    std::vector<VkMemoryPropertyFlags> mem_flags_;

    const Meshes *meshes_;

    VkRenderPass render_pass_;
    VkShaderModule vs_;
    VkShaderModule fs_;
    VkDescriptorSetLayout desc_set_layout_;
    VkPipelineLayout pipeline_layout_;
    VkPipeline pipeline_;

    VkCommandPool primary_cmd_pool_;
    VkCommandBuffer primary_cmd_;
    VkCommandBufferBeginInfo primary_cmd_begin_info_;
    VkSubmitInfo primary_cmd_submit_info_;

    // called by attach_swapchain
    void prepare_viewport(const VkExtent2D &extent);
    void prepare_framebuffers(VkSwapchainKHR swapchain);

    VkExtent2D extent_;
    VkViewport viewport_;
    VkRect2D scissor_;
    glm::mat4 view_projection_;

    std::vector<VkImage> images_;
    std::vector<VkImageView> image_views_;
    std::vector<VkFramebuffer> framebuffers_;

    // called by workers
    void step_object(Object &obj, float obj_time) const;
    void draw_object(const Object &obj, VkCommandBuffer cmd) const;
    void step_objects(const Worker &worker);
    void draw_objects(Worker &worker);
};

#endif // HOLOGRAM_H
