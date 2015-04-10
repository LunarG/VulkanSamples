/*
 * Draw a textured triangle with depth testing.  This is written against Intel
 * ICD.  It does not do state transition nor object memory binding like it
 * should.  It also does no error checking.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <xcb/xcb.h>
#include <vulkan.h>
#include <vkDbg.h>
#include <vkWsiX11Ext.h>

#include "icd-spv.h"

#define DEMO_BUFFER_COUNT 2
#define DEMO_TEXTURE_COUNT 1
#define VERTEX_BUFFER_BIND_ID 0

struct texture_object {
    VkSampler sampler;

    VkImage image;
    VkImageLayout imageLayout;

    uint32_t  num_mem;
    VkGpuMemory *mem;
    VkImageView view;
    int32_t tex_width, tex_height;
};

struct demo {
    xcb_connection_t *connection;
    xcb_screen_t *screen;

    VkInstance inst;
    VkPhysicalGpu gpu;
    VkDevice device;
    VkQueue queue;
    VkPhysicalGpuProperties *gpu_props;
    VkPhysicalGpuQueueProperties *queue_props;
    uint32_t graphics_queue_node_index;

    int width, height;
    VkFormat format;

    struct {
        VkImage image;
        VkGpuMemory mem;

        VkColorAttachmentView view;
        VkFence fence;
    } buffers[DEMO_BUFFER_COUNT];

    struct {
        VkFormat format;

        VkImage image;
        uint32_t  num_mem;
        VkGpuMemory *mem;
        VkDepthStencilView view;
    } depth;

    struct texture_object textures[DEMO_TEXTURE_COUNT];

    struct {
        VkBuffer buf;
        uint32_t  num_mem;
        VkGpuMemory *mem;

        VkPipelineVertexInputCreateInfo vi;
        VkVertexInputBindingDescription vi_bindings[1];
        VkVertexInputAttributeDescription vi_attrs[2];
    } vertices;

    VkCmdBuffer cmd;  // Buffer for initialization commands
    VkDescriptorSetLayoutChain desc_layout_chain;
    VkDescriptorSetLayout desc_layout;
    VkPipeline pipeline;

    VkDynamicVpState viewport;
    VkDynamicRsState raster;
    VkDynamicCbState color_blend;
    VkDynamicDsState depth_stencil;

    VkDescriptorPool desc_pool;
    VkDescriptorSet desc_set;

    xcb_window_t window;
    xcb_intern_atom_reply_t *atom_wm_delete_window;

    bool quit;
    bool use_staging_buffer;
    uint32_t current_buffer;
};

static void demo_flush_init_cmd(struct demo *demo)
{
    VkResult err;

    if (demo->cmd == VK_NULL_HANDLE)
        return;

    err = vkEndCommandBuffer(demo->cmd);
    assert(!err);

    const VkCmdBuffer cmd_bufs[] = { demo->cmd };

    err = vkQueueSubmit(demo->queue, 1, cmd_bufs, VK_NULL_HANDLE);
    assert(!err);

    err = vkQueueWaitIdle(demo->queue);
    assert(!err);

    vkDestroyObject(demo->cmd);
    demo->cmd = VK_NULL_HANDLE;
}

static void demo_add_mem_refs(
        struct demo *demo,
        int num_refs, VkGpuMemory *mem)
{
    for (int i = 0; i < num_refs; i++) {
        vkQueueAddMemReference(demo->queue, mem[i]);
    }
}

static void demo_remove_mem_refs(
        struct demo *demo,
        int num_refs, VkGpuMemory *mem)
{
    for (int i = 0; i < num_refs; i++) {
        vkQueueRemoveMemReference(demo->queue, mem[i]);
    }
}

static void demo_set_image_layout(
        struct demo *demo,
        VkImage image,
        VkImageLayout old_image_layout,
        VkImageLayout new_image_layout)
{
    VkResult err;

    if (demo->cmd == VK_NULL_HANDLE) {
        const VkCmdBufferCreateInfo cmd = {
            .sType = VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO,
            .pNext = NULL,
            .queueNodeIndex = demo->graphics_queue_node_index,
            .flags = 0,
        };

        err = vkCreateCommandBuffer(demo->device, &cmd, &demo->cmd);
        assert(!err);

        VkCmdBufferBeginInfo cmd_buf_info = {
            .sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO,
            .pNext = NULL,
            .flags = VK_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT |
                VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT,
        };
        err = vkBeginCommandBuffer(demo->cmd, &cmd_buf_info);
    }

    VkImageMemoryBarrier image_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .outputMask = 0,
        .inputMask = 0,
        .oldLayout = old_image_layout,
        .newLayout = new_image_layout,
        .image = image,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR, 0, 1, 0, 0 }
    };

    if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL) {
        /* Make sure anything that was copying from this image has completed */
        image_memory_barrier.inputMask = VK_MEMORY_INPUT_COPY_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        /* Make sure any Copy or CPU writes to image are flushed */
        image_memory_barrier.outputMask = VK_MEMORY_OUTPUT_COPY_BIT | VK_MEMORY_OUTPUT_CPU_WRITE_BIT;
    }

    VkImageMemoryBarrier *pmemory_barrier = &image_memory_barrier;

    VkPipeEvent set_events[] = { VK_PIPE_EVENT_TOP_OF_PIPE };

    VkPipelineBarrier pipeline_barrier;
    pipeline_barrier.sType = VK_STRUCTURE_TYPE_PIPELINE_BARRIER;
    pipeline_barrier.pNext = NULL;
    pipeline_barrier.eventCount = 1;
    pipeline_barrier.pEvents = set_events;
    pipeline_barrier.waitEvent = VK_WAIT_EVENT_TOP_OF_PIPE;
    pipeline_barrier.memBarrierCount = 1;
    pipeline_barrier.ppMemBarriers = (const void **)&pmemory_barrier;

    vkCmdPipelineBarrier(demo->cmd, &pipeline_barrier);
}

