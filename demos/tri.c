#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <xcb/xcb.h>
#include <xgl.h>
#include <xglDbg.h>
#include <xglWsiX11Ext.h>

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
        XGL_MEMORY_VIEW_ATTACH_INFO view;
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
    XGL_RESULT err;

    demo_draw_build_cmd(demo);

    err = xglQueueSubmit(demo->queue, 1, &demo->cmd,
            0, NULL, XGL_NULL_HANDLE);
    assert(!err);

    err = xglWsiX11QueuePresent(demo->queue, &present, XGL_NULL_HANDLE);
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
    }
}

static void demo_prepare_depth(struct demo *demo)
{
    const XGL_FORMAT depth_format = { XGL_CH_FMT_R16, XGL_NUM_FMT_DS };
    const uint16_t depth_value = (uint16_t) (0.5f * 65535);
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
    XGL_SIZE mem_reqs_size;
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

    /* clear the buffer */
    {
        const XGL_INT tw = 128 / sizeof(uint16_t);
        const XGL_INT th = 32;
        XGL_INT i, j, w, h;
        XGL_VOID *data;

        w = (demo->width + tw - 1) / tw;
        h = (demo->height + th - 1) / th;

        err = xglMapMemory(demo->depth.mem, 0, &data);
        assert(!err);

        for (i = 0; i < w * h; i++) {
            uint16_t *tile = (uint16_t *) ((char *) data + 4096 * i);

            for (j = 0; j < 2048; j++)
                tile[j] = depth_value;
        }

        err = xglUnmapMemory(demo->depth.mem);
        assert(!err);
    }
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
        XGL_SIZE mem_reqs_size;

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
        { -1.0f, -1.0f, -1.0f,      0.0f, 0.0f },
        {  1.0f, -1.0f, -1.0f,      1.0f, 0.0f },
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

    demo->vertices.view.sType = XGL_STRUCTURE_TYPE_MEMORY_VIEW_ATTACH_INFO;
    demo->vertices.view.pNext = NULL;
    demo->vertices.view.mem = demo->vertices.mem;
    demo->vertices.view.offset = 0;
    demo->vertices.view.range = sizeof(vb);
    demo->vertices.view.stride = sizeof(vb[0]);
    demo->vertices.view.format.channelFormat = XGL_CH_FMT_UNDEFINED;
    demo->vertices.view.format.numericFormat = XGL_NUM_FMT_UNDEFINED;
    demo->vertices.view.state = XGL_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY;
}

static void demo_prepare_descriptor_set(struct demo *demo)
{
    const XGL_DESCRIPTOR_SET_CREATE_INFO descriptor_set = {
        .sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO,
        .pNext = NULL,
        .slots = DEMO_TEXTURE_COUNT * 2 + 1,
    };
    XGL_RESULT err;
    XGL_UINT i;

    err = xglCreateDescriptorSet(demo->device, &descriptor_set, &demo->dset);
    assert(!err);

    xglBeginDescriptorSetUpdate(demo->dset);
    xglClearDescriptorSetSlots(demo->dset, 0, DEMO_TEXTURE_COUNT * 2 + 1);

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

    xglAttachMemoryViewDescriptors(demo->dset, 2 * i, 1, &demo->vertices.view);

    xglEndDescriptorSetUpdate(demo->dset);
}

static XGL_SHADER demo_prepare_shader(struct demo *demo,
                                      const void *code,
                                      XGL_SIZE size)
{
    const XGL_SHADER_CREATE_INFO info = {
        .sType = XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO,
        .pNext = NULL,
        .codeSize = size,
        .pCode = (const XGL_VOID *) code,
        .flags = 0,
    };
    XGL_SHADER shader;
    XGL_RESULT err;

    err = xglCreateShader(demo->device, &info, &shader);
    assert(!err);

    return shader;
}

