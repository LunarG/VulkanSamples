#ifndef HOLOGRAM_H
#define HOLOGRAM_H

#include <memory>
#include <string>
#include <vector>
#include <random>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

    void on_frame(float frame_pred);

private:
    class Worker {
    public:
        Worker(Hologram &hologram, int object_begin, int object_end);

        void start();
        void stop();
        void step_objects();
        void draw_objects(VkFramebuffer fb);
        void wait_idle();

        Hologram &hologram_;

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

    // called by the constructor
    void init_workers();
    void init_objects();

    std::random_device random_dev_;

    bool multithread_;
    std::vector<std::unique_ptr<Worker>> workers_;
    std::vector<Object> objects_;

    // called by attach_shell
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

    const Meshes *meshes_;

    VkRenderPass render_pass_;
    VkClearValue render_pass_clear_value_;
    VkRenderPassBeginInfo render_pass_begin_info_;

    VkShaderModule vs_;
    VkShaderModule fs_;
    VkPipelineLayout pipeline_layout_;
    VkPipeline pipeline_;

    VkCommandPool primary_cmd_pool_;
    VkCommandBuffer primary_cmd_;
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
    void step_object(Object &obj, float obj_time) const;
    void draw_object(const Object &obj, VkCommandBuffer cmd) const;
    void step_objects(const Worker &worker);
    void draw_objects(Worker &worker);
};

#endif // HOLOGRAM_H