static void demo_draw_build_cmd(struct demo *demo)
{
    const VkColorAttachmentBindInfo color_attachment = {
        .view = demo->buffers[demo->current_buffer].view,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkDepthStencilBindInfo depth_stencil = {
        .view = demo->depth.view,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    const VkClearColor clear_color = {
        .color.floatColor = { 0.2f, 0.2f, 0.2f, 0.2f },
        .useRawValue = false,
    };
    const float clear_depth = 0.9f;
    VkImageSubresourceRange clear_range;
    VkCmdBufferBeginInfo cmd_buf_info = {
        .sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT |
            VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT,
    };
    VkResult err;
    VkAttachmentLoadOp load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    VkAttachmentStoreOp store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    const VkFramebufferCreateInfo fb_info = {
         .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
         .pNext = NULL,
         .colorAttachmentCount = 1,
         .pColorAttachments = (VkColorAttachmentBindInfo*) &color_attachment,
         .pDepthStencilAttachment = (VkDepthStencilBindInfo*) &depth_stencil,
         .sampleCount = 1,
         .width  = demo->width,
         .height = demo->height,
         .layers = 1,
    };
    VkRenderPassCreateInfo rp_info;
    VkRenderPassBegin rp_begin;

    memset(&rp_info, 0 , sizeof(rp_info));
    err = vkCreateFramebuffer(demo->device, &fb_info, &rp_begin.framebuffer);
    assert(!err);
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.renderArea.extent.width = demo->width;
    rp_info.renderArea.extent.height = demo->height;
    rp_info.colorAttachmentCount = fb_info.colorAttachmentCount;
    rp_info.pColorFormats = &demo->format;
    rp_info.pColorLayouts = &color_attachment.layout;
    rp_info.pColorLoadOps = &load_op;
    rp_info.pColorStoreOps = &store_op;
    rp_info.pColorLoadClearValues = &clear_color;
    rp_info.depthStencilFormat = VK_FMT_D16_UNORM;
    rp_info.depthStencilLayout = depth_stencil.layout;
    rp_info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    rp_info.depthLoadClearValue = clear_depth;
    rp_info.depthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    rp_info.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    rp_info.stencilLoadClearValue = 0;
    rp_info.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    err = vkCreateRenderPass(demo->device, &rp_info, &(rp_begin.renderPass));
    assert(!err);

    err = vkBeginCommandBuffer(demo->cmd, &cmd_buf_info);
    assert(!err);

    vkCmdBindPipeline(demo->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  demo->pipeline);
    vkCmdBindDescriptorSets(demo->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            demo->desc_layout_chain, 0, 1, & demo->desc_set, NULL);

    vkCmdBindDynamicStateObject(demo->cmd, VK_STATE_BIND_VIEWPORT, demo->viewport);
    vkCmdBindDynamicStateObject(demo->cmd, VK_STATE_BIND_RASTER, demo->raster);
    vkCmdBindDynamicStateObject(demo->cmd, VK_STATE_BIND_COLOR_BLEND,
                                     demo->color_blend);
    vkCmdBindDynamicStateObject(demo->cmd, VK_STATE_BIND_DEPTH_STENCIL,
                                     demo->depth_stencil);


    vkCmdBindVertexBuffer(demo->cmd, demo->vertices.buf, 0, VERTEX_BUFFER_BIND_ID);

    vkCmdBeginRenderPass(demo->cmd, &rp_begin);
    clear_range.aspect = VK_IMAGE_ASPECT_COLOR;
    clear_range.baseMipLevel = 0;
    clear_range.mipLevels = 1;
    clear_range.baseArraySlice = 0;
    clear_range.arraySize = 1;
    vkCmdClearColorImage(demo->cmd,
            demo->buffers[demo->current_buffer].image,
            VK_IMAGE_LAYOUT_CLEAR_OPTIMAL,
            clear_color, 1, &clear_range);

    clear_range.aspect = VK_IMAGE_ASPECT_DEPTH;
    vkCmdClearDepthStencil(demo->cmd,
            demo->depth.image, VK_IMAGE_LAYOUT_CLEAR_OPTIMAL,
            clear_depth, 0, 1, &clear_range);

    vkCmdDraw(demo->cmd, 0, 3, 0, 1);
    vkCmdEndRenderPass(demo->cmd, rp_begin.renderPass);

    err = vkEndCommandBuffer(demo->cmd);
    assert(!err);

    vkDestroyObject(rp_begin.renderPass);
    vkDestroyObject(rp_begin.framebuffer);
}

static void demo_draw(struct demo *demo)
{
    const VK_WSI_X11_PRESENT_INFO present = {
        .destWindow = demo->window,
        .srcImage = demo->buffers[demo->current_buffer].image,
    };
    VkFence fence = demo->buffers[demo->current_buffer].fence;
    VkResult err;

    demo_draw_build_cmd(demo);

    err = vkWaitForFences(demo->device, 1, &fence, VK_TRUE, ~((uint64_t) 0));
    assert(err == VK_SUCCESS || err == VK_ERROR_UNAVAILABLE);

    err = vkQueueSubmit(demo->queue, 1, &demo->cmd, VK_NULL_HANDLE);
    assert(!err);

    err = vkWsiX11QueuePresent(demo->queue, &present, fence);
    assert(!err);

    demo->current_buffer = (demo->current_buffer + 1) % DEMO_BUFFER_COUNT;
}

static void demo_prepare_buffers(struct demo *demo)
{
    const VK_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO presentable_image = {
        .format = demo->format,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .extent = {
            .width = demo->width,
            .height = demo->height,
        },
        .flags = 0,
    };
    const VkFenceCreateInfo fence = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
    };
    VkResult err;
    uint32_t i;

    for (i = 0; i < DEMO_BUFFER_COUNT; i++) {
        VkColorAttachmentViewCreateInfo color_attachment_view = {
            .sType = VK_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO,
            .pNext = NULL,
            .format = demo->format,
            .mipLevel = 0,
            .baseArraySlice = 0,
            .arraySize = 1,
        };

        err = vkWsiX11CreatePresentableImage(demo->device, &presentable_image,
                &demo->buffers[i].image, &demo->buffers[i].mem);
        assert(!err);
        demo_add_mem_refs(demo, 1, &demo->buffers[i].mem);
        demo_set_image_layout(demo, demo->buffers[i].image,
                               VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        color_attachment_view.image = demo->buffers[i].image;

        err = vkCreateColorAttachmentView(demo->device,
                &color_attachment_view, &demo->buffers[i].view);
        assert(!err);

        err = vkCreateFence(demo->device,
                &fence, &demo->buffers[i].fence);
        assert(!err);
    }

    demo->current_buffer = 0;
}

static void demo_prepare_depth(struct demo *demo)
{
    const VkFormat depth_format = VK_FMT_D16_UNORM;
    const VkImageCreateInfo image = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_2D,
        .format = depth_format,
        .extent = { demo->width, demo->height, 1 },
        .mipLevels = 1,
        .arraySize = 1,
        .samples = 1,
        .tiling = VK_OPTIMAL_TILING,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_BIT,
        .flags = 0,
    };
    VkMemoryAllocImageInfo img_alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO,
        .pNext = NULL,
    };
    VkMemoryAllocInfo mem_alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = &img_alloc,
        .allocationSize = 0,
        .memProps = VK_MEMORY_PROPERTY_GPU_ONLY,
        .memType = VK_MEMORY_TYPE_IMAGE,
        .memPriority = VK_MEMORY_PRIORITY_NORMAL,
    };
    VkDepthStencilViewCreateInfo view = {
        .sType = VK_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO,
        .pNext = NULL,
        .image = VK_NULL_HANDLE,
        .mipLevel = 0,
        .baseArraySlice = 0,
        .arraySize = 1,
        .flags = 0,
    };

    VkMemoryRequirements *mem_reqs;
    size_t mem_reqs_size = sizeof(VkMemoryRequirements);
    VkImageMemoryRequirements img_reqs;
    size_t img_reqs_size = sizeof(VkImageMemoryRequirements);
    VkResult err;
    uint32_t num_allocations = 0;
    size_t num_alloc_size = sizeof(num_allocations);

    demo->depth.format = depth_format;

    /* create image */
    err = vkCreateImage(demo->device, &image,
            &demo->depth.image);
    assert(!err);

    err = vkGetObjectInfo(demo->depth.image, VK_INFO_TYPE_MEMORY_ALLOCATION_COUNT, &num_alloc_size, &num_allocations);
    assert(!err && num_alloc_size == sizeof(num_allocations));
    mem_reqs = malloc(num_allocations * sizeof(VkMemoryRequirements));
    demo->depth.mem = malloc(num_allocations * sizeof(VkGpuMemory));
    demo->depth.num_mem = num_allocations;
    err = vkGetObjectInfo(demo->depth.image,
                    VK_INFO_TYPE_MEMORY_REQUIREMENTS,
                    &mem_reqs_size, mem_reqs);
    assert(!err && mem_reqs_size == num_allocations * sizeof(VkMemoryRequirements));
    err = vkGetObjectInfo(demo->depth.image,
                    VK_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS,
                    &img_reqs_size, &img_reqs);
    assert(!err && img_reqs_size == sizeof(VkImageMemoryRequirements));
    img_alloc.usage = img_reqs.usage;
    img_alloc.formatClass = img_reqs.formatClass;
    img_alloc.samples = img_reqs.samples;
    for (uint32_t i = 0; i < num_allocations; i ++) {
        mem_alloc.allocationSize = mem_reqs[i].size;

        /* allocate memory */
        err = vkAllocMemory(demo->device, &mem_alloc,
                    &(demo->depth.mem[i]));
        assert(!err);

        /* bind memory */
        err = vkBindObjectMemory(demo->depth.image, i,
                demo->depth.mem[i], 0);
        assert(!err);
    }

    demo_set_image_layout(demo, demo->depth.image,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    demo_add_mem_refs(demo, demo->depth.num_mem, demo->depth.mem);

    /* create image view */
    view.image = demo->depth.image;
    err = vkCreateDepthStencilView(demo->device, &view,
            &demo->depth.view);
    assert(!err);
}

