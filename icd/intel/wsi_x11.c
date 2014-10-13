/*
 * XGL
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
#include "dev.h"
#include "fence.h"
#include "gpu.h"
#include "img.h"
#include "mem.h"
#include "queue.h"
#include "wsi_x11.h"

struct intel_wsi_x11 {
    xcb_connection_t *c;
    xcb_window_t root;
    xcb_randr_provider_t provider;
    int root_depth;

    int dri3_major, dri3_minor;
    int present_major, present_minor;

    int fd;

    xcb_present_event_t present_special_event_id;
    xcb_special_event_t *present_special_event;

    struct {
        uint32_t serial;
    } local;

    struct {
        uint32_t serial;
        XGL_UINT64 msc;
    } remote;
};

/**
 * Return true if DRI3 and Present are supported by the server.
 */
static bool wsi_x11_has_dri3_and_present(xcb_connection_t *c)
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
 * Return the depth of the root window.
 */
static int wsi_x11_get_root_depth(struct intel_wsi_x11 *x11)
{
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;

    setup = xcb_get_setup(x11->c);

    iter = xcb_setup_roots_iterator(setup);
    for (; iter.rem; xcb_screen_next(&iter)) {
        if (iter.data->root == x11->root)
            return iter.data->root_depth;
    }

    return 0;
}

/**
 * Query DRI3 and Present versions and return an intel_wsi_x11.
 */
static struct intel_wsi_x11 *wsi_x11_create(xcb_connection_t *c,
                                            xcb_window_t root,
                                            xcb_randr_provider_t provider)
{
    xcb_dri3_query_version_cookie_t dri3_cookie;
    xcb_dri3_query_version_reply_t *dri3_reply;
    xcb_present_query_version_cookie_t present_cookie;
    xcb_present_query_version_reply_t *present_reply;
    struct intel_wsi_x11 *x11;

    dri3_cookie = xcb_dri3_query_version(c,
            XCB_DRI3_MAJOR_VERSION, XCB_DRI3_MINOR_VERSION);
    present_cookie = xcb_present_query_version(c,
            XCB_PRESENT_MAJOR_VERSION, XCB_PRESENT_MINOR_VERSION);

    x11 = icd_alloc(sizeof(*x11), 0, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!x11)
        return NULL;
    memset(x11, 0, sizeof(*x11));

    x11->c = c;
    x11->root = root;
    x11->provider = provider;

    x11->root_depth = wsi_x11_get_root_depth(x11);

    dri3_reply = xcb_dri3_query_version_reply(c, dri3_cookie, NULL);
    if (!dri3_reply) {
        icd_free(x11);
        return NULL;
    }

    x11->dri3_major = dri3_reply->major_version;
    x11->dri3_minor = dri3_reply->minor_version;
    free(dri3_reply);

    present_reply = xcb_present_query_version_reply(c, present_cookie, NULL);
    if (!present_reply) {
        icd_free(x11);
        return NULL;
    }

    x11->present_major = present_reply->major_version;
    x11->present_minor = present_reply->minor_version;
    free(present_reply);

    x11->fd = -1;

    return x11;
}

/**
 * Return true if x11->fd points to the primary or render node of the GPU.
 */
static bool wsi_x11_uses_gpu(const struct intel_wsi_x11 *x11,
                             const struct intel_gpu *gpu)
{
    struct stat x11_stat, gpu_stat;

    if (fstat(x11->fd, &x11_stat))
        return false;

    /* is it the primary node? */
    if (!stat(gpu->primary_node, &gpu_stat) &&
        !memcmp(&x11_stat, &gpu_stat, sizeof(x11_stat)))
        return true;

    /* is it the render node? */
    if (gpu->render_node && !stat(gpu->render_node, &gpu_stat) &&
        !memcmp(&x11_stat, &gpu_stat, sizeof(x11_stat)))
        return true;

    return false;
}

/**
 * Send a DRI3Open to get the server GPU fd.
 */
static XGL_RESULT wsi_x11_dri3_open(struct intel_wsi_x11 *x11)
{
    xcb_dri3_open_cookie_t cookie;
    xcb_dri3_open_reply_t *reply;
    int fd;

    cookie = xcb_dri3_open(x11->c, x11->root, x11->provider);
    reply = xcb_dri3_open_reply(x11->c, cookie, NULL);
    if (!reply)
        return XGL_ERROR_UNKNOWN;

    fd = (reply->nfd == 1) ? xcb_dri3_open_reply_fds(x11->c, reply)[0] : -1;
    free(reply);

    if (fd < 0)
        return XGL_ERROR_UNKNOWN;

    fcntl(fd, F_SETFD, FD_CLOEXEC);
    x11->fd = fd;

    return XGL_SUCCESS;
}

/**
 * Send a DRI3PixmapFromBuffer to create a Pixmap from \p mem for \p img.
 */
static XGL_RESULT wsi_x11_dri3_pixmap_from_buffer(struct intel_wsi_x11 *x11,
                                                  struct intel_dev *dev,
                                                  struct intel_img *img,
                                                  struct intel_mem *mem)
{
    struct intel_winsys_handle export;
    xcb_pixmap_t pixmap;

