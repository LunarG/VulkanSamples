/*
 * Vulkan
 *
 * Copyright (C) 2014 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *   Chia-I Wu <olv@lunarg.com>
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <xcb/xcb.h>
#include <xcb/dri3.h>
#include <xcb/present.h>

#include "kmd/winsys.h"
#include "kmd/libdrm/xf86drmMode.h"
#include "dev.h"
#include "fence.h"
#include "gpu.h"
#include "img.h"
#include "mem.h"
#include "queue.h"
#include "wsi.h"

struct intel_x11_display {
    struct intel_handle handle;

    int fd;
    uint32_t connector_id;

    char name[32];
    VK_EXTENT2D physical_dimension;
    VK_EXTENT2D physical_resolution;

    drmModeModeInfoPtr modes;
    uint32_t mode_count;
};

struct intel_x11_swap_chain {
    struct intel_handle handle;

    xcb_connection_t *c;
    xcb_window_t window;

    xcb_present_event_t present_special_event_id;
    xcb_special_event_t *present_special_event;

    struct {
        uint32_t serial;
    } local;

    struct {
        uint32_t serial;
        uint64_t msc;
    } remote;

    struct intel_x11_swap_chain *next;
};

struct intel_wsi_x11 {
    struct intel_handle handle;

    xcb_connection_t *c;
    xcb_window_t root;
    int root_depth;

    int dri3_major, dri3_minor;
    int present_major, present_minor;

    struct intel_x11_swap_chain *swap_chains;
};

struct intel_x11_img_data {
    struct intel_mem *mem;
    int prime_fd;
    uint32_t pixmap;
};

struct intel_x11_fence_data {
    struct intel_x11_swap_chain *swap_chain;
    uint32_t serial;
};

static bool x11_is_format_presentable(const struct intel_dev *dev,
                                      VK_FORMAT format)
{
    /* this is what DDX expects */
    switch (format) {
    case VK_FMT_B5G6R5_UNORM:
    case VK_FMT_B8G8R8A8_UNORM:
    case VK_FMT_B8G8R8A8_SRGB:
        return true;
    default:
        return false;
    }
}

static int x11_export_prime_fd(struct intel_dev *dev,
                               struct intel_bo *bo,
                               const struct intel_layout *layout)
{
    struct intel_winsys_handle export;
    enum intel_tiling_mode tiling;

    export.type = INTEL_WINSYS_HANDLE_FD;

    switch (layout->tiling) {
    case GEN6_TILING_X:
        tiling = INTEL_TILING_X;
        break;
    case GEN6_TILING_Y:
        tiling = INTEL_TILING_Y;
        break;
    default:
        assert(layout->tiling == GEN6_TILING_NONE);
        tiling = INTEL_TILING_NONE;
        break;
    }

    if (intel_bo_set_tiling(bo, tiling, layout->bo_stride))
        return -1;

    if (intel_winsys_export_handle(dev->winsys, bo, tiling,
                layout->bo_stride, layout->bo_height, &export))
        return -1;

    return (int) export.handle;
}

/**
 * Return true if fd points to the primary or render node of the GPU.
 */
static bool x11_gpu_match_fd(const struct intel_gpu *gpu, int fd)
{
    struct stat fd_stat, gpu_stat;

    if (fstat(fd, &fd_stat))
        return false;

    /* is it the primary node? */
    if (!stat(gpu->primary_node, &gpu_stat) &&
        !memcmp(&fd_stat, &gpu_stat, sizeof(fd_stat)))
        return true;

    /* is it the render node? */
    if (gpu->render_node && !stat(gpu->render_node, &gpu_stat) &&
        !memcmp(&fd_stat, &gpu_stat, sizeof(fd_stat)))
        return true;

    return false;
}

/*
 * Return the depth of \p drawable.
 */
static int x11_get_drawable_depth(xcb_connection_t *c,
                                  xcb_drawable_t drawable)
{
    xcb_get_geometry_cookie_t cookie;
    xcb_get_geometry_reply_t *reply;
    uint8_t depth;

    cookie = xcb_get_geometry(c, drawable);
    reply = xcb_get_geometry_reply(c, cookie, NULL);

    if (reply) {
        depth = reply->depth;
        free(reply);
    } else {
        depth = 0;
    }

    return depth;
}