static void demo_prepare_texture_image(struct demo *demo,
                                       const uint32_t *tex_colors,
                                       struct texture_object *tex_obj,
                                       VkImageTiling tiling,
                                       VkFlags mem_props)
{
    const VkFormat tex_format = VK_FMT_B8G8R8A8_UNORM;
    const int32_t tex_width = 2;
    const int32_t tex_height = 2;
    VkResult err;

    tex_obj->tex_width = tex_width;
    tex_obj->tex_height = tex_height;

    const VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_2D,
        .format = tex_format,
        .extent = { tex_width, tex_height, 1 },
        .mipLevels = 1,
        .arraySize = 1,
        .samples = 1,
        .tiling = tiling,
        .usage = VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT,
        .flags = 0,
    };
    VkMemoryAllocImageInfo img_alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO,
        .pNext = NULL,
    };
    VkMemoryAllocInfo mem_alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = &img_alloc,
        .allocationSize = 0,
        .memProps = mem_props,
        .memType = VK_MEMORY_TYPE_IMAGE,
        .memPriority = VK_MEMORY_PRIORITY_NORMAL,
    };

    VkMemoryRequirements *mem_reqs;
    size_t mem_reqs_size = sizeof(VkMemoryRequirements);
    VkImageMemoryRequirements img_reqs;
    size_t img_reqs_size = sizeof(VkImageMemoryRequirements);
    uint32_t num_allocations = 0;
    size_t num_alloc_size = sizeof(num_allocations);

    err = vkCreateImage(demo->device, &image_create_info,
            &tex_obj->image);
    assert(!err);

    err = vkGetObjectInfo(tex_obj->image,
                VK_INFO_TYPE_MEMORY_ALLOCATION_COUNT,
                &num_alloc_size, &num_allocations);
    assert(!err && num_alloc_size == sizeof(num_allocations));
    mem_reqs = malloc(num_allocations * sizeof(VkMemoryRequirements));
    tex_obj->mem = malloc(num_allocations * sizeof(VkGpuMemory));
    err = vkGetObjectInfo(tex_obj->image,
                VK_INFO_TYPE_MEMORY_REQUIREMENTS,
                &mem_reqs_size, mem_reqs);
    assert(!err && mem_reqs_size == num_allocations * sizeof(VkMemoryRequirements));
    err = vkGetObjectInfo(tex_obj->image,
                    VK_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS,
                    &img_reqs_size, &img_reqs);
    assert(!err && img_reqs_size == sizeof(VkImageMemoryRequirements));
    img_alloc.usage = img_reqs.usage;
    img_alloc.formatClass = img_reqs.formatClass;
    img_alloc.samples = img_reqs.samples;
    mem_alloc.memProps = VK_MEMORY_PROPERTY_CPU_VISIBLE_BIT;
    for (uint32_t j = 0; j < num_allocations; j ++) {
        mem_alloc.allocationSize = mem_reqs[j].size;
        mem_alloc.memType = mem_reqs[j].memType;

        /* allocate memory */
        err = vkAllocMemory(demo->device, &mem_alloc,
                    &(tex_obj->mem[j]));
        assert(!err);

        /* bind memory */
        err = vkBindObjectMemory(tex_obj->image, j, tex_obj->mem[j], 0);
        assert(!err);
    }
    free(mem_reqs);
    mem_reqs = NULL;

    tex_obj->num_mem = num_allocations;

    if (mem_props & VK_MEMORY_PROPERTY_CPU_VISIBLE_BIT) {
        const VkImageSubresource subres = {
            .aspect = VK_IMAGE_ASPECT_COLOR,
            .mipLevel = 0,
            .arraySlice = 0,
        };
        VkSubresourceLayout layout;
        size_t layout_size = sizeof(VkSubresourceLayout);
        void *data;
        int32_t x, y;

        err = vkGetImageSubresourceInfo(tex_obj->image, &subres,
                                         VK_INFO_TYPE_SUBRESOURCE_LAYOUT,
                                         &layout_size, &layout);
        assert(!err && layout_size == sizeof(layout));
        /* Linear texture must be within a single memory object */
        assert(num_allocations == 1);

        err = vkMapMemory(tex_obj->mem[0], 0, &data);
        assert(!err);

        for (y = 0; y < tex_height; y++) {
            uint32_t *row = (uint32_t *) ((char *) data + layout.rowPitch * y);
            for (x = 0; x < tex_width; x++)
                row[x] = tex_colors[(x & 1) ^ (y & 1)];
        }

        err = vkUnmapMemory(tex_obj->mem[0]);
        assert(!err);
    }

    tex_obj->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    demo_set_image_layout(demo, tex_obj->image,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           tex_obj->imageLayout);
    /* setting the image layout does not reference the actual memory so no need to add a mem ref */
}

