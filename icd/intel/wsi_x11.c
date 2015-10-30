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
 *   Ian Elliott <ian@lunarg.com>
 */

#define _GNU_SOURCE 1
#include <time.h>
#include <poll.h>
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
    VkExtent2D physical_dimension;
    VkExtent2D physical_resolution;

    drmModeModeInfoPtr modes;
    uint32_t mode_count;
};

typedef enum intel_x11_swap_chain_image_state_
{
    INTEL_SC_STATE_UNUSED = 0,
    INTEL_SC_STATE_APP_OWNED = 1,
    INTEL_SC_STATE_QUEUED_FOR_PRESENT = 2,
    INTEL_SC_STATE_DISPLAYED = 3,
} intel_x11_swap_chain_image_state;

struct intel_x11_swap_chain {
    struct intel_handle handle;

    xcb_connection_t *c;
    xcb_window_t window;
    uint32_t width;  // To compare with XCB_PRESENT_EVENT_CONFIGURE_NOTIFY's
    uint32_t height; // ditto
    bool out_of_date;
    bool being_deleted;
    bool force_copy;
    VkPresentModeKHR present_mode;

    int dri3_major, dri3_minor;
    int present_major, present_minor;

    xcb_special_event_t *present_special_event;

    struct intel_img **persistent_images;
    uint32_t persistent_image_count;
    intel_x11_swap_chain_image_state *image_state;
    uint32_t *present_queue;
    uint32_t present_queue_length;
    uint32_t present_serial;

    struct {
        uint32_t serial;
    } local;

