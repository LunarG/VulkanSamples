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

#if defined(__linux__)
#include <xcb/xcb.h>
#endif
#include <xgl.h>
#include <xglDbg.h>
#if defined(__linux__)
#include <xglWsiX11Ext.h>
#endif

#include "icd-bil.h"

#define DEMO_BUFFER_COUNT 2
#define DEMO_TEXTURE_COUNT 1

struct demo {
#if defined(__linux__)
    xcb_connection_t *connection;
    xcb_screen_t *screen;
#endif

    XGL_INSTANCE inst;
    XGL_PHYSICAL_GPU gpu;
    XGL_DEVICE device;
    XGL_QUEUE queue;

    int width, height;
    XGL_FORMAT format;

    struct {
        XGL_IMAGE image;
        XGL_GPU_MEMORY mem;

        XGL_COLOR_ATTACHMENT_VIEW view;
        XGL_FENCE fence;
    } buffers[DEMO_BUFFER_COUNT];

    struct {
        XGL_FORMAT format;

        XGL_IMAGE image;
        uint32_t  num_mem;
        XGL_GPU_MEMORY *mem;
        XGL_DEPTH_STENCIL_VIEW view;
    } depth;

    struct {
        XGL_SAMPLER sampler;

        XGL_IMAGE image;
        uint32_t  num_mem;
        XGL_GPU_MEMORY *mem;
        XGL_IMAGE_VIEW view;
    } textures[DEMO_TEXTURE_COUNT];

    struct {
        XGL_BUFFER buf;
        uint32_t  num_mem;
        XGL_GPU_MEMORY *mem;

        XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO vi;
        XGL_VERTEX_INPUT_BINDING_DESCRIPTION vi_bindings[1];
        XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION vi_attrs[2];
    } vertices;

    XGL_DESCRIPTOR_SET_LAYOUT desc_layout;
    XGL_PIPELINE pipeline;

    XGL_DYNAMIC_VP_STATE_OBJECT viewport;
    XGL_DYNAMIC_RS_STATE_OBJECT raster;
    XGL_DYNAMIC_CB_STATE_OBJECT color_blend;
    XGL_DYNAMIC_DS_STATE_OBJECT depth_stencil;

    XGL_CMD_BUFFER cmd;

    XGL_DESCRIPTOR_REGION desc_region;
    XGL_DESCRIPTOR_SET desc_set;

#if defined(__linux__)
    xcb_window_t window;
    xcb_intern_atom_reply_t *atom_wm_delete_window;
#endif

    bool quit;
    uint32_t current_buffer;
};