/**
 * Return true if DRI3 and Present are supported by the server.
 */
static bool x11_is_dri3_and_present_supported(xcb_connection_t *c)
{
    const xcb_query_extension_reply_t *ext;

    xcb_prefetch_extension_data(c, &xcb_dri3_id);
    xcb_prefetch_extension_data(c, &xcb_present_id);

    ext = xcb_get_extension_data(c, &xcb_dri3_id);
    if (!ext || !ext->present)
        return false;

    ext = xcb_get_extension_data(c, &xcb_present_id);
    if (!ext || !ext->present)
        return false;

    return true;
}

/**
 * Send a DRI3Open to get the server GPU fd.
 */
static int x11_dri3_open(xcb_connection_t *c,
                         xcb_drawable_t drawable,
                         xcb_randr_provider_t provider)
{
    xcb_dri3_open_cookie_t cookie;
    xcb_dri3_open_reply_t *reply;
    int fd;

    cookie = xcb_dri3_open(c, drawable, provider);
    reply = xcb_dri3_open_reply(c, cookie, NULL);
    if (!reply)
        return -1;

    fd = (reply->nfd == 1) ? xcb_dri3_open_reply_fds(c, reply)[0] : -1;
    free(reply);

    return fd;
}

/**
 * Send a DRI3PixmapFromBuffer to create a pixmap from \p prime_fd.
 */
static xcb_pixmap_t x11_dri3_pixmap_from_buffer(xcb_connection_t *c,
                                                xcb_drawable_t drawable,
                                                uint8_t depth, int prime_fd,
                                                const struct intel_layout *layout)
{
    xcb_pixmap_t pixmap;

    pixmap = xcb_generate_id(c);

    xcb_dri3_pixmap_from_buffer(c, pixmap, drawable,
            layout->bo_stride * layout->bo_height,
            layout->width0, layout->height0,
            layout->bo_stride, depth,
            layout->block_size * 8, prime_fd);

    return pixmap;
}

/**
 * Send DRI3QueryVersion and PresentQueryVersion to query extension versions.
 */
static bool wsi_x11_dri3_and_present_query_version(struct intel_wsi_x11 *x11)
{
    xcb_dri3_query_version_cookie_t dri3_cookie;
    xcb_dri3_query_version_reply_t *dri3_reply;
    xcb_present_query_version_cookie_t present_cookie;
    xcb_present_query_version_reply_t *present_reply;

    dri3_cookie = xcb_dri3_query_version(x11->c,
            XCB_DRI3_MAJOR_VERSION, XCB_DRI3_MINOR_VERSION);
    present_cookie = xcb_present_query_version(x11->c,
            XCB_PRESENT_MAJOR_VERSION, XCB_PRESENT_MINOR_VERSION);

    dri3_reply = xcb_dri3_query_version_reply(x11->c, dri3_cookie, NULL);
    if (!dri3_reply)
        return false;

    x11->dri3_major = dri3_reply->major_version;
    x11->dri3_minor = dri3_reply->minor_version;
    free(dri3_reply);

    present_reply = xcb_present_query_version_reply(x11->c, present_cookie, NULL);
    if (!present_reply)
        return false;

    x11->present_major = present_reply->major_version;
    x11->present_minor = present_reply->minor_version;
    free(present_reply);

    return true;
}

/**
 * Send a PresentSelectInput to select interested events.
 */
static VK_RESULT x11_swap_chain_present_select_input(struct intel_x11_swap_chain *sc)
{
    xcb_void_cookie_t cookie;
    xcb_generic_error_t *error;

    /* create the event queue */
    sc->present_special_event_id = xcb_generate_id(sc->c);
    sc->present_special_event = xcb_register_for_special_xge(sc->c,
            &xcb_present_id, sc->present_special_event_id, NULL);

    cookie = xcb_present_select_input_checked(sc->c,
            sc->present_special_event_id, sc->window,
            XCB_PRESENT_EVENT_MASK_COMPLETE_NOTIFY);

    error = xcb_request_check(sc->c, cookie);
    if (error) {
        free(error);
        return VK_ERROR_UNKNOWN;
    }

    return VK_SUCCESS;
}

