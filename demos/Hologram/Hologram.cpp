#include <array>
#include "Helpers.h"
#include "Hologram.h"
#include "Meshes.h"
#include "Shell.h"

#define OBJECT_COUNT 10000

Hologram::Hologram(const std::vector<std::string> &args)
    : Game("Hologram", args), random_dev_(), multithread_(true),
      render_pass_clear_value_(), render_pass_begin_info_(),
      primary_cmd_begin_info_(), primary_cmd_submit_info_()
{
    for (auto it = args.begin(); it != args.end(); ++it) {
        if (*it == "-s")
            multithread_ = false;
    }

    init_workers();
    init_objects();
}

Hologram::~Hologram()
{
}

void Hologram::init_workers()
{
    int worker_count = std::thread::hardware_concurrency() - 2;

    // not enough cores
    if (!multithread_ || worker_count < 0) {
        multithread_ = false;
        worker_count = 1;
    }

    const int object_per_worker = OBJECT_COUNT / worker_count;
    int object_begin = 0, object_end = 0;

    workers_.reserve(worker_count);
    for (int i = 0; i < worker_count; i++) {
        object_begin = object_end;
        if (i < worker_count - 1)
            object_end += object_per_worker;
        else
            object_end = OBJECT_COUNT;

        Worker *worker = new Worker(*this, object_begin, object_end);
        workers_.emplace_back(std::unique_ptr<Worker>(worker));
    }
}

void Hologram::init_objects()
{
    objects_.reserve(OBJECT_COUNT);
    for (int i = 0; i < OBJECT_COUNT; i++) {
        Object obj = { i, random_dev_() };
        objects_.push_back(obj);
    }
}

void Hologram::attach_shell(Shell &sh)
{
    Game::attach_shell(sh);

    const Shell::Context &ctx = sh.context();
    physical_dev_ = ctx.physical_dev;
    dev_ = ctx.dev;
    queue_ = ctx.game_queue;
    queue_family_ = ctx.game_queue_family;
    format_ = ctx.format.format;

    meshes_ = new Meshes(random_dev_(), physical_dev_, dev_, OBJECT_COUNT);

    create_render_pass();
    create_shader_modules();
    create_pipeline_layout();
    create_pipeline();
    create_primary_cmd();

    start_workers();
}

void Hologram::detach_shell()
{
    stop_workers();

    vk::DestroyCommandPool(dev_, primary_cmd_pool_, nullptr);

    vk::DestroyPipeline(dev_, pipeline_, nullptr);
    vk::DestroyPipelineLayout(dev_, pipeline_layout_, nullptr);
    vk::DestroyShaderModule(dev_, fs_, nullptr);
    vk::DestroyShaderModule(dev_, vs_, nullptr);

    vk::DestroyRenderPass(dev_, render_pass_, nullptr);

    delete meshes_;

    Game::detach_shell();
}

void Hologram::create_render_pass()
{
    VkAttachmentDescription attachment = {};
    attachment.format = format_;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference attachment_ref = {};
    attachment_ref.attachment = 0;
    attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachment_ref;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    vk::assert_success(vk::CreateRenderPass(dev_, &render_pass_info, nullptr, &render_pass_));

    render_pass_begin_info_.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info_.renderPass = render_pass_;
    render_pass_begin_info_.clearValueCount = 1;
    render_pass_begin_info_.pClearValues = &render_pass_clear_value_;
}

void Hologram::create_shader_modules()
{
    VkShaderModuleCreateInfo sh_info = {};
    sh_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
#include "Hologram.vert.h"
    sh_info.codeSize = sizeof(Hologram_vert);
    sh_info.pCode = Hologram_vert;
    vk::assert_success(vk::CreateShaderModule(dev_, &sh_info, nullptr, &vs_));

#include "Hologram.frag.h"
    sh_info.codeSize = sizeof(Hologram_frag);
    sh_info.pCode = Hologram_frag;
    vk::assert_success(vk::CreateShaderModule(dev_, &sh_info, nullptr, &fs_));
}

