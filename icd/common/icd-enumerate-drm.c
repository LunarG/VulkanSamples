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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libudev.h>

#include "icd-instance.h"
#include "icd-utils.h"
#include "icd-enumerate-drm.h"

static enum icd_drm_minor_type get_minor_type(struct udev_device *minor_dev)
{
    const char *minor;

    minor = udev_device_get_property_value(minor_dev, "MINOR");
    if (!minor)
        return ICD_DRM_MINOR_INVALID;

    switch (atoi(minor) >> 6) {
    case 0:
        return ICD_DRM_MINOR_LEGACY;
    case 2:
        return ICD_DRM_MINOR_RENDER;
    default:
        return ICD_DRM_MINOR_INVALID;
    }
}

static void get_pci_id(struct udev_device *pci_dev, int *vendor, int *devid)
{
    const char *pci_id;

    pci_id = udev_device_get_property_value(pci_dev, "PCI_ID");
    if (sscanf(pci_id, "%x:%x", vendor, devid) != 2) {
        *vendor = 0;
        *devid = 0;
    }
}

static struct icd_drm_device *find_dev(struct icd_drm_device *devices,
                                       const char *parent_syspath)
{
    struct icd_drm_device *dev = devices;

    while (dev) {
        if (!strcmp((const char *) dev->id, parent_syspath))
            break;
        dev = dev->next;
    }

    return dev;
}

static struct icd_drm_device *probe_syspath(const struct icd_instance *instance,
                                            struct icd_drm_device *devices,
                                            struct udev *udev, const char *syspath,
                                            int vendor_id_match)
{
    struct udev_device *minor, *parent;
    enum icd_drm_minor_type type;
    const char *parent_syspath;
    struct icd_drm_device *dev;
    int vendor, devid;

    minor = udev_device_new_from_syspath(udev, syspath);
    if (!minor)
        return devices;

    type = get_minor_type(minor);
    if (type == ICD_DRM_MINOR_INVALID) {
        udev_device_unref(minor);
        return devices;
    }

    parent = udev_device_get_parent(minor);
    if (!parent) {
        udev_device_unref(minor);
        return devices;
    }

    get_pci_id(parent, &vendor, &devid);
    if (vendor_id_match && vendor != vendor_id_match) {
        udev_device_unref(minor);
        return devices;
    }

    parent_syspath = udev_device_get_syspath(parent);

    dev = find_dev(devices, parent_syspath);
    if (dev) {
        assert(dev->devid == devid);

        assert(!dev->minors[type]);
        if (dev->minors[type])
            udev_device_unref((struct udev_device *) dev->minors[type]);

        dev->minors[type] = (void *) minor;

        return devices;
    } else {
        dev = icd_instance_alloc(instance, sizeof(*dev), 0,
                VK_SYSTEM_ALLOC_TYPE_INTERNAL_TEMP);
        if (!dev)
            return devices;

        memset(dev, 0, sizeof(*dev));

        dev->id = (const void *) parent_syspath;
        dev->devid = devid;
        dev->minors[type] = (void *) minor;

        dev->next = devices;

        return dev;
    }
}

struct icd_drm_device *icd_drm_enumerate(const struct icd_instance *instance,
                                         int vendor_id)
{
    struct icd_drm_device *devices = NULL;
    struct udev *udev;
    struct udev_enumerate *e;
    struct udev_list_entry *entry;

    udev = udev_new();
    if (udev == NULL) {
        icd_instance_log(instance, VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0,
                VK_NULL_HANDLE, 0, 0, "failed to initialize udev context");

        return NULL;
    }

    e = udev_enumerate_new(udev);
    if (e == NULL) {
        icd_instance_log(instance, VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0,
                VK_NULL_HANDLE, 0, 0,
                "failed to initialize udev enumerate context");
        udev_unref(udev);

        return NULL;
    }

    /* we are interested in DRM minors */
    udev_enumerate_add_match_subsystem(e, "drm");
    udev_enumerate_add_match_property(e, "DEVTYPE", "drm_minor");
    udev_enumerate_scan_devices(e);

    udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e)) {
        devices = probe_syspath(instance, devices, udev,
                udev_list_entry_get_name(entry), vendor_id);
    }

    free(e);
    free(udev);
    return devices;
}

void icd_drm_release(const struct icd_instance *instance,
                     struct icd_drm_device *devices)
{
    struct icd_drm_device *dev = devices;

    while (dev) {
        struct icd_drm_device *next = dev->next;
        size_t i;

        for (i = 0; i < ARRAY_SIZE(dev->minors); i++)
            udev_device_unref((struct udev_device *) dev->minors[i]);

        icd_instance_free(instance, dev);
        dev = next;
    }
}

const char *icd_drm_get_devnode(struct icd_drm_device *dev,
                                enum icd_drm_minor_type minor)
{
    return (dev->minors[minor]) ?
        udev_device_get_devnode((struct udev_device *) dev->minors[minor]) :
        NULL;
}