static void demo_destroy_texture_image(struct texture_object *tex_obj)
{
    /* clean up staging resources */
    for (uint32_t j = 0; j < tex_obj->num_mem; j ++) {
        vkBindObjectMemory(tex_obj->image, j, VK_NULL_HANDLE, 0);
        vkFreeMemory(tex_obj->mem[j]);
    }

    free(tex_obj->mem);
    vkDestroyObject(tex_obj->image);
}

static void demo_prepare_textures(struct demo *demo)
{
    const VkFormat tex_format = VK_FMT_B8G8R8A8_UNORM;
    VkFormatProperties props;
    size_t size = sizeof(props);
    const uint32_t tex_colors[DEMO_TEXTURE_COUNT][2] = {
        { 0xffff0000, 0xff00ff00 },
    };
    VkResult err;
    uint32_t i;

    err = vkGetFormatInfo(demo->device, tex_format,
                           VK_INFO_TYPE_FORMAT_PROPERTIES,
                           &size, &props);
    assert(!err);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        if ((props.linearTilingFeatures & VK_FORMAT_IMAGE_SHADER_READ_BIT) && !demo->use_staging_buffer) {
            /* Device can texture using linear textures */
            demo_prepare_texture_image(demo, tex_colors[i], &demo->textures[i],
                                       VK_LINEAR_TILING, VK_MEMORY_PROPERTY_CPU_VISIBLE_BIT);
        } else if (props.optimalTilingFeatures & VK_FORMAT_IMAGE_SHADER_READ_BIT){
            /* Must use staging buffer to copy linear texture to optimized */
            struct texture_object staging_texture;

            memset(&staging_texture, 0, sizeof(staging_texture));
            demo_prepare_texture_image(demo, tex_colors[i], &staging_texture,
                                       VK_LINEAR_TILING, VK_MEMORY_PROPERTY_CPU_VISIBLE_BIT);

            demo_prepare_texture_image(demo, tex_colors[i], &demo->textures[i],
                                       VK_OPTIMAL_TILING, VK_MEMORY_PROPERTY_GPU_ONLY);

            demo_set_image_layout(demo, staging_texture.image,
                                   staging_texture.imageLayout,
                                   VK_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL);

            demo_set_image_layout(demo, demo->textures[i].image,
                                   demo->textures[i].imageLayout,
                                   VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL);

            VkImageCopy copy_region = {
                .srcSubresource = { VK_IMAGE_ASPECT_COLOR, 0, 0 },
                .srcOffset = { 0, 0, 0 },
                .destSubresource = { VK_IMAGE_ASPECT_COLOR, 0, 0 },
                .destOffset = { 0, 0, 0 },
                .extent = { staging_texture.tex_width, staging_texture.tex_height, 1 },
            };
            vkCmdCopyImage(demo->cmd,
                            staging_texture.image, VK_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL,
                            demo->textures[i].image, VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL,
                            1, &copy_region);

            demo_add_mem_refs(demo, staging_texture.num_mem, staging_texture.mem);
            demo_add_mem_refs(demo, demo->textures[i].num_mem, demo->textures[i].mem);

            demo_set_image_layout(demo, demo->textures[i].image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL,
                                   demo->textures[i].imageLayout);

            demo_flush_init_cmd(demo);

            demo_destroy_texture_image(&staging_texture);
            demo_remove_mem_refs(demo, staging_texture.num_mem, staging_texture.mem);
        } else {
            /* Can't support VK_FMT_B8G8R8A8_UNORM !? */
            assert(!"No support for B8G8R8A8_UNORM as texture image format");
        }

        const VkSamplerCreateInfo sampler = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = NULL,
            .magFilter = VK_TEX_FILTER_NEAREST,
            .minFilter = VK_TEX_FILTER_NEAREST,
            .mipMode = VK_TEX_MIPMAP_BASE,
            .addressU = VK_TEX_ADDRESS_WRAP,
            .addressV = VK_TEX_ADDRESS_WRAP,
            .addressW = VK_TEX_ADDRESS_WRAP,
            .mipLodBias = 0.0f,
            .maxAnisotropy = 1,
            .compareFunc = VK_COMPARE_NEVER,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColorType = VK_BORDER_COLOR_OPAQUE_WHITE,
        };
        VkImageViewCreateInfo view = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .image = VK_NULL_HANDLE,
            .viewType = VK_IMAGE_VIEW_2D,
            .format = tex_format,
            .channels = { VK_CHANNEL_SWIZZLE_R,
                          VK_CHANNEL_SWIZZLE_G,
                          VK_CHANNEL_SWIZZLE_B,
                          VK_CHANNEL_SWIZZLE_A, },
            .subresourceRange = { VK_IMAGE_ASPECT_COLOR, 0, 1, 0, 1 },
            .minLod = 0.0f,
        };

        /* create sampler */
        err = vkCreateSampler(demo->device, &sampler,
                &demo->textures[i].sampler);
        assert(!err);

        /* create image view */
        view.image = demo->textures[i].image;
        err = vkCreateImageView(demo->device, &view,
                                 &demo->textures[i].view);
        assert(!err);
    }
}

