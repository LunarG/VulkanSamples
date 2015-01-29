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
 *   Courtney Goeltzenleuchter <courtney@lunarg.com>
 *   Chia-I Wu <olv@lunarg.com>
 */

#include "icd-enumerate-drm.h"
#include "gpu.h"
#include "intel.h"

static int intel_devid_override;
int intel_debug = -1;

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

ICD_EXPORT XGL_RESULT XGLAPI xglInitAndEnumerateGpus(
    const XGL_APPLICATION_INFO*                 pAppInfo,
    const XGL_ALLOC_CALLBACKS*                  pAllocCb,
    uint32_t                                    maxGpus,
    uint32_t*                                   pGpuCount,
    XGL_PHYSICAL_GPU*                           pGpus)
{
    struct icd_drm_device *devices, *dev;
    XGL_RESULT ret;
    uint32_t count;

    intel_debug_init();

    ret = icd_allocator_init(pAllocCb);
    if (ret != XGL_SUCCESS)
        return ret;

    /*
     * xglInitAndEnumerateGpus() can be called multiple times. Calling it more
     * than once forces driver reinitialization.
     */
    intel_gpu_remove_all();

    if (!maxGpus) {
        *pGpuCount = 0;
        return XGL_SUCCESS;
    }

    devices = icd_drm_enumerate(0x8086);

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
        ret = intel_gpu_add(devid, primary_node, render_node, &gpu);
        if (ret == XGL_SUCCESS) {
            pGpus[count++] = (XGL_PHYSICAL_GPU) gpu;
            if (count >= maxGpus)
                break;
        }

        dev = dev->next;
    }

    icd_drm_release(devices);

    *pGpuCount = count;

    return (count > 0) ? XGL_SUCCESS : XGL_ERROR_UNAVAILABLE;
}

ICD_EXPORT XGL_RESULT XGLAPI xglEnumerateLayers(
    XGL_PHYSICAL_GPU                            gpu,
    size_t                                      maxLayerCount,
    size_t                                      maxStringSize,
    size_t*                                     pOutLayerCount,
    char* const*                                pOutLayers,
    void*                                       pReserved)
{
    if (!pOutLayerCount)
        return XGL_ERROR_INVALID_POINTER;

    *pOutLayerCount = 0;

    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglDbgRegisterMsgCallback(
    XGL_DBG_MSG_CALLBACK_FUNCTION               pfnMsgCallback,
    void*                                       pUserData)
{
    return icd_logger_add_callback(pfnMsgCallback, pUserData);
}

ICD_EXPORT XGL_RESULT XGLAPI xglDbgUnregisterMsgCallback(
    XGL_DBG_MSG_CALLBACK_FUNCTION               pfnMsgCallback)
{
    return icd_logger_remove_callback(pfnMsgCallback);
}

ICD_EXPORT XGL_RESULT XGLAPI xglDbgSetGlobalOption(
    XGL_DBG_GLOBAL_OPTION                       dbgOption,
    size_t                                      dataSize,
    const void*                                 pData)
{
    XGL_RESULT res = XGL_SUCCESS;

    if (dataSize == 0)
        return XGL_ERROR_INVALID_VALUE;

    switch (dbgOption) {
    case XGL_DBG_OPTION_DEBUG_ECHO_ENABLE:
    case XGL_DBG_OPTION_BREAK_ON_ERROR:
    case XGL_DBG_OPTION_BREAK_ON_WARNING:
        res = icd_logger_set_bool(dbgOption, *((const bool *) pData));
        break;
    default:
        res = XGL_ERROR_INVALID_VALUE;
        break;
    }

    return res;
}