static void demo_draw_build_cmd(struct demo *demo)
{
#if defined(__linux__)
    const XGL_COLOR_ATTACHMENT_BIND_INFO color_attachment = {
        .view = demo->buffers[demo->current_buffer].view,
        .layout = XGL_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
#endif
    const XGL_DEPTH_STENCIL_BIND_INFO depth_stencil = {
        .view = demo->depth.view,
        .layout = XGL_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    const float clear_color[4] = { 0.2f, 0.2f, 0.2f, 0.2f };
    const float clear_depth = 0.9f;
    XGL_IMAGE_SUBRESOURCE_RANGE clear_range;
    XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO graphics_cmd_buf_info = {
        .sType = XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO,
        .pNext = NULL,
    };
    XGL_CMD_BUFFER_BEGIN_INFO cmd_buf_info = {
        .sType = XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO,
        .pNext = &graphics_cmd_buf_info,
        .flags = XGL_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT |
            XGL_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT,
    };
    XGL_RESULT err;
    XGL_ATTACHMENT_LOAD_OP load_op = XGL_ATTACHMENT_LOAD_OP_DONT_CARE;
    XGL_ATTACHMENT_STORE_OP store_op = XGL_ATTACHMENT_STORE_OP_DONT_CARE;
    const XGL_FRAMEBUFFER_CREATE_INFO fb_info = {
         .sType = XGL_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
         .pNext = NULL,
         .colorAttachmentCount = 1,
#if defined(__linux__)
         .pColorAttachments = (XGL_COLOR_ATTACHMENT_BIND_INFO*) &color_attachment,
#endif
         .pDepthStencilAttachment = (XGL_DEPTH_STENCIL_BIND_INFO*) &depth_stencil,
         .sampleCount = 1,
         .width  = demo->width,
         .height = demo->height,
         .layers = 1,
    };
    XGL_RENDER_PASS_CREATE_INFO rp_info;

    memset(&rp_info, 0 , sizeof(rp_info));
    err = xglCreateFramebuffer(demo->device, &fb_info, &(rp_info.framebuffer));
    assert(!err);
    rp_info.sType = XGL_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.renderArea.extent.width = demo->width;
    rp_info.renderArea.extent.height = demo->height;
    rp_info.pColorLoadOps = &load_op;
    rp_info.pColorStoreOps = &store_op;
    rp_info.depthLoadOp = XGL_ATTACHMENT_LOAD_OP_DONT_CARE;
    rp_info.depthStoreOp = XGL_ATTACHMENT_STORE_OP_DONT_CARE;
    rp_info.stencilLoadOp = XGL_ATTACHMENT_LOAD_OP_DONT_CARE;
    rp_info.stencilStoreOp = XGL_ATTACHMENT_STORE_OP_DONT_CARE;
    err = xglCreateRenderPass(demo->device, &rp_info, &(graphics_cmd_buf_info.renderPass));
    assert(!err);

    err = xglBeginCommandBuffer(demo->cmd, &cmd_buf_info);
    assert(!err);

    xglCmdBindPipeline(demo->cmd, XGL_PIPELINE_BIND_POINT_GRAPHICS,
                                  demo->pipeline);
    xglCmdBindDescriptorSet(demo->cmd, XGL_PIPELINE_BIND_POINT_GRAPHICS,
            demo->desc_set, NULL);

    xglCmdBindDynamicStateObject(demo->cmd, XGL_STATE_BIND_VIEWPORT, demo->viewport);
    xglCmdBindDynamicStateObject(demo->cmd, XGL_STATE_BIND_RASTER, demo->raster);
    xglCmdBindDynamicStateObject(demo->cmd, XGL_STATE_BIND_COLOR_BLEND,
                                     demo->color_blend);
    xglCmdBindDynamicStateObject(demo->cmd, XGL_STATE_BIND_DEPTH_STENCIL,
                                     demo->depth_stencil);


    xglCmdBindVertexBuffer(demo->cmd, demo->vertices.buf, 0, 0);

    clear_range.aspect = XGL_IMAGE_ASPECT_COLOR;
    clear_range.baseMipLevel = 0;
    clear_range.mipLevels = 1;
    clear_range.baseArraySlice = 0;
    clear_range.arraySize = 1;
    xglCmdClearColorImage(demo->cmd,
            demo->buffers[demo->current_buffer].image,
            clear_color, 1, &clear_range);

    clear_range.aspect = XGL_IMAGE_ASPECT_DEPTH;
    xglCmdClearDepthStencil(demo->cmd, demo->depth.image,
            clear_depth, 0, 1, &clear_range);

    xglCmdDraw(demo->cmd, 0, 3, 0, 1);

    err = xglEndCommandBuffer(demo->cmd);
    assert(!err);
}

static void demo_draw(struct demo *demo)
{
#if defined(__linux__)
    const XGL_WSI_X11_PRESENT_INFO present = {
        .destWindow = demo->window,
        .srcImage = demo->buffers[demo->current_buffer].image,
    };
#endif
    XGL_FENCE fence = demo->buffers[demo->current_buffer].fence;
    XGL_RESULT err;

    demo_draw_build_cmd(demo);

    err = xglWaitForFences(demo->device, 1, &fence, XGL_TRUE, ~((uint64_t) 0));
    assert(err == XGL_SUCCESS || err == XGL_ERROR_UNAVAILABLE);

    uint32_t i, idx = 0;
    XGL_MEMORY_REF *memRefs;
    memRefs = malloc(sizeof(XGL_MEMORY_REF) * (2 + demo->depth.num_mem +
                     demo->textures[0].num_mem + demo->vertices.num_mem));
    for (i = 0; i < demo->depth.num_mem; i++, idx++) {
        memRefs[idx].mem = demo->depth.mem[i];
        memRefs[idx].flags = 0;
    }
    for (i = 0; i < demo->textures[0].num_mem; i++, idx++) {
        memRefs[idx].mem = demo->textures[0].mem[i];
        memRefs[idx].flags = 0;
    }
    memRefs[idx].mem = demo->buffers[0].mem;
    memRefs[idx++].flags = 0;
    memRefs[idx].mem = demo->buffers[1].mem;
    memRefs[idx++].flags = 0;
    for (i = 0; i < demo->vertices.num_mem; i++, idx++) {
        memRefs[idx].mem = demo->vertices.mem[i];
        memRefs[idx].flags = 0;
    }
    err = xglQueueSubmit(demo->queue, 1, &demo->cmd,
            idx, memRefs, XGL_NULL_HANDLE);
    assert(!err);

#if defined(__linux__)
    err = xglWsiX11QueuePresent(demo->queue, &present, fence);
    assert(!err);
#endif

    demo->current_buffer = (demo->current_buffer + 1) % DEMO_BUFFER_COUNT;
}

static void demo_prepare_buffers(struct demo *demo)
{
#if defined(__linux__)
    const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO presentable_image = {
        .format = demo->format,
        .usage = XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .extent = {
            .width = demo->width,
            .height = demo->height,
        },
        .flags = 0,
    };
#endif
    const XGL_FENCE_CREATE_INFO fence = {
        .sType = XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
    };
    XGL_RESULT err;
    uint32_t i;

    for (i = 0; i < DEMO_BUFFER_COUNT; i++) {
        XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO color_attachment_view = {
            .sType = XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO,
            .pNext = NULL,
            .format = demo->format,
            .mipLevel = 0,
            .baseArraySlice = 0,
            .arraySize = 1,
        };

#if defined(__linux__)
        err = xglWsiX11CreatePresentableImage(demo->device, &presentable_image,
                &demo->buffers[i].image, &demo->buffers[i].mem);
        assert(!err);
#else
        demo->buffers[i].image = NULL;
        demo->buffers[i].mem = NULL;
#endif

        color_attachment_view.image = demo->buffers[i].image;

        err = xglCreateColorAttachmentView(demo->device,
                &color_attachment_view, &demo->buffers[i].view);
        assert(!err);

        err = xglCreateFence(demo->device,
                &fence, &demo->buffers[i].fence);
        assert(!err);
    }
}

static void demo_prepare_depth(struct demo *demo)
{
    const XGL_FORMAT depth_format = XGL_FMT_D16_UNORM;
    const XGL_IMAGE_CREATE_INFO image = {
        .sType = XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = XGL_IMAGE_2D,
        .format = depth_format,
        .extent = { demo->width, demo->height, 1 },
        .mipLevels = 1,
        .arraySize = 1,
        .samples = 1,
        .tiling = XGL_OPTIMAL_TILING,
        .usage = XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT,
        .flags = 0,
    };
    XGL_MEMORY_ALLOC_IMAGE_INFO img_alloc = {
        .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO,
        .pNext = NULL,
    };
    XGL_MEMORY_ALLOC_INFO mem_alloc = {
        .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = &img_alloc,
        .allocationSize = 0,
        .memProps = XGL_MEMORY_PROPERTY_GPU_ONLY,
        .memType = XGL_MEMORY_TYPE_IMAGE,
        .memPriority = XGL_MEMORY_PRIORITY_NORMAL,
    };
    XGL_DEPTH_STENCIL_VIEW_CREATE_INFO view = {
        .sType = XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO,
        .pNext = NULL,
        .image = XGL_NULL_HANDLE,
        .mipLevel = 0,
        .baseArraySlice = 0,
        .arraySize = 1,
        .flags = 0,
    };

    XGL_MEMORY_REQUIREMENTS *mem_reqs;
    size_t mem_reqs_size = sizeof(XGL_MEMORY_REQUIREMENTS);
    XGL_IMAGE_MEMORY_REQUIREMENTS img_reqs;
    size_t img_reqs_size = sizeof(XGL_IMAGE_MEMORY_REQUIREMENTS);
    XGL_RESULT err;
    uint32_t num_allocations = 0;
    size_t num_alloc_size = sizeof(num_allocations);

    demo->depth.format = depth_format;

    /* create image */
    err = xglCreateImage(demo->device, &image,
            &demo->depth.image);
    assert(!err);


    err = xglGetObjectInfo(demo->depth.image, XGL_INFO_TYPE_MEMORY_ALLOCATION_COUNT, &num_alloc_size, &num_allocations);
    assert(!err && num_alloc_size == sizeof(num_allocations));
    mem_reqs = malloc(num_allocations * sizeof(XGL_MEMORY_REQUIREMENTS));
    demo->depth.mem = malloc(num_allocations * sizeof(XGL_GPU_MEMORY));
    demo->depth.num_mem = num_allocations;
    err = xglGetObjectInfo(demo->depth.image,
                    XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                    &mem_reqs_size, mem_reqs);
    assert(!err && mem_reqs_size == num_allocations * sizeof(XGL_MEMORY_REQUIREMENTS));
    err = xglGetObjectInfo(demo->depth.image,
                    XGL_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS,
                    &img_reqs_size, &img_reqs);
    assert(!err && img_reqs_size == sizeof(XGL_IMAGE_MEMORY_REQUIREMENTS));
    img_alloc.usage = img_reqs.usage;
    img_alloc.formatClass = img_reqs.formatClass;
    img_alloc.samples = img_reqs.samples;
    for (uint32_t i = 0; i < num_allocations; i ++) {
        mem_alloc.allocationSize = mem_reqs[i].size;

        /* allocate memory */
        err = xglAllocMemory(demo->device, &mem_alloc,
                    &(demo->depth.mem[i]));
        assert(!err);

        /* bind memory */
        err = xglBindObjectMemory(demo->depth.image, i,
                demo->depth.mem[i], 0);
        assert(!err);
    }

    /* create image view */
    view.image = demo->depth.image;
    err = xglCreateDepthStencilView(demo->device, &view,
            &demo->depth.view);
    assert(!err);
}

static void demo_prepare_textures(struct demo *demo)
{
    const XGL_FORMAT tex_format = XGL_FMT_B8G8R8A8_UNORM;
    const int32_t tex_width = 2;
    const int32_t tex_height = 2;
    const uint32_t tex_colors[DEMO_TEXTURE_COUNT][2] = {
        { 0xffff0000, 0xff00ff00 },
    };
    XGL_RESULT err;
    uint32_t i;

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        const XGL_SAMPLER_CREATE_INFO sampler = {
            .sType = XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = NULL,
            .magFilter = XGL_TEX_FILTER_NEAREST,
            .minFilter = XGL_TEX_FILTER_NEAREST,
            .mipMode = XGL_TEX_MIPMAP_BASE,
            .addressU = XGL_TEX_ADDRESS_WRAP,
            .addressV = XGL_TEX_ADDRESS_WRAP,
            .addressW = XGL_TEX_ADDRESS_WRAP,
            .mipLodBias = 0.0f,
            .maxAnisotropy = 0,
            .compareFunc = XGL_COMPARE_NEVER,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColorType = XGL_BORDER_COLOR_OPAQUE_WHITE,
        };
        const XGL_IMAGE_CREATE_INFO image = {
            .sType = XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = NULL,
            .imageType = XGL_IMAGE_2D,
            .format = tex_format,
            .extent = { tex_width, tex_height, 1 },
            .mipLevels = 1,
            .arraySize = 1,
            .samples = 1,
            .tiling = XGL_LINEAR_TILING,
            .usage = XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT,
            .flags = 0,
        };
        XGL_MEMORY_ALLOC_IMAGE_INFO img_alloc = {
            .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO,
            .pNext = NULL,
        };
        XGL_MEMORY_ALLOC_INFO mem_alloc = {
            .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
            .pNext = &img_alloc,
            .allocationSize = 0,
            .memProps = XGL_MEMORY_PROPERTY_GPU_ONLY,
            .memType = XGL_MEMORY_TYPE_IMAGE,
            .memPriority = XGL_MEMORY_PRIORITY_NORMAL,
        };
        XGL_IMAGE_VIEW_CREATE_INFO view = {
            .sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .image = XGL_NULL_HANDLE,
            .viewType = XGL_IMAGE_VIEW_2D,
            .format = image.format,
            .channels = { XGL_CHANNEL_SWIZZLE_R,
                          XGL_CHANNEL_SWIZZLE_G,
                          XGL_CHANNEL_SWIZZLE_B,
                          XGL_CHANNEL_SWIZZLE_A, },
            .subresourceRange = { XGL_IMAGE_ASPECT_COLOR, 0, 1, 0, 1 },
            .minLod = 0.0f,
        };

        XGL_MEMORY_REQUIREMENTS *mem_reqs;
        size_t mem_reqs_size = sizeof(XGL_MEMORY_REQUIREMENTS);
        XGL_IMAGE_MEMORY_REQUIREMENTS img_reqs;
        size_t img_reqs_size = sizeof(XGL_IMAGE_MEMORY_REQUIREMENTS);
        uint32_t num_allocations = 0;
        size_t num_alloc_size = sizeof(num_allocations);

        /* create sampler */
        err = xglCreateSampler(demo->device, &sampler,
                &demo->textures[i].sampler);
        assert(!err);

        /* create image */
        err = xglCreateImage(demo->device, &image,
                &demo->textures[i].image);
        assert(!err);

        err = xglGetObjectInfo(demo->textures[i].image,
                    XGL_INFO_TYPE_MEMORY_ALLOCATION_COUNT,
                    &num_alloc_size, &num_allocations);
        assert(!err && num_alloc_size == sizeof(num_allocations));
        mem_reqs = malloc(num_allocations * sizeof(XGL_MEMORY_REQUIREMENTS));
        demo->textures[i].mem = malloc(num_allocations * sizeof(XGL_GPU_MEMORY));
        demo->textures[i].num_mem = num_allocations;
        err = xglGetObjectInfo(demo->textures[i].image,
                    XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                    &mem_reqs_size, mem_reqs);
        assert(!err && mem_reqs_size == num_allocations * sizeof(XGL_MEMORY_REQUIREMENTS));
        err = xglGetObjectInfo(demo->textures[i].image,
                        XGL_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS,
                        &img_reqs_size, &img_reqs);
        assert(!err && img_reqs_size == sizeof(XGL_IMAGE_MEMORY_REQUIREMENTS));
        img_alloc.usage = img_reqs.usage;
        img_alloc.formatClass = img_reqs.formatClass;
        img_alloc.samples = img_reqs.samples;
        for (uint32_t j = 0; j < num_allocations; j ++) {
            mem_alloc.allocationSize = mem_reqs[j].size;

            /* allocate memory */
            err = xglAllocMemory(demo->device, &mem_alloc,
                        &(demo->textures[i].mem[j]));
            assert(!err);

            /* bind memory */
            err = xglBindObjectMemory(demo->textures[i].image, j,
                    demo->textures[i].mem[j], 0);
            assert(!err);
        }

        /* create image view */
        view.image = demo->textures[i].image;
        err = xglCreateImageView(demo->device, &view,
                &demo->textures[i].view);
        assert(!err);
    }

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        const XGL_IMAGE_SUBRESOURCE subres = {
            .aspect = XGL_IMAGE_ASPECT_COLOR,
            .mipLevel = 0,
            .arraySlice = 0,
        };
        XGL_SUBRESOURCE_LAYOUT layout;
        size_t layout_size = sizeof(XGL_SUBRESOURCE_LAYOUT);
        void *data;
        int32_t x, y;

        err = xglGetImageSubresourceInfo(demo->textures[i].image, &subres,
                XGL_INFO_TYPE_SUBRESOURCE_LAYOUT, &layout_size, &layout);
        assert(!err && layout_size == sizeof(layout));
        assert(demo->textures[i].num_mem == 1);

        err = xglMapMemory(demo->textures[i].mem[0], 0, &data);
        assert(!err);

        for (y = 0; y < tex_height; y++) {
            uint32_t *row = (uint32_t *) ((char *) data + layout.rowPitch * y);
            for (x = 0; x < tex_width; x++)
                row[x] = tex_colors[i][(x & 1) ^ (y & 1)];
        }

        err = xglUnmapMemory(demo->textures[i].mem[0]);
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
    const XGL_BUFFER_CREATE_INFO buf_info = {
        .sType = XGL_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .size = sizeof(vb),
        .usage = XGL_BUFFER_USAGE_VERTEX_FETCH_BIT,
        .flags = 0,
    };
    XGL_MEMORY_ALLOC_BUFFER_INFO buf_alloc = {
        .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO,
        .pNext = NULL,
    };
    XGL_MEMORY_ALLOC_INFO mem_alloc = {
        .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = &buf_alloc,
        .allocationSize = 0,
        .memProps = XGL_MEMORY_PROPERTY_CPU_VISIBLE_BIT,
        .memType = XGL_MEMORY_TYPE_BUFFER,
        .memPriority = XGL_MEMORY_PRIORITY_NORMAL,
    };
    XGL_MEMORY_REQUIREMENTS *mem_reqs;
    size_t mem_reqs_size = sizeof(XGL_MEMORY_REQUIREMENTS);
    XGL_BUFFER_MEMORY_REQUIREMENTS buf_reqs;
    size_t buf_reqs_size = sizeof(XGL_BUFFER_MEMORY_REQUIREMENTS);
    uint32_t num_allocations = 0;
    size_t num_alloc_size = sizeof(num_allocations);
    XGL_RESULT err;
    void *data;

    memset(&demo->vertices, 0, sizeof(demo->vertices));

    err = xglCreateBuffer(demo->device, &buf_info, &demo->vertices.buf);
    assert(!err);

    err = xglGetObjectInfo(demo->vertices.buf,
                           XGL_INFO_TYPE_MEMORY_ALLOCATION_COUNT,
                           &num_alloc_size, &num_allocations);
    assert(!err && num_alloc_size == sizeof(num_allocations));
    mem_reqs = malloc(num_allocations * sizeof(XGL_MEMORY_REQUIREMENTS));
    demo->vertices.mem = malloc(num_allocations * sizeof(XGL_GPU_MEMORY));
    demo->vertices.num_mem = num_allocations;
    err = xglGetObjectInfo(demo->vertices.buf,
            XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
            &mem_reqs_size, mem_reqs);
    assert(!err && mem_reqs_size == sizeof(*mem_reqs));
    err = xglGetObjectInfo(demo->vertices.buf,
                    XGL_INFO_TYPE_BUFFER_MEMORY_REQUIREMENTS,
                    &buf_reqs_size, &buf_reqs);
    assert(!err && buf_reqs_size == sizeof(XGL_BUFFER_MEMORY_REQUIREMENTS));
    buf_alloc.usage = buf_reqs.usage;
    for (uint32_t i = 0; i < num_allocations; i ++) {
        mem_alloc.allocationSize = mem_reqs[i].size;

        err = xglAllocMemory(demo->device, &mem_alloc, &demo->vertices.mem[i]);
        assert(!err);

        err = xglMapMemory(demo->vertices.mem[i], 0, &data);
        assert(!err);

        memcpy(data, vb, sizeof(vb));

        err = xglUnmapMemory(demo->vertices.mem[i]);
        assert(!err);

        err = xglBindObjectMemory(demo->vertices.buf, i, demo->vertices.mem[i], 0);
        assert(!err);
    }

    demo->vertices.vi.sType = XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO;
    demo->vertices.vi.pNext = NULL;
    demo->vertices.vi.bindingCount = 1;
    demo->vertices.vi.pVertexBindingDescriptions = demo->vertices.vi_bindings;
    demo->vertices.vi.attributeCount = 2;
    demo->vertices.vi.pVertexAttributeDescriptions = demo->vertices.vi_attrs;

    demo->vertices.vi_bindings[0].strideInBytes = sizeof(vb[0]);
    demo->vertices.vi_bindings[0].stepRate = XGL_VERTEX_INPUT_STEP_RATE_VERTEX;

    demo->vertices.vi_attrs[0].binding = 0;
    demo->vertices.vi_attrs[0].format = XGL_FMT_R32G32B32_SFLOAT;
    demo->vertices.vi_attrs[0].offsetInBytes = 0;

    demo->vertices.vi_attrs[1].binding = 0;
    demo->vertices.vi_attrs[1].format = XGL_FMT_R32G32_SFLOAT;
    demo->vertices.vi_attrs[1].offsetInBytes = sizeof(float) * 3;
}

static void demo_prepare_descriptor_layout(struct demo *demo)
{
    const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO descriptor_layout = {
        .sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .descriptorType = XGL_DESCRIPTOR_TYPE_SAMPLER_TEXTURE,
        .count = DEMO_TEXTURE_COUNT,
        .stageFlags = XGL_SHADER_STAGE_FLAGS_FRAGMENT_BIT,
        .immutableSampler = XGL_NULL_HANDLE,
    };
    const uint32_t bind_point = 0;
    XGL_RESULT err;

    err = xglCreateDescriptorSetLayout(demo->device,
            XGL_SHADER_STAGE_FLAGS_ALL, &bind_point,
            XGL_NULL_HANDLE, &descriptor_layout,
            &demo->desc_layout);
    assert(!err);
}

static XGL_SHADER demo_prepare_shader(struct demo *demo,
                                      XGL_PIPELINE_SHADER_STAGE stage,
                                      const void *code,
                                      size_t size)
{
    XGL_SHADER_CREATE_INFO createInfo;
    XGL_SHADER shader;
    XGL_RESULT err;

    createInfo.sType = XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    createInfo.pNext = NULL;

    // Create fake BIL structure to feed GLSL
    // to the driver "under the covers"
    createInfo.codeSize = 3 * sizeof(uint32_t) + size + 1;
    createInfo.pCode = malloc(createInfo.codeSize);
    createInfo.flags = 0;

    /* try version 0 first: XGL_PIPELINE_SHADER_STAGE followed by GLSL */
    ((uint32_t *) createInfo.pCode)[0] = ICD_BIL_MAGIC;
    ((uint32_t *) createInfo.pCode)[1] = 0;
    ((uint32_t *) createInfo.pCode)[2] = stage;
    memcpy(((uint32_t *) createInfo.pCode + 3), code, size + 1);

    err = xglCreateShader(demo->device, &createInfo, &shader);
    if (err) {
        free((void *) createInfo.pCode);
        return NULL;
    }

    return shader;
}

static XGL_SHADER demo_prepare_vs(struct demo *demo)
{
    static const char *vertShaderText =
            "#version 130\n"
            "in vec4 pos;\n"
            "in vec2 attr;\n"
            "out vec2 texcoord;\n"
            "void main() {\n"
            "   texcoord = attr;\n"
            "   gl_Position = pos;\n"
            "}\n";

    return demo_prepare_shader(demo, XGL_SHADER_STAGE_VERTEX,
                               (const void *) vertShaderText,
                               strlen(vertShaderText));
}

static XGL_SHADER demo_prepare_fs(struct demo *demo)
{
    static const char *fragShaderText =
            "#version 130\n"
            "uniform sampler2D tex;\n"
            "in vec2 texcoord;\n"
            "void main() {\n"
            "   gl_FragColor = texture(tex, texcoord);\n"
            "}\n";

    return demo_prepare_shader(demo, XGL_SHADER_STAGE_FRAGMENT,
                               (const void *) fragShaderText,
                               strlen(fragShaderText));
}

static void demo_prepare_pipeline(struct demo *demo)
{
    XGL_GRAPHICS_PIPELINE_CREATE_INFO pipeline;
    XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO vi;
    XGL_PIPELINE_IA_STATE_CREATE_INFO ia;
    XGL_PIPELINE_RS_STATE_CREATE_INFO rs;
    XGL_PIPELINE_CB_STATE_CREATE_INFO cb;
    XGL_PIPELINE_DS_STATE_CREATE_INFO ds;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO vs;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO fs;
    XGL_PIPELINE_VP_STATE_CREATE_INFO vp;
    XGL_PIPELINE_MS_STATE_CREATE_INFO ms;
    XGL_RESULT err;

    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.sType = XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.lastSetLayout = demo->desc_layout;

    vi = demo->vertices.vi;

    memset(&ia, 0, sizeof(ia));
    ia.sType = XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO;
    ia.topology = XGL_TOPOLOGY_TRIANGLE_LIST;

    memset(&rs, 0, sizeof(rs));
    rs.sType = XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO;
    rs.fillMode = XGL_FILL_SOLID;
    rs.cullMode = XGL_CULL_NONE;
    rs.frontFace = XGL_FRONT_FACE_CCW;

    memset(&cb, 0, sizeof(cb));
    cb.sType = XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO;
    XGL_PIPELINE_CB_ATTACHMENT_STATE att_state[1];
    memset(att_state, 0, sizeof(att_state));
    att_state[0].format = demo->format;
    att_state[0].channelWriteMask = 0xf;
    att_state[0].blendEnable = XGL_FALSE;
    cb.attachmentCount = 1;
    cb.pAttachments = att_state;


    memset(&vp, 0, sizeof(vp));
    vp.sType = XGL_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO;


    memset(&ds, 0, sizeof(ds));
    ds.sType = XGL_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO;
    ds.format = demo->depth.format;
    ds.depthTestEnable = XGL_TRUE;
    ds.depthWriteEnable = XGL_TRUE;
    ds.depthFunc = XGL_COMPARE_LESS_EQUAL;
    ds.depthBoundsEnable = XGL_FALSE;
    ds.back.stencilFailOp = XGL_STENCIL_OP_KEEP;
    ds.back.stencilPassOp = XGL_STENCIL_OP_KEEP;
    ds.back.stencilFunc = XGL_COMPARE_ALWAYS;
    ds.stencilTestEnable = XGL_FALSE;
    ds.front = ds.back;

    memset(&vs, 0, sizeof(vs));
    vs.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs.shader.stage = XGL_SHADER_STAGE_VERTEX;
    vs.shader.shader = demo_prepare_vs(demo);
    vs.shader.linkConstBufferCount = 0;

    memset(&fs, 0, sizeof(fs));
    fs.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fs.shader.stage = XGL_SHADER_STAGE_FRAGMENT;
    fs.shader.shader = demo_prepare_fs(demo);

    memset(&ms, 0, sizeof(ms));
    ms.sType = XGL_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO;
    ms.sampleMask = 1;
    ms.multisampleEnable = XGL_FALSE;
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

    err = xglCreateGraphicsPipeline(demo->device, &pipeline, &demo->pipeline);
    assert(!err);

    xglDestroyObject(vs.shader.shader);
    xglDestroyObject(fs.shader.shader);
}

static void demo_prepare_dynamic_states(struct demo *demo)
{
    XGL_DYNAMIC_VP_STATE_CREATE_INFO viewport_create;
    XGL_DYNAMIC_RS_STATE_CREATE_INFO raster;
    XGL_DYNAMIC_CB_STATE_CREATE_INFO color_blend;
    XGL_DYNAMIC_DS_STATE_CREATE_INFO depth_stencil;
    XGL_RESULT err;

    memset(&viewport_create, 0, sizeof(viewport_create));
    viewport_create.sType = XGL_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO;
    viewport_create.viewportAndScissorCount = 1;
    XGL_VIEWPORT viewport;
    XGL_RECT scissor;
    memset(&viewport, 0, sizeof(viewport));
    viewport.height = (float) demo->height;
    viewport.width = (float) demo->width;
    viewport.minDepth = (float) 0.0f;
    viewport.maxDepth = (float) 1.0f;
    scissor.extent.width = demo->width;
    scissor.extent.height = demo->height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    viewport_create.pViewports = &viewport;
    viewport_create.pScissors = &scissor;

    memset(&raster, 0, sizeof(raster));
    raster.sType = XGL_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO;

    memset(&color_blend, 0, sizeof(color_blend));
    color_blend.sType = XGL_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO;

    memset(&depth_stencil, 0, sizeof(depth_stencil));
    depth_stencil.sType = XGL_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO;
    depth_stencil.stencilBackRef = 0;
    depth_stencil.stencilFrontRef = 0;
    depth_stencil.stencilReadMask = 0xff;
    depth_stencil.stencilWriteMask = 0xff;

    err = xglCreateDynamicViewportState(demo->device, &viewport_create, &demo->viewport);
    assert(!err);

    err = xglCreateDynamicRasterState(demo->device, &raster, &demo->raster);
    assert(!err);

    err = xglCreateDynamicColorBlendState(demo->device,
            &color_blend, &demo->color_blend);
    assert(!err);

    err = xglCreateDynamicDepthStencilState(demo->device,
            &depth_stencil, &demo->depth_stencil);
    assert(!err);
}

static void demo_prepare_descriptor_region(struct demo *demo)
{
    const XGL_DESCRIPTOR_TYPE_COUNT type_count = {
        .type = XGL_DESCRIPTOR_TYPE_SAMPLER_TEXTURE,
        .count = DEMO_TEXTURE_COUNT,
    };
    const XGL_DESCRIPTOR_REGION_CREATE_INFO descriptor_region = {
        .sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_REGION_CREATE_INFO,
        .pNext = NULL,
        .count = 1,
        .pTypeCount = &type_count,
    };
    XGL_RESULT err;

    err = xglCreateDescriptorRegion(demo->device,
            XGL_DESCRIPTOR_REGION_USAGE_ONE_SHOT, 1,
            &descriptor_region, &demo->desc_region);
    assert(!err);
}

static void demo_prepare_descriptor_set(struct demo *demo)
{
    XGL_IMAGE_VIEW_ATTACH_INFO view_info[DEMO_TEXTURE_COUNT];
    XGL_SAMPLER_IMAGE_VIEW_INFO combined_info[DEMO_TEXTURE_COUNT];
    XGL_UPDATE_SAMPLER_TEXTURES update;
    XGL_RESULT err;
    uint32_t count;
    uint32_t i;

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        view_info[i].sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO;
        view_info[i].pNext = NULL;
        view_info[i].view = demo->textures[i].view,
        view_info[i].layout = XGL_IMAGE_LAYOUT_GENERAL;

        combined_info[i].pSampler = demo->textures[i].sampler;
        combined_info[i].pImageView = &view_info[i];
    }

    memset(&update, 0, sizeof(update));
    update.sType = XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES;
    update.count = DEMO_TEXTURE_COUNT;
    update.pSamplerImageViews = combined_info;

    err = xglAllocDescriptorSets(demo->desc_region,
            XGL_DESCRIPTOR_SET_USAGE_STATIC,
            1, &demo->desc_layout,
            &demo->desc_set, &count);
    assert(!err && count == 1);

    xglBeginDescriptorRegionUpdate(demo->device,
            XGL_DESCRIPTOR_UPDATE_MODE_FASTEST);

    xglClearDescriptorSets(demo->desc_region, 1, &demo->desc_set);
    xglUpdateDescriptors(demo->desc_set, &update);

    xglEndDescriptorRegionUpdate(demo->device, demo->cmd);
}