static XGL_SHADER demo_prepare_vs(struct demo *demo)
{
    XGL_RESULT err;
    XGL_SIZE size;
    XGL_PHYSICAL_GPU_PROPERTIES props;
    int gen;

    static const uint32_t gen6_vs[] = {
        0x07230203, 99, 'v',
        0x01600110, 0x200f1ca4, 0x00600020, 0x00000000, // cmp.z.f0(8)     null            g1<4,4,1>.xD    0D              { align16 1Q };
        0x00670122, 0x000a108f, 0x000e0004, 0x000e0004, // (+f0.all4h) if(8) JIP: 10                                       { align16 1Q };
        0x00600501, 0x204303fd, 0x00000000, 0xbf800000, // mov(8)          g2<1>.xyF       -1F                             { align16 NoDDClr 1Q };
        0x00600d01, 0x204403fd, 0x00000000, 0x00000000, // mov(8)          g2<1>.zF        0F                              { align16 NoDDClr,NoDDChk 1Q };
        0x00600901, 0x204803fd, 0x00000000, 0x3f800000, // mov(8)          g2<1>.wF        1F                              { align16 NoDDChk 1Q };
        0x00600124, 0x0014108f, 0x006e0004, 0x006e0004, // else(8)         JIP: 20                                         { align16 1Q };
        0x01600110, 0x200f1ca4, 0x00600020, 0x00000001, // cmp.z.f0(8)     null            g1<4,4,1>.xD    1D              { align16 1Q };
        0x00670122, 0x000a108f, 0x000e0004, 0x000e0004, // (+f0.all4h) if(8) JIP: 10                                       { align16 1Q };
        0x00600501, 0x204903fd, 0x00000000, 0x3f800000, // mov(8)          g2<1>.xwF       1F                              { align16 NoDDClr 1Q };
        0x00600d01, 0x204203fd, 0x00000000, 0xbf800000, // mov(8)          g2<1>.yF        -1F                             { align16 NoDDClr,NoDDChk 1Q };
        0x00600901, 0x204403fd, 0x00000000, 0x00000000, // mov(8)          g2<1>.zF        0F                              { align16 NoDDChk 1Q };
        0x00600124, 0x0006108f, 0x006e0004, 0x006e0004, // else(8)         JIP: 6                                          { align16 1Q };
        0x00600501, 0x204503fd, 0x00000000, 0x00000000, // mov(8)          g2<1>.xzF       0F                              { align16 NoDDClr 1Q };
        0x00600901, 0x204a03fd, 0x00000000, 0x3f800000, // mov(8)          g2<1>.ywF       1F                              { align16 NoDDChk 1Q };
        0x00600125, 0x0002108f, 0x006e0004, 0x006e0002, // endif(8)        JIP: 2                                          { align16 1Q };
        0x00600125, 0x0002108f, 0x006e0004, 0x006e0002, // endif(8)        JIP: 2                                          { align16 1Q };
        0x00600101, 0x204f0062, 0x00000000, 0x00000000, // mov(8)          m2<1>UD         0x00000000UD                    { align16 1Q };
        0x00600101, 0x206f03be, 0x006e0044, 0x00000000, // mov(8)          m3<1>F          g2<4,4,1>F                      { align16 1Q };
        0x00600301, 0x202f0022, 0x006e0004, 0x00000000, // mov(8)          m1<1>UD         g0<4,4,1>UD                     { align16 WE_all 1Q };
        0x06600131, 0x200f1fdc, 0x006e0024, 0x8608c400, // send(8)         null            m1<4,4,1>F
                                                        // urb 0 urb_write interleave used complete mlen 3 rlen 0 { align16 1Q EOT };
    };

    static const uint32_t gen7_vs[] = {
        0x07230203, 99, 'v',
        0x01608110, 0x200f1ca4, 0x00600020, 0x00000000, // cmp.z.f0(8)     null            g1<4,4,1>.xD    0D              { align16 1Q switch };
        0x00670122, 0x200f0c84, 0x000e0004, 0x001c000a, // (+f0.all4h) if(8) JIP: 10       UIP: 28                         { align16 1Q };
        0x00600501, 0x204303fd, 0x00000000, 0xbf800000, // mov(8)          g2<1>.xyF       -1F                             { align16 NoDDClr 1Q };
        0x00600d01, 0x204403fd, 0x00000000, 0x00000000, // mov(8)          g2<1>.zF        0F                              { align16 NoDDClr,NoDDChk 1Q };
        0x00600901, 0x204803fd, 0x00000000, 0x3f800000, // mov(8)          g2<1>.wF        1F                              { align16 NoDDChk 1Q };
        0x00600124, 0x200f0c84, 0x006e0004, 0x00000014, // else(8)         JIP: 20                                         { align16 1Q };
        0x01608110, 0x200f1ca4, 0x00600020, 0x00000001, // cmp.z.f0(8)     null            g1<4,4,1>.xD    1D              { align16 1Q switch };
        0x00670122, 0x200f0c84, 0x000e0004, 0x000e000a, // (+f0.all4h) if(8) JIP: 10       UIP: 14                         { align16 1Q };
        0x00600501, 0x204903fd, 0x00000000, 0x3f800000, // mov(8)          g2<1>.xwF       1F                              { align16 NoDDClr 1Q };
        0x00600d01, 0x204203fd, 0x00000000, 0xbf800000, // mov(8)          g2<1>.yF        -1F                             { align16 NoDDClr,NoDDChk 1Q };
        0x00600901, 0x204403fd, 0x00000000, 0x00000000, // mov(8)          g2<1>.zF        0F                              { align16 NoDDChk 1Q };
        0x00600124, 0x200f0c84, 0x006e0004, 0x00000006, // else(8)         JIP: 6                                          { align16 1Q };
        0x00600501, 0x204503fd, 0x00000000, 0x00000000, // mov(8)          g2<1>.xzF       0F                              { align16 NoDDClr 1Q };
        0x00600901, 0x204a03fd, 0x00000000, 0x3f800000, // mov(8)          g2<1>.ywF       1F                              { align16 NoDDChk 1Q };
        0x00600125, 0x200f0c84, 0x006e0004, 0x00000002, // endif(8)        JIP: 2                                          { align16 1Q };
        0x00600125, 0x200f0c84, 0x006e0004, 0x00000002, // endif(8)        JIP: 2                                          { align16 1Q };
        0x00600101, 0x2e4f0061, 0x00000000, 0x00000000, // mov(8)          g114<1>UD       0x00000000UD                    { align16 1Q };
        0x00600101, 0x2e6f03bd, 0x006e0044, 0x00000000, // mov(8)          g115<1>F        g2<4,4,1>F                      { align16 1Q };
        0x00600301, 0x2e2f0021, 0x006e0004, 0x00000000, // mov(8)          g113<1>UD       g0<4,4,1>UD                     { align16 WE_all 1Q };
        0x00000206, 0x2e340c21, 0x00000014, 0x0000ff00, // or(1)           g113.5<1>UD     g0.5<0,1,0>UD   0x0000ff00UD    { align1 WE_all };
        0x06600131, 0x200f1fbc, 0x006e0e24, 0x8608c000, // send(8)         null            g113<4,4,1>F
                                                        // urb 0 write HWord interleave complete mlen 3 rlen 0 { align16 1Q EOT };
    };


    err = xglGetGpuInfo(demo->gpu,
                        XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES,
                        &size, &props);
    assert(!err);
    assert(size == sizeof(props));

    gen = (strstr((const char *) props.gpuName, "Sandybridge")) ? 6 : 7;

    if (gen == 6) {
        return demo_prepare_shader(demo, (const void *) gen6_vs, sizeof(gen6_vs));
    }
    return demo_prepare_shader(demo, (const void *) gen7_vs, sizeof(gen7_vs));
}

