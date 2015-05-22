/*
 * Vulkan
 *
 * Copyright (C) 2015 LunarG, Inc.
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
 *   Jon Ashburn <jon@lunarg.com>
 *   Courtney Goeltzenleuchter <courtney@lunarg.com>
 */

#include <string.h>
#include "loader_platform.h"
#include "loader.h"
#include "wsi_lunarg.h"

/************  Trampoline entrypoints *******************/
/* since one entrypoint is instance level will make available all entrypoints */
VkResult VKAPI wsi_lunarg_GetDisplayInfoWSI(
        VkDisplayWSI                            display,
        VkDisplayInfoTypeWSI                    infoType,
        size_t*                                 pDataSize,
        void*                                   pData)
{
    const VkLayerInstanceDispatchTable *disp;

    disp = loader_get_instance_dispatch(display);

    return disp->GetDisplayInfoWSI(display, infoType, pDataSize, pData);
}

VkResult wsi_lunarg_CreateSwapChainWSI(
        VkDevice                                device,
        const VkSwapChainCreateInfoWSI*         pCreateInfo,
        VkSwapChainWSI*                         pSwapChain)
{
    const VkLayerDispatchTable *disp;
    VkResult res;

    disp = loader_get_dispatch(device);

    res = disp->CreateSwapChainWSI(device, pCreateInfo, pSwapChain);
    if (res == VK_SUCCESS) {
        loader_init_dispatch(*pSwapChain, disp);
    }

    return res;
}

VkResult wsi_lunarg_DestroySwapChainWSI(
        VkSwapChainWSI                          swapChain)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(swapChain);

    return disp->DestroySwapChainWSI(swapChain);
}

static VkResult wsi_lunarg_GetSwapChainInfoWSI(
        VkSwapChainWSI                          swapChain,
        VkSwapChainInfoTypeWSI                  infoType,
        size_t*                                 pDataSize,
        void*                                   pData)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(swapChain);

    return disp->GetSwapChainInfoWSI(swapChain, infoType, pDataSize, pData);
}

static VkResult wsi_lunarg_QueuePresentWSI(
        VkQueue                                 queue,
        const VkPresentInfoWSI*                 pPresentInfo)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(queue);

    return disp->QueuePresentWSI(queue, pPresentInfo);
}

/************  loader instance chain termination entrypoints ***************/
VkResult loader_GetDisplayInfoWSI(
        VkDisplayWSI                            display,
        VkDisplayInfoTypeWSI                    infoType,
        size_t*                                 pDataSize,
        void*                                   pData)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd((const VkBaseLayerObject *) display, &gpu_index);
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->GetDisplayInfoWSI)
        res = icd->GetDisplayInfoWSI(display, infoType, pDataSize, pData);

    return res;
}


/************ extension enablement ***************/
static bool wsi_enabled = false;

struct ext_props {
    uint32_t version;
    const char * const name;
};

#define WSI_LUNARG_EXT_ARRAY_SIZE 1
static const struct ext_props wsi_lunarg_exts[WSI_LUNARG_EXT_ARRAY_SIZE] = {
    {VK_WSI_LUNARG_REVISION, VK_WSI_LUNARG_EXTENSION_NAME},
};

void wsi_lunarg_register_extensions(
        const VkInstanceCreateInfo*             pCreateInfo)
{
    uint32_t i, ext_idx;

    for (i = 0; i < pCreateInfo->extensionCount; i++) {
        for (ext_idx = 0; ext_idx < WSI_LUNARG_EXT_ARRAY_SIZE; ext_idx++) {
            /* TODO: Should we include version number as well as extension name? */
            if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], wsi_lunarg_exts[ext_idx].name) == 0) {
                /* Found a matching extension name, mark it enabled */
                wsi_enabled = true;
            }

        }
    }

}

void *wsi_lunarg_GetInstanceProcAddr(
        VkInstance                              instance,
        const char*                             pName)
{
    if (instance == VK_NULL_HANDLE)
        return NULL;

    if (wsi_enabled == false)
	return NULL;

    /* since two of these entrypoints must be loader handled will report all */
    if (!strcmp(pName, "vkGetDisplayInfoWSI"))
        return (void*) wsi_lunarg_GetDisplayInfoWSI;
    if (!strcmp(pName, "vkCreateSwapChainWSI"))
        return (void*) wsi_lunarg_CreateSwapChainWSI;
    if (!strcmp(pName, "vkDestroySwapChainWSI"))
        return (void*) wsi_lunarg_DestroySwapChainWSI;
    if (!strcmp(pName, "vkGetSwapChainInfoWSI"))
        return (void*) wsi_lunarg_GetSwapChainInfoWSI;
    if (!strcmp(pName, "vkQueuePresentWSI"))
        return (void*) wsi_lunarg_QueuePresentWSI;

    return NULL;
}

void *wsi_lunarg_GetDeviceProcAddr(
        VkDevice                                device,
        const char*                             name)
{
    if (device == VK_NULL_HANDLE)
        return NULL;

    if (wsi_enabled == false)
	    return NULL;

    /* only handle device entrypoints that are loader special cases */
    if (!strcmp(name, "vkCreateSwapChainWSI"))
        return (void*) wsi_lunarg_CreateSwapChainWSI;

    return NULL;
}