static void demo_prepare(struct demo *demo)
{
    const XGL_CMD_BUFFER_CREATE_INFO cmd = {
        .sType = XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .queueType = XGL_QUEUE_TYPE_GRAPHICS,
        .flags = 0,
    };
    XGL_RESULT err;

    demo_prepare_buffers(demo);
    demo_prepare_depth(demo);
    demo_prepare_textures(demo);
    demo_prepare_vertices(demo);
    demo_prepare_descriptor_layout(demo);
    demo_prepare_pipeline(demo);
    demo_prepare_dynamic_states(demo);

    err = xglCreateCommandBuffer(demo->device, &cmd, &demo->cmd);
    assert(!err);

    demo_prepare_descriptor_region(demo);
    demo_prepare_descriptor_set(demo);
}

#if defined(__linux__)
static void demo_handle_event(struct demo *demo,
                              const xcb_generic_event_t *event)
#else
static void demo_handle_event(struct demo *demo)
#endif
{
#if defined(__linux__)
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
#else
    // Since we don't yet support events, we'll just do a draw
    demo_draw(demo);
#endif
}

static void demo_run(struct demo *demo)
{
#if defined(__linux__)
    xcb_flush(demo->connection);

    while (!demo->quit) {
        xcb_generic_event_t *event;

        event = xcb_wait_for_event(demo->connection);
        if (event) {
            demo_handle_event(demo, event);
            free(event);
        }
    }
#else
    // Since we don't yet support events, just call demo_handle_event to do a draw
    demo_handle_event(demo);

#endif
}