void Hologram::create_pipeline_layout()
{
    VkPushConstantRange push_const_range = {};
    push_const_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_const_range.offset = 0;
    push_const_range.size = sizeof(glm::mat4);

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pSetLayouts = nullptr;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_const_range;

    vk::assert_success(vk::CreatePipelineLayout(dev_, &pipeline_layout_info, nullptr, &pipeline_layout_));
}

void Hologram::create_pipeline()
{
    VkPipelineShaderStageCreateInfo stage_info[2] = {};
    stage_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stage_info[0].module = vs_;
    stage_info[0].pName = "main";
    stage_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stage_info[1].module = fs_;
    stage_info[1].pName = "main";

    VkPipelineViewportStateCreateInfo viewport_info = {};
    viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    // both dynamic
    viewport_info.viewportCount = 1;
    viewport_info.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rast_info = {};
    rast_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rast_info.depthClampEnable = false;
    rast_info.rasterizerDiscardEnable = false;
    rast_info.polygonMode = VK_POLYGON_MODE_FILL;
    rast_info.cullMode = VK_CULL_MODE_NONE;
    rast_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rast_info.depthBiasEnable = false;
    rast_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_info = {};
    multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_info.sampleShadingEnable = false;
    multisample_info.pSampleMask = nullptr;
    multisample_info.alphaToCoverageEnable = false;
    multisample_info.alphaToOneEnable = false;

    VkPipelineColorBlendAttachmentState blend_attachment = {};
    blend_attachment.blendEnable = true;
    blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                      VK_COLOR_COMPONENT_G_BIT |
                                      VK_COLOR_COMPONENT_B_BIT |
                                      VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend_info = {};
    blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_info.logicOpEnable = false;
    blend_info.attachmentCount = 1;
    blend_info.pAttachments = &blend_attachment;

    std::array<VkDynamicState, 2> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    struct VkPipelineDynamicStateCreateInfo dynamic_info = {};
    dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_info.dynamicStateCount = dynamic_states.size();
    dynamic_info.pDynamicStates = dynamic_states.data();

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = stage_info;
    pipeline_info.pVertexInputState = &meshes_->vertex_input_state();
    pipeline_info.pInputAssemblyState = &meshes_->input_assembly_state();
    pipeline_info.pTessellationState = nullptr;
    pipeline_info.pViewportState = &viewport_info;
    pipeline_info.pRasterizationState = &rast_info;
    pipeline_info.pMultisampleState = &multisample_info;
    pipeline_info.pDepthStencilState = nullptr;
    pipeline_info.pColorBlendState = &blend_info;
    pipeline_info.pDynamicState = &dynamic_info;
    pipeline_info.layout = pipeline_layout_;
    pipeline_info.renderPass = render_pass_;
    pipeline_info.subpass = 0;
    vk::assert_success(vk::CreateGraphicsPipelines(dev_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline_));
}

void Hologram::create_primary_cmd()
{
    // create a pool first
    VkCommandPoolCreateInfo cmd_pool_info = {};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_info.queueFamilyIndex = queue_family_;

    vk::assert_success(vk::CreateCommandPool(dev_, &cmd_pool_info, nullptr, &primary_cmd_pool_));

    VkCommandBufferAllocateInfo cmd_buf_info = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buf_info.commandPool = primary_cmd_pool_;
    cmd_buf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buf_info.commandBufferCount = 1;

    vk::assert_success(vk::AllocateCommandBuffers(dev_, &cmd_buf_info, &primary_cmd_));

    primary_cmd_begin_info_.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    primary_cmd_begin_info_.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    primary_cmd_submit_info_.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    primary_cmd_submit_info_.commandBufferCount = 1;
    primary_cmd_submit_info_.pCommandBuffers = &primary_cmd_;
}

void Hologram::start_workers()
{
    // should have been stopped
    assert(worker_cmds_.empty());

    worker_cmd_pools_.reserve(workers_.size());
    worker_cmds_.reserve(workers_.size());

    for (auto &worker : workers_) {
        VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cmd_pool_info.queueFamilyIndex = queue_family_;

        VkCommandPool cmd_pool;
        vk::assert_success(vk::CreateCommandPool(dev_, &cmd_pool_info, nullptr, &cmd_pool));

        VkCommandBufferAllocateInfo cmd_buf_info = {};
        cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_buf_info.commandPool = cmd_pool;
        cmd_buf_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        cmd_buf_info.commandBufferCount = 1;

        VkCommandBuffer cmd;
        vk::assert_success(vk::AllocateCommandBuffers(dev_, &cmd_buf_info, &cmd));

        worker_cmd_pools_.push_back(cmd_pool);
        worker_cmds_.push_back(cmd);

        worker->cmd_ = cmd;

        if (multithread_)
            worker->start();
    }
}