    struct {
        uint32_t serial;
        uint64_t msc;
    } remote;
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

/* these are what DDX expects */
static const VkFormat x11_presentable_formats[] = {
    VK_FORMAT_B8G8R8A8_UNORM,
    VK_FORMAT_B8G8R8A8_SRGB,
    VK_FORMAT_B5G6R5_UNORM,
};

static inline struct intel_x11_swap_chain *x11_swap_chain(VkSwapchainKHR sc)
{
    return (struct intel_x11_swap_chain *) sc;
}

static bool x11_is_format_presentable(const struct intel_dev *dev,
                                      VkFormat format)
{
    uint32_t i;

    for (i = 0; i < ARRAY_SIZE(x11_presentable_formats); i++) {
        if (x11_presentable_formats[i] == format)
            return true;
    }

    return false;
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

static VkResult x11_get_surface_properties(
    const VkSurfaceDescriptionKHR *pSurfaceDescription,
    VkSurfacePropertiesKHR *pSurfaceProperties)
{
    const VkSurfaceDescriptionWindowKHR* pSurfaceDescriptionWindow =
        (VkSurfaceDescriptionWindowKHR*) pSurfaceDescription;
    VkPlatformHandleXcbKHR *pPlatformHandleXcb = (VkPlatformHandleXcbKHR *)
        pSurfaceDescriptionWindow->pPlatformHandle;
    xcb_connection_t *c = (xcb_connection_t *)
        pPlatformHandleXcb->connection;
    xcb_window_t window = *((xcb_window_t *)
                            pSurfaceDescriptionWindow->pPlatformWindow);
    xcb_get_geometry_cookie_t cookie;
    xcb_get_geometry_reply_t *reply;

    cookie = xcb_get_geometry(c, window);
    reply = xcb_get_geometry_reply(c, cookie, NULL);

    if (reply) {
        pSurfaceProperties->currentExtent.width = reply->width;
        pSurfaceProperties->currentExtent.height = reply->height;
        free(reply);
    } else {
        pSurfaceProperties->currentExtent.width = 0;
        pSurfaceProperties->currentExtent.height = 0;
    }

    pSurfaceProperties->minImageCount = 2;
    pSurfaceProperties->maxImageCount = 0;

    pSurfaceProperties->minImageExtent.width =
        pSurfaceProperties->currentExtent.width;
    pSurfaceProperties->minImageExtent.height =
        pSurfaceProperties->currentExtent.height;
    pSurfaceProperties->maxImageExtent.width =
        pSurfaceProperties->currentExtent.width;
    pSurfaceProperties->maxImageExtent.height =
        pSurfaceProperties->currentExtent.height;
    pSurfaceProperties->supportedTransforms = VK_SURFACE_TRANSFORM_NONE_BIT_KHR;
    pSurfaceProperties->currentTransform = VK_SURFACE_TRANSFORM_NONE_KHR;
    pSurfaceProperties->maxImageArraySize = 0;
    pSurfaceProperties->supportedUsageFlags =
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    return VK_SUCCESS;
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
static bool x11_swap_chain_dri3_and_present_query_version(struct intel_x11_swap_chain *sc)
{
    xcb_dri3_query_version_cookie_t dri3_cookie;
    xcb_dri3_query_version_reply_t *dri3_reply;
    xcb_present_query_version_cookie_t present_cookie;
    xcb_present_query_version_reply_t *present_reply;

    dri3_cookie = xcb_dri3_query_version(sc->c,
            XCB_DRI3_MAJOR_VERSION, XCB_DRI3_MINOR_VERSION);
    present_cookie = xcb_present_query_version(sc->c,
            XCB_PRESENT_MAJOR_VERSION, XCB_PRESENT_MINOR_VERSION);

    dri3_reply = xcb_dri3_query_version_reply(sc->c, dri3_cookie, NULL);
    if (!dri3_reply)
        return false;

    sc->dri3_major = dri3_reply->major_version;
    sc->dri3_minor = dri3_reply->minor_version;
    free(dri3_reply);

    present_reply = xcb_present_query_version_reply(sc->c, present_cookie, NULL);
    if (!present_reply)
        return false;

    sc->present_major = present_reply->major_version;
    sc->present_minor = present_reply->minor_version;
    free(present_reply);

    return true;
}

/**
 * Send a PresentSelectInput to select interested events.
 */
static bool x11_swap_chain_present_select_input(struct intel_x11_swap_chain *sc, struct intel_x11_swap_chain *old_sc)
{
    xcb_void_cookie_t cookie;
    xcb_generic_error_t *error;
    xcb_present_event_t present_special_event_id;

    if (old_sc && (old_sc->c == sc->c) && (old_sc->window == sc->window)) {
        // Reuse a previous event.  (They can never be stopped.)
        sc->present_special_event = old_sc->present_special_event;
        old_sc->present_special_event = NULL;
        return true;
    }
    /* create the event queue */
    present_special_event_id = xcb_generate_id(sc->c);
    sc->present_special_event = xcb_register_for_special_xge(sc->c,
            &xcb_present_id, present_special_event_id, NULL);

    cookie = xcb_present_select_input_checked(sc->c,
            present_special_event_id, sc->window,
            XCB_PRESENT_EVENT_MASK_COMPLETE_NOTIFY |
            XCB_PRESENT_EVENT_MASK_CONFIGURE_NOTIFY);

    error = xcb_request_check(sc->c, cookie);
    if (error) {
        free(error);
        return false;
    }

    return true;
}

static struct intel_img *x11_swap_chain_create_persistent_image(struct intel_x11_swap_chain *sc,
                                                                struct intel_dev *dev,
                                                                const VkImageCreateInfo *img_info)
{
    struct intel_img *img;
    struct intel_mem *mem;
    struct intel_x11_img_data *data;
    VkMemoryAllocateInfo mem_info;
    int prime_fd;
    xcb_pixmap_t pixmap;
    VkResult ret;

    ret = intel_img_create(dev, img_info, NULL, true, &img);
    if (ret != VK_SUCCESS)
        return NULL;

    memset(&mem_info, 0, sizeof(mem_info));
    mem_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    mem_info.allocationSize = img->total_size;
    mem_info.memoryTypeIndex = 0;

    ret = intel_mem_alloc(dev, &mem_info, &mem);
    if (ret != VK_SUCCESS) {
        intel_img_destroy(img);
        return NULL;
    }

    prime_fd = x11_export_prime_fd(dev, mem->bo, &img->layout);
    if (prime_fd < 0) {
        intel_mem_free(mem);
        intel_img_destroy(img);
        return NULL;
    }

    pixmap = x11_dri3_pixmap_from_buffer(sc->c, sc->window,
            x11_get_drawable_depth(sc->c, sc->window),
            prime_fd, &img->layout);

    data = (struct intel_x11_img_data *) img->wsi_data;
    data->mem = mem;
    data->prime_fd = prime_fd;
    data->pixmap = pixmap;

    intel_obj_bind_mem(&img->obj, mem, 0);

    return img;
}

static bool x11_swap_chain_create_persistent_images(struct intel_x11_swap_chain *sc,
                                                    struct intel_dev *dev,
                                                    const VkSwapchainCreateInfoKHR *info)
{
    struct intel_img **images;
    intel_x11_swap_chain_image_state *image_state;
    uint32_t *present_queue;
    VkImageCreateInfo img_info;
    uint32_t i;

    images = intel_alloc(sc, sizeof(*images) * info->minImageCount,
            0, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
    if (!images)
        return false;
    image_state = intel_alloc(
            sc, sizeof(intel_x11_swap_chain_image_state) * info->minImageCount,
            0, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
    if (!image_state) {
        for (i = 0; i < info->minImageCount; i++) {
            intel_img_destroy(images[i]);
        }
        intel_free(sc, images);
        return false;
    }
    present_queue = intel_alloc(
            sc, sizeof(uint32_t) * info->minImageCount,
            0, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
    if (!present_queue) {
        for (i = 0; i < info->minImageCount; i++) {
            intel_img_destroy(images[i]);
        }
        intel_free(sc, images);
        intel_free(sc, image_state);
        return false;
    }

    memset(&img_info, 0, sizeof(img_info));
    img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_info.imageType = VK_IMAGE_TYPE_2D;
    img_info.format = info->imageFormat;
    img_info.extent.width = info->imageExtent.width;
    img_info.extent.height = info->imageExtent.height;
    img_info.extent.depth = 1;
    img_info.mipLevels = 1;
    img_info.arrayLayers = info->imageArraySize;
    img_info.samples = VK_SAMPLE_COUNT_1_BIT;
    img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    img_info.usage = info->imageUsageFlags;
    img_info.flags = 0;

    for (i = 0; i < info->minImageCount; i++) {
        images[i] = x11_swap_chain_create_persistent_image(sc,
                dev, &img_info);
        if (!images[i])
            break;
        image_state[i] = INTEL_SC_STATE_UNUSED;
    }

    if (i < info->minImageCount) {
        uint32_t j;
        for (j = 0; j < i; j++)
            intel_img_destroy(images[j]);

        intel_free(sc, images);

        if (image_state) {
            intel_free(sc, image_state);
        }

        if (present_queue) {
            intel_free(sc, present_queue);
        }

        return false;
    }

    sc->persistent_images = images;
    sc->persistent_image_count = info->minImageCount;
    sc->image_state = image_state;
    sc->present_queue = present_queue;
    sc->present_queue_length = 0;

    return true;
}

/**
 * Send a PresentPixmap.
 */
static VkResult x11_swap_chain_present_pixmap(struct intel_x11_swap_chain *sc,
                                              struct intel_img *img)
{
    struct intel_x11_img_data *data =
        (struct intel_x11_img_data *) img->wsi_data;
    uint32_t options = XCB_PRESENT_OPTION_NONE;
    uint32_t target_msc, divisor, remainder;
    xcb_void_cookie_t cookie;
    xcb_generic_error_t *err;

    target_msc = 0;
    divisor = 1;
    remainder = 0;
    if (sc->present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
        options |= XCB_PRESENT_OPTION_ASYNC;
    }

    if (sc->force_copy)
        options |= XCB_PRESENT_OPTION_COPY;

    cookie = xcb_present_pixmap_checked(sc->c,
            sc->window,
            data->pixmap,
            ++sc->local.serial,
            0, /* valid-area */
            0, /* update-area */
            0, /* x-off */
            0, /* y-off */
            0, /* crtc */
            0, /* wait-fence */
            0, /* idle-fence */
            options,
            target_msc,
            divisor,
            remainder,
            0, NULL);

    err = xcb_request_check(sc->c, cookie);
    /* TODOVV: Can this be validated */
    if (err) {
        free(err);
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    return VK_SUCCESS;
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
        const xcb_present_configure_notify_event_t *configure;
    } u = { .ev = ev };

    switch (u.ev->evtype) {
    case XCB_PRESENT_COMPLETE_NOTIFY:
        sc->remote.serial = u.complete->serial;
        sc->remote.msc = u.complete->msc;
        assert(sc->present_queue_length > 0);
        if (sc->image_state[sc->present_queue[0]] == INTEL_SC_STATE_DISPLAYED) {
            // Remove the previously-displayed image from the present queue:
            sc->image_state[sc->present_queue[0]] = INTEL_SC_STATE_UNUSED;
            sc->present_queue_length--;
            for (int j = 0; j < sc->present_queue_length; j++) {
                sc->present_queue[j] = sc->present_queue[j+1];
            }
        }
        assert(sc->present_queue_length > 0);
        assert(sc->image_state[sc->present_queue[0]] == INTEL_SC_STATE_QUEUED_FOR_PRESENT);
        sc->image_state[sc->present_queue[0]] = INTEL_SC_STATE_DISPLAYED;
        break;
    case XCB_PRESENT_EVENT_CONFIGURE_NOTIFY:
        if ((u.configure->width != sc->width) ||
            (u.configure->height != sc->height)) {
            // The swapchain is now considered "out of date" with the window:
            sc->out_of_date = true;
        }
        break;
    default:
        break;
    }
}

/*
 * Wait for an event on a swap chain.
 * Uses polling because xcb_wait_for_special_event won't time out.
 */
static VkResult x11_swap_chain_wait(struct intel_x11_swap_chain *sc,
                                    uint32_t serial, int64_t timeout)
{
    struct timespec current_time; // current time for planning wait
    struct timespec stop_time;  // time when timeout will elapse
    bool wait;
    // Don't wait on destroyed swap chain
    if (sc->present_special_event == NULL ) {
        return VK_SUCCESS;
    }
    if (timeout == 0){
        wait = false;
    } else {
        wait = true;
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        if (timeout == -1) {
            // wait approximately forever
            stop_time.tv_nsec = current_time.tv_nsec;
            stop_time.tv_sec = current_time.tv_sec + 10*365*24*60*60;
        } else {
            stop_time.tv_nsec = current_time.tv_nsec + (timeout % 1000000000);
            stop_time.tv_sec = current_time.tv_sec + (timeout / 1000000000);
            // Carry overflow from tv_nsec to tv_sec
            while (stop_time.tv_nsec > 1000000000) {
                stop_time.tv_sec += 1;
                stop_time.tv_nsec -= 1000000000;
            }
        }
    }

    while (sc->remote.serial < serial) {
        xcb_present_generic_event_t *ev;
        xcb_intern_atom_reply_t *reply;

        ev = (xcb_present_generic_event_t *)
            xcb_poll_for_special_event(sc->c, sc->present_special_event);
        if (wait) {
            int poll_timeout;
            struct pollfd fds;

            fds.fd = xcb_get_file_descriptor(sc->c);
            fds.events = POLLIN;

            while (!ev) {
                clock_gettime(CLOCK_MONOTONIC, &current_time);
                if (current_time.tv_sec > stop_time.tv_sec ||
                    (current_time.tv_sec == stop_time.tv_sec && current_time.tv_nsec > stop_time.tv_nsec)) {
                    // Time has run out
                    return VK_TIMEOUT;
                }
                poll_timeout = 1000/60; // milliseconds for 60 HZ
                if (current_time.tv_sec >= stop_time.tv_sec-1) { // Remaining timeout may be under 1/60 seconds.
                    int remaining_timeout;
                    remaining_timeout = 1000 * (stop_time.tv_sec - current_time.tv_sec) + (stop_time.tv_nsec - current_time.tv_nsec) / 1000000;
                    if (poll_timeout > remaining_timeout) {
                        poll_timeout = remaining_timeout; // milliseconds for remainder of timeout
                    }
                }

                // Wait for any input on the xcb connection or a timeout.
                // Events may come in and be queued up before poll.  Timing out handles that.
                poll(&fds, 1, poll_timeout);

                // Another thread may have handled events and updated sc->remote.serial
                if (sc->remote.serial >= serial) {
                    return VK_SUCCESS;
                }

                // Use xcb_intern_atom_reply just to make xcb really read events from socket.
                // Calling xcb_poll_for_special_event fails to actually look for new packets.
                reply = xcb_intern_atom_reply(sc->c, xcb_intern_atom(sc->c, 1, 1, "a"), NULL);
                if (reply) {
                    free(reply);
                }

                ev = (xcb_present_generic_event_t *)
                    xcb_poll_for_special_event(sc->c, sc->present_special_event);
            }
        } else {
            if (!ev)
                return VK_NOT_READY;
        }

        x11_swap_chain_present_event(sc, ev);

        free(ev);
    }

    return VK_SUCCESS;
}

static void x11_swap_chain_destroy_begin(struct intel_x11_swap_chain *sc)
{
    if (sc->persistent_images) {
        uint32_t i;

        for (i = 0; i < sc->persistent_image_count; i++)
            intel_img_destroy(sc->persistent_images[i]);
        intel_free(sc, sc->persistent_images);
    }

    if (sc->image_state) {
        intel_free(sc, sc->image_state);
    }

    if (sc->present_queue) {
        intel_free(sc, sc->present_queue);
    }

    // Don't unregister because another swap chain may be using this event queue.
    //if (sc->present_special_event)
    //    xcb_unregister_for_special_event(sc->c, sc->present_special_event);
}

static void x11_swap_chain_destroy_end(struct intel_x11_swap_chain *sc)
{
    // Leave memory around for now because fences use it without reference count.
    //intel_free(sc, sc);
}

static VkResult x11_swap_chain_create(struct intel_dev *dev,
                                      const VkSwapchainCreateInfoKHR *info,
                                      struct intel_x11_swap_chain **sc_ret)
{
    const xcb_randr_provider_t provider = 0;
    const VkSurfaceDescriptionWindowKHR* pSurfaceDescriptionWindow =
        (VkSurfaceDescriptionWindowKHR*) info->pSurfaceDescription;
    VkPlatformHandleXcbKHR *pPlatformHandleXcb = (VkPlatformHandleXcbKHR *)
        pSurfaceDescriptionWindow->pPlatformHandle;
    xcb_connection_t *c = (xcb_connection_t *)
        pPlatformHandleXcb->connection;
    xcb_window_t window = *((xcb_window_t *)
                            pSurfaceDescriptionWindow->pPlatformWindow);
    struct intel_x11_swap_chain *sc;
    int fd;

    /* TODOVV: Add test to validation layer */
    if (!x11_is_format_presentable(dev, info->imageFormat)) {
        intel_dev_log(dev, VK_DBG_REPORT_ERROR_BIT,
                      VK_NULL_HANDLE, 0, 0, "invalid presentable image format");
//        return VK_ERROR_INVALID_VALUE;
        return VK_ERROR_VALIDATION_FAILED;
    }

    /* TODOVV: Can we add test to validation layer? */
    if (!x11_is_dri3_and_present_supported(c)) {
//        return VK_ERROR_INVALID_VALUE;
        return VK_ERROR_VALIDATION_FAILED;
    }

    /* TODOVV: Can we add test to validation layer? */
    fd = x11_dri3_open(c, window, provider);
    if (fd < 0 || !x11_gpu_match_fd(dev->gpu, fd)) {
        if (fd >= 0)
            close(fd);
//        return VK_ERROR_INVALID_VALUE;
        return VK_ERROR_VALIDATION_FAILED;
    }

    close(fd);

    sc = intel_alloc(dev, sizeof(*sc), 0, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
    if (!sc)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    memset(sc, 0, sizeof(*sc));
    intel_handle_init(&sc->handle, VK_OBJECT_TYPE_SWAPCHAIN_KHR, dev->base.handle.instance);

    sc->c = c;
    sc->window = window;

    /* always copy unless flip bit is set */
    sc->present_mode = info->presentMode;
    // Record the swapchain's width and height, so that we can determine when
    // it is "out of date" w.r.t. the window:
    sc->width = info->imageExtent.width;
    sc->height = info->imageExtent.height;
    sc->out_of_date = false;
    sc->being_deleted = false;
    sc->force_copy = true;

    if (!x11_swap_chain_dri3_and_present_query_version(sc) ||
        !x11_swap_chain_present_select_input(sc, x11_swap_chain(info->oldSwapchain)) ||
        !x11_swap_chain_create_persistent_images(sc, dev, info)) {
        x11_swap_chain_destroy_begin(sc);
        x11_swap_chain_destroy_end(sc);
        return VK_ERROR_VALIDATION_FAILED;
    }

    *sc_ret = sc;

    return VK_SUCCESS;
}

static void x11_display_destroy(struct intel_x11_display *dpy)
{
    intel_free(dpy, dpy->modes);
    intel_free(dpy, dpy);
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
}

VkResult intel_wsi_img_init(struct intel_img *img)
{
    struct intel_x11_img_data *data;

    data = intel_alloc(img, sizeof(*data), 0, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
    if (!data)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

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

VkResult intel_wsi_fence_init(struct intel_fence *fence)
{
    struct intel_x11_fence_data *data;

    data = intel_alloc(fence, sizeof(*data), 0, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
    if (!data)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

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
    if (!fence->wsi_data) {
        return;
    }
    memcpy(fence->wsi_data, src->wsi_data,
            sizeof(struct intel_x11_fence_data));
}

VkResult intel_wsi_fence_wait(struct intel_fence *fence,
                              int64_t timeout_ns)
{
    struct intel_x11_fence_data *data =
        (struct intel_x11_fence_data *) fence->wsi_data;

    if (!data)
        return VK_SUCCESS;

    if (!data->swap_chain)
        return VK_SUCCESS;

    return x11_swap_chain_wait(data->swap_chain, data->serial, timeout_ns);
}

ICD_EXPORT VkResult VKAPI vkGetPhysicalDeviceSurfaceSupportKHR(
    VkPhysicalDevice                        physicalDevice,
    uint32_t                                queueNodeIndex,
    const VkSurfaceDescriptionKHR*          pSurfaceDescription,
    VkBool32*                               pSupported)
{
    VkResult ret = VK_SUCCESS;
    const VkSurfaceDescriptionWindowKHR* pSurfaceDescriptionWindow =
        (VkSurfaceDescriptionWindowKHR*) pSurfaceDescription;

    *pSupported = false;

    // TODOVV: Move this check to a validation layer (i.e. the driver should
    // assume the correct data type, and not check):
    if (pSurfaceDescriptionWindow->sType != VK_STRUCTURE_TYPE_SURFACE_DESCRIPTION_WINDOW_KHR) {
        return VK_ERROR_VALIDATION_FAILED;
    }

    // TODOVV: NEED TO ALSO CHECK:
    // - queueNodeIndex
    // - pSurfaceDescriptionWindow->pPlatformHandle (can try to use it)
    // - pSurfaceDescriptionWindow->pPlatformWindow (can try to use it)
    if (pSurfaceDescriptionWindow->platform == VK_PLATFORM_XCB_KHR) {
        *pSupported = true;
    }

    return ret;
}

VkResult VKAPI vkGetSurfacePropertiesKHR(
    VkDevice                                 device,
    const VkSurfaceDescriptionKHR*           pSurfaceDescription,
    VkSurfacePropertiesKHR*                  pSurfaceProperties)
{
    // TODOVV: Move this check to a validation layer (i.e. the driver should
    // assume the correct data type, and not check):
    assert(pSurfaceProperties);

    return x11_get_surface_properties(pSurfaceDescription, pSurfaceProperties);
}

VkResult VKAPI vkGetSurfaceFormatsKHR(
    VkDevice                                 device,
    const VkSurfaceDescriptionKHR*           pSurfaceDescription,
    uint32_t*                                pCount,
    VkSurfaceFormatKHR*                      pSurfaceFormats)
{
    VkResult ret = VK_SUCCESS;

    // TODOVV: Move this check to a validation layer (i.e. the driver should
    // assume the correct data type, and not check):
    if (!pCount) {
//        return VK_ERROR_INVALID_POINTER;
        return VK_ERROR_VALIDATION_FAILED;
    }

    if (pSurfaceFormats) {
        uint32_t i;
        for (i = 0; i < *pCount; i++) {
            pSurfaceFormats[i].format = x11_presentable_formats[i];
            pSurfaceFormats[i].colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        }
    } else {
        *pCount = ARRAY_SIZE(x11_presentable_formats);
    }

    return ret;
}

VkResult VKAPI vkGetSurfacePresentModesKHR(
    VkDevice                                 device,
    const VkSurfaceDescriptionKHR*           pSurfaceDescription,
    uint32_t*                                pCount,
    VkPresentModeKHR*                        pPresentModes)
{
    VkResult ret = VK_SUCCESS;

    // TODOVV: Move this check to a validation layer (i.e. the driver should
    // assume the correct data type, and not check):
    if (!pCount) {
//        return VK_ERROR_INVALID_POINTER;
        return VK_ERROR_VALIDATION_FAILED;
    }

    if (pPresentModes) {
        pPresentModes[0] = VK_PRESENT_MODE_IMMEDIATE_KHR;
        pPresentModes[1] = VK_PRESENT_MODE_FIFO_KHR;
        // TODO: Consider adding VK_PRESENT_MODE_MAILBOX_KHR sometime
    } else {
        *pCount = 2;
    }

    return ret;
}

ICD_EXPORT VkResult VKAPI vkCreateSwapchainKHR(
    VkDevice                                device,
    const VkSwapchainCreateInfoKHR*         pCreateInfo,
    VkSwapchainKHR*                         pSwapchain)
{
    struct intel_dev *dev = intel_dev(device);

    if (pCreateInfo->oldSwapchain) {
        // TODO: Eventually, do more than simply up-front destroy the
        // oldSwapchain (but just do that for now):
        struct intel_x11_swap_chain *sc =
            x11_swap_chain(pCreateInfo->oldSwapchain);

        sc->being_deleted = true;
        x11_swap_chain_destroy_begin(sc);
    }

    return x11_swap_chain_create(dev, pCreateInfo,
            (struct intel_x11_swap_chain **) pSwapchain);
}

ICD_EXPORT VkResult VKAPI vkDestroySwapchainKHR(
    VkDevice                                 device,
    VkSwapchainKHR                           swapchain)
{
    struct intel_x11_swap_chain *sc = x11_swap_chain(swapchain);

    if (!sc->being_deleted) {
        x11_swap_chain_destroy_begin(sc);
    }
    x11_swap_chain_destroy_end(sc);

    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkGetSwapchainImagesKHR(
    VkDevice                                 device,
    VkSwapchainKHR                           swapchain,
    uint32_t*                                pCount,
    VkImage*                                 pSwapchainImages)
{
    struct intel_x11_swap_chain *sc = x11_swap_chain(swapchain);
    VkResult ret = VK_SUCCESS;

    // TODOVV: Move this check to a validation layer (i.e. the driver should
    // assume the correct data type, and not check):
    if (!pCount) {
//        return VK_ERROR_INVALID_POINTER;
        return VK_ERROR_VALIDATION_FAILED;
    }

    if (pSwapchainImages) {
        uint32_t i;
        for (i = 0; i < *pCount; i++) {
            pSwapchainImages[i] = (VkImage) sc->persistent_images[i];
        }
    } else {
        *pCount = sc->persistent_image_count;
    }

    return ret;
}

ICD_EXPORT VkResult VKAPI vkAcquireNextImageKHR(
    VkDevice                                 device,
    VkSwapchainKHR                           swapchain,
    uint64_t                                 timeout,
    VkSemaphore                              semaphore,
    uint32_t*                                pImageIndex)
{
    struct intel_x11_swap_chain *sc = x11_swap_chain(swapchain);
    VkResult ret = VK_SUCCESS;

    if (sc->out_of_date) {
        // The window was resized, and the swapchain must be re-created:
        return VK_ERROR_OUT_OF_DATE_KHR;
    }

    // Find an unused image to return:
    for (int i = 0; i < sc->persistent_image_count; i++) {
        if (sc->image_state[i] == INTEL_SC_STATE_UNUSED) {
            sc->image_state[i] = INTEL_SC_STATE_APP_OWNED;
            *pImageIndex = i;
            return ret;
        }
    }

    // If no image is ready, wait for a present to finish
    ret = x11_swap_chain_wait(sc, sc->remote.serial+1, timeout);
    if (ret != VK_SUCCESS) {
        return ret;
    }

    // Find an unused image to return:
    for (int i = 0; i < sc->persistent_image_count; i++) {
        if (sc->image_state[i] == INTEL_SC_STATE_UNUSED) {
            sc->image_state[i] = INTEL_SC_STATE_APP_OWNED;
            *pImageIndex = i;
            return ret;
        }
    }
    // NOTE: Should never get here, but in case we do, do something:
    assert(0);
    return VK_ERROR_VALIDATION_FAILED;
}


ICD_EXPORT VkResult VKAPI vkQueuePresentKHR(
    VkQueue                                  queue_,
    VkPresentInfoKHR*                        pPresentInfo)
{
    struct intel_queue *queue = intel_queue(queue_);
    uint32_t i;
    uint32_t num_swapchains = pPresentInfo->swapchainCount;
    VkResult rtn = VK_SUCCESS;

    // Wait for queue to idle before out-of-band xcb present operation.
    const VkResult r = intel_queue_wait(queue, -1);
    (void) r;

    for (i = 0; i < num_swapchains; i++) {
        struct intel_x11_swap_chain *sc =
            x11_swap_chain(pPresentInfo->swapchains[i]);
        struct intel_img *img = 
            sc->persistent_images[pPresentInfo->imageIndices[i]];
        struct intel_x11_fence_data *data =
            (struct intel_x11_fence_data *) queue->fence->wsi_data;
        VkResult ret;

        if (sc->out_of_date) {
            // The window was resized, and the swapchain must be re-created:
            rtn = VK_ERROR_OUT_OF_DATE_KHR;
            // TODO: Potentially change this to match the result of Bug 14952
            // (which deals with some of the swapchains being out-of-date, but
            // not all of them).  For now, just present the swapchains that
            // aren't out-of-date, and skip the ones that are out-of-date:
            continue;
        }

        ret = x11_swap_chain_present_pixmap(sc, img);
        if (ret != VK_SUCCESS) {
            return ret;
        }

        // Record the state change for this image, and add this image to the
        // present queue for the swap chain:
        sc->image_state[pPresentInfo->imageIndices[i]] =
            INTEL_SC_STATE_QUEUED_FOR_PRESENT;
        sc->present_queue[sc->present_queue_length++] =
            pPresentInfo->imageIndices[i];

        data->swap_chain = sc;
        data->serial = sc->local.serial;
        sc->present_serial = sc->local.serial;
        intel_fence_set_seqno(queue->fence, img->obj.mem->bo);
    }

    return rtn;
}