static void demo_create_window(struct demo *demo)
{
#if defined(__linux__)
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
#endif
}

static void demo_init_xgl(struct demo *demo)
{
    const XGL_APPLICATION_INFO app = {
        .sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pAppName = "tri",
        .appVersion = 0,
        .pEngineName = "tri",
        .engineVersion = 0,
        .apiVersion = XGL_MAKE_VERSION(0, 22, 0),
    };
#if defined(__linux__)
    const XGL_WSI_X11_CONNECTION_INFO connection = {
        .pConnection = demo->connection,
        .root = demo->screen->root,
        .provider = 0,
    };
#endif
    const XGL_DEVICE_QUEUE_CREATE_INFO queue = {
        .queueNodeIndex = 0,
        .queueCount = 1,
    };
    const char *ext_names[] = {
#if defined(__linux__)
        "XGL_WSI_X11",
#else
        "XGL_WSI_WINDOWS",
#endif
    };
    const XGL_DEVICE_CREATE_INFO device = {
        .sType = XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueRecordCount = 1,
        .pRequestedQueues = &queue,
        .extensionCount = 1,
        .ppEnabledExtensionNames = ext_names,
        .maxValidationLevel = XGL_VALIDATION_LEVEL_END_RANGE,
        .flags = XGL_DEVICE_CREATE_VALIDATION_BIT,
    };
    XGL_RESULT err;
    uint32_t gpu_count;
    uint32_t i;

    err = xglCreateInstance(&app, NULL, &demo->inst);
    assert(!err);
    err = xglEnumerateGpus(demo->inst, 1, &gpu_count, &demo->gpu);
    assert(!err && gpu_count == 1);

    for (i = 0; i < device.extensionCount; i++) {
        err = xglGetExtensionSupport(demo->gpu, ext_names[i]);
        assert(!err);
    }

#if defined(__linux__)
    err = xglWsiX11AssociateConnection(demo->gpu, &connection);
    assert(!err);
#endif

    err = xglCreateDevice(demo->gpu, &device, &demo->device);
    assert(!err);

    err = xglGetDeviceQueue(demo->device, XGL_QUEUE_TYPE_GRAPHICS,
            0, &demo->queue);
    assert(!err);
}