    /* get prime fd of the bo first */
    export.type = INTEL_WINSYS_HANDLE_FD;
    if (intel_winsys_export_handle(dev->winsys, mem->bo, img->layout.tiling,
                img->layout.bo_stride, img->layout.bo_height, &export))
        return XGL_ERROR_UNKNOWN;

    pixmap = xcb_generate_id(x11->c);

    /* create a pixmap from the prime fd */
    xcb_dri3_pixmap_from_buffer(x11->c, pixmap,
            x11->root, img->total_size,
            img->layout.width0, img->layout.height0,
            img->layout.bo_stride, x11->root_depth,
            img->layout.block_size * 8, export.handle);

    img->x11_prime_fd = export.handle;
    img->x11_pixmap = pixmap;

    return XGL_SUCCESS;
}

/**
 * Send a PresentSelectInput to select interested events.
 */
static XGL_RESULT wsi_x11_present_select_input(struct intel_wsi_x11 *x11)
{
    xcb_void_cookie_t cookie;
    xcb_generic_error_t *error;

    /* create the event queue */
    x11->present_special_event_id = xcb_generate_id(x11->c);
    x11->present_special_event = xcb_register_for_special_xge(x11->c,
            &xcb_present_id, x11->present_special_event_id, NULL);

    cookie = xcb_present_select_input_checked(x11->c,
            x11->present_special_event_id, x11->root,
            XCB_PRESENT_EVENT_MASK_COMPLETE_NOTIFY);

    error = xcb_request_check(x11->c, cookie);
    if (error) {
        free(error);
        return XGL_ERROR_UNKNOWN;
    }

    return XGL_SUCCESS;
}

/**
 * Send a PresentPixmap.
 */
