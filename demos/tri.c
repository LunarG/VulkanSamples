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
    XGL_RESULT err;

    err = xglBeginCommandBuffer(demo->cmd,
            XGL_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT |
            XGL_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT);
    assert(!err);

    xglCmdBindPipeline(demo->cmd, XGL_PIPELINE_BIND_POINT_GRAPHICS,
                                  demo->pipeline);

    xglCmdBindStateObject(demo->cmd, XGL_STATE_BIND_VIEWPORT, demo->viewport);
    xglCmdBindStateObject(demo->cmd, XGL_STATE_BIND_RASTER, demo->raster);
    xglCmdBindStateObject(demo->cmd, XGL_STATE_BIND_MSAA, demo->msaa);
    xglCmdBindStateObject(demo->cmd, XGL_STATE_BIND_COLOR_BLEND,
                                     demo->color_blend);
    xglCmdBindStateObject(demo->cmd, XGL_STATE_BIND_DEPTH_STENCIL,
                                     demo->depth_stencil);

    xglCmdBindAttachments(demo->cmd, 1, &color_attachment, NULL);

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

    return demo_prepare_shader(demo, (const void *) gen6_vs, sizeof(gen6_vs));
}

static XGL_SHADER demo_prepare_fs(struct demo *demo)
{
    static const uint32_t gen6_fs[] = {
        0x07230203, 99, 'w',
        0x00600001, 0x202003fe, 0x00000000, 0x3f800000, // mov(8)          m1<1>F          1F                              { align1 1Q };
        0x00600001, 0x204003fe, 0x00000000, 0x00000000, // mov(8)          m2<1>F          0F                              { align1 1Q };
        0x00600001, 0x206003fe, 0x00000000, 0x00000000, // mov(8)          m3<1>F          0F                              { align1 1Q };
        0x00600001, 0x208003fe, 0x00000000, 0x3f800000, // mov(8)          m4<1>F          1F                              { align1 1Q };
        0x05600032, 0x20001fc8, 0x008d0020, 0x88019400, // sendc(8)        null            m1<8,8,1>F
                                                        // render RT write SIMD8 LastRT Surface = 0 mlen 4 rlen 0 { align1 1Q EOT };
    };

    return demo_prepare_shader(demo, (const void *) gen6_fs, sizeof(gen6_fs));
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
    XGL_RESULT err;

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

    memset(&vs, 0, sizeof(vs));
    vs.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs.shader.stage = XGL_SHADER_STAGE_VERTEX;
    vs.shader.shader = demo_prepare_vs(demo);

    memset(&fs, 0, sizeof(fs));
    fs.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fs.shader.stage = XGL_SHADER_STAGE_FRAGMENT;
    fs.shader.shader = demo_prepare_fs(demo);

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