static VK_RESULT wsi_x11_dri3_pixmap_from_buffer(struct intel_wsi_x11 *x11,
                                                 struct intel_dev *dev,
                                                 struct intel_img *img,
                                                 struct intel_mem *mem)
{
    struct intel_x11_img_data *data =
        (struct intel_x11_img_data *) img->wsi_data;

    data->prime_fd = x11_export_prime_fd(dev, mem->bo, &img->layout);
    if (data->prime_fd < 0)
        return VK_ERROR_UNKNOWN;

    data->pixmap = x11_dri3_pixmap_from_buffer(x11->c, x11->root,
            x11->root_depth, data->prime_fd, &img->layout);

    data->mem = mem;

    return VK_SUCCESS;
}

/**
 * Create a presentable image.
 */
static VK_RESULT wsi_x11_img_create(struct intel_wsi_x11 *x11,
                                    struct intel_dev *dev,
                                    const VK_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO *info,
                                    struct intel_img **img_ret)
{
    VK_IMAGE_CREATE_INFO img_info;
    VkMemoryAllocInfo mem_info;
    struct intel_img *img;
    struct intel_mem *mem;
    VK_RESULT ret;

    if (!x11_is_format_presentable(dev, info->format)) {
        intel_dev_log(dev, VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0,
                VK_NULL_HANDLE, 0, 0, "invalid presentable image format");
        return VK_ERROR_INVALID_VALUE;
    }

    /* create image */
    memset(&img_info, 0, sizeof(img_info));
    img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_info.imageType = VK_IMAGE_2D;
    img_info.format = info->format;
    img_info.extent.width = info->extent.width;
    img_info.extent.height = info->extent.height;
    img_info.extent.depth = 1;
    img_info.mipLevels = 1;
    img_info.arraySize = 1;
    img_info.samples = 1;
    img_info.tiling = VK_OPTIMAL_TILING;
    img_info.usage = info->usage;
    img_info.flags = 0;

    ret = intel_img_create(dev, &img_info, true, &img);
    if (ret != VK_SUCCESS)
        return ret;

    /* allocate memory */
    memset(&mem_info, 0, sizeof(mem_info));
    mem_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    mem_info.allocationSize = img->total_size;
    mem_info.memProps =  0;
    mem_info.memType = VK_MEMORY_TYPE_IMAGE;
    mem_info.memPriority = VK_MEMORY_PRIORITY_HIGH;

    ret = intel_mem_alloc(dev, &mem_info, &mem);
    if (ret != VK_SUCCESS) {
        intel_img_destroy(img);
        return ret;
    }

    ret = wsi_x11_dri3_pixmap_from_buffer(x11, dev, img, mem);
    if (ret != VK_SUCCESS) {
        intel_mem_free(mem);
        intel_img_destroy(img);
        return ret;
    }

    intel_obj_bind_mem(&img->obj, mem, 0);

    *img_ret = img;

    return VK_SUCCESS;
}

/**
 * Send a PresentPixmap.
 */
static VK_RESULT x11_swap_chain_present_pixmap(struct intel_x11_swap_chain *sc,
                                               const VK_WSI_X11_PRESENT_INFO *info)
{
    struct intel_img *img = intel_img(info->srcImage);
    struct intel_x11_img_data *data =
        (struct intel_x11_img_data *) img->wsi_data;
    uint32_t options = XCB_PRESENT_OPTION_NONE;
    xcb_void_cookie_t cookie;
    xcb_generic_error_t *err;

    if (info->async)
        options |= XCB_PRESENT_OPTION_ASYNC;
    if (!info->flip)
        options |= XCB_PRESENT_OPTION_COPY;

    cookie = xcb_present_pixmap(sc->c,
            sc->window,
            data->pixmap,
            ++sc->local.serial,
            0, /* valid-area */
            0, /* update-area */
            0, /* x-off */
            0, /* y-off */
            info->crtc,
            0, /* wait-fence */
            0, /* idle-fence */
            options,
            info->target_msc,
            info->divisor,
            info->remainder,
            0, NULL);

    err = xcb_request_check(sc->c, cookie);
    if (err) {
        free(err);
        return VK_ERROR_UNKNOWN;
    }

    return VK_SUCCESS;
}