static void demo_prepare_vertices(struct demo *demo)
{
    const float vb[3][5] = {
        /*      position             texcoord */
        { -1.0f, -1.0f, -0.6f,      0.0f, 0.0f },
        {  1.0f, -1.0f, -0.5f,      1.0f, 0.0f },
        {  0.0f,  1.0f,  1.0f,      0.5f, 1.0f },
    };
    const VkBufferCreateInfo buf_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .size = sizeof(vb),
        .usage = VK_BUFFER_USAGE_VERTEX_FETCH_BIT,
        .flags = 0,
    };
    VkMemoryAllocBufferInfo buf_alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO,
        .pNext = NULL,
    };
    VkMemoryAllocInfo mem_alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = &buf_alloc,
        .allocationSize = 0,
        .memProps = VK_MEMORY_PROPERTY_CPU_VISIBLE_BIT,
        .memType = VK_MEMORY_TYPE_BUFFER,
        .memPriority = VK_MEMORY_PRIORITY_NORMAL,
    };
    VkMemoryRequirements *mem_reqs;
    size_t mem_reqs_size = sizeof(VkMemoryRequirements);
    VkBufferMemoryRequirements buf_reqs;
    size_t buf_reqs_size = sizeof(VkBufferMemoryRequirements);
    uint32_t num_allocations = 0;
    size_t num_alloc_size = sizeof(num_allocations);
    VkResult err;
    void *data;

    memset(&demo->vertices, 0, sizeof(demo->vertices));

    err = vkCreateBuffer(demo->device, &buf_info, &demo->vertices.buf);
    assert(!err);

    err = vkGetObjectInfo(demo->vertices.buf,
                           VK_INFO_TYPE_MEMORY_ALLOCATION_COUNT,
                           &num_alloc_size, &num_allocations);
    assert(!err && num_alloc_size == sizeof(num_allocations));
    mem_reqs = malloc(num_allocations * sizeof(VkMemoryRequirements));
    demo->vertices.mem = malloc(num_allocations * sizeof(VkGpuMemory));
    demo->vertices.num_mem = num_allocations;
    err = vkGetObjectInfo(demo->vertices.buf,
            VK_INFO_TYPE_MEMORY_REQUIREMENTS,
            &mem_reqs_size, mem_reqs);
    assert(!err && mem_reqs_size == sizeof(*mem_reqs));
    err = vkGetObjectInfo(demo->vertices.buf,
                    VK_INFO_TYPE_BUFFER_MEMORY_REQUIREMENTS,
                    &buf_reqs_size, &buf_reqs);
    assert(!err && buf_reqs_size == sizeof(VkBufferMemoryRequirements));
    buf_alloc.usage = buf_reqs.usage;
    for (uint32_t i = 0; i < num_allocations; i ++) {
        mem_alloc.allocationSize = mem_reqs[i].size;

        err = vkAllocMemory(demo->device, &mem_alloc, &demo->vertices.mem[i]);
        assert(!err);

        err = vkMapMemory(demo->vertices.mem[i], 0, &data);
        assert(!err);

        memcpy(data, vb, sizeof(vb));

        err = vkUnmapMemory(demo->vertices.mem[i]);
        assert(!err);

        err = vkBindObjectMemory(demo->vertices.buf, i, demo->vertices.mem[i], 0);
        assert(!err);
    }

    demo_add_mem_refs(demo, demo->vertices.num_mem, demo->vertices.mem);

    demo->vertices.vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO;
    demo->vertices.vi.pNext = NULL;
    demo->vertices.vi.bindingCount = 1;
    demo->vertices.vi.pVertexBindingDescriptions = demo->vertices.vi_bindings;
    demo->vertices.vi.attributeCount = 2;
    demo->vertices.vi.pVertexAttributeDescriptions = demo->vertices.vi_attrs;

    demo->vertices.vi_bindings[0].binding = VERTEX_BUFFER_BIND_ID;
    demo->vertices.vi_bindings[0].strideInBytes = sizeof(vb[0]);
    demo->vertices.vi_bindings[0].stepRate = VK_VERTEX_INPUT_STEP_RATE_VERTEX;

    demo->vertices.vi_attrs[0].binding = VERTEX_BUFFER_BIND_ID;
    demo->vertices.vi_attrs[0].location = 0;
    demo->vertices.vi_attrs[0].format = VK_FMT_R32G32B32_SFLOAT;
    demo->vertices.vi_attrs[0].offsetInBytes = 0;

    demo->vertices.vi_attrs[1].binding = VERTEX_BUFFER_BIND_ID;
    demo->vertices.vi_attrs[1].location = 1;
    demo->vertices.vi_attrs[1].format = VK_FMT_R32G32_SFLOAT;
    demo->vertices.vi_attrs[1].offsetInBytes = sizeof(float) * 3;
}

static void demo_prepare_descriptor_layout(struct demo *demo)
{
    const VkDescriptorSetLayoutBinding layout_binding = {
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER_TEXTURE,
        .count = DEMO_TEXTURE_COUNT,
        .stageFlags = VK_SHADER_STAGE_FLAGS_FRAGMENT_BIT,
        .pImmutableSamplers = NULL,
    };
    const VkDescriptorSetLayoutCreateInfo descriptor_layout = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .count = 1,
        .pBinding = &layout_binding,
    };
    VkResult err;

    err = vkCreateDescriptorSetLayout(demo->device,
            &descriptor_layout, &demo->desc_layout);
    assert(!err);

    err = vkCreateDescriptorSetLayoutChain(demo->device,
            1, &demo->desc_layout, &demo->desc_layout_chain);
    assert(!err);
}

