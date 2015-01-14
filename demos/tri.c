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
#include <xgl.h>
#include <xglDbg.h>
#include <xglWsiX11Ext.h>

#include "icd-bil.h"

#define DEMO_BUFFER_COUNT 2
#define DEMO_TEXTURE_COUNT 1

struct demo {
    xcb_connection_t *connection;
    xcb_screen_t *screen;

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
        XGL_GPU_MEMORY mem;
        XGL_DEPTH_STENCIL_VIEW view;
    } depth;

    struct {
        XGL_SAMPLER sampler;

        XGL_IMAGE image;
        XGL_GPU_MEMORY mem;
        XGL_IMAGE_VIEW view;
    } textures[DEMO_TEXTURE_COUNT];

    struct {
        XGL_GPU_MEMORY mem;

        XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO vi;
        XGL_VERTEX_INPUT_BINDING_DESCRIPTION vi_bindings[1];
        XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION vi_attrs[2];
    } vertices;

    XGL_DESCRIPTOR_SET dset;

    XGL_PIPELINE pipeline;

    XGL_VIEWPORT_STATE_OBJECT viewport;
    XGL_RASTER_STATE_OBJECT raster;
    XGL_MSAA_STATE_OBJECT msaa;
    XGL_COLOR_BLEND_STATE_OBJECT color_blend;
    XGL_DEPTH_STENCIL_STATE_OBJECT depth_stencil;

    XGL_CMD_BUFFER cmd;

    xcb_window_t window;
    xcb_intern_atom_reply_t *atom_wm_delete_window;

    bool quit;
    XGL_UINT current_buffer;
};

static void demo_draw_build_cmd(struct demo *demo)
{
    const XGL_COLOR_ATTACHMENT_BIND_INFO color_attachment = {
        .view = demo->buffers[demo->current_buffer].view,
        .colorAttachmentState = XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL,
    };
    const XGL_DEPTH_STENCIL_BIND_INFO depth_stencil = {
        .view = demo->depth.view,
        .depthState = XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL,
        .stencilState = XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL,
    };
    const XGL_FLOAT clear_color[4] = { 0.2f, 0.2f, 0.2f, 0.2f };
    const XGL_FLOAT clear_depth = 0.9f;
    XGL_IMAGE_SUBRESOURCE_RANGE clear_range;
    XGL_RESULT err;

    err = xglBeginCommandBuffer(demo->cmd,
            XGL_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT |
            XGL_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT);
    assert(!err);

    xglCmdBindPipeline(demo->cmd, XGL_PIPELINE_BIND_POINT_GRAPHICS,
                                  demo->pipeline);
    xglCmdBindDescriptorSet(demo->cmd, XGL_PIPELINE_BIND_POINT_GRAPHICS,
            0, demo->dset, 0);

    xglCmdBindStateObject(demo->cmd, XGL_STATE_BIND_VIEWPORT, demo->viewport);
    xglCmdBindStateObject(demo->cmd, XGL_STATE_BIND_RASTER, demo->raster);
    xglCmdBindStateObject(demo->cmd, XGL_STATE_BIND_MSAA, demo->msaa);
    xglCmdBindStateObject(demo->cmd, XGL_STATE_BIND_COLOR_BLEND,
                                     demo->color_blend);
    xglCmdBindStateObject(demo->cmd, XGL_STATE_BIND_DEPTH_STENCIL,
                                     demo->depth_stencil);

    xglCmdBindAttachments(demo->cmd, 1, &color_attachment, &depth_stencil);

    xglCmdBindVertexData(demo->cmd, demo->vertices.mem, 0, 0);

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
    const XGL_WSI_X11_PRESENT_INFO present = {
        .destWindow = demo->window,
        .srcImage = demo->buffers[demo->current_buffer].image,
    };
    XGL_FENCE fence = demo->buffers[demo->current_buffer].fence;
    XGL_RESULT err;

    demo_draw_build_cmd(demo);

    err = xglWaitForFences(demo->device, 1, &fence, XGL_TRUE, ~((XGL_UINT64) 0));
    assert(err == XGL_SUCCESS || err == XGL_ERROR_UNAVAILABLE);

    static const uint32_t NUM_MEM_REFS = 5;
    XGL_MEMORY_REF memRefs[NUM_MEM_REFS];
    memRefs[0].mem = demo->depth.mem;
    memRefs[0].flags = 0;
    memRefs[1].mem = demo->textures[0].mem;
    memRefs[1].flags = 0;
    memRefs[2].mem = demo->buffers[0].mem;
    memRefs[2].flags = 0;
    memRefs[3].mem = demo->buffers[1].mem;
    memRefs[3].flags = 0;
    memRefs[4].mem = demo->vertices.mem;
    memRefs[4].flags = 0;
    err = xglQueueSubmit(demo->queue, 1, &demo->cmd,
            NUM_MEM_REFS, memRefs, XGL_NULL_HANDLE);
    assert(!err);

    err = xglWsiX11QueuePresent(demo->queue, &present, fence);
    assert(!err);

    demo->current_buffer = (demo->current_buffer + 1) % DEMO_BUFFER_COUNT;
}