/**
 * Send a PresentNotifyMSC for the current MSC.
 */
static void x11_swap_chain_present_notify_msc(struct intel_x11_swap_chain *sc)
{
    /* cannot specify CRTC? */
    xcb_present_notify_msc(sc->c, sc->window, ++sc->local.serial, 0, 0, 0);

    xcb_flush(sc->c);
}

/**
 * Handle a Present event.
 */
static void x11_swap_chain_present_event(struct intel_x11_swap_chain *sc,
                                         const xcb_present_generic_event_t *ev)
{
    union {
        const xcb_present_generic_event_t *ev;
        const xcb_present_complete_notify_event_t *complete;
    } u = { .ev = ev };

    switch (u.ev->evtype) {
    case XCB_PRESENT_COMPLETE_NOTIFY:
        sc->remote.serial = u.complete->serial;
        sc->remote.msc = u.complete->msc;
        break;
    default:
        break;
    }
}

static VK_RESULT x11_swap_chain_wait(struct intel_x11_swap_chain *sc,
                                      uint32_t serial, int64_t timeout)
{
    const bool wait = (timeout != 0);

    while (sc->remote.serial < serial) {
        xcb_present_generic_event_t *ev;

        if (wait) {
            ev = (xcb_present_generic_event_t *)
                xcb_wait_for_special_event(sc->c, sc->present_special_event);
            if (!ev)
                return VK_ERROR_UNKNOWN;
        } else {
            ev = (xcb_present_generic_event_t *)
                xcb_poll_for_special_event(sc->c, sc->present_special_event);
            if (!ev)
                return VK_NOT_READY;
        }

        x11_swap_chain_present_event(sc, ev);

        free(ev);
    }

    return VK_SUCCESS;
}

static void x11_swap_chain_destroy(struct intel_x11_swap_chain *sc)
{
    if (sc->present_special_event)
        xcb_unregister_for_special_event(sc->c, sc->present_special_event);

    intel_free(sc, sc);
}

static void wsi_x11_destroy(struct intel_wsi_x11 *x11)
{
    struct intel_x11_swap_chain *sc = x11->swap_chains;

    while (sc) {
        struct intel_x11_swap_chain *next = sc->next;
        x11_swap_chain_destroy(sc);
        sc = next;
    }

    intel_free(x11, x11);
}

static struct intel_wsi_x11 *wsi_x11_create(struct intel_gpu *gpu,
                                            const VK_WSI_X11_CONNECTION_INFO *info)
{
    struct intel_wsi_x11 *x11;
    int depth, fd;

    if (!x11_is_dri3_and_present_supported(info->pConnection))
        return NULL;

    depth = x11_get_drawable_depth(info->pConnection, info->root);
    if (!depth)
        return NULL;

    fd = x11_dri3_open(info->pConnection, info->root, info->provider);
    if (fd < 0 || !x11_gpu_match_fd(gpu, fd)) {
        if (fd >= 0)
            close(fd);
        return NULL;
    }

    x11 = intel_alloc(gpu, sizeof(*x11), 0, VK_SYSTEM_ALLOC_API_OBJECT);
    if (!x11)
        return NULL;

    memset(x11, 0, sizeof(*x11));
    /* there is no VK_DBG_OBJECT_WSI_DISPLAY */
    intel_handle_init(&x11->handle, VK_DBG_OBJECT_UNKNOWN, gpu->handle.icd);

    x11->c = info->pConnection;
    x11->root = info->root;

    x11->root_depth = depth;

    if (!wsi_x11_dri3_and_present_query_version(x11)) {
        wsi_x11_destroy(x11);
        return NULL;
    }

    return x11;
}