static VkShader demo_prepare_shader(struct demo *demo,
                                      VkPipelineShaderStage stage,
                                      const void *code,
                                      size_t size)
{
    VkShaderCreateInfo createInfo;
    VkShader shader;
    VkResult err;

    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    createInfo.pNext = NULL;

    // Create fake SPV structure to feed GLSL
    // to the driver "under the covers"
    createInfo.codeSize = 3 * sizeof(uint32_t) + size + 1;
    createInfo.pCode = malloc(createInfo.codeSize);
    createInfo.flags = 0;

    /* try version 0 first: VkPipelineShaderStage followed by GLSL */
    ((uint32_t *) createInfo.pCode)[0] = ICD_SPV_MAGIC;
    ((uint32_t *) createInfo.pCode)[1] = 0;
    ((uint32_t *) createInfo.pCode)[2] = stage;
    memcpy(((uint32_t *) createInfo.pCode + 3), code, size + 1);

    err = vkCreateShader(demo->device, &createInfo, &shader);
    if (err) {
        free((void *) createInfo.pCode);
        return NULL;
    }

    return shader;
}

static VkShader demo_prepare_vs(struct demo *demo)
{
    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec4 pos;\n"
            "layout (location = 1) in vec2 attr;\n"
            "out vec2 texcoord;\n"
            "void main() {\n"
            "   texcoord = attr;\n"
            "   gl_Position = pos;\n"
            "}\n";

    return demo_prepare_shader(demo, VK_SHADER_STAGE_VERTEX,
                               (const void *) vertShaderText,
                               strlen(vertShaderText));
}

static VkShader demo_prepare_fs(struct demo *demo)
{
    static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (binding = 0) uniform sampler2D tex;\n"
            "layout (location = 0) in vec2 texcoord;\n"
            "void main() {\n"
            "   gl_FragColor = texture(tex, texcoord);\n"
            "}\n";

    return demo_prepare_shader(demo, VK_SHADER_STAGE_FRAGMENT,
                               (const void *) fragShaderText,
                               strlen(fragShaderText));
}

static void demo_prepare_pipeline(struct demo *demo)
{
    VkGraphicsPipelineCreateInfo pipeline;
    VkPipelineVertexInputCreateInfo vi;
    VkPipelineIaStateCreateInfo ia;
    VkPipelineRsStateCreateInfo rs;
    VkPipelineCbStateCreateInfo cb;
    VkPipelineDsStateCreateInfo ds;
    VkPipelineShaderStageCreateInfo vs;
    VkPipelineShaderStageCreateInfo fs;
    VkPipelineVpStateCreateInfo vp;
    VkPipelineMsStateCreateInfo ms;
    VkResult err;

    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.pSetLayoutChain = demo->desc_layout_chain;

    vi = demo->vertices.vi;

    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO;
    ia.topology = VK_TOPOLOGY_TRIANGLE_LIST;

    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO;
    rs.fillMode = VK_FILL_SOLID;
    rs.cullMode = VK_CULL_NONE;
    rs.frontFace = VK_FRONT_FACE_CCW;

    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO;
    VkPipelineCbAttachmentState att_state[1];
    memset(att_state, 0, sizeof(att_state));
    att_state[0].format = demo->format;
    att_state[0].channelWriteMask = 0xf;
    att_state[0].blendEnable = VK_FALSE;
    cb.attachmentCount = 1;
    cb.pAttachments = att_state;


    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO;
    vp.numViewports = 1;
    vp.clipOrigin = VK_COORDINATE_ORIGIN_UPPER_LEFT;

    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO;
    ds.format = demo->depth.format;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthFunc = VK_COMPARE_LESS_EQUAL;
    ds.depthBoundsEnable = VK_FALSE;
    ds.back.stencilFailOp = VK_STENCIL_OP_KEEP;
    ds.back.stencilPassOp = VK_STENCIL_OP_KEEP;
    ds.back.stencilFunc = VK_COMPARE_ALWAYS;
    ds.stencilTestEnable = VK_FALSE;
    ds.front = ds.back;

    memset(&vs, 0, sizeof(vs));
    vs.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs.shader.stage = VK_SHADER_STAGE_VERTEX;
    vs.shader.shader = demo_prepare_vs(demo);
    vs.shader.linkConstBufferCount = 0;

    memset(&fs, 0, sizeof(fs));
    fs.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fs.shader.stage = VK_SHADER_STAGE_FRAGMENT;
    fs.shader.shader = demo_prepare_fs(demo);

    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO;
    ms.sampleMask = 1;
    ms.multisampleEnable = VK_FALSE;
    ms.samples = 1;

    pipeline.pNext = (const void *) &vi;
    vi.pNext = (void *) &ia;
    ia.pNext = (const void *) &rs;
    rs.pNext = (const void *) &cb;
    cb.pNext = (const void *) &ms;
    ms.pNext = (const void *) &vp;
    vp.pNext = (const void *) &ds;
    ds.pNext = (const void *) &vs;
    vs.pNext = (const void *) &fs;

    err = vkCreateGraphicsPipeline(demo->device, &pipeline, &demo->pipeline);
    assert(!err);

    vkDestroyObject(vs.shader.shader);
    vkDestroyObject(fs.shader.shader);
}

static void demo_prepare_dynamic_states(struct demo *demo)
{
    VkDynamicVpStateCreateInfo viewport_create;
    VkDynamicRsStateCreateInfo raster;
    VkDynamicCbStateCreateInfo color_blend;
    VkDynamicDsStateCreateInfo depth_stencil;
    VkResult err;

    memset(&viewport_create, 0, sizeof(viewport_create));
    viewport_create.sType = VK_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO;
    viewport_create.viewportAndScissorCount = 1;
    VkViewport viewport;
    memset(&viewport, 0, sizeof(viewport));
    viewport.height = (float) demo->height;
    viewport.width = (float) demo->width;
    viewport.minDepth = (float) 0.0f;
    viewport.maxDepth = (float) 1.0f;
    viewport_create.pViewports = &viewport;
    VkRect scissor;
    memset(&scissor, 0, sizeof(scissor));
    scissor.extent.width = demo->width;
    scissor.extent.height = demo->height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    viewport_create.pScissors = &scissor;

    memset(&raster, 0, sizeof(raster));
    raster.sType = VK_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO;
    raster.pointSize = 1.0;
    raster.lineWidth = 1.0;

    memset(&color_blend, 0, sizeof(color_blend));
    color_blend.sType = VK_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO;
    color_blend.blendConst[0] = 1.0f;
    color_blend.blendConst[1] = 1.0f;
    color_blend.blendConst[2] = 1.0f;
    color_blend.blendConst[3] = 1.0f;

    memset(&depth_stencil, 0, sizeof(depth_stencil));
    depth_stencil.sType = VK_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO;
    depth_stencil.minDepth = 0.0f;
    depth_stencil.maxDepth = 1.0f;
    depth_stencil.stencilBackRef = 0;
    depth_stencil.stencilFrontRef = 0;
    depth_stencil.stencilReadMask = 0xff;
    depth_stencil.stencilWriteMask = 0xff;

    err = vkCreateDynamicViewportState(demo->device, &viewport_create, &demo->viewport);
    assert(!err);

    err = vkCreateDynamicRasterState(demo->device, &raster, &demo->raster);
    assert(!err);

    err = vkCreateDynamicColorBlendState(demo->device,
            &color_blend, &demo->color_blend);
    assert(!err);

    err = vkCreateDynamicDepthStencilState(demo->device,
            &depth_stencil, &demo->depth_stencil);
    assert(!err);
}