static void demo_prepare_buffers(struct demo *demo)
{
    const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO presentable_image = {
        .format = demo->format,
        .usage = XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .extent = {
            .width = demo->width,
            .height = demo->height,
        },
        .flags = 0,
    };
    const XGL_FENCE_CREATE_INFO fence = {
        .sType = XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
    };
    XGL_RESULT err;
    XGL_UINT i;

    for (i = 0; i < DEMO_BUFFER_COUNT; i++) {
        XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO color_attachment_view = {
            .sType = XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO,
            .pNext = NULL,
            .format = demo->format,
            .mipLevel = 0,
            .baseArraySlice = 0,
            .arraySize = 1,
        };

        err = xglWsiX11CreatePresentableImage(demo->device, &presentable_image,
                &demo->buffers[i].image, &demo->buffers[i].mem);
        assert(!err);

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
    const XGL_FORMAT depth_format = { XGL_CH_FMT_R16, XGL_NUM_FMT_DS };
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
    XGL_MEMORY_ALLOC_INFO mem_alloc = {
        .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = NULL,
        .allocationSize = 0,
        .alignment = 0,
        .flags = 0,
        .heapCount = 0,
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
    XGL_MEMORY_REQUIREMENTS mem_reqs;
    XGL_SIZE mem_reqs_size= sizeof(XGL_MEMORY_REQUIREMENTS);
    XGL_RESULT err;

    demo->depth.format = depth_format;

    /* create image */
    err = xglCreateImage(demo->device, &image,
            &demo->depth.image);
    assert(!err);

    err = xglGetObjectInfo(demo->depth.image,
            XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
            &mem_reqs_size, &mem_reqs);
    assert(!err && mem_reqs_size == sizeof(mem_reqs));

    mem_alloc.allocationSize = mem_reqs.size;
    mem_alloc.alignment = mem_reqs.alignment;
    mem_alloc.heapCount = mem_reqs.heapCount;
    memcpy(mem_alloc.heaps, mem_reqs.heaps,
            sizeof(mem_reqs.heaps[0]) * mem_reqs.heapCount);

    /* allocate memory */
    err = xglAllocMemory(demo->device, &mem_alloc,
            &demo->depth.mem);
    assert(!err);

    /* bind memory */
    err = xglBindObjectMemory(demo->depth.image,
            demo->depth.mem, 0);
    assert(!err);

    /* create image view */
    view.image = demo->depth.image;
    err = xglCreateDepthStencilView(demo->device, &view,
            &demo->depth.view);
    assert(!err);
}

static void demo_prepare_textures(struct demo *demo)
{
    const XGL_FORMAT tex_format = { XGL_CH_FMT_B8G8R8A8, XGL_NUM_FMT_UNORM };
    const XGL_INT tex_width = 2;
    const XGL_INT tex_height = 2;
    const uint32_t tex_colors[DEMO_TEXTURE_COUNT][2] = {
        { 0xffff0000, 0xff00ff00 },
    };
    XGL_RESULT err;
    XGL_UINT i;

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
        XGL_MEMORY_ALLOC_INFO mem_alloc = {
            .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
            .pNext = NULL,
            .allocationSize = 0,
            .alignment = 0,
            .flags = 0,
            .heapCount = 0,
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
        XGL_MEMORY_REQUIREMENTS mem_reqs;
        XGL_SIZE mem_reqs_size= sizeof(XGL_MEMORY_REQUIREMENTS);

        /* create sampler */
        err = xglCreateSampler(demo->device, &sampler,
                &demo->textures[i].sampler);
        assert(!err);

        /* create image */
        err = xglCreateImage(demo->device, &image,
                &demo->textures[i].image);
        assert(!err);

        err = xglGetObjectInfo(demo->textures[i].image,
                XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                &mem_reqs_size, &mem_reqs);
        assert(!err && mem_reqs_size == sizeof(mem_reqs));

        mem_alloc.allocationSize = mem_reqs.size;
        mem_alloc.alignment = mem_reqs.alignment;
        mem_alloc.heapCount = mem_reqs.heapCount;
        memcpy(mem_alloc.heaps, mem_reqs.heaps,
                sizeof(mem_reqs.heaps[0]) * mem_reqs.heapCount);

        /* allocate memory */
        err = xglAllocMemory(demo->device, &mem_alloc,
                &demo->textures[i].mem);
        assert(!err);

        /* bind memory */
        err = xglBindObjectMemory(demo->textures[i].image,
                demo->textures[i].mem, 0);
        assert(!err);

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
        XGL_SIZE layout_size;
        XGL_VOID *data;
        XGL_INT x, y;

        err = xglGetImageSubresourceInfo(demo->textures[i].image, &subres,
                XGL_INFO_TYPE_SUBRESOURCE_LAYOUT, &layout_size, &layout);
        assert(!err && layout_size == sizeof(layout));

        err = xglMapMemory(demo->textures[i].mem, 0, &data);
        assert(!err);

        for (y = 0; y < tex_height; y++) {
            uint32_t *row = (uint32_t *) ((char *) data + layout.rowPitch * y);
            for (x = 0; x < tex_width; x++)
                row[x] = tex_colors[i][(x & 1) ^ (y & 1)];
        }

        err = xglUnmapMemory(demo->textures[i].mem);
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
    const XGL_MEMORY_ALLOC_INFO mem_alloc = {
        .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = NULL,
        .allocationSize = sizeof(vb),
        .alignment = 0,
        .flags = 0,
        .heapCount = 1,
        .heaps[0] = 0,
        .memPriority = XGL_MEMORY_PRIORITY_NORMAL,
    };
    XGL_RESULT err;
    void *data;

    memset(&demo->vertices, 0, sizeof(demo->vertices));

    err = xglAllocMemory(demo->device, &mem_alloc, &demo->vertices.mem);
    assert(!err);

    err = xglMapMemory(demo->vertices.mem, 0, &data);
    assert(!err);

    memcpy(data, vb, sizeof(vb));

    err = xglUnmapMemory(demo->vertices.mem);
    assert(!err);

    demo->vertices.vi.sType = XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO;
    demo->vertices.vi.pNext = NULL;
    demo->vertices.vi.bindingCount = 1;
    demo->vertices.vi.pVertexBindingDescriptions = demo->vertices.vi_bindings;
    demo->vertices.vi.attributeCount = 2;
    demo->vertices.vi.pVertexAttributeDescriptions = demo->vertices.vi_attrs;

    demo->vertices.vi_bindings[0].strideInBytes = sizeof(vb[0]);
    demo->vertices.vi_bindings[0].stepRate = XGL_VERTEX_INPUT_STEP_RATE_VERTEX;

    demo->vertices.vi_attrs[0].binding = 0;
    demo->vertices.vi_attrs[0].format.channelFormat = XGL_CH_FMT_R32G32B32;
    demo->vertices.vi_attrs[0].format.numericFormat = XGL_NUM_FMT_FLOAT;
    demo->vertices.vi_attrs[0].offsetInBytes = 0;

    demo->vertices.vi_attrs[1].binding = 0;
    demo->vertices.vi_attrs[1].format.channelFormat = XGL_CH_FMT_R32G32;
    demo->vertices.vi_attrs[1].format.numericFormat = XGL_NUM_FMT_FLOAT;
    demo->vertices.vi_attrs[1].offsetInBytes = sizeof(float) * 3;
}

static void demo_prepare_descriptor_set(struct demo *demo)
{
    const XGL_DESCRIPTOR_SET_CREATE_INFO descriptor_set = {
        .sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO,
        .pNext = NULL,
        .slots = DEMO_TEXTURE_COUNT * 2,
    };
    XGL_RESULT err;
    XGL_UINT i;

    err = xglCreateDescriptorSet(demo->device, &descriptor_set, &demo->dset);
    assert(!err);

    xglBeginDescriptorSetUpdate(demo->dset);
    xglClearDescriptorSetSlots(demo->dset, 0, DEMO_TEXTURE_COUNT * 2);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        const XGL_IMAGE_VIEW_ATTACH_INFO image_view = {
            .sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO,
            .pNext = NULL,
            .view = demo->textures[i].view,
            .state = XGL_IMAGE_STATE_GRAPHICS_SHADER_READ_ONLY,
        };

        xglAttachSamplerDescriptors(demo->dset, 2 * i, 1,
                &demo->textures[i].sampler);
        xglAttachImageViewDescriptors(demo->dset, 2 * i + 1, 1,
                &image_view);
    }

    xglEndDescriptorSetUpdate(demo->dset);
}

static XGL_SHADER demo_prepare_shader(struct demo *demo,
                                      XGL_PIPELINE_SHADER_STAGE stage,
                                      const void *code,
                                      XGL_SIZE size)
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
    XGL_PIPELINE_CB_STATE cb;
    XGL_PIPELINE_DB_STATE_CREATE_INFO db;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO vs;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO fs;
    XGL_DESCRIPTOR_SLOT_INFO fs_slots[DEMO_TEXTURE_COUNT * 2];
    XGL_RESULT err;
    XGL_UINT i;

    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.sType = XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    vi = demo->vertices.vi;

    memset(&ia, 0, sizeof(ia));
    ia.sType = XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO;
    ia.topology = XGL_TOPOLOGY_TRIANGLE_LIST;

    memset(&rs, 0, sizeof(rs));
    rs.sType = XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO;

    memset(&cb, 0, sizeof(cb));
    cb.sType = XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO;
    cb.attachment[0].format = demo->format;
    cb.attachment[0].channelWriteMask = 0xf;

    memset(&db, 0, sizeof(db));
    db.sType = XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO;
    db.format = demo->depth.format;

    memset(&fs_slots, 0, sizeof(fs_slots));
    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        fs_slots[2 * i].slotObjectType = XGL_SLOT_SHADER_SAMPLER;
        fs_slots[2 * i].shaderEntityIndex = i;
        fs_slots[2 * i + 1].slotObjectType = XGL_SLOT_SHADER_TEXTURE_RESOURCE;
        fs_slots[2 * i + 1].shaderEntityIndex = i;
    }

    memset(&vs, 0, sizeof(vs));
    vs.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs.shader.stage = XGL_SHADER_STAGE_VERTEX;
    vs.shader.shader = demo_prepare_vs(demo);

    memset(&fs, 0, sizeof(fs));
    fs.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fs.shader.stage = XGL_SHADER_STAGE_FRAGMENT;
    fs.shader.shader = demo_prepare_fs(demo);
    fs.shader.descriptorSetMapping[0].descriptorCount =
        DEMO_TEXTURE_COUNT * 2;
    fs.shader.descriptorSetMapping[0].pDescriptorInfo = fs_slots;

    pipeline.pNext = (const XGL_VOID *) &vi;
    vi.pNext = (XGL_VOID *) &ia;
    ia.pNext = (const XGL_VOID *) &rs;
    rs.pNext = (const XGL_VOID *) &cb;
    cb.pNext = (const XGL_VOID *) &db;
    db.pNext = (const XGL_VOID *) &vs;
    vs.pNext = (const XGL_VOID *) &fs;

    err = xglCreateGraphicsPipeline(demo->device, &pipeline, &demo->pipeline);
    assert(!err);

    xglDestroyObject(vs.shader.shader);
    xglDestroyObject(fs.shader.shader);
}

