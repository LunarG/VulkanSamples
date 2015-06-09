/*
 * Vulkan 3-D graphics library
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
 *   Courtney Goeltzenleuchter <courtney@lunarg.com>
 *   Chia-I Wu <olv@lunarg.com>
 */

#include "icd-enumerate-drm.h"
#include "gpu.h"
#include "instance.h"

static int intel_devid_override;
int intel_debug = -1;

void *intel_alloc(const void *handle,
                                size_t size, size_t alignment,
                                VkSystemAllocType type)
{
    assert(intel_handle_validate(handle));
    return icd_instance_alloc(((const struct intel_handle *) handle)->instance->icd,
            size, alignment, type);
}

void intel_free(const void *handle, void *ptr)
{
    assert(intel_handle_validate(handle));
    icd_instance_free(((const struct intel_handle *) handle)->instance->icd, ptr);
}

void intel_logv(const void *handle,
                VkFlags msg_flags,
                VkObjectType obj_type, VkObject src_object,
                size_t location, int32_t msg_code,
                const char *format, va_list ap)
{
    char msg[256];
    int ret;

    ret = vsnprintf(msg, sizeof(msg), format, ap);
    if (ret >= sizeof(msg) || ret < 0)
        msg[sizeof(msg) - 1] = '\0';

    assert(intel_handle_validate(handle));
    icd_instance_log(((const struct intel_handle *) handle)->instance->icd,
                     msg_flags,
                     obj_type, src_object,              /* obj_type, object */
                     location, msg_code,                /* location, msg_code */
                     msg);
}

static void intel_debug_init(void)
{
    const char *env;

    if (intel_debug >= 0)
        return;

    intel_debug = 0;

    /* parse comma-separated debug options */
    env = getenv("INTEL_DEBUG");
    while (env) {
        const char *p = strchr(env, ',');
        size_t len;

        if (p)
            len = p - env;
        else
            len = strlen(env);

        if (len > 0) {
            if (strncmp(env, "batch", len) == 0) {
                intel_debug |= INTEL_DEBUG_BATCH;
            } else if (strncmp(env, "nohw", len) == 0) {
                intel_debug |= INTEL_DEBUG_NOHW;
            } else if (strncmp(env, "nocache", len) == 0) {
                intel_debug |= INTEL_DEBUG_NOCACHE;
            } else if (strncmp(env, "nohiz", len) == 0) {
                intel_debug |= INTEL_DEBUG_NOHIZ;
            } else if (strncmp(env, "hang", len) == 0) {
                intel_debug |= INTEL_DEBUG_HANG;
            } else if (strncmp(env, "0x", 2) == 0) {
                intel_debug |= INTEL_DEBUG_NOHW;
                intel_devid_override = strtol(env, NULL, 16);
            }
        }

        if (!p)
            break;

        env = p + 1;
    }
}

static void intel_instance_add_gpu(struct intel_instance *instance,
                                   struct intel_gpu *gpu)
{
    gpu->next = instance->gpus;
    instance->gpus = gpu;
}

static void intel_instance_remove_gpus(struct intel_instance *instance)
{
    struct intel_gpu *gpu = instance->gpus;

    while (gpu) {
        struct intel_gpu *next = gpu->next;

        intel_gpu_destroy(gpu);
        gpu = next;
    }

    instance->gpus = NULL;
}

static void intel_instance_destroy(struct intel_instance *instance)
{
    struct icd_instance *icd = instance->icd;

    intel_instance_remove_gpus(instance);
    icd_instance_free(icd, instance);

    icd_instance_destroy(icd);
}

static struct intel_instance *intel_instance_create(const VkInstanceCreateInfo* info)
{
    struct intel_instance *instance;
    struct icd_instance *icd;
    uint32_t i;

    intel_debug_init();

    icd = icd_instance_create(info->pAppInfo, info->pAllocCb);
    if (!icd)
        return NULL;

    instance = icd_instance_alloc(icd, sizeof(*instance), 0,
            VK_SYSTEM_ALLOC_TYPE_API_OBJECT);
    if (!instance) {
        icd_instance_destroy(icd);
        return NULL;
    }

