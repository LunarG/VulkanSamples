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

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>

#include <pciaccess.h>

#include <xf86drm.h>
#include <i915_drm.h>

#include <libudev.h>
#include <dlfcn.h>

#ifdef HAVE_VALGRIND
#include <valgrind.h>
#include <memcheck.h>
#define VG(x) x
#else
#define VG(x)
#endif

#define VG_CLEAR(s) VG(memset(&s, 0, sizeof(s)))

#include "xgl.h"
#include "xglDbg.h"

#include "driver.h"
#include "intel_chipset.h"
#include "gen7_functions.h"
#include "dispatch_tables.h"


/*
 * Global to contain application allocation call-backs
 * and GPU enumeration info.
 */
struct _xgl_config icd_data;

static void loader_err(XGL_INT msg_code, const char *msg)
{
    icd_log(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0,
            XGL_NULL_HANDLE, 0, msg_code, msg);
}

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
    struct _xgl_device *pXglDev;

    ret = icd_set_alloc_callbacks(pAllocCb);
    if (ret != XGL_SUCCESS)
        return ret;

    ret = XGL_ERROR_UNAVAILABLE;

    // TODO: Do we need to keep track of previous calls to xglInitAndEnumerageGpus?
    /*
     * xglInitAndEnumerateGpus() can be called multiple times. Calling it more than once
     * forces driver reinitialization.
     */

    if (icd_data.num_gpus > 0) {
        // TODO: Free resources and re-initialize
    }
    // TODO: Do we need any other validation for incoming pointers?

    udev = udev_new();
    if (udev == NULL) {
        loader_err(0, "failed to initialize udev context");
        abort();
    }

    memset(icd_data.gpu_info, 0,
           sizeof(icd_data.gpu_info[XGL_MAX_PHYSICAL_GPUS]));
    memset(icd_data.render_node_list, 0,
           sizeof(icd_data.render_node_list[XGL_MAX_PHYSICAL_GPUS]));

    fd = -1;
    e = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(e, "drm");
    udev_enumerate_scan_devices(e);
    udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e)) {
        path = udev_list_entry_get_name(entry);
        device = udev_device_new_from_syspath(udev, path);
        parent = udev_device_get_parent(device);
        usub = udev_device_get_subsystem(parent);
        /* Filter out KMS output devices. */
        if (!usub || (strcmp(usub, "pci") != 0))
            continue;
        pci_id = udev_device_get_property_value(parent, "PCI_ID");
        if (fnmatch(pci_glob, pci_id, 0) != 0)
            continue;
        dnode = udev_device_get_devnode(device);
        fd = open(dnode, O_RDWR);
        if (fd < 0)
            continue;
        if (!is_render_node(fd, &st)) {
            close(fd);
            fd = -1;
            continue;
        }

        pXglDev = &icd_data.gpu_info[count];

        sscanf(pci_id, "%x:%x", &pXglDev->ven_id, &pXglDev->dev_id);
        /*
         * Currently only allowing Ivybridge and Haswell
         */
        if (!IS_HASWELL(pXglDev->dev_id) && !IS_IVYBRIDGE(pXglDev->dev_id)) {
            close(fd);
            fd = -1;
            continue;
        }

        /*
         * We'll keep the fd open for any subsequent
         * xglGetGpuInfo calls.
         */
        pXglDev->fd = fd;
        strncpy(pXglDev->device_path, dnode, XGL_MAX_PHYSICAL_GPU_NAME);
        icd_data.render_node_list[count] = pXglDev;
        pXglDev->exec = &intel_normal_dispatch_table;

        ret = XGL_SUCCESS;
        count++;
    }
    icd_data.num_gpus = count;
    udev_enumerate_unref(e);
    udev_unref(udev);

    if (ret == XGL_SUCCESS) {
        *pGpuCount = icd_data.num_gpus;
        memcpy(pGpus, icd_data.render_node_list,
               (icd_data.num_gpus >
                maxGpus ? maxGpus : icd_data.num_gpus) *
               sizeof(XGL_PHYSICAL_GPU));
    }

    return ret;
}

XGL_RESULT XGLAPI intelGetExtensionSupport(
    XGL_PHYSICAL_GPU                            gpu,
    const XGL_CHAR*                             pExtName)
{
    // struct _xgl_device *pdev = (struct _xgl_device *) gpu;

    // TODO: Check requested extension

    return XGL_ERROR_INVALID_EXTENSION;
}