static XGL_SHADER demo_prepare_fs(struct demo *demo)
{
    XGL_RESULT err;
    XGL_SIZE size;
    XGL_PHYSICAL_GPU_PROPERTIES props;
    int gen;

    static const uint32_t gen6_fs[] = {
        0x07230203, 99, 'w',
        0x00600001, 0x202003fe, 0x00000000, 0x3f800000, // mov(8)          m1<1>F          1F                              { align1 1Q };
        0x00600001, 0x204003fe, 0x00000000, 0x00000000, // mov(8)          m2<1>F          0F                              { align1 1Q };
        0x00600001, 0x206003fe, 0x00000000, 0x00000000, // mov(8)          m3<1>F          0F                              { align1 1Q };
        0x00600001, 0x208003fe, 0x00000000, 0x3f800000, // mov(8)          m4<1>F          1F                              { align1 1Q };
        0x05600032, 0x20001fc8, 0x008d0020, 0x88019400, // sendc(8)        null            m1<8,8,1>F
                                                        // render RT write SIMD8 LastRT Surface = 0 mlen 4 rlen 0 { align1 1Q EOT };
    };

    static const uint32_t gen7_fs[] = {
        0x07230203, 99, 'w',
        0x00600001, 0x2e2003fd, 0x00000000, 0x3f800000, // mov(8)          g113<1>F        1F                              { align1 1Q };
        0x00600001, 0x2e4003fd, 0x00000000, 0x00000000, // mov(8)          g114<1>F        0F                              { align1 1Q };
        0x00600001, 0x2e6003fd, 0x00000000, 0x00000000, // mov(8)          g115<1>F        0F                              { align1 1Q };
        0x00600001, 0x2e8003fd, 0x00000000, 0x3f800000, // mov(8)          g116<1>F        1F                              { align1 1Q };
        0x05600032, 0x20001fa8, 0x008d0e20, 0x88031400, // sendc(8)        null            g113<8,8,1>F
                                                        // render RT write SIMD8 LastRT Surface = 0 mlen 4 rlen 0 { align1 1Q EOT };
    };

    err = xglGetGpuInfo(demo->gpu,
                        XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES,
                        &size, &props);
    assert(!err);
    assert(size == sizeof(props));

    gen = (strstr((const char *) props.gpuName, "Sandybridge")) ? 6 : 7;

    if (gen == 6) {
        return demo_prepare_shader(demo, (const void *) gen6_fs, sizeof(gen6_fs));
    }
    return demo_prepare_shader(demo, (const void *) gen7_fs, sizeof(gen7_fs));
}