static struct intel_x11_swap_chain *x11_swap_chain_create(struct intel_dev *dev,
                                                          xcb_window_t window)
{
    struct intel_wsi_x11 *x11 = (struct intel_wsi_x11 *) dev->gpu->wsi_data;
    struct intel_x11_swap_chain *sc;

    sc = intel_alloc(dev, sizeof(*sc), 0, VK_SYSTEM_ALLOC_API_OBJECT);
    if (!sc)
        return NULL;

    memset(sc, 0, sizeof(*sc));
    /* there is no VK_DBG_OBJECT_WSI_SWAP_CHAIN */
    intel_handle_init(&sc->handle, VK_DBG_OBJECT_UNKNOWN,
            dev->base.handle.icd);

    sc->c = x11->c;
    sc->window = window;

    if (x11_swap_chain_present_select_input(sc) != VK_SUCCESS) {
        intel_free(dev, sc);
        return NULL;
    }

    return sc;
}

static struct intel_x11_swap_chain *x11_swap_chain_lookup(struct intel_dev *dev,
                                                          xcb_window_t window)
{
    struct intel_wsi_x11 *x11 = (struct intel_wsi_x11 *) dev->gpu->wsi_data;
    struct intel_x11_swap_chain *sc = x11->swap_chains;

    while (sc) {
        if (sc->window == window)
            break;
        sc = sc->next;
    }

    /* lookup failed */
    if (!sc) {
        sc = x11_swap_chain_create(dev, window);
        if (sc) {
            sc->next = x11->swap_chains;
            x11->swap_chains = sc;
        }
    }

    return sc;
}

static VK_RESULT intel_wsi_gpu_init(struct intel_gpu *gpu,
                                     const VK_WSI_X11_CONNECTION_INFO *info)
{
    struct intel_wsi_x11 *x11;

    x11 = wsi_x11_create(gpu, info);
    if (!x11)
        return VK_ERROR_UNKNOWN;

    gpu->wsi_data = x11;

    return VK_SUCCESS;
}

static void x11_display_init_modes(struct intel_x11_display *dpy,
                                   const drmModeConnectorPtr conn)
{
    int i;

    if (!conn->count_modes)
        return;

    dpy->modes = intel_alloc(dpy, sizeof(dpy->modes[0]) * conn->count_modes,
            0, VK_SYSTEM_ALLOC_INTERNAL);
    if (!dpy->modes)
        return;

    for (i = 0; i < conn->count_modes; i++) {
        dpy->modes[i] = conn->modes[i];

        if (dpy->physical_resolution.width  < conn->modes[i].hdisplay &&
            dpy->physical_resolution.height < conn->modes[i].vdisplay) {
            dpy->physical_resolution.width  = conn->modes[i].hdisplay;
            dpy->physical_resolution.height = conn->modes[i].vdisplay;
        }
    }

    dpy->mode_count = conn->count_modes;

    dpy->physical_dimension.width = conn->mmWidth;
    dpy->physical_dimension.height = conn->mmHeight;
}

static void x11_display_init_name(struct intel_x11_display *dpy,
                                  const drmModeConnectorPtr conn)
{
    static const char *connector_names[] = {
        [DRM_MODE_CONNECTOR_Unknown]        = "Unknown",
        [DRM_MODE_CONNECTOR_VGA]            = "VGA",
        [DRM_MODE_CONNECTOR_DVII]           = "DVII",
        [DRM_MODE_CONNECTOR_DVID]           = "DVID",
        [DRM_MODE_CONNECTOR_DVIA]           = "DVIA",
        [DRM_MODE_CONNECTOR_Composite]      = "Composite",
        [DRM_MODE_CONNECTOR_SVIDEO]         = "SVIDEO",
        [DRM_MODE_CONNECTOR_LVDS]           = "LVDS",
        [DRM_MODE_CONNECTOR_Component]      = "COMPONENT",
        [DRM_MODE_CONNECTOR_9PinDIN]        = "9PinDIN",
        [DRM_MODE_CONNECTOR_DisplayPort]    = "DisplayPort",
        [DRM_MODE_CONNECTOR_HDMIA]          = "HDMIA",
        [DRM_MODE_CONNECTOR_HDMIB]          = "HDMIB",
        [DRM_MODE_CONNECTOR_TV]             = "TV",
        [DRM_MODE_CONNECTOR_eDP]            = "eDP",
        [DRM_MODE_CONNECTOR_VIRTUAL]        = "VIRTUAL",
        [DRM_MODE_CONNECTOR_DSI]            = "DSI",
    };
    const char *name;

    name = (conn->connector_type < ARRAY_SIZE(connector_names)) ?
        connector_names[conn->connector_type] : NULL;
    if (!name)
        name = connector_names[DRM_MODE_CONNECTOR_Unknown];

    snprintf(dpy->name, sizeof(dpy->name),
            "%s%d", name, conn->connector_type_id);
}

