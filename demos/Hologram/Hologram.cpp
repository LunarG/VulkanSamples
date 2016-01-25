#include <array>
#include "Helpers.h"
#include "Hologram.h"
#include "Meshes.h"
#include "Shell.h"

#define OBJECT_COUNT 10000

Hologram::Hologram(const std::vector<std::string> &args)
    : Game("Hologram", args), random_dev_(),
      multithread_(true), use_push_constants_(false),
      frame_data_(), render_pass_clear_value_(), render_pass_begin_info_(),
      primary_cmd_begin_info_(), primary_cmd_submit_info_(),
      eye_pos_(8.0f), pause_objects_(false)
{
    for (auto it = args.begin(); it != args.end(); ++it) {
        if (*it == "-s")
            multithread_ = false;
        else if (*it == "-p")
            use_push_constants_ = true;
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
        Object obj = { i, random_dev_(), random_dev_() };
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

    vk::GetPhysicalDeviceProperties(physical_dev_, &physical_dev_props_);

    VkPhysicalDeviceMemoryProperties mem_props;
    vk::GetPhysicalDeviceMemoryProperties(physical_dev_, &mem_props);
    mem_flags_.reserve(mem_props.memoryTypeCount);
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++)
        mem_flags_.push_back(mem_props.memoryTypes[i].propertyFlags);

    meshes_ = new Meshes(random_dev_(), physical_dev_, dev_, OBJECT_COUNT);

    create_frame_data();
    create_descriptor_set();
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

    vk::DestroyFence(dev_, primary_cmd_fence_, nullptr);
    vk::DestroyCommandPool(dev_, primary_cmd_pool_, nullptr);

    vk::DestroyPipeline(dev_, pipeline_, nullptr);
    vk::DestroyPipelineLayout(dev_, pipeline_layout_, nullptr);
    vk::DestroyShaderModule(dev_, fs_, nullptr);
    vk::DestroyShaderModule(dev_, vs_, nullptr);

    vk::DestroyRenderPass(dev_, render_pass_, nullptr);

    if (!use_push_constants_) {
        vk::DestroyDescriptorSetLayout(dev_, desc_set_layout_, nullptr);
        vk::DestroyDescriptorPool(dev_, desc_pool_, nullptr);

        vk::UnmapMemory(dev_, frame_data_.mem);
        vk::FreeMemory(dev_, frame_data_.mem, nullptr);
        vk::DestroyBuffer(dev_, frame_data_.buf, nullptr);
    }

    delete meshes_;

    Game::detach_shell();
}