void Hologram::stop_workers()
{
    if (multithread_) {
        for (auto &worker : workers_)
            worker->stop();
    }

    for (auto &pool : worker_cmd_pools_)
        vk::DestroyCommandPool(dev_, pool, nullptr);

    worker_cmd_pools_.clear();
    worker_cmds_.clear();
}

void Hologram::attach_swapchain()
{
    const Shell::Context &ctx = shell_->context();

    prepare_viewport(ctx.extent);
    prepare_images(ctx.swapchain);
    prepare_framebuffers();
}

void Hologram::detach_swapchain()
{
    for (auto fb : framebuffers_)
        vk::DestroyFramebuffer(dev_, fb, nullptr);
    for (auto view : image_views_)
        vk::DestroyImageView(dev_, view, nullptr);

    framebuffers_.clear();
    image_views_.clear();
    images_.clear();
}

void Hologram::prepare_viewport(const VkExtent2D &extent)
{
    extent_ = extent;

    viewport_.x = 0.0f;
    viewport_.y = 0.0f;
    viewport_.width = static_cast<float>(extent.width);
    viewport_.height = static_cast<float>(extent.height);
    viewport_.minDepth = 0.0f;
    viewport_.maxDepth = 1.0f;

    scissor_.offset = { 0, 0 };
    scissor_.extent = extent_;

    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
    const glm::vec3 eye(5.0f, 5.0f, 5.0f);
    const glm::vec3 center(0.0f);
    const glm::vec3 up(0.f, 0.0f, 1.0f);
    const glm::mat4 view = glm::lookAt(eye, center, up);
    const glm::mat4 projection = glm::perspective(0.4f, aspect, 0.1f, 100.0f);
    view_projection_ = projection * view;
}

void Hologram::prepare_images(VkSwapchainKHR swapchain)
{
    // get swapchain images
    vk::get(dev_, swapchain, images_);

    // transition to PRESENT_SRC_KHR
    VkImageMemoryBarrier img_barrier = {};
    img_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    img_barrier.srcAccessMask = 0;
    img_barrier.dstAccessMask = 0;
    img_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    img_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    img_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    img_barrier.subresourceRange.levelCount = 1;
    img_barrier.subresourceRange.layerCount = 1;

    std::vector<VkImageMemoryBarrier> img_barriers;
    img_barriers.reserve(images_.size());
    for (auto img : images_) {
        img_barrier.image = img;
        img_barriers.push_back(img_barrier);
    }

    vk::BeginCommandBuffer(primary_cmd_, &primary_cmd_begin_info_);
    vk::CmdPipelineBarrier(primary_cmd_,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr,
            img_barriers.size(), img_barriers.data());
    vk::EndCommandBuffer(primary_cmd_);

    vk::QueueSubmit(queue_, 1, &primary_cmd_submit_info_, VK_NULL_HANDLE);
}