static void x11_display_destroy(struct intel_x11_display *dpy)
{
    intel_free(dpy, dpy->modes);
    intel_free(dpy, dpy);
}

static struct intel_x11_display *x11_display_create(struct intel_gpu *gpu,
                                                    int fd,
                                                    uint32_t connector_id)
{
    struct intel_x11_display *dpy;
    drmModeConnectorPtr conn;

    dpy = intel_alloc(gpu, sizeof(*dpy), 0, VK_SYSTEM_ALLOC_API_OBJECT);
    if (!dpy)
        return NULL;

    memset(dpy, 0, sizeof(*dpy));
    /* there is no VK_DBG_OBJECT_WSI_DISPLAY */
    intel_handle_init(&dpy->handle, VK_DBG_OBJECT_UNKNOWN, gpu->handle.icd);

    dpy->fd = fd;
    dpy->connector_id = connector_id;

    conn = drmModeGetConnector(fd, connector_id);
    if (!conn) {
        x11_display_destroy(dpy);
        return NULL;
    }

    x11_display_init_name(dpy, conn);
    x11_display_init_modes(dpy, conn);

    drmModeFreeConnector(conn);

    return dpy;
}

static void x11_display_scan(struct intel_gpu *gpu)
{
    struct intel_x11_display **displays;
    drmModeResPtr res;
    int fd, i;

    fd = intel_gpu_get_primary_fd(gpu);
    if (fd < 0)
        return;

    res = drmModeGetResources(fd);
    if (!res)
        return;

    displays = intel_alloc(gpu, sizeof(*displays) * res->count_connectors,
            0, VK_SYSTEM_ALLOC_INTERNAL);
    if (!displays) {
        drmModeFreeResources(res);
        return;
    }

    for (i = 0; i < res->count_connectors; i++) {
        displays[i] = x11_display_create(gpu, fd, res->connectors[i]);
        if (!displays[i])
            break;
    }

    drmModeFreeResources(res);

    gpu->displays = (struct intel_wsi_display **) displays;
    gpu->display_count = i;
}

VK_RESULT intel_wsi_gpu_get_info(struct intel_gpu *gpu,
                                  VK_PHYSICAL_GPU_INFO_TYPE type,
                                  size_t *size, void *data)
{
    if (false)
        x11_display_scan(gpu);

    return VK_ERROR_INVALID_VALUE;
}

void intel_wsi_gpu_cleanup(struct intel_gpu *gpu)
{
    if (gpu->displays) {
        uint32_t i;

        for (i = 0; i < gpu->display_count; i++) {
            struct intel_x11_display *dpy =
                (struct intel_x11_display *) gpu->displays[i];
            x11_display_destroy(dpy);
        }
        intel_free(gpu, gpu->displays);
    }

    if (gpu->wsi_data) {
        struct intel_wsi_x11 *x11 = (struct intel_wsi_x11 *) gpu->wsi_data;
        wsi_x11_destroy(x11);
    }
}

VK_RESULT intel_wsi_img_init(struct intel_img *img)
{
    struct intel_x11_img_data *data;

    data = intel_alloc(img, sizeof(*data), 0, VK_SYSTEM_ALLOC_INTERNAL);
    if (!data)
        return VK_ERROR_OUT_OF_MEMORY;

    memset(data, 0, sizeof(*data));

    assert(!img->wsi_data);
    img->wsi_data = data;

    return VK_SUCCESS;
}

void intel_wsi_img_cleanup(struct intel_img *img)
{
    struct intel_x11_img_data *data =
        (struct intel_x11_img_data *) img->wsi_data;

    if (data->mem) {
        close(data->prime_fd);
        intel_mem_free(data->mem);
    }

    intel_free(img, img->wsi_data);
}