static void demo_init_connection(struct demo *demo)
{
#if defined(__linux__)
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

    demo->connection = xcb_connect(NULL, &scr);

    setup = xcb_get_setup(demo->connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0)
        xcb_screen_next(&iter);

    demo->screen = iter.data;
#endif
}

static void demo_init(struct demo *demo)
{
    memset(demo, 0, sizeof(*demo));

    demo_init_connection(demo);
    demo_init_xgl(demo);

    demo->width = 300;
    demo->height = 300;
    demo->format = XGL_FMT_B8G8R8A8_UNORM;
}

static void demo_cleanup(struct demo *demo)
{
    uint32_t i, j;

    xglDestroyObject(demo->desc_set);
    xglDestroyObject(demo->desc_region);

    xglDestroyObject(demo->cmd);

    xglDestroyObject(demo->viewport);
    xglDestroyObject(demo->raster);
    xglDestroyObject(demo->color_blend);
    xglDestroyObject(demo->depth_stencil);

    xglDestroyObject(demo->pipeline);
    xglDestroyObject(demo->desc_layout);

    xglBindObjectMemory(demo->vertices.buf, 0, XGL_NULL_HANDLE, 0);
    xglDestroyObject(demo->vertices.buf);
    for (j = 0; j < demo->vertices.num_mem; j++)
        xglFreeMemory(demo->vertices.mem[j]);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        xglDestroyObject(demo->textures[i].view);
        xglBindObjectMemory(demo->textures[i].image, 0, XGL_NULL_HANDLE, 0);
        xglDestroyObject(demo->textures[i].image);
        for (j = 0; j < demo->textures[i].num_mem; j++)
            xglFreeMemory(demo->textures[i].mem[j]);
        xglDestroyObject(demo->textures[i].sampler);
    }

    xglDestroyObject(demo->depth.view);
    xglBindObjectMemory(demo->depth.image, 0, XGL_NULL_HANDLE, 0);
    xglDestroyObject(demo->depth.image);
    for (j = 0; j < demo->depth.num_mem; j++)
        xglFreeMemory(demo->depth.mem[j]);

    for (i = 0; i < DEMO_BUFFER_COUNT; i++) {
        xglDestroyObject(demo->buffers[i].fence);
        xglDestroyObject(demo->buffers[i].view);
#if defined(__linux__)
        xglDestroyObject(demo->buffers[i].image);
#endif
    }

    xglDestroyDevice(demo->device);
    xglDestroyInstance(demo->inst);

#if defined(__linux__)
    xcb_destroy_window(demo->connection, demo->window);
    xcb_disconnect(demo->connection);
#endif
}

int main(void)
{
    struct demo demo;

    demo_init(&demo);

    demo_prepare(&demo);
    demo_create_window(&demo);
    demo_run(&demo);

    demo_cleanup(&demo);

#if defined(__linux__)
#else
    printf("try exitting...\n"); fflush(stdout);
#endif
    return 0;
}