static XGL_RESULT wsi_x11_present_pixmap(struct intel_wsi_x11 *x11,
                                         const XGL_WSI_X11_PRESENT_INFO *info)
{
    struct intel_img *img = intel_img(info->srcImage);
    uint32_t options = XCB_PRESENT_OPTION_NONE;
    xcb_void_cookie_t cookie;
    xcb_generic_error_t *err;

    if (info->async)
        options |= XCB_PRESENT_OPTION_ASYNC;
    if (!info->flip)
        options |= XCB_PRESENT_OPTION_COPY;

    cookie = xcb_present_pixmap(x11->c,
            info->destWindow,
            img->x11_pixmap,
            ++x11->local.serial,
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

    err = xcb_request_check(x11->c, cookie);
    if (err) {
        free(err);
        return XGL_ERROR_UNKNOWN;
    }

    return XGL_SUCCESS;
}

/**
 * Send a PresentNotifyMSC for the current MSC.
 */
static void wsi_x11_present_notify_msc(struct intel_wsi_x11 *x11,
                                       xcb_randr_crtc_t crtc)
{
    /* cannot specify CRTC? */
    xcb_present_notify_msc(x11->c, x11->root, ++x11->local.serial,
            0, 0, 0);

    xcb_flush(x11->c);
}

/**
 * Handle a Present event.
 */
static void wsi_x11_present_event(struct intel_wsi_x11 *x11,
                                  const xcb_present_generic_event_t *ev)
{
    union {
        const xcb_present_generic_event_t *ev;
        const xcb_present_complete_notify_event_t *complete;
    } u = { .ev = ev };

    switch (u.ev->evtype) {
    case XCB_PRESENT_COMPLETE_NOTIFY:
        x11->remote.serial = u.complete->serial;
        x11->remote.msc = u.complete->msc;
        break;
    default:
        break;
    }
}

void intel_wsi_x11_destroy(struct intel_wsi_x11 *x11)
{
    if (x11->present_special_event)
        xcb_unregister_for_special_event(x11->c, x11->present_special_event);

    if (x11->fd >= 0)
        close(x11->fd);

    icd_free(x11);
}

XGL_RESULT intel_wsi_x11_wait(struct intel_wsi_x11 *x11,
                              uint32_t serial, bool wait)
{
    while (x11->remote.serial < serial) {
        xcb_present_generic_event_t *ev;

        if (wait) {
            ev = (xcb_present_generic_event_t *)
                xcb_wait_for_special_event(x11->c,
                        x11->present_special_event);
            if (!ev)
                return XGL_ERROR_UNKNOWN;
        } else {
            ev = (xcb_present_generic_event_t *)
                xcb_poll_for_special_event(x11->c,
                        x11->present_special_event);
            if (!ev)
                return XGL_NOT_READY;
        }

        wsi_x11_present_event(x11, ev);

        free(ev);
    }

    return XGL_SUCCESS;
}

/**
 * Create a presentable image.
 */
static XGL_RESULT wsi_x11_img_create(struct intel_wsi_x11 *x11,
                                     struct intel_dev *dev,
                                     const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO *info,
                                     struct intel_img **img_ret)
{
    XGL_IMAGE_CREATE_INFO img_info;
    XGL_MEMORY_ALLOC_INFO mem_info;
    struct intel_img *img;
    struct intel_mem *mem;
    XGL_RESULT ret;

    /* create image */
    memset(&img_info, 0, sizeof(img_info));
    img_info.sType = XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_info.imageType = XGL_IMAGE_2D;
    img_info.format = info->format;
    img_info.extent.width = info->extent.width;
    img_info.extent.height = info->extent.height;
    img_info.extent.depth = 1;
    img_info.mipLevels = 1;
    img_info.arraySize = 1;
    img_info.samples = 1;
    img_info.tiling = XGL_OPTIMAL_TILING;
    img_info.usage = info->usage;
    img_info.flags = 0;

    ret = intel_img_create(dev, &img_info, true, &img);
    if (ret != XGL_SUCCESS)
        return ret;

    /* allocate memory */
    memset(&mem_info, 0, sizeof(mem_info));
    mem_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    mem_info.allocationSize = img->total_size;
    mem_info.alignment = 4096;
    mem_info.flags = 0;
    mem_info.heapCount = 1;
    mem_info.memPriority = XGL_MEMORY_PRIORITY_HIGH;

    ret = intel_mem_alloc(dev, &mem_info, &mem);
    if (ret != XGL_SUCCESS) {
        intel_img_destroy(img);
        return ret;
    }

    ret = wsi_x11_dri3_pixmap_from_buffer(x11, dev, img, mem);
    if (ret != XGL_SUCCESS) {
        intel_mem_free(mem);
        intel_img_destroy(img);
        return ret;
    }

    intel_obj_bind_mem(&img->obj, mem, 0);

    *img_ret = img;

    return XGL_SUCCESS;
}

XGL_RESULT XGLAPI intelWsiX11AssociateConnection(
    XGL_PHYSICAL_GPU                            gpu_,
    const XGL_WSI_X11_CONNECTION_INFO*          pConnectionInfo)
{
    struct intel_gpu *gpu = intel_gpu(gpu_);
    struct intel_wsi_x11 *x11;
    XGL_RESULT ret;

    if (gpu->x11)
        return XGL_SUCCESS;

    if (gpu->winsys)
        return XGL_ERROR_DEVICE_ALREADY_CREATED;

    if (!wsi_x11_has_dri3_and_present(pConnectionInfo->pConnection))
        return XGL_ERROR_UNKNOWN;

    x11 = wsi_x11_create(pConnectionInfo->pConnection,
            pConnectionInfo->root, pConnectionInfo->provider);
    if (!x11)
        return XGL_ERROR_UNKNOWN;

    ret = wsi_x11_dri3_open(x11);
    if (ret != XGL_SUCCESS) {
        intel_wsi_x11_destroy(x11);
        return ret;
    }

    if (!wsi_x11_uses_gpu(x11, gpu)) {
        intel_wsi_x11_destroy(x11);
        return XGL_ERROR_UNKNOWN;
    }

    ret = wsi_x11_present_select_input(x11);
    if (ret != XGL_SUCCESS) {
        intel_wsi_x11_destroy(x11);
        return ret;
    }

    intel_gpu_associate_x11(gpu, x11, x11->fd);

    return XGL_SUCCESS;
}

XGL_RESULT XGLAPI intelWsiX11GetMSC(
    XGL_DEVICE                                  device,
    xcb_randr_crtc_t                            crtc,
    XGL_UINT64*                                 pMsc)
{
    struct intel_dev *dev = intel_dev(device);
    struct intel_wsi_x11 *x11 = dev->gpu->x11;
    XGL_RESULT ret;

    wsi_x11_present_notify_msc(x11, crtc);

    /* wait for the event */
    ret = intel_wsi_x11_wait(x11, x11->local.serial, -1);
    if (ret != XGL_SUCCESS)
        return ret;

    *pMsc = x11->remote.msc;

    return XGL_SUCCESS;
}

XGL_RESULT XGLAPI intelWsiX11CreatePresentableImage(
    XGL_DEVICE                                  device,
    const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo,
    XGL_IMAGE*                                  pImage,
    XGL_GPU_MEMORY*                             pMem)
{
    struct intel_dev *dev = intel_dev(device);
    struct intel_wsi_x11 *x11 = dev->gpu->x11;
    struct intel_img *img;
    XGL_RESULT ret;

    ret = wsi_x11_img_create(x11, dev, pCreateInfo, &img);
    if (ret == XGL_SUCCESS) {
        *pImage = (XGL_IMAGE) img;
        *pMem = (XGL_GPU_MEMORY) img->obj.mem;
    }

    return ret;
}

XGL_RESULT XGLAPI intelWsiX11QueuePresent(
    XGL_QUEUE                                   queue_,
    const XGL_WSI_X11_PRESENT_INFO*             pPresentInfo,
    XGL_FENCE                                   fence_)
{
    struct intel_queue *queue = intel_queue(queue_);
    struct intel_fence *fence = intel_fence(fence_);
    struct intel_wsi_x11 *x11 = queue->dev->gpu->x11;
    XGL_RESULT ret;

    ret = wsi_x11_present_pixmap(x11, pPresentInfo);
    if (ret != XGL_SUCCESS)
        return ret;

    if (fence)
        intel_fence_set_x11(fence, x11, x11->local.serial);

    return XGL_SUCCESS;
}
