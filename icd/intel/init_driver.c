/*
 * XGL 3-D graphics library
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
 *    Courtney Goeltzenleuchter <courtney@lunarg.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <fnmatch.h>

#include <libudev.h>

#include "gen7_functions.h"
#include "gpu.h"
#include "intel.h"

static int is_render_node(int fd, struct stat *st)
{
    if (fstat(fd, st))
        return 0;

    if (!S_ISCHR(st->st_mode))
        return 0;

    return st->st_rdev & 0x80;
}

ICD_EXPORT XGL_RESULT XGLAPI xglInitAndEnumerateGpus(const XGL_APPLICATION_INFO *
                                            pAppInfo,
                                            const XGL_ALLOC_CALLBACKS *
                                            pAllocCb, XGL_UINT maxGpus,
                                            XGL_UINT * pGpuCount,
                                            XGL_PHYSICAL_GPU * pGpus)
{
    struct udev *udev;
    struct udev_enumerate *e;
    struct udev_device *device, *parent;
    struct udev_list_entry *entry;
    const char *pci_id, *path;
    const char *usub, *dnode;
    int fd;
    struct stat st;
    char *pci_glob = "*:*";
    XGL_RESULT ret;
    XGL_UINT count = 0;

    ret = icd_set_allocator(pAllocCb);
    if (ret != XGL_SUCCESS)
        return ret;

    /*
     * xglInitAndEnumerateGpus() can be called multiple times. Calling it more than once
     * forces driver reinitialization.
     */
    intel_gpu_remove_all();

    if (!maxGpus)
        return XGL_SUCCESS;

    // TODO: Do we need any other validation for incoming pointers?

    udev = udev_new();
    if (udev == NULL) {
        icd_log(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0,
                XGL_NULL_HANDLE, 0, 0, "failed to initialize udev context");
        return XGL_ERROR_OUT_OF_MEMORY;
    }

    fd = -1;
    e = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(e, "drm");
    udev_enumerate_scan_devices(e);
    udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e)) {
        unsigned int ven_id, dev_id;
        struct intel_gpu *gpu;

        path = udev_list_entry_get_name(entry);
        device = udev_device_new_from_syspath(udev, path);
        parent = udev_device_get_parent(device);
        usub = udev_device_get_subsystem(parent);
        /* Filter out KMS output devices. */
        if (!usub || (strcmp(usub, "pci") != 0)) {
            udev_device_unref(device);
            continue;
        }
        pci_id = udev_device_get_property_value(parent, "PCI_ID");
        if (fnmatch(pci_glob, pci_id, 0) != 0) {
            udev_device_unref(device);
            continue;
        }
        sscanf(pci_id, "%x:%x", &ven_id, &dev_id);
        if (ven_id != 0x8086) {
            udev_device_unref(device);
            continue;
        }

        dnode = udev_device_get_devnode(device);
        /* TODO do not open the device at this point */
        fd = open(dnode, O_RDWR);
        if (fd < 0) {
            udev_device_unref(device);
            continue;
        }
        if (!is_render_node(fd, &st)) {
            close(fd);
            fd = -1;
            udev_device_unref(device);
            continue;
        }
        close(fd);

        ret = intel_gpu_add(dev_id, dnode, &gpu);

        udev_device_unref(device);

        if (ret == XGL_SUCCESS) {
            pGpus[count++] = (XGL_PHYSICAL_GPU) gpu;
            if (count >= maxGpus)
                break;
        }
    }

    udev_enumerate_unref(e);
    udev_unref(udev);

    *pGpuCount = count;

    return (count > 0) ? XGL_SUCCESS : XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI intelGetExtensionSupport(
    XGL_PHYSICAL_GPU                            gpu_,
    const XGL_CHAR*                             pExtName)
{
    struct intel_gpu *gpu = intel_gpu(gpu_);

    return (intel_gpu_has_extension(gpu, (const char *) pExtName)) ?
        XGL_SUCCESS : XGL_ERROR_INVALID_EXTENSION;
}
