#include <array>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Helpers.h"
#include "Hologram.h"
#include "Meshes.h"
#include "Shell.h"

Hologram::Hologram(const std::vector<std::string> &args)
    : Game("Hologram", args), random_dev_(),
      speed_rng_(random_dev_()), speed_dist_(1.0f, 10.0f),
      multithread_(true),
      render_pass_clear_value_(), render_pass_begin_info_(),
      primary_cmd_begin_info_(), primary_cmd_submit_info_()
{
    for (auto it = args.begin(); it != args.end(); ++it) {
        if (*it == "-s")
            multithread_ = false;
        else if (*it == "-p")
            use_push_constants_ = true;
    }

    init_workers();
}

Hologram::~Hologram()
{
}

void Hologram::init_workers()
{
    int worker_count = std::thread::hardware_concurrency();

    // not enough cores
    if (!multithread_ || worker_count < 2) {
        multithread_ = false;
        worker_count = 1;
    }

    const int object_per_worker = sim_.objects().size() / worker_count;
    int object_begin = 0, object_end = 0;

    workers_.reserve(worker_count);
    for (int i = 0; i < worker_count; i++) {
        object_begin = object_end;
        if (i < worker_count - 1)
            object_end += object_per_worker;
        else
            object_end = sim_.objects().size();

        Worker *worker = new Worker(*this, i, object_begin, object_end);
        workers_.emplace_back(std::unique_ptr<Worker>(worker));
    }
}