static void demo_prepare_descriptor_pool(struct demo *demo)
{
    const VkDescriptorTypeCount type_count = {
        .type = VK_DESCRIPTOR_TYPE_SAMPLER_TEXTURE,
        .count = DEMO_TEXTURE_COUNT,
    };
    const VkDescriptorPoolCreateInfo descriptor_pool = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .count = 1,
        .pTypeCount = &type_count,
    };
    VkResult err;

    err = vkCreateDescriptorPool(demo->device,
            VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1,
            &descriptor_pool, &demo->desc_pool);
    assert(!err);
}

static void demo_prepare_descriptor_set(struct demo *demo)
{
    VkImageViewAttachInfo view_info[DEMO_TEXTURE_COUNT];
    VkSamplerImageViewInfo combined_info[DEMO_TEXTURE_COUNT];
    VkUpdateSamplerTextures update;
    const void *update_array[1] = { &update };
    VkResult err;
    uint32_t count;
    uint32_t i;

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        view_info[i].sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO;
        view_info[i].pNext = NULL;
        view_info[i].view = demo->textures[i].view,
        view_info[i].layout = VK_IMAGE_LAYOUT_GENERAL;

        combined_info[i].sampler = demo->textures[i].sampler;
        combined_info[i].pImageView = &view_info[i];
    }

    memset(&update, 0, sizeof(update));
    update.sType = VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES;
    update.count = DEMO_TEXTURE_COUNT;
    update.pSamplerImageViews = combined_info;

    err = vkAllocDescriptorSets(demo->desc_pool,
            VK_DESCRIPTOR_SET_USAGE_STATIC,
            1, &demo->desc_layout,
            &demo->desc_set, &count);
    assert(!err && count == 1);

    vkBeginDescriptorPoolUpdate(demo->device,
            VK_DESCRIPTOR_UPDATE_MODE_FASTEST);

    vkClearDescriptorSets(demo->desc_pool, 1, &demo->desc_set);
    vkUpdateDescriptors(demo->desc_set, 1, update_array);

    vkEndDescriptorPoolUpdate(demo->device, demo->cmd);
}

static void demo_prepare(struct demo *demo)
{
    const VkCmdBufferCreateInfo cmd = {
        .sType = VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .queueNodeIndex = demo->graphics_queue_node_index,
        .flags = 0,
    };
    VkResult err;

    demo_prepare_buffers(demo);
    demo_prepare_depth(demo);
    demo_prepare_textures(demo);
    demo_prepare_vertices(demo);
    demo_prepare_descriptor_layout(demo);
    demo_prepare_pipeline(demo);
    demo_prepare_dynamic_states(demo);

    err = vkCreateCommandBuffer(demo->device, &cmd, &demo->cmd);
    assert(!err);

    demo_prepare_descriptor_pool(demo);
    demo_prepare_descriptor_set(demo);
}

static void demo_handle_event(struct demo *demo,
                              const xcb_generic_event_t *event)
{
    switch (event->response_type & 0x7f) {
    case XCB_EXPOSE:
        demo_draw(demo);
        break;
    case XCB_CLIENT_MESSAGE:
        if((*(xcb_client_message_event_t*)event).data.data32[0] ==
           (*demo->atom_wm_delete_window).atom) {
            demo->quit = true;
        }
        break;
    case XCB_KEY_RELEASE:
        {
            const xcb_key_release_event_t *key =
                (const xcb_key_release_event_t *) event;

            if (key->detail == 0x9)
                demo->quit = true;
        }
        break;
    case XCB_DESTROY_NOTIFY:
        demo->quit = true;
        break;
    default:
        break;
    }
}

static void demo_run(struct demo *demo)
{
    xcb_flush(demo->connection);

    while (!demo->quit) {
        xcb_generic_event_t *event;

        event = xcb_wait_for_event(demo->connection);
        if (event) {
            demo_handle_event(demo, event);
            free(event);
        }
    }
}