    memset(instance, 0, sizeof(*instance));
    intel_handle_init(&instance->handle, VK_OBJECT_TYPE_INSTANCE, icd);

    instance->icd = icd;

    for (i = 0; i < info->extensionCount; i++) {
        const enum intel_global_ext_type ext = intel_gpu_lookup_global_extension(NULL,
                &info->pEnabledExtensions[i]);

        if (ext != INTEL_GLOBAL_EXT_INVALID)
            instance->exts[ext] = true;
    }
    return instance;
}

static const VkExtensionProperties intel_global_exts[INTEL_GLOBAL_EXT_COUNT] = {
    {
        .sType = VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES,
        .name = VK_WSI_LUNARG_EXTENSION_NAME,
        .version = VK_WSI_LUNARG_REVISION,
        .description = "Intel sample driver",
    }
};

enum intel_global_ext_type intel_gpu_lookup_global_extension(
        const struct intel_instance *instance,
        const VkExtensionProperties *ext)
{
    enum intel_global_ext_type type;

    for (type = 0; type < ARRAY_SIZE(intel_global_exts); type++) {
        if (compare_vk_extension_properties(&intel_global_exts[type], ext))
            break;
    }

    assert(type < INTEL_GLOBAL_EXT_COUNT || type == INTEL_GLOBAL_EXT_INVALID);

    return type;
}

ICD_EXPORT VkResult VKAPI vkCreateInstance(
    const VkInstanceCreateInfo*             pCreateInfo,
    VkInstance*                               pInstance)
{
    struct intel_instance *instance;

    instance = intel_instance_create(pCreateInfo);
    if (!instance)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    *pInstance = (VkInstance) instance;

    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkDestroyInstance(
    VkInstance                                pInstance)
{
    struct intel_instance *instance = intel_instance(pInstance);

    intel_instance_destroy(instance);

    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkEnumeratePhysicalDevices(
    VkInstance                                instance_,
    uint32_t*                                 pPhysicalDeviceCount,
    VkPhysicalDevice*                            pPhysicalDevices)
{
    struct intel_instance *instance = intel_instance(instance_);
    struct icd_drm_device *devices, *dev;
    VkResult ret;
    uint32_t count;

    if (pPhysicalDevices == NULL) {
        *pPhysicalDeviceCount = 1;
        return VK_SUCCESS;
    }

    intel_instance_remove_gpus(instance);

    devices = icd_drm_enumerate(instance->icd, 0x8086);

    count = 0;
    dev = devices;
    while (dev) {
        const char *primary_node, *render_node;
        int devid;
        struct intel_gpu *gpu;

        primary_node = icd_drm_get_devnode(dev, ICD_DRM_MINOR_LEGACY);
        if (!primary_node)
            continue;

        render_node = icd_drm_get_devnode(dev, ICD_DRM_MINOR_RENDER);

        devid = (intel_devid_override) ? intel_devid_override : dev->devid;
        ret = intel_gpu_create(instance, devid,
                primary_node, render_node, &gpu);
        if (ret == VK_SUCCESS) {
            intel_instance_add_gpu(instance, gpu);

            pPhysicalDevices[count++] = (VkPhysicalDevice) gpu;
            if (count >= *pPhysicalDeviceCount)
                break;
        }

        dev = dev->next;
    }

    icd_drm_release(instance->icd, devices);

    *pPhysicalDeviceCount = count;

    return (count > 0) ? VK_SUCCESS : VK_ERROR_UNAVAILABLE;
}

ICD_EXPORT VkResult VKAPI vkDbgCreateMsgCallback(
    VkInstance                          instance,
    VkFlags                    msgFlags,
    PFN_vkDbgMsgCallback                pfnMsgCallback,
    void*                               pUserData,
    VkDbgMsgCallback*                   pMsgCallback)
{
    struct intel_instance *inst = intel_instance(instance);

    return icd_instance_create_logger(inst->icd, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
}

ICD_EXPORT VkResult VKAPI vkDbgDestroyMsgCallback(
    VkInstance                          instance,
    VkDbgMsgCallback                    msgCallback)
{
    struct intel_instance *inst = intel_instance(instance);

    return icd_instance_destroy_logger(inst->icd, msgCallback);
}
