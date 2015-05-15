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

#ifndef LOADER_H
#define LOADER_H

#include <vulkan.h>
#include <vkDbg.h>
#include <vk_wsi_lunarg.h>
#include <vkLayer.h>
#include <vkIcd.h>
#include <assert.h>

#if defined(__GNUC__) && __GNUC__ >= 4
#  define LOADER_EXPORT __attribute__((visibility("default")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
#  define LOADER_EXPORT __attribute__((visibility("default")))
#else
#  define LOADER_EXPORT
#endif

#define MAX_LAYER_LIBRARIES 64
#define MAX_GPUS_FOR_LAYER 16

struct loader_scanned_layers {
    char *name;
    uint32_t extension_count;
    struct extension_property *extensions;
};

struct loader_layers {
    loader_platform_dl_handle lib_handle;
    char name[256];
};

struct loader_icd {
    const struct loader_scanned_icds *scanned_icds;

    VkLayerDispatchTable *loader_dispatch;
    uint32_t layer_count[MAX_GPUS_FOR_LAYER];
    struct loader_layers layer_libs[MAX_GPUS_FOR_LAYER][MAX_LAYER_LIBRARIES];
    VkBaseLayerObject *wrappedGpus[MAX_GPUS_FOR_LAYER];
    uint32_t gpu_count;
    VkBaseLayerObject *gpus;
    VkInstance instance;       // instance object from the icd
    PFN_vkGetProcAddr GetProcAddr;
    PFN_vkDestroyInstance DestroyInstance;
    PFN_vkEnumeratePhysicalDevices EnumeratePhysicalDevices;
    PFN_vkGetPhysicalDeviceInfo GetPhysicalDeviceInfo;
    PFN_vkCreateDevice CreateDevice;
    PFN_vkGetPhysicalDeviceExtensionInfo GetPhysicalDeviceExtensionInfo;
    PFN_vkEnumerateLayers EnumerateLayers;
    PFN_vkGetMultiDeviceCompatibility GetMultiDeviceCompatibility;
    PFN_vkDbgRegisterMsgCallback DbgRegisterMsgCallback;
    PFN_vkDbgUnregisterMsgCallback DbgUnregisterMsgCallback;
    PFN_vkDbgSetGlobalOption DbgSetGlobalOption;
    PFN_vkGetDisplayInfoWSI GetDisplayInfoWSI;
    struct loader_icd *next;
};

struct loader_instance {
    VkLayerInstanceDispatchTable *disp; // must be first entry in structure

    uint32_t layer_count;
    struct loader_layers layer_libs[MAX_LAYER_LIBRARIES];
    VkBaseLayerObject *wrappedInstance;
    uint32_t total_gpu_count;
    struct loader_icd *icds;
    struct loader_instance *next;
    uint32_t  extension_count;
    char **extension_names;
};

struct loader_struct {
    struct loader_instance *instances;
    bool icds_scanned;
    struct loader_scanned_icds *scanned_icd_list;
    bool layer_scanned;
    char *layer_dirs;
    unsigned int scanned_layer_count;
    struct loader_scanned_layers scanned_layers[MAX_LAYER_LIBRARIES];
    size_t scanned_ext_list_capacity;
    uint32_t scanned_ext_list_count;      // coalesced from all layers/drivers
    struct extension_property **scanned_ext_list;
};

static inline void loader_set_dispatch(VkObject obj, const void *data)
{
    *((const void **) obj) = data;
}

static inline VkLayerDispatchTable *loader_get_dispatch(const VkObject obj)
{
    return *((VkLayerDispatchTable **) obj);
}

static inline VkLayerInstanceDispatchTable *loader_get_instance_dispatch(const VkObject obj)
{
    return *((VkLayerInstanceDispatchTable **) obj);
}

static inline void loader_init_dispatch(VkObject obj, const void *data)
{
#ifdef DEBUG
    assert(valid_loader_magic_value(obj) &&
            "Incompatible ICD, first dword must be initialized to ICD_LOADER_MAGIC. See loader/README.md for details.");
#endif

    loader_set_dispatch(obj, data);
}

/* global variables used across files */
extern struct loader_struct loader;
extern LOADER_PLATFORM_THREAD_ONCE_DEFINITION(once_icd);
extern LOADER_PLATFORM_THREAD_ONCE_DEFINITION(once_layer);
extern LOADER_PLATFORM_THREAD_ONCE_DEFINITION(once_exts);
extern VkLayerInstanceDispatchTable instance_disp;

/* instance layer chain termination entrypoint definitions */
VkResult loader_CreateInstance(
        const VkInstanceCreateInfo*             pCreateInfo,
        VkInstance*                             pInstance);

VkResult loader_DestroyInstance(
        VkInstance                              instance);

VkResult loader_EnumeratePhysicalDevices(
        VkInstance                              instance,
        uint32_t*                               pPhysicalDeviceCount,
        VkPhysicalDevice*                       pPhysicalDevices);
VkResult loader_GetPhysicalDeviceInfo(
        VkPhysicalDevice                        gpu,
        VkPhysicalDeviceInfoType                infoType,
        size_t*                                 pDataSize,
        void*                                   pData);

VkResult loader_CreateDevice(
        VkPhysicalDevice                        gpu,
        const VkDeviceCreateInfo*               pCreateInfo,
        VkDevice*                               pDevice);

VkResult VKAPI loader_GetGlobalExtensionInfo(
        VkExtensionInfoType                     infoType,
        uint32_t                                extensionIndex,
        size_t*                                 pDataSize,
        void*                                   pData);

VkResult loader_GetPhysicalDeviceExtensionInfo(
        VkPhysicalDevice                        gpu,
        VkExtensionInfoType                     infoType,
        uint32_t                                extensionIndex,
        size_t*                                 pDataSize,
        void*                                   pData);

VkResult loader_EnumerateLayers(
        VkPhysicalDevice                        gpu,
        size_t                                  maxStringSize,
        size_t*                                 pLayerCount,
        char* const*                            pOutLayers,
        void*                                   pReserved);

VkResult loader_GetMultiDeviceCompatibility(
        VkPhysicalDevice                        gpu0,
        VkPhysicalDevice                        gpu1,
        VkPhysicalDeviceCompatibilityInfo*      pInfo);

VkResult loader_DbgRegisterMsgCallback(
        VkInstance                              instance,
        VK_DBG_MSG_CALLBACK_FUNCTION            pfnMsgCallback,
        void*                                   pUserData);

VkResult loader_DbgUnregisterMsgCallback(
        VkInstance                              instance,
        VK_DBG_MSG_CALLBACK_FUNCTION            pfnMsgCallback);

VkResult loader_DbgSetGlobalOption(
        VkInstance                              instance,
        VK_DBG_GLOBAL_OPTION                    dbgOption,
        size_t                                  dataSize,
        const void*                             pData);

VkResult loader_GetDisplayInfoWSI(
        VkDisplayWSI                            display,
        VkDisplayInfoTypeWSI                    infoType,
        size_t*                                 pDataSize,
        void*                                   pData);

/* function definitions */
bool loader_is_extension_scanned(const char *name);
void loader_icd_scan(void);
void layer_lib_scan(void);
void loader_coalesce_extensions(void);
struct loader_icd * loader_get_icd(const VkBaseLayerObject *gpu,
                                   uint32_t *gpu_index);
uint32_t loader_activate_instance_layers(struct loader_instance *inst);
#endif /* LOADER_H */