void Hologram::prepare_framebuffers()
{
    assert(framebuffers_.empty());

    image_views_.reserve(images_.size());
    framebuffers_.reserve(images_.size());
    for (auto img : images_) {
        VkImageViewCreateInfo view_info = {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = img;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = format_;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.layerCount = 1;

        VkImageView view;
        vk::assert_success(vk::CreateImageView(dev_, &view_info, nullptr, &view));
        image_views_.push_back(view);

        VkFramebufferCreateInfo fb_info = {};
        fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_info.renderPass = render_pass_;
        fb_info.attachmentCount = 1;
        fb_info.pAttachments = &view;
        fb_info.width = extent_.width;
        fb_info.height = extent_.height;
        fb_info.layers = 1;

        VkFramebuffer fb;
        vk::assert_success(vk::CreateFramebuffer(dev_, &fb_info, nullptr, &fb));
        framebuffers_.push_back(fb);
    }
}

void Hologram::step_object(Object &obj, Worker &worker) const
{
    glm::vec3 pos = obj.path.position(worker.frame_time_);
    glm::vec3 scale = glm::vec3(0.01f);

    obj.model = glm::mat4(1.0f);
    obj.model = glm::translate(obj.model, pos);
    obj.model = glm::scale(obj.model, scale);
}

void Hologram::draw_object(const Object &obj, VkCommandBuffer cmd) const
{
    glm::mat4 mvp = view_projection_ * obj.model;
    vk::CmdPushConstants(cmd, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT,
            0, sizeof(mvp), glm::value_ptr(mvp));

    meshes_->cmd_draw(cmd, obj.mesh);
}

void Hologram::update_objects(Worker &worker)
{
    auto cmd = worker.cmd_;

    VkCommandBufferInheritanceInfo inherit_info = {};
    inherit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inherit_info.renderPass = render_pass_;
    inherit_info.framebuffer = framebuffers_[worker.fb_];

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    begin_info.pInheritanceInfo = &inherit_info;

    vk::BeginCommandBuffer(cmd, &begin_info);

    vk::CmdSetViewport(cmd, 0, 1, &viewport_);
    vk::CmdSetScissor(cmd, 0, 1, &scissor_);

    vk::CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

    meshes_->cmd_bind_buffers(cmd);

    for (int i = worker.object_begin_; i < worker.object_end_; i++) {
        auto &obj = objects_[i];

        step_object(obj, worker);
        draw_object(obj, cmd);
    }

    vk::EndCommandBuffer(cmd);
}

void Hologram::on_key(Key key)
{
    switch (key) {
    case KEY_SHUTDOWN:
    case KEY_ESC:
        shell_->quit();
        break;
    default:
        break;
    }
}

void Hologram::on_frame(float frame_time, int fb)
{
    for (auto &worker : workers_)
        worker->render(frame_time, fb);

    for (auto &worker : workers_)
        worker->wait_render();

    VkResult res = vk::BeginCommandBuffer(primary_cmd_, &primary_cmd_begin_info_);

    render_pass_begin_info_.framebuffer = framebuffers_[fb];
    render_pass_begin_info_.renderArea.extent = extent_;
    vk::CmdBeginRenderPass(primary_cmd_, &render_pass_begin_info_,
            VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    // record render pass commands
    vk::CmdExecuteCommands(primary_cmd_, worker_cmds_.size(), worker_cmds_.data());

    vk::CmdEndRenderPass(primary_cmd_);
    vk::EndCommandBuffer(primary_cmd_);

    res = vk::QueueSubmit(queue_, 1, &primary_cmd_submit_info_, VK_NULL_HANDLE);

    (void) res;
}

Hologram::Worker::Worker(Hologram &hologram, int object_begin, int object_end)
    : hologram_(hologram), object_begin_(object_begin), object_end_(object_end), state_(INIT)
{
}

void Hologram::Worker::start()
{
    state_ = WAIT;
    thread_ = std::thread(Hologram::Worker::thread_loop, this);
}

void Hologram::Worker::stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = INIT;
    }
    state_cv_.notify_one();

    thread_.join();
}

void Hologram::Worker::render(float frame_time, int fb)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        bool started = (state_ != INIT);

        frame_time_ = frame_time;
        fb_ = fb;
        state_ = RENDER;

        // render directly
        if (!started) {
            hologram_.update_objects(*this);
            state_ = INIT;
        }
    }
    state_cv_.notify_one();
}

void Hologram::Worker::wait_render()
{
    std::unique_lock<std::mutex> lock(mutex_);
    bool started = (state_ != INIT);

    if (started)
        state_cv_.wait(lock, [this] { return (state_ == WAIT); });
}

void Hologram::Worker::render_loop()
{
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);

        state_cv_.wait(lock, [this] { return (state_ != WAIT); });
        if (state_ == INIT)
            break;

        assert(state_ == RENDER);
        hologram_.update_objects(*this);

        state_ = WAIT;
        lock.unlock();
        state_cv_.notify_one();
    }
}