void Hologram::init_objects()
{
    objects_.reserve(OBJECT_COUNT);
    for (int i = 0; i < OBJECT_COUNT; i++) {
        float x = speed_dist_(speed_rng_) / 5.0f;
        float y = speed_dist_(speed_rng_) / 5.0f;
        float z = speed_dist_(speed_rng_) / 5.0f;

        Object obj = {};

        obj.mesh = i;

        obj.scale = glm::vec3(0.01f);
        obj.speed = speed_dist_(speed_rng_);
        obj.pos = glm::vec3(x, y, z);

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

    meshes_ = new Meshes(sim_.rng_seed(), mem_flags_, dev_, sim_.objects().size());

    create_render_pass();
    create_shader_modules();
    create_descriptor_set_layout();
    create_pipeline_layout();
    create_pipeline();

    create_frame_data(2);

    render_pass_begin_info_.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info_.renderPass = render_pass_;
    render_pass_begin_info_.clearValueCount = 1;
    render_pass_begin_info_.pClearValues = &render_pass_clear_value_;

    primary_cmd_begin_info_.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    primary_cmd_begin_info_.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // we will render to the swapchain images
    primary_cmd_submit_wait_stages_ = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    primary_cmd_submit_info_.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    primary_cmd_submit_info_.waitSemaphoreCount = 1;
    primary_cmd_submit_info_.pWaitDstStageMask = &primary_cmd_submit_wait_stages_;
    primary_cmd_submit_info_.commandBufferCount = 1;
    primary_cmd_submit_info_.signalSemaphoreCount = 1;

    if (multithread_) {
        for (auto &worker : workers_)
            worker->start();
    }
}

void Hologram::detach_shell()
{
    if (multithread_) {
        for (auto &worker : workers_)
            worker->stop();
    }

    vk::DestroyCommandPool(dev_, primary_cmd_pool_, nullptr);

    vk::DestroyPipeline(dev_, pipeline_, nullptr);
    vk::DestroyPipelineLayout(dev_, pipeline_layout_, nullptr);
    if (!use_push_constants_)
        vk::DestroyDescriptorSetLayout(dev_, desc_set_layout_, nullptr);
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

void Hologram::create_descriptor_set_layout()
{
    if (use_push_constants_)
        return;

    VkDescriptorSetLayoutBinding layout_binding = {};
    layout_binding.binding = 0;
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    layout_binding.descriptorCount = 1;
    layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = &layout_binding;

    vk::assert_success(vk::CreateDescriptorSetLayout(dev_, &layout_info,
                nullptr, &desc_set_layout_));
}

void Hologram::create_pipeline_layout()
{
    VkPushConstantRange push_const_range = {};
    push_const_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_const_range.offset = 0;
    push_const_range.size = sizeof(glm::mat4);

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (use_push_constants_) {
        push_const_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_const_range.offset = 0;
        push_const_range.size = sizeof(glm::mat4);

        pipeline_layout_info.pushConstantRangeCount = 1;
        pipeline_layout_info.pPushConstantRanges = &push_const_range;
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

void Hologram::create_frame_data(int count)
{
    frame_data_.resize(count);

    create_fences();
    create_command_buffers();

    if (!use_push_constants_) {
        create_buffers();
        create_buffer_memory();
        create_descriptor_sets();
    }

    frame_data_index_ = 0;
}

void Hologram::destroy_frame_data()
{
    if (!use_push_constants_) {
        vk::DestroyDescriptorPool(dev_, desc_pool_, nullptr);

        for (auto cmd_pool : worker_cmd_pools_)
            vk::DestroyCommandPool(dev_, cmd_pool, nullptr);
        worker_cmd_pools_.clear();
        vk::DestroyCommandPool(dev_, primary_cmd_pool_, nullptr);

        vk::UnmapMemory(dev_, frame_data_mem_);
        vk::FreeMemory(dev_, frame_data_mem_, nullptr);

        for (auto &data : frame_data_)
            vk::DestroyBuffer(dev_, data.buf, nullptr);
    }

    for (auto &data : frame_data_)
        vk::DestroyFence(dev_, data.fence, nullptr);

    frame_data_.clear();
}

void Hologram::create_fences()
{
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto &data : frame_data_)
        vk::assert_success(vk::CreateFence(dev_, &fence_info, nullptr, &data.fence));
}

void Hologram::create_command_buffers()
{
    VkCommandPoolCreateInfo cmd_pool_info = {};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_info.queueFamilyIndex = queue_family_;

    VkCommandBufferAllocateInfo cmd_info = {};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_info.commandBufferCount = static_cast<uint32_t>(frame_data_.size());

    // create command pools and buffers
    std::vector<VkCommandPool> cmd_pools(workers_.size() + 1, VK_NULL_HANDLE);
    std::vector<std::vector<VkCommandBuffer>> cmds_vec(workers_.size() + 1,
            std::vector<VkCommandBuffer>(frame_data_.size(), VK_NULL_HANDLE));
    for (size_t i = 0; i < cmd_pools.size(); i++) {
        auto &cmd_pool = cmd_pools[i];
        auto &cmds = cmds_vec[i];

        vk::assert_success(vk::CreateCommandPool(dev_, &cmd_pool_info,
                    nullptr, &cmd_pool));

    primary_cmd_begin_info_.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    primary_cmd_begin_info_.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    primary_cmd_submit_info_.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    primary_cmd_submit_info_.commandBufferCount = 1;
    primary_cmd_submit_info_.pCommandBuffers = &primary_cmd_;
}

void Hologram::create_buffers()
{
    VkDeviceSize object_data_size = sizeof(glm::mat4);
    // align object data to device limit
    const VkDeviceSize &alignment =
        physical_dev_props_.limits.minStorageBufferOffsetAlignment;
    if (object_data_size % alignment)
        object_data_size += alignment - (object_data_size % alignment);

    // update simulation
    sim_.set_frame_data_size(object_data_size);

    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.size = object_data_size * sim_.objects().size();
    buf_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    for (auto &data : frame_data_)
        vk::assert_success(vk::CreateBuffer(dev_, &buf_info, nullptr, &data.buf));
}

void Hologram::create_buffer_memory()
{
    VkMemoryRequirements mem_reqs;
    vk::GetBufferMemoryRequirements(dev_, frame_data_[0].buf, &mem_reqs);

    VkDeviceSize aligned_size = mem_reqs.size;
    if (aligned_size % mem_reqs.alignment)
        aligned_size += mem_reqs.alignment - (aligned_size % mem_reqs.alignment);

    // allocate memory
    VkMemoryAllocateInfo mem_info = {};
    mem_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_info.allocationSize = aligned_size * (frame_data_.size() - 1) +
        mem_reqs.size;

    for (uint32_t idx = 0; idx < mem_flags_.size(); idx++) {
        if ((mem_reqs.memoryTypeBits & (1 << idx)) &&
            (mem_flags_[idx] & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
            (mem_flags_[idx] & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            // TODO is this guaranteed to exist?
            mem_info.memoryTypeIndex = idx;
            break;
        }
    }

    vk::AllocateMemory(dev_, &mem_info, nullptr, &frame_data_mem_);

    void *ptr;
    vk::MapMemory(dev_, frame_data_mem_, 0, VK_WHOLE_SIZE, 0, &ptr);

    VkDeviceSize offset = 0;
    for (auto &data : frame_data_) {
        vk::BindBufferMemory(dev_, data.buf, frame_data_mem_, offset);
        data.base = reinterpret_cast<uint8_t *>(ptr) + offset;
        offset += aligned_size;
    }
}

void Hologram::create_descriptor_sets()
{
    VkDescriptorPoolSize desc_pool_size = {};
    desc_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    desc_pool_size.descriptorCount = frame_data_.size();

    VkDescriptorPoolCreateInfo desc_pool_info = {};
    desc_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    desc_pool_info.maxSets = frame_data_.size();
    desc_pool_info.poolSizeCount = 1;
    desc_pool_info.pPoolSizes = &desc_pool_size;

    // create descriptor pool
    vk::assert_success(vk::CreateDescriptorPool(dev_, &desc_pool_info,
                nullptr, &desc_pool_));

    std::vector<VkDescriptorSetLayout> set_layouts(frame_data_.size(), desc_set_layout_);
    VkDescriptorSetAllocateInfo set_info = {};
    set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_info.descriptorPool = desc_pool_;
    set_info.descriptorSetCount = static_cast<uint32_t>(set_layouts.size());
    set_info.pSetLayouts = set_layouts.data();

    // create descriptor sets
    std::vector<VkDescriptorSet> desc_sets(frame_data_.size(), VK_NULL_HANDLE);
    vk::assert_success(vk::AllocateDescriptorSets(dev_, &set_info, desc_sets.data()));

    std::vector<VkDescriptorBufferInfo> desc_bufs(frame_data_.size());
    std::vector<VkWriteDescriptorSet> desc_writes(frame_data_.size());

    for (size_t i = 0; i < frame_data_.size(); i++) {
        auto &data = frame_data_[i];

        data.desc_set = desc_sets[i];

        VkDescriptorBufferInfo desc_buf = {};
        desc_buf.buffer = data.buf;
        desc_buf.offset = 0;
        desc_buf.range = VK_WHOLE_SIZE;
        desc_bufs[i] = desc_buf;

        VkWriteDescriptorSet desc_write = {};
        desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc_write.dstSet = data.desc_set;
        desc_write.dstBinding = 0;
        desc_write.dstArrayElement = 0;
        desc_write.descriptorCount = 1;
        desc_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        desc_write.pBufferInfo = &desc_bufs[i];
        desc_writes[i] = desc_write;
    }

    vk::UpdateDescriptorSets(dev_,
            static_cast<uint32_t>(desc_writes.size()),
            desc_writes.data(), 0, nullptr);
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
    glm::vec3 disp(worker.distribution_(worker.rng_),
                   worker.distribution_(worker.rng_),
                   worker.distribution_(worker.rng_));

    obj.pos += disp * obj.speed * worker.time_delta_;
}

void Hologram::draw_object(const Object &obj, VkCommandBuffer cmd) const
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, obj.pos);
    model = glm::scale(model, obj.scale);

    glm::mat4 mvp = view_projection_ * model;
    vk::CmdPushConstants(cmd, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mvp), glm::value_ptr(mvp));

    meshes_->cmd_draw(cmd, obj.mesh);
}

void Hologram::update_objects(Worker &worker)
{
    auto &data = frame_data_[frame_data_index_];
    auto cmd = data.worker_cmds[worker.index_];

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
        auto &obj = sim_.objects()[i];

        step_object(obj, worker);
        draw_object(obj, cmd);
    }

    vk::EndCommandBuffer(cmd);
}

void Hologram::on_frame(float frame_time, int fb)
{
    if (sim_paused_)
        return;

    for (auto &worker : workers_)
        worker->render(frame_time, fb);

    for (auto &worker : workers_)
        worker->wait_render();

    VkResult res = vk::BeginCommandBuffer(data.primary_cmd, &primary_cmd_begin_info_);

    render_pass_begin_info_.framebuffer = framebuffers_[fb];
    render_pass_begin_info_.renderArea.extent = extent_;
    vk::CmdBeginRenderPass(data.primary_cmd, &render_pass_begin_info_,
            VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    // record render pass commands
    vk::CmdExecuteCommands(primary_cmd_, worker_cmds_.size(), worker_cmds_.data());

    vk::CmdEndRenderPass(data.primary_cmd);
    vk::EndCommandBuffer(data.primary_cmd);

    res = vk::QueueSubmit(queue_, 1, &primary_cmd_submit_info_, VK_NULL_HANDLE);

    (void) res;
}

Hologram::Worker::Worker(Hologram &hologram, int object_begin, int object_end)
    : hologram_(hologram), object_begin_(object_begin), object_end_(object_end),
      rng_(hologram_.random_dev_()), distribution_(-0.02f, 0.02f), state_(INIT)
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

        time_delta_ = frame_time - frame_time_;
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