static void demo_prepare_pipeline(struct demo *demo)
{
    XGL_GRAPHICS_PIPELINE_CREATE_INFO pipeline;
    XGL_PIPELINE_IA_STATE_CREATE_INFO ia;
    XGL_PIPELINE_RS_STATE_CREATE_INFO rs;
    XGL_PIPELINE_CB_STATE cb;
    XGL_PIPELINE_DB_STATE_CREATE_INFO db;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO vs;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO fs;
    XGL_DESCRIPTOR_SLOT_INFO vs_slots[DEMO_TEXTURE_COUNT * 2 + 1];
    XGL_DESCRIPTOR_SLOT_INFO fs_slots[DEMO_TEXTURE_COUNT * 2 + 1];
    XGL_RESULT err;
    XGL_UINT i;

    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.sType = XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

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

    memset(&vs_slots, 0, sizeof(vs_slots));
    vs_slots[2 * DEMO_TEXTURE_COUNT].slotObjectType = XGL_SLOT_SHADER_RESOURCE;
    vs_slots[2 * DEMO_TEXTURE_COUNT].shaderEntityIndex = 0;

    memset(&fs_slots, 0, sizeof(fs_slots));
    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        fs_slots[2 * i].slotObjectType = XGL_SLOT_SHADER_SAMPLER;
        fs_slots[2 * i].shaderEntityIndex = i;
        fs_slots[2 * i + 1].slotObjectType = XGL_SLOT_SHADER_RESOURCE;
        fs_slots[2 * i + 1].shaderEntityIndex = i;
    }

    memset(&vs, 0, sizeof(vs));
    vs.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs.shader.stage = XGL_SHADER_STAGE_VERTEX;
    vs.shader.shader = demo_prepare_vs(demo);
    vs.shader.descriptorSetMapping[0].descriptorCount =
        DEMO_TEXTURE_COUNT * 2 + 1;
    vs.shader.descriptorSetMapping[0].pDescriptorInfo = vs_slots;

    memset(&fs, 0, sizeof(fs));
    fs.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fs.shader.stage = XGL_SHADER_STAGE_FRAGMENT;
    fs.shader.shader = demo_prepare_fs(demo);
    fs.shader.descriptorSetMapping[0].descriptorCount =
        DEMO_TEXTURE_COUNT * 2 + 1;
    fs.shader.descriptorSetMapping[0].pDescriptorInfo = fs_slots;

    pipeline.pNext = (const XGL_VOID *) &ia;
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
    depth_stencil.depthFunc = XGL_COMPARE_GREATER;
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
    case XCB_KEY_RELEASE:
        {
            const xcb_key_release_event_t *key =
                (const xcb_key_release_event_t *) event;

            if (key->detail == 0x9)
                demo->quit = true;
        }
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
                    XCB_EVENT_MASK_EXPOSURE;

    xcb_create_window(demo->connection,
            XCB_COPY_FROM_PARENT,
            demo->window, demo->screen->root,
            0, 0, demo->width, demo->height, 0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            demo->screen->root_visual,
            value_mask, value_list);

    xcb_map_window(demo->connection, demo->window);
}

static void demo_init_xgl(struct demo *demo)
{
    const XGL_APPLICATION_INFO app = {
        .sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pAppName = (const XGL_CHAR *) "tri",
        .appVersion = 0,
        .pEngineName = (const XGL_CHAR *) "tri",
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
        (const XGL_CHAR *) "XGL_WSI_X11",
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
        xglDestroyObject(demo->textures[i].image);
        xglFreeMemory(demo->textures[i].mem);
        xglDestroyObject(demo->textures[i].sampler);
    }

    xglDestroyObject(demo->depth.view);
    xglDestroyObject(demo->depth.image);
    xglFreeMemory(demo->depth.mem);

    for (i = 0; i < DEMO_BUFFER_COUNT; i++) {
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