VK_RESULT intel_wsi_fence_init(struct intel_fence *fence)
{
    struct intel_x11_fence_data *data;

    data = intel_alloc(fence, sizeof(*data), 0, VK_SYSTEM_ALLOC_INTERNAL);
    if (!data)
        return VK_ERROR_OUT_OF_MEMORY;

    memset(data, 0, sizeof(*data));

    assert(!fence->wsi_data);
    fence->wsi_data = data;

    return VK_SUCCESS;
}

void intel_wsi_fence_cleanup(struct intel_fence *fence)
{
    intel_free(fence, fence->wsi_data);
}

void intel_wsi_fence_copy(struct intel_fence *fence,
                          const struct intel_fence *src)
{
    memcpy(fence->wsi_data, src->wsi_data,
            sizeof(struct intel_x11_fence_data));
}

VK_RESULT intel_wsi_fence_wait(struct intel_fence *fence,
                                int64_t timeout_ns)
{
    struct intel_x11_fence_data *data =
        (struct intel_x11_fence_data *) fence->wsi_data;

    if (!data->swap_chain)
        return VK_SUCCESS;

    return x11_swap_chain_wait(data->swap_chain, data->serial, timeout_ns);
}

ICD_EXPORT VK_RESULT VKAPI vkWsiX11AssociateConnection(
    VK_PHYSICAL_GPU                            gpu_,
    const VK_WSI_X11_CONNECTION_INFO*          pConnectionInfo)
{
    struct intel_gpu *gpu = intel_gpu(gpu_);

    return intel_wsi_gpu_init(gpu, pConnectionInfo);
}

ICD_EXPORT VK_RESULT VKAPI vkWsiX11GetMSC(
    VK_DEVICE                                   device,
    xcb_window_t                                window,
    xcb_randr_crtc_t                            crtc,
    uint64_t  *                                 pMsc)
{
    struct intel_dev *dev = intel_dev(device);
    struct intel_x11_swap_chain *sc;
    VK_RESULT ret;

    sc = x11_swap_chain_lookup(dev, window);
    if (!sc)
        return VK_ERROR_UNKNOWN;

    x11_swap_chain_present_notify_msc(sc);

    /* wait for the event */
    ret = x11_swap_chain_wait(sc, sc->local.serial, -1);
    if (ret != VK_SUCCESS)
        return ret;

    *pMsc = sc->remote.msc;

    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkWsiX11CreatePresentableImage(
    VK_DEVICE                                  device,
    const VK_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo,
    VK_IMAGE*                                  pImage,
    VK_GPU_MEMORY*                             pMem)
{
    struct intel_dev *dev = intel_dev(device);
    struct intel_wsi_x11 *x11 = (struct intel_wsi_x11 *) dev->gpu->wsi_data;
    struct intel_img *img;
    VK_RESULT ret;

    ret = wsi_x11_img_create(x11, dev, pCreateInfo, &img);
    if (ret == VK_SUCCESS) {
        *pImage = (VK_IMAGE) img;
        *pMem = (VK_GPU_MEMORY) img->obj.mem;
    }

    return ret;
}

ICD_EXPORT VK_RESULT VKAPI vkWsiX11QueuePresent(
    VK_QUEUE                                   queue_,
    const VK_WSI_X11_PRESENT_INFO*             pPresentInfo,
    VK_FENCE                                   fence_)
{
    struct intel_queue *queue = intel_queue(queue_);
    struct intel_dev *dev = queue->dev;
    struct intel_x11_fence_data *data =
        (struct intel_x11_fence_data *) queue->fence->wsi_data;
    struct intel_img *img = intel_img(pPresentInfo->srcImage);
    struct intel_x11_swap_chain *sc;
    VK_RESULT ret;

    sc = x11_swap_chain_lookup(dev, pPresentInfo->destWindow);
    if (!sc)
        return VK_ERROR_UNKNOWN;

    ret = x11_swap_chain_present_pixmap(sc, pPresentInfo);
    if (ret != VK_SUCCESS)
        return ret;

    data->swap_chain = sc;
    data->serial = sc->local.serial;
    intel_fence_set_seqno(queue->fence, img->obj.mem->bo);

    if (fence_ != VK_NULL_HANDLE) {
        struct intel_fence *fence = intel_fence(fence_);
        intel_fence_copy(fence, queue->fence);
    }

    return VK_SUCCESS;
}