static void demo_create_window(struct demo *demo)
{
    uint32_t value_mask, value_list[32];

    demo->window = xcb_generate_id(demo->connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = demo->screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE |
                    XCB_EVENT_MASK_EXPOSURE |
                    XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    xcb_create_window(demo->connection,
            XCB_COPY_FROM_PARENT,
            demo->window, demo->screen->root,
            0, 0, demo->width, demo->height, 0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            demo->screen->root_visual,
            value_mask, value_list);

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(demo->connection, 1, 12,
                                                      "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(demo->connection, cookie, 0);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(demo->connection, 0, 16, "WM_DELETE_WINDOW");
    demo->atom_wm_delete_window = xcb_intern_atom_reply(demo->connection, cookie2, 0);

    xcb_change_property(demo->connection, XCB_PROP_MODE_REPLACE,
                        demo->window, (*reply).atom, 4, 32, 1,
                        &(*demo->atom_wm_delete_window).atom);
    free(reply);

    xcb_map_window(demo->connection, demo->window);
}

static void demo_init_vk(struct demo *demo)
{
    const VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pAppName = "tri",
        .appVersion = 0,
        .pEngineName = "tri",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION,
    };
    const VkInstanceCreateInfo inst_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .pAppInfo = &app,
        .pAllocCb = NULL,
        .extensionCount = 0,
        .ppEnabledExtensionNames = NULL,
    };
    const VK_WSI_X11_CONNECTION_INFO connection = {
        .pConnection = demo->connection,
        .root = demo->screen->root,
        .provider = 0,
    };
    const VkDeviceQueueCreateInfo queue = {
        .queueNodeIndex = 0,
        .queueCount = 1,
    };
    const char *ext_names[] = {
        "VK_WSI_X11",
    };
    const VkDeviceCreateInfo device = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueRecordCount = 1,
        .pRequestedQueues = &queue,
        .extensionCount = 1,
        .ppEnabledExtensionNames = ext_names,
        .maxValidationLevel = VK_VALIDATION_LEVEL_END_RANGE,
        .flags = VK_DEVICE_CREATE_VALIDATION_BIT,
    };
    VkResult err;
    uint32_t gpu_count;
    uint32_t i;
    size_t data_size;
    uint32_t queue_count;

    err = vkCreateInstance(&inst_info, &demo->inst);
    if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
        printf("Cannot find a compatible Vulkan installable client driver "
               "(ICD).\nExiting ...\n");
        fflush(stdout);
        exit(1);
    } else {
        assert(!err);
    }

    err = vkEnumerateGpus(demo->inst, 1, &gpu_count, &demo->gpu);
    assert(!err && gpu_count == 1);

    for (i = 0; i < device.extensionCount; i++) {
        err = vkGetExtensionSupport(demo->gpu, ext_names[i]);
        assert(!err);
    }

    err = vkWsiX11AssociateConnection(demo->gpu, &connection);
    assert(!err);

    err = vkCreateDevice(demo->gpu, &device, &demo->device);
    assert(!err);

    err = vkGetGpuInfo(demo->gpu, VK_INFO_TYPE_PHYSICAL_GPU_PROPERTIES,
                        &data_size, NULL);
    assert(!err);

    demo->gpu_props = (VkPhysicalGpuProperties *) malloc(data_size);
    err = vkGetGpuInfo(demo->gpu, VK_INFO_TYPE_PHYSICAL_GPU_PROPERTIES,
                        &data_size, demo->gpu_props);
    assert(!err);

    err = vkGetGpuInfo(demo->gpu, VK_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES,
                        &data_size, NULL);
    assert(!err);

    demo->queue_props = (VkPhysicalGpuQueueProperties *) malloc(data_size);
    err = vkGetGpuInfo(demo->gpu, VK_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES,
                        &data_size, demo->queue_props);
    assert(!err);
	queue_count = (uint32_t) (data_size / sizeof(VkPhysicalGpuQueueProperties));
    assert(queue_count >= 1);

    for (i = 0; i < queue_count; i++) {
        if (demo->queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            break;
    }
    assert(i < queue_count);
    demo->graphics_queue_node_index = i;

    err = vkGetDeviceQueue(demo->device, demo->graphics_queue_node_index,
            0, &demo->queue);
    assert(!err);
}

static void demo_init_connection(struct demo *demo)
{
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

    demo->connection = xcb_connect(NULL, &scr);
    if (demo->connection == NULL) {
        printf("Cannot find a compatible Vulkan installable client driver "
               "(ICD).\nExiting ...\n");
        fflush(stdout);
        exit(1);
    }

    setup = xcb_get_setup(demo->connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0)
        xcb_screen_next(&iter);

    demo->screen = iter.data;
}

static void demo_init(struct demo *demo, const int argc, const char *argv[])
{
    memset(demo, 0, sizeof(*demo));

    for (int i = 0; i < argc; i++) {
        if (strncmp(argv[i], "--use_staging", strlen("--use_staging")) == 0)
            demo->use_staging_buffer = true;
    }

    demo_init_connection(demo);
    demo_init_vk(demo);

    demo->width = 300;
    demo->height = 300;
    demo->format = VK_FMT_B8G8R8A8_UNORM;
}

static void demo_cleanup(struct demo *demo)
{
    uint32_t i, j;

    vkDestroyObject(demo->desc_set);
    vkDestroyObject(demo->desc_pool);

    vkDestroyObject(demo->cmd);

    vkDestroyObject(demo->viewport);
    vkDestroyObject(demo->raster);
    vkDestroyObject(demo->color_blend);
    vkDestroyObject(demo->depth_stencil);

    vkDestroyObject(demo->pipeline);
    vkDestroyObject(demo->desc_layout_chain);
    vkDestroyObject(demo->desc_layout);

    vkBindObjectMemory(demo->vertices.buf, 0, VK_NULL_HANDLE, 0);
    vkDestroyObject(demo->vertices.buf);
    demo_remove_mem_refs(demo, demo->vertices.num_mem, demo->vertices.mem);
    for (j = 0; j < demo->vertices.num_mem; j++)
        vkFreeMemory(demo->vertices.mem[j]);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        vkDestroyObject(demo->textures[i].view);
        vkBindObjectMemory(demo->textures[i].image, 0, VK_NULL_HANDLE, 0);
        vkDestroyObject(demo->textures[i].image);
        demo_remove_mem_refs(demo, demo->textures[i].num_mem, demo->textures[i].mem);
        for (j = 0; j < demo->textures[i].num_mem; j++)
            vkFreeMemory(demo->textures[i].mem[j]);
        free(demo->textures[i].mem);
        vkDestroyObject(demo->textures[i].sampler);
    }

    vkDestroyObject(demo->depth.view);
    vkBindObjectMemory(demo->depth.image, 0, VK_NULL_HANDLE, 0);
    demo_remove_mem_refs(demo, demo->depth.num_mem, demo->depth.mem);
    vkDestroyObject(demo->depth.image);
    for (j = 0; j < demo->depth.num_mem; j++)
        vkFreeMemory(demo->depth.mem[j]);

    for (i = 0; i < DEMO_BUFFER_COUNT; i++) {
        vkDestroyObject(demo->buffers[i].fence);
        vkDestroyObject(demo->buffers[i].view);
        vkDestroyObject(demo->buffers[i].image);
        demo_remove_mem_refs(demo, 1, &demo->buffers[i].mem);
    }

    vkDestroyDevice(demo->device);
    vkDestroyInstance(demo->inst);

    xcb_destroy_window(demo->connection, demo->window);
    xcb_disconnect(demo->connection);
}

int main(const int argc, const char *argv[])
{
    struct demo demo;

    demo_init(&demo, argc, argv);

    demo_prepare(&demo);
    demo_create_window(&demo);
    demo_run(&demo);

    demo_cleanup(&demo);

    return 0;
}