static void demo_prepare_dynamic_states(struct demo *demo)
{
    XGL_VIEWPORT_STATE_CREATE_INFO viewport;
    XGL_RASTER_STATE_CREATE_INFO raster;
    XGL_MSAA_STATE_CREATE_INFO msaa;
    XGL_COLOR_BLEND_STATE_CREATE_INFO color_blend;
    XGL_DEPTH_STENCIL_STATE_CREATE_INFO depth_stencil;
    XGL_RESULT err;

    memset(&viewport, 0, sizeof(viewport));
    viewport.viewportCount = 1;
    viewport.scissorEnable = XGL_FALSE;
    viewport.viewports[0].width = (XGL_FLOAT) demo->width;
    viewport.viewports[0].height = (XGL_FLOAT) demo->height;
    viewport.viewports[0].minDepth = (XGL_FLOAT) 0.0f;
    viewport.viewports[0].maxDepth = (XGL_FLOAT) 1.0f;

    memset(&raster, 0, sizeof(raster));
    raster.sType = XGL_STRUCTURE_TYPE_RASTER_STATE_CREATE_INFO;
    raster.fillMode = XGL_FILL_SOLID;
    raster.cullMode = XGL_CULL_NONE;
    raster.frontFace = XGL_FRONT_FACE_CCW;

    memset(&msaa, 0, sizeof(msaa));
    msaa.sType = XGL_STRUCTURE_TYPE_MSAA_STATE_CREATE_INFO;
    msaa.samples = 1;
    msaa.sampleMask = 0x1;

    memset(&color_blend, 0, sizeof(color_blend));
    color_blend.sType = XGL_STRUCTURE_TYPE_COLOR_BLEND_STATE_CREATE_INFO;

    memset(&depth_stencil, 0, sizeof(depth_stencil));
    depth_stencil.sType = XGL_STRUCTURE_TYPE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = XGL_TRUE;
    depth_stencil.depthWriteEnable = XGL_TRUE;
    depth_stencil.depthFunc = XGL_COMPARE_LESS_EQUAL;
    depth_stencil.depthBoundsEnable = XGL_FALSE;

    err = xglCreateViewportState(demo->device, &viewport, &demo->viewport);
    assert(!err);

    err = xglCreateRasterState(demo->device, &raster, &demo->raster);
    assert(!err);

    err = xglCreateMsaaState(demo->device, &msaa, &demo->msaa);
    assert(!err);

    err = xglCreateColorBlendState(demo->device,
            &color_blend, &demo->color_blend);
    assert(!err);

    err = xglCreateDepthStencilState(demo->device,
            &depth_stencil, &demo->depth_stencil);
    assert(!err);
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
    demo_prepare_descriptor_set(demo);

    demo_prepare_pipeline(demo);
    demo_prepare_dynamic_states(demo);

    err = xglCreateCommandBuffer(demo->device, &cmd, &demo->cmd);
    assert(!err);
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
    const XGL_WSI_X11_CONNECTION_INFO connection = {
        .pConnection = demo->connection,
        .root = demo->screen->root,
        .provider = 0,
    };
    const XGL_DEVICE_QUEUE_CREATE_INFO queue = {
        .queueNodeIndex = 0,
        .queueCount = 1,
    };
    const XGL_CHAR *ext_names[] = {
        "XGL_WSI_X11",
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
    XGL_UINT gpu_count;
    XGL_UINT i;

    err = xglInitAndEnumerateGpus(&app, NULL, 1, &gpu_count, &demo->gpu);
    assert(!err && gpu_count == 1);

    for (i = 0; i < device.extensionCount; i++) {
        err = xglGetExtensionSupport(demo->gpu, ext_names[i]);
        assert(!err);
    }

    err = xglWsiX11AssociateConnection(demo->gpu, &connection);
    assert(!err);

    err = xglCreateDevice(demo->gpu, &device, &demo->device);
    assert(!err);

    err = xglGetDeviceQueue(demo->device, XGL_QUEUE_TYPE_GRAPHICS,
            0, &demo->queue);
    assert(!err);
}

static void demo_init_connection(struct demo *demo)
{
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

    demo->connection = xcb_connect(NULL, &scr);

    setup = xcb_get_setup(demo->connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0)
        xcb_screen_next(&iter);

    demo->screen = iter.data;
}

static void demo_init(struct demo *demo)
{
    memset(demo, 0, sizeof(*demo));

    demo_init_connection(demo);
    demo_init_xgl(demo);

    demo->width = 300;
    demo->height = 300;
    demo->format.channelFormat = XGL_CH_FMT_B8G8R8A8;
    demo->format.numericFormat = XGL_NUM_FMT_UNORM;
}

static void demo_cleanup(struct demo *demo)
{
    XGL_UINT i;

    xglDestroyObject(demo->cmd);

    xglDestroyObject(demo->viewport);
    xglDestroyObject(demo->raster);
    xglDestroyObject(demo->msaa);
    xglDestroyObject(demo->color_blend);
    xglDestroyObject(demo->depth_stencil);

    xglDestroyObject(demo->pipeline);

    xglDestroyObject(demo->dset);

    xglFreeMemory(demo->vertices.mem);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        xglDestroyObject(demo->textures[i].view);
        xglBindObjectMemory(demo->textures[i].image, 0, XGL_NULL_HANDLE);
        xglDestroyObject(demo->textures[i].image);
        xglFreeMemory(demo->textures[i].mem);
        xglDestroyObject(demo->textures[i].sampler);
    }

    xglDestroyObject(demo->depth.view);
    xglBindObjectMemory(demo->depth.image, 0, XGL_NULL_HANDLE);
    xglDestroyObject(demo->depth.image);
    xglFreeMemory(demo->depth.mem);

    for (i = 0; i < DEMO_BUFFER_COUNT; i++) {
        xglDestroyObject(demo->buffers[i].fence);
        xglDestroyObject(demo->buffers[i].view);
        xglDestroyObject(demo->buffers[i].image);
    }

    xglDestroyDevice(demo->device);

    xcb_destroy_window(demo->connection, demo->window);
    xcb_disconnect(demo->connection);
}

int main(void)
{
    struct demo demo;

    demo_init(&demo);

    demo_prepare(&demo);
    demo_create_window(&demo);
    demo_run(&demo);

    demo_cleanup(&demo);

    return 0;
}