void Hologram::create_frame_data()
{
    VkDeviceSize object_data_size = sizeof(glm::mat4);

    if (use_push_constants_) {
        frame_data_.push_const_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        frame_data_.push_const_range.offset = 0;
        frame_data_.push_const_range.size = object_data_size;

        frame_data_.object_mvp.resize(objects_.size());
        for (size_t i = 0; i < objects_.size(); i++)
            objects_[i].frame_data_offset = i;

        return;
    }

    // align object data to device limit
    const VkDeviceSize &alignment =
        physical_dev_props_.limits.minStorageBufferOffsetAlignment;
    if (object_data_size % alignment)
        object_data_size += alignment - (object_data_size % alignment);

    uint32_t frame_data_offset = 0;
    for (auto &obj : objects_) {
        obj.frame_data_offset = frame_data_offset;
        frame_data_offset += object_data_size;
    }

    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.size = frame_data_offset;
    buf_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vk::assert_success(vk::CreateBuffer(dev_, &buf_info, nullptr, &frame_data_.buf));

    VkMemoryRequirements mem_reqs;
    vk::GetBufferMemoryRequirements(dev_, frame_data_.buf, &mem_reqs);

    // allocate memory
    VkMemoryAllocateInfo mem_info = {};
    mem_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_info.allocationSize = mem_reqs.size;

    for (uint32_t idx = 0; idx < mem_flags_.size(); idx++) {
        if ((mem_reqs.memoryTypeBits & (1 << idx)) &&
            (mem_flags_[idx] & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
            (mem_flags_[idx] & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            // TODO is this guaranteed to exist?
            mem_info.memoryTypeIndex = idx;
            break;
        }
    }

    vk::AllocateMemory(dev_, &mem_info, nullptr, &frame_data_.mem);

    vk::MapMemory(dev_, frame_data_.mem, 0, VK_WHOLE_SIZE, 0,
            reinterpret_cast<void **>(&frame_data_.base));

    vk::BindBufferMemory(dev_, frame_data_.buf, frame_data_.mem, 0);
}

void Hologram::create_descriptor_set()
{
    if (use_push_constants_)
        return;

    const VkDescriptorType desc_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    const uint32_t desc_count = 1;

    VkDescriptorPoolSize pool_size = {};
    pool_size.type = desc_type;
    pool_size.descriptorCount = desc_count;

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;

    vk::assert_success(vk::CreateDescriptorPool(dev_, &pool_info, nullptr,
                &desc_pool_));

    VkDescriptorSetLayoutBinding layout_binding = {};
    layout_binding.binding = 0;
    layout_binding.descriptorType = desc_type;
    layout_binding.descriptorCount = desc_count;
    layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = &layout_binding;

    vk::assert_success(vk::CreateDescriptorSetLayout(dev_, &layout_info, nullptr,
                &desc_set_layout_));

    VkDescriptorSetAllocateInfo set_info = {};
    set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_info.descriptorPool = desc_pool_;
    set_info.descriptorSetCount = 1;
    set_info.pSetLayouts = &desc_set_layout_;

    vk::assert_success(vk::AllocateDescriptorSets(dev_, &set_info,
                &desc_set_));

    VkDescriptorBufferInfo desc_buf = {};
    desc_buf.buffer = frame_data_.buf;
    desc_buf.offset = 0;
    desc_buf.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet set_write = {};
    set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set_write.dstSet = desc_set_;
    set_write.dstBinding = 0;
    set_write.dstArrayElement = 0;
    set_write.descriptorCount = desc_count;
    set_write.descriptorType = desc_type;
    set_write.pBufferInfo = &desc_buf;

    vk::UpdateDescriptorSets(dev_, 1, &set_write, 0, nullptr);
}

void Hologram::create_render_pass()
{
    VkAttachmentDescription attachment = {};
    attachment.format = format_;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference attachment_ref = {};
    attachment_ref.attachment = 0;
    attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachment_ref;

    std::array<VkSubpassDependency, 2> subpass_deps;
    subpass_deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_deps[0].dstSubpass = 0;
    subpass_deps[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpass_deps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_deps[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpass_deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    subpass_deps[1].srcSubpass = 0;
    subpass_deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpass_deps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_deps[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpass_deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_deps[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpass_deps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = (uint32_t)subpass_deps.size();
    render_pass_info.pDependencies = subpass_deps.data();

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
    if (use_push_constants_) {
#include "Hologram.push_constant.vert.h"
        sh_info.codeSize = sizeof(Hologram_push_constant_vert);
        sh_info.pCode = Hologram_push_constant_vert;
    } else {
#include "Hologram.vert.h"
        sh_info.codeSize = sizeof(Hologram_vert);
        sh_info.pCode = Hologram_vert;
    }
    vk::assert_success(vk::CreateShaderModule(dev_, &sh_info, nullptr, &vs_));

#include "Hologram.frag.h"
    sh_info.codeSize = sizeof(Hologram_frag);
    sh_info.pCode = Hologram_frag;
    vk::assert_success(vk::CreateShaderModule(dev_, &sh_info, nullptr, &fs_));
}

void Hologram::create_pipeline_layout()
{
    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (use_push_constants_) {
        pipeline_layout_info.pushConstantRangeCount = 1;
        pipeline_layout_info.pPushConstantRanges = &frame_data_.push_const_range;
    } else {
        pipeline_layout_info.setLayoutCount = 1;
        pipeline_layout_info.pSetLayouts = &desc_set_layout_;
    }

    vk::assert_success(vk::CreatePipelineLayout(dev_, &pipeline_layout_info,
                nullptr, &pipeline_layout_));
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
    dynamic_info.dynamicStateCount = (uint32_t)dynamic_states.size();
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

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vk::assert_success(vk::CreateFence(dev_, &fence_info, nullptr, &primary_cmd_fence_));

    primary_cmd_begin_info_.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    primary_cmd_begin_info_.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // we will render to the swapchain images
    primary_cmd_submit_wait_stages_ = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    primary_cmd_submit_info_.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    primary_cmd_submit_info_.waitSemaphoreCount = 1;
    primary_cmd_submit_info_.pWaitDstStageMask = &primary_cmd_submit_wait_stages_;
    primary_cmd_submit_info_.commandBufferCount = 1;
    primary_cmd_submit_info_.pCommandBuffers = &primary_cmd_;
    primary_cmd_submit_info_.signalSemaphoreCount = 1;
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
    prepare_framebuffers(ctx.swapchain);

    update_projection();
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
}

void Hologram::prepare_framebuffers(VkSwapchainKHR swapchain)
{
    // get swapchain images
    vk::get(dev_, swapchain, images_);

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

void Hologram::update_projection()
{
    float aspect = static_cast<float>(extent_.width) / static_cast<float>(extent_.height);
    const glm::vec3 center(0.0f);
    const glm::vec3 up(0.f, 0.0f, 1.0f);
    const glm::mat4 view = glm::lookAt(eye_pos_, center, up);
    const glm::mat4 projection = glm::perspective(0.4f, aspect, 0.1f, 100.0f);
    view_projection_ = projection * view;
}

void Hologram::step_object(Object &obj, float obj_time, FrameData &data) const
{
    if (!pause_objects_) {
        glm::vec3 pos = obj.path.position(obj_time);
        glm::mat4 trans = obj.animation.transformation(obj_time);
        obj.model = glm::translate(glm::mat4(1.0f), pos) * trans;
    }

    if (use_push_constants_) {
        auto &mvp = data.object_mvp[obj.frame_data_offset];
        mvp = view_projection_ * obj.model;
    } else {
        glm::mat4 mvp = view_projection_ * obj.model;
        uint8_t *dst = data.base + obj.frame_data_offset;
        memcpy(dst, glm::value_ptr(mvp), sizeof(mvp));
    }
}

void Hologram::draw_object(const Object &obj, VkCommandBuffer cmd) const
{
    if (use_push_constants_) {
        const auto &mvp = frame_data_.object_mvp[obj.frame_data_offset];
        vk::CmdPushConstants(cmd, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT,
                0, sizeof(mvp), glm::value_ptr(mvp));
    } else {
        vk::CmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline_layout_, 0, 1, &desc_set_, 1, &obj.frame_data_offset);
    }

    meshes_->cmd_draw(cmd, obj.mesh);
}

void Hologram::step_objects(const Worker &worker)
{
    for (int i = worker.object_begin_; i < worker.object_end_; i++) {
        auto &obj = objects_[i];

        step_object(obj, worker.object_time_, frame_data_);
    }
}

void Hologram::draw_objects(Worker &worker)
{
    auto cmd = worker.cmd_;

    VkCommandBufferInheritanceInfo inherit_info = {};
    inherit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inherit_info.renderPass = render_pass_;
    inherit_info.framebuffer = worker.fb_;

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
    case KEY_UP:
        eye_pos_ -= glm::vec3(0.05f);
        update_projection();
        break;
    case KEY_DOWN:
        eye_pos_ += glm::vec3(0.05f);
        update_projection();
        break;
    case KEY_SPACE:
        pause_objects_ = !pause_objects_;
        break;
    default:
        break;
    }
}

void Hologram::on_tick()
{
    for (auto &worker : workers_)
        worker->step_objects();
}

void Hologram::on_frame(float frame_pred)
{
    // wait for the last submission since we reuse command buffers
    vk::assert_success(vk::WaitForFences(dev_, 1, &primary_cmd_fence_, true, UINT64_MAX));
    vk::assert_success(vk::ResetFences(dev_, 1, &primary_cmd_fence_));

    const Shell::BackBuffer &back = shell_->context().acquired_back_buffer;

    // ignore frame_pred
    for (auto &worker : workers_)
        worker->draw_objects(framebuffers_[back.image_index]);

    VkResult res = vk::BeginCommandBuffer(primary_cmd_, &primary_cmd_begin_info_);

    render_pass_begin_info_.framebuffer = framebuffers_[back.image_index];
    render_pass_begin_info_.renderArea.extent = extent_;
    vk::CmdBeginRenderPass(primary_cmd_, &render_pass_begin_info_,
            VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    // record render pass commands
    for (auto &worker : workers_)
        worker->wait_idle();
    vk::CmdExecuteCommands(primary_cmd_, (uint32_t)worker_cmds_.size(), worker_cmds_.data());

    vk::CmdEndRenderPass(primary_cmd_);
    vk::EndCommandBuffer(primary_cmd_);

    // wait for the image to be owned and signal for render completion
    primary_cmd_submit_info_.pWaitSemaphores = &back.acquire_semaphore;
    primary_cmd_submit_info_.pSignalSemaphores = &back.render_semaphore;

    res = vk::QueueSubmit(queue_, 1, &primary_cmd_submit_info_, primary_cmd_fence_);

    (void) res;
}

Hologram::Worker::Worker(Hologram &hologram, int object_begin, int object_end)
    : hologram_(hologram), object_begin_(object_begin), object_end_(object_end),
      object_time_(0.0f), state_(INIT),
      tick_interval_(1.0f / hologram.settings_.ticks_per_second)
{
}

void Hologram::Worker::start()
{
    state_ = IDLE;
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

void Hologram::Worker::step_objects()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        bool started = (state_ != INIT);

        object_time_ += tick_interval_;
        state_ = STEP;

        // step directly
        if (!started) {
            hologram_.step_objects(*this);
            state_ = INIT;
        }
    }
    state_cv_.notify_one();
}

void Hologram::Worker::draw_objects(VkFramebuffer fb)
{
    // wait for step_objects first
    wait_idle();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        bool started = (state_ != INIT);

        fb_ = fb;
        state_ = DRAW;

        // render directly
        if (!started) {
            hologram_.draw_objects(*this);
            state_ = INIT;
        }
    }
    state_cv_.notify_one();
}

void Hologram::Worker::wait_idle()
{
    std::unique_lock<std::mutex> lock(mutex_);
    bool started = (state_ != INIT);

    if (started)
        state_cv_.wait(lock, [this] { return (state_ == IDLE); });
}

void Hologram::Worker::update_loop()
{
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);

        state_cv_.wait(lock, [this] { return (state_ != IDLE); });
        if (state_ == INIT)
            break;

        assert(state_ == STEP || state_ == DRAW);
        if (state_ == STEP)
            hologram_.step_objects(*this);
        else
            hologram_.draw_objects(*this);

        state_ = IDLE;
        lock.unlock();
        state_cv_.notify_one();
    }
}
