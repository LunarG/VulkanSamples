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
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#ifdef _WIN32
#include <Windows.h>
#include <fcntl.h>
#include <io.h>
#endif

#include "vk_ext_khr_swapchain.h"
#include "vk_ext_khr_device_swapchain.h"

#include <vulkan.h>

#define ERR(err) printf("%s:%d: failed with %s\n", \
    __FILE__, __LINE__, vk_result_string(err));

#ifdef _WIN32

#define snprintf _snprintf

bool consoleCreated = false;

#define WAIT_FOR_CONSOLE_DESTROY \
    do { \
        if (consoleCreated) \
            Sleep(INFINITE); \
    } while (0)
#else
    #define WAIT_FOR_CONSOLE_DESTROY
#endif


#define ERR_EXIT(err) \
    do { \
        ERR(err); \
        fflush(stdout); \
        WAIT_FOR_CONSOLE_DESTROY; \
        exit(-1); \
   } while (0)

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define MAX_GPUS 8

#define MAX_QUEUE_TYPES 5
#define APP_SHORT_NAME "vulkaninfo"

struct app_gpu;

struct app_dev {
    struct app_gpu *gpu; /* point back to the GPU */

    VkDevice obj;


    VkFormatProperties format_props[VK_FORMAT_NUM];
};

struct layer_extension_list {
    VkLayerProperties layer_properties;
    uint32_t extension_count;
    VkExtensionProperties *extension_properties;
};

struct app_instance {
    VkInstance  instance;
    uint32_t global_layer_count;
    struct layer_extension_list *global_layers;
    uint32_t global_extension_count;
    VkExtensionProperties *global_extensions;
};

struct app_gpu {
    uint32_t id;
    VkPhysicalDevice obj;

    VkPhysicalDeviceProperties props;

    uint32_t queue_count;
    VkQueueFamilyProperties *queue_props;
    VkDeviceQueueCreateInfo *queue_reqs;

    VkPhysicalDeviceMemoryProperties memory_props;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDevice limits;

    uint32_t device_layer_count;
    struct layer_extension_list *device_layers;

    uint32_t device_extension_count;
    VkExtensionProperties *device_extensions;

    struct app_dev dev;
};

static const char *vk_result_string(VkResult err)
{
    switch (err) {
#define STR(r) case r: return #r
    STR(VK_SUCCESS);
    STR(VK_NOT_READY);
    STR(VK_TIMEOUT);
    STR(VK_EVENT_SET);
    STR(VK_EVENT_RESET);
    STR(VK_ERROR_INITIALIZATION_FAILED);
    STR(VK_ERROR_OUT_OF_HOST_MEMORY);
    STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
    STR(VK_ERROR_DEVICE_LOST);
    STR(VK_ERROR_LAYER_NOT_PRESENT);
    STR(VK_ERROR_EXTENSION_NOT_PRESENT);
    STR(VK_ERROR_MEMORY_MAP_FAILED);
    STR(VK_ERROR_INCOMPATIBLE_DRIVER);
#undef STR
    default: return "UNKNOWN_RESULT";
    }
}

static const char *vk_physical_device_type_string(VkPhysicalDeviceType type)
{
    switch (type) {
#define STR(r) case VK_PHYSICAL_DEVICE_TYPE_ ##r: return #r
    STR(OTHER);
    STR(INTEGRATED_GPU);
    STR(DISCRETE_GPU);
    STR(VIRTUAL_GPU);
#undef STR
    default: return "UNKNOWN_DEVICE";
    }
}

static const char *vk_format_string(VkFormat fmt)
{
    switch (fmt) {
#define STR(r) case VK_FORMAT_ ##r: return #r
    STR(UNDEFINED);
    STR(R4G4_UNORM);
    STR(R4G4_USCALED);
    STR(R4G4B4A4_UNORM);
    STR(R4G4B4A4_USCALED);
    STR(R5G6B5_UNORM);
    STR(R5G6B5_USCALED);
    STR(R5G5B5A1_UNORM);
    STR(R5G5B5A1_USCALED);
    STR(R8_UNORM);
    STR(R8_SNORM);
    STR(R8_USCALED);
    STR(R8_SSCALED);
    STR(R8_UINT);
    STR(R8_SINT);
    STR(R8_SRGB);
    STR(R8G8_UNORM);
    STR(R8G8_SNORM);
    STR(R8G8_USCALED);
    STR(R8G8_SSCALED);
    STR(R8G8_UINT);
    STR(R8G8_SINT);
    STR(R8G8_SRGB);
    STR(R8G8B8_UNORM);
    STR(R8G8B8_SNORM);
    STR(R8G8B8_USCALED);
    STR(R8G8B8_SSCALED);
    STR(R8G8B8_UINT);
    STR(R8G8B8_SINT);
    STR(R8G8B8_SRGB);
    STR(R8G8B8A8_UNORM);
    STR(R8G8B8A8_SNORM);
    STR(R8G8B8A8_USCALED);
    STR(R8G8B8A8_SSCALED);
    STR(R8G8B8A8_UINT);
    STR(R8G8B8A8_SINT);
    STR(R8G8B8A8_SRGB);
    STR(R10G10B10A2_UNORM);
    STR(R10G10B10A2_SNORM);
    STR(R10G10B10A2_USCALED);
    STR(R10G10B10A2_SSCALED);
    STR(R10G10B10A2_UINT);
    STR(R10G10B10A2_SINT);
    STR(R16_UNORM);
    STR(R16_SNORM);
    STR(R16_USCALED);
    STR(R16_SSCALED);
    STR(R16_UINT);
    STR(R16_SINT);
    STR(R16_SFLOAT);
    STR(R16G16_UNORM);
    STR(R16G16_SNORM);
    STR(R16G16_USCALED);
    STR(R16G16_SSCALED);
    STR(R16G16_UINT);
    STR(R16G16_SINT);
    STR(R16G16_SFLOAT);
    STR(R16G16B16_UNORM);
    STR(R16G16B16_SNORM);
    STR(R16G16B16_USCALED);
    STR(R16G16B16_SSCALED);
    STR(R16G16B16_UINT);
    STR(R16G16B16_SINT);
    STR(R16G16B16_SFLOAT);
    STR(R16G16B16A16_UNORM);
    STR(R16G16B16A16_SNORM);
    STR(R16G16B16A16_USCALED);
    STR(R16G16B16A16_SSCALED);
    STR(R16G16B16A16_UINT);
    STR(R16G16B16A16_SINT);
    STR(R16G16B16A16_SFLOAT);
    STR(R32_UINT);
    STR(R32_SINT);
    STR(R32_SFLOAT);
    STR(R32G32_UINT);
    STR(R32G32_SINT);
    STR(R32G32_SFLOAT);
    STR(R32G32B32_UINT);
    STR(R32G32B32_SINT);
    STR(R32G32B32_SFLOAT);
    STR(R32G32B32A32_UINT);
    STR(R32G32B32A32_SINT);
    STR(R32G32B32A32_SFLOAT);
    STR(R64_SFLOAT);
    STR(R64G64_SFLOAT);
    STR(R64G64B64_SFLOAT);
    STR(R64G64B64A64_SFLOAT);
    STR(R11G11B10_UFLOAT);
    STR(R9G9B9E5_UFLOAT);
    STR(D16_UNORM);
    STR(D24_UNORM_X8);
    STR(D32_SFLOAT);
    STR(S8_UINT);
    STR(D16_UNORM_S8_UINT);
    STR(D24_UNORM_S8_UINT);
    STR(D32_SFLOAT_S8_UINT);
    STR(BC1_RGB_UNORM);
    STR(BC1_RGB_SRGB);
    STR(BC2_UNORM);
    STR(BC2_SRGB);
    STR(BC3_UNORM);
    STR(BC3_SRGB);
    STR(BC4_UNORM);
    STR(BC4_SNORM);
    STR(BC5_UNORM);
    STR(BC5_SNORM);
    STR(BC6H_UFLOAT);
    STR(BC6H_SFLOAT);
    STR(BC7_UNORM);
    STR(BC7_SRGB);
    STR(ETC2_R8G8B8_UNORM);
    STR(ETC2_R8G8B8A1_UNORM);
    STR(ETC2_R8G8B8A8_UNORM);
    STR(EAC_R11_UNORM);
    STR(EAC_R11_SNORM);
    STR(EAC_R11G11_UNORM);
    STR(EAC_R11G11_SNORM);
    STR(ASTC_4x4_UNORM);
    STR(ASTC_4x4_SRGB);
    STR(ASTC_5x4_UNORM);
    STR(ASTC_5x4_SRGB);
    STR(ASTC_5x5_UNORM);
    STR(ASTC_5x5_SRGB);
    STR(ASTC_6x5_UNORM);
    STR(ASTC_6x5_SRGB);
    STR(ASTC_6x6_UNORM);
    STR(ASTC_6x6_SRGB);
    STR(ASTC_8x5_UNORM);
    STR(ASTC_8x5_SRGB);
    STR(ASTC_8x6_UNORM);
    STR(ASTC_8x6_SRGB);
    STR(ASTC_8x8_UNORM);
    STR(ASTC_8x8_SRGB);
    STR(ASTC_10x5_UNORM);
    STR(ASTC_10x5_SRGB);
    STR(ASTC_10x6_UNORM);
    STR(ASTC_10x6_SRGB);
    STR(ASTC_10x8_UNORM);
    STR(ASTC_10x8_SRGB);
    STR(ASTC_10x10_UNORM);
    STR(ASTC_10x10_SRGB);
    STR(ASTC_12x10_UNORM);
    STR(ASTC_12x10_SRGB);
    STR(ASTC_12x12_UNORM);
    STR(ASTC_12x12_SRGB);
    STR(B5G6R5_UNORM);
    STR(B5G6R5_USCALED);
    STR(B8G8R8_UNORM);
    STR(B8G8R8_SNORM);
    STR(B8G8R8_USCALED);
    STR(B8G8R8_SSCALED);
    STR(B8G8R8_UINT);
    STR(B8G8R8_SINT);
    STR(B8G8R8_SRGB);
    STR(B8G8R8A8_UNORM);
    STR(B8G8R8A8_SNORM);
    STR(B8G8R8A8_USCALED);
    STR(B8G8R8A8_SSCALED);
    STR(B8G8R8A8_UINT);
    STR(B8G8R8A8_SINT);
    STR(B8G8R8A8_SRGB);
    STR(B10G10R10A2_UNORM);
    STR(B10G10R10A2_SNORM);
    STR(B10G10R10A2_USCALED);
    STR(B10G10R10A2_SSCALED);
    STR(B10G10R10A2_UINT);
    STR(B10G10R10A2_SINT);
#undef STR
    default: return "UNKNOWN_FORMAT";
    }
}

static void app_dev_init_formats(struct app_dev *dev)
{
    VkFormat f;

    for (f = 0; f < VK_FORMAT_NUM; f++) {
        const VkFormat fmt = f;

        vkGetPhysicalDeviceFormatProperties(dev->gpu->obj, fmt, &dev->format_props[f]);
    }
}

static void extract_version(uint32_t version, uint32_t *major, uint32_t *minor, uint32_t *patch)
{
    *major = version >> 22;
    *minor = (version >> 12) & 0x3ff;
    *patch = version & 0xfff;
}

static void app_get_physical_device_layer_extensions(
        struct app_gpu *gpu,
        char *layer_name,
        uint32_t *extension_count,
        VkExtensionProperties **extension_properties)
{
    VkResult err;
    uint32_t ext_count = 0;
    VkExtensionProperties *ext_ptr = NULL;

    /* repeat get until VK_INCOMPLETE goes away */
    do {
        err = vkEnumerateDeviceExtensionProperties(gpu->obj, layer_name, &ext_count, NULL);
        assert(!err);

        if (ext_ptr) {
            free(ext_ptr);
        }
        ext_ptr = malloc(ext_count * sizeof(VkExtensionProperties));
        err = vkEnumerateDeviceExtensionProperties(gpu->obj, layer_name, &ext_count, ext_ptr);
    } while (err == VK_INCOMPLETE);
    assert(!err);

    *extension_count = ext_count;
    *extension_properties = ext_ptr;
}

static void app_dev_init(struct app_dev *dev, struct app_gpu *gpu)
{
    VkDeviceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .requestedQueueCount = 0,
        .pRequestedQueues = NULL,
        .layerCount = 0,
        .ppEnabledLayerNames = NULL,
        .extensionCount = 0,
        .ppEnabledExtensionNames = NULL,
    };
    VkResult U_ASSERT_ONLY err;
    // Extensions to enable
    static const char *known_extensions[] = {
        VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME,
    };

    uint32_t count = 0;

    /* Scan layers */
    VkLayerProperties *device_layer_properties = NULL;
    struct layer_extension_list *device_layers = NULL;

    do {
        err = vkEnumerateDeviceLayerProperties(gpu->obj, &count, NULL);
        assert(!err);

        if (device_layer_properties) {
            free(device_layer_properties);
        }
        device_layer_properties = malloc(sizeof(VkLayerProperties) * count);
        assert(device_layer_properties);

        if (device_layers) {
            free(device_layers);
        }
        device_layers = malloc(sizeof(struct layer_extension_list) * count);
        assert(device_layers);

        err = vkEnumerateDeviceLayerProperties(gpu->obj, &count, device_layer_properties);
    } while (err == VK_INCOMPLETE);
    assert(!err);

    gpu->device_layer_count = count;
    gpu->device_layers = device_layers;

    for (uint32_t i = 0; i < gpu->device_layer_count; i++) {
        VkLayerProperties *src_info = &device_layer_properties[i];
        struct layer_extension_list *dst_info = &gpu->device_layers[i];
        memcpy(&dst_info->layer_properties, src_info, sizeof(VkLayerProperties));

        /* Save away layer extension info for report */
        app_get_physical_device_layer_extensions(
                    gpu,
                    src_info->layerName,
                    &dst_info->extension_count,
                    &dst_info->extension_properties);
    }
    free(device_layer_properties);

    app_get_physical_device_layer_extensions(
                gpu,
                NULL,
                &gpu->device_extension_count,
                &gpu->device_extensions);

    fflush(stdout);

    uint32_t enabled_extension_count = 0;
    uint32_t known_extension_count = ARRAY_SIZE(known_extensions);

    for (uint32_t i = 0; i < known_extension_count; i++) {
        VkBool32 extension_found = 0;
        for (uint32_t j = 0; j < gpu->device_extension_count; j++) {
            VkExtensionProperties *ext_prop = &gpu->device_extensions[j];
            if (!strcmp(known_extensions[i], ext_prop->extName)) {

                extension_found = 1;
                enabled_extension_count++;
            }
        }
        if (!extension_found) {
            printf("Cannot find extension: %s\n", known_extensions[i]);
            ERR_EXIT(VK_ERROR_EXTENSION_NOT_PRESENT);
        }
    }

    /* request all queues */
    info.requestedQueueCount = gpu->queue_count;
    info.pRequestedQueues = gpu->queue_reqs;

    info.layerCount = 0;
    info.ppEnabledLayerNames = NULL;
    info.extensionCount = enabled_extension_count;
    info.ppEnabledExtensionNames = (const char*const*) known_extensions;
    dev->gpu = gpu;
    err = vkCreateDevice(gpu->obj, &info, &dev->obj);
    if (err)
        ERR_EXIT(err);

}

static void app_dev_destroy(struct app_dev *dev)
{
    vkDestroyDevice(dev->obj);
}

static void app_get_global_layer_extensions(
        char *layer_name,
        uint32_t *extension_count,
        VkExtensionProperties **extension_properties)
{
    VkResult err;
    uint32_t ext_count = 0;
    VkExtensionProperties *ext_ptr = NULL;

    /* repeat get until VK_INCOMPLETE goes away */
    do {
        err = vkEnumerateInstanceExtensionProperties(layer_name, &ext_count, NULL);
        assert(!err);

        if (ext_ptr) {
            free(ext_ptr);
        }
        ext_ptr = malloc(ext_count * sizeof(VkExtensionProperties));
        err = vkEnumerateInstanceExtensionProperties(layer_name, &ext_count, ext_ptr);
    } while (err == VK_INCOMPLETE);
    assert(!err);

    *extension_count = ext_count;
    *extension_properties = ext_ptr;
}

static void app_create_instance(struct app_instance *inst)
{
    const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pAppName = APP_SHORT_NAME,
        .appVersion = 1,
        .pEngineName = APP_SHORT_NAME,
        .engineVersion = 1,
        .apiVersion = VK_API_VERSION,
    };
    VkInstanceCreateInfo inst_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .pAppInfo = &app_info,
        .pAllocCb = NULL,
        .layerCount = 0,
        .ppEnabledLayerNames = NULL,
        .extensionCount = 0,
        .ppEnabledExtensionNames = NULL,
    };
    VkResult U_ASSERT_ONLY err;
    // Global Extensions to enable
    static char *known_extensions[] = {
        "VK_EXT_KHR_swapchain",
    };

    uint32_t global_extension_count = 0;
    uint32_t count = 0;

    /* Scan layers */
    VkLayerProperties *global_layer_properties = NULL;
    struct layer_extension_list *global_layers = NULL;

    do {
        err = vkEnumerateInstanceLayerProperties(&count, NULL);
        assert(!err);

        if (global_layer_properties) {
            free(global_layer_properties);
        }
        global_layer_properties = malloc(sizeof(VkLayerProperties) * count);
        assert(global_layer_properties);

        if (global_layers) {
            free(global_layers);
        }
        global_layers = malloc(sizeof(struct layer_extension_list) * count);
        assert(global_layers);

        err = vkEnumerateInstanceLayerProperties(&count, global_layer_properties);
    } while (err == VK_INCOMPLETE);
    assert(!err);

    inst->global_layer_count = count;
    inst->global_layers = global_layers;

    for (uint32_t i = 0; i < inst->global_layer_count; i++) {
        VkLayerProperties *src_info = &global_layer_properties[i];
        struct layer_extension_list *dst_info = &inst->global_layers[i];
        memcpy(&dst_info->layer_properties, src_info, sizeof(VkLayerProperties));

        /* Save away layer extension info for report */
        app_get_global_layer_extensions(
                    src_info->layerName,
                    &dst_info->extension_count,
                    &dst_info->extension_properties);
    }
    free(global_layer_properties);

    /* Collect global extensions */
    inst->global_extension_count = 0;
    app_get_global_layer_extensions(
                NULL,
                &inst->global_extension_count,
                &inst->global_extensions);

    for (uint32_t i = 0; i < ARRAY_SIZE(known_extensions); i++) {
        VkBool32 extension_found = 0;
        for (uint32_t j = 0; j < inst->global_extension_count; j++) {
            VkExtensionProperties *extension_prop = &inst->global_extensions[j];
            if (!strcmp(known_extensions[i], extension_prop->extName)) {

                extension_found = 1;
                global_extension_count++;
            }
        }
        if (!extension_found) {
            printf("Cannot find extension: %s\n", known_extensions[i]);
            ERR_EXIT(VK_ERROR_EXTENSION_NOT_PRESENT);
        }
    }

    inst_info.extensionCount = global_extension_count;
    inst_info.ppEnabledExtensionNames = (const char * const *) known_extensions;

    err = vkCreateInstance(&inst_info, &inst->instance);
    if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
        printf("Cannot create Vulkan instance.\n");
        ERR_EXIT(err);
    } else if (err) {
        ERR_EXIT(err);
    }
}

static void app_destroy_instance(struct app_instance *inst)
{
    free(inst->global_extensions);
    vkDestroyInstance(inst->instance);
}


static void app_gpu_init(struct app_gpu *gpu, uint32_t id, VkPhysicalDevice obj)
{
    uint32_t i;

    memset(gpu, 0, sizeof(*gpu));

    gpu->id = id;
    gpu->obj = obj;

    vkGetPhysicalDeviceProperties(gpu->obj, &gpu->props);

    /* get queue count */
    vkGetPhysicalDeviceQueueFamilyProperties(gpu->obj, &gpu->queue_count, NULL);

    gpu->queue_props =
            malloc(sizeof(gpu->queue_props[0]) * gpu->queue_count);

    if (!gpu->queue_props)
        ERR_EXIT(VK_ERROR_OUT_OF_HOST_MEMORY);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu->obj, &gpu->queue_count, gpu->queue_props);

    /* set up queue requests */
    gpu->queue_reqs = malloc(sizeof(*gpu->queue_reqs) * gpu->queue_count);
    if (!gpu->queue_reqs)
        ERR_EXIT(VK_ERROR_OUT_OF_HOST_MEMORY);
    for (i = 0; i < gpu->queue_count; i++) {
        float *queue_priorities = malloc(gpu->queue_props[i].queueCount * sizeof(float));
        memset(queue_priorities, 0, gpu->queue_props[i].queueCount * sizeof(float));
        gpu->queue_reqs[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        gpu->queue_reqs[i].pNext = NULL;
        gpu->queue_reqs[i].queueFamilyIndex = i;
        gpu->queue_reqs[i].queueCount = gpu->queue_props[i].queueCount;
        gpu->queue_reqs[i].pQueuePriorities = queue_priorities;
    }

    vkGetPhysicalDeviceMemoryProperties(gpu->obj, &gpu->memory_props);

    vkGetPhysicalDeviceFeatures(gpu->obj, &gpu->features);

    app_dev_init(&gpu->dev, gpu);
    app_dev_init_formats(&gpu->dev);
}

static void app_gpu_destroy(struct app_gpu *gpu)
{
    app_dev_destroy(&gpu->dev);
    free(gpu->device_extensions);
    free(gpu->queue_reqs);
    for (uint32_t i = 0; i < gpu->queue_count; i++) {
        free((void *) gpu->queue_reqs[i].pQueuePriorities);
    }
    free(gpu->queue_props);
}

static void app_dev_dump_format_props(const struct app_dev *dev, VkFormat fmt)
{
    const VkFormatProperties *props = &dev->format_props[fmt];
    struct {
        const char *name;
        VkFlags flags;
    } features[3];
    uint32_t i;

    features[0].name  = "linearTiling   FormatFeatureFlags";
    features[0].flags = props->linearTilingFeatures;
    features[1].name  = "optimalTiling  FormatFeatureFlags";
    features[1].flags = props->optimalTilingFeatures;
    features[2].name  = "bufferFeatures FormatFeatureFlags";
    features[2].flags = props->bufferFeatures;

    printf("\nFORMAT_%s:", vk_format_string(fmt));
    for (i = 0; i < ARRAY_SIZE(features); i++) {
        printf("\n\t%s:", features[i].name);
        if (features[i].flags == 0) {
            printf("\n\t\tNone");
        } else {
            printf("%s%s%s%s%s%s%s%s%s%s%s%s",
               ((features[i].flags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)               ? "\n\t\tVK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT"               : ""),
               ((features[i].flags & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)               ? "\n\t\tVK_FORMAT_FEATURE_STORAGE_IMAGE_BIT"               : ""),
               ((features[i].flags & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT)        ? "\n\t\tVK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT"        : ""),
               ((features[i].flags & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)            ? "\n\t\tVK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT"            : ""),
               ((features[i].flags & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT)      ? "\n\t\tVK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT"      : ""),
               ((features[i].flags & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)    ? "\n\t\tVK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT"    : ""),
               ((features[i].flags & VK_FORMAT_FEATURE_BLIT_SOURCE_BIT)                 ? "\n\t\tVK_FORMAT_FEATURE_BLIT_SOURCE_BIT"                 : ""),
               ((features[i].flags & VK_FORMAT_FEATURE_BLIT_DESTINATION_BIT)            ? "\n\t\tVK_FORMAT_FEATURE_BLIT_DESTINATION_BIT"            : ""),
               ((features[i].flags & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT)        ? "\n\t\tVK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT"        : ""),
               ((features[i].flags & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT)        ? "\n\t\tVK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT"        : ""),
               ((features[i].flags & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT) ? "\n\t\tVK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT" : ""),
               ((features[i].flags & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT)               ? "\n\t\tVK_FORMAT_FEATURE_VERTEX_BUFFER_BIT"               : ""));
        }
    }
    printf("\n");
}


static void
app_dev_dump(const struct app_dev *dev)
{
    VkFormat fmt;

    for (fmt = 0; fmt < VK_FORMAT_NUM; fmt++) {
        app_dev_dump_format_props(dev, fmt);
    }
}

#ifdef _WIN32
#define PRINTF_SIZE_T_SPECIFIER    "%Iu"
#else
#define PRINTF_SIZE_T_SPECIFIER    "%zu"
#endif

static void app_gpu_dump_features(const struct app_gpu *gpu)
{
    const VkPhysicalDeviceFeatures *features = &gpu->features;

    printf("VkPhysicalDeviceFeatures:\n");
    printf("=========================\n");

    printf("\trobustBufferAccess                      = %u\n", features->robustBufferAccess                     );
    printf("\tfullDrawIndexUint32                     = %u\n", features->fullDrawIndexUint32                    );
    printf("\timageCubeArray                          = %u\n", features->imageCubeArray                         );
    printf("\tindependentBlend                        = %u\n", features->independentBlend                       );
    printf("\tgeometryShader                          = %u\n", features->geometryShader                         );
    printf("\ttessellationShader                      = %u\n", features->tessellationShader                     );
    printf("\tsampleRateShading                       = %u\n", features->sampleRateShading                      );
    printf("\tdualSourceBlend                         = %u\n", features->dualSourceBlend                        );
    printf("\tlogicOp                                 = %u\n", features->logicOp                                );
    printf("\tmultiDrawIndirect                       = %u\n", features->multiDrawIndirect                      );
    printf("\tdepthClip                               = %u\n", features->depthClamp                              );
    printf("\tdepthBiasClamp                          = %u\n", features->depthBiasClamp                         );
    printf("\tfillModeNonSolid                        = %u\n", features->fillModeNonSolid                       );
    printf("\tdepthBounds                             = %u\n", features->depthBounds                            );
    printf("\twideLines                               = %u\n", features->wideLines                              );
    printf("\tlargePoints                             = %u\n", features->largePoints                            );
    printf("\ttextureCompressionETC2                  = %u\n", features->textureCompressionETC2                 );
    printf("\ttextureCompressionASTC_LDR              = %u\n", features->textureCompressionASTC_LDR             );
    printf("\ttextureCompressionBC                    = %u\n", features->textureCompressionBC                   );
    printf("\tpipelineStatisticsQuery                 = %u\n", features->pipelineStatisticsQuery                );
    printf("\tvertexSideEffects                       = %u\n", features->vertexSideEffects                      );
    printf("\ttessellationSideEffects                 = %u\n", features->tessellationSideEffects                );
    printf("\tgeometrySideEffects                     = %u\n", features->geometrySideEffects                    );
    printf("\tfragmentSideEffects                     = %u\n", features->fragmentSideEffects                    );
    printf("\tshaderTessellationPointSize             = %u\n", features->shaderTessellationPointSize            );
    printf("\tshaderGeometryPointSize                 = %u\n", features->shaderGeometryPointSize                );
    printf("\tshaderImageGatherExtended               = %u\n", features->shaderImageGatherExtended              );
    printf("\tshaderStorageImageExtendedFormats       = %u\n", features->shaderStorageImageExtendedFormats      );
    printf("\tshaderStorageImageMultisample           = %u\n", features->shaderStorageImageMultisample          );
    printf("\tshaderUniformBufferArrayDynamicIndexing = %u\n", features->shaderUniformBufferArrayDynamicIndexing);
    printf("\tshaderSampledImageArrayDynamicIndexing  = %u\n", features->shaderSampledImageArrayDynamicIndexing );
    printf("\tshaderStorageBufferArrayDynamicIndexing = %u\n", features->shaderStorageBufferArrayDynamicIndexing);
    printf("\tshaderStorageImageArrayDynamicIndexing  = %u\n", features->shaderStorageImageArrayDynamicIndexing );
    printf("\tshaderClipDistance                      = %u\n", features->shaderClipDistance                     );
    printf("\tshaderCullDistance                      = %u\n", features->shaderCullDistance                     );
    printf("\tshaderFloat64                           = %u\n", features->shaderFloat64                          );
    printf("\tshaderInt64                             = %u\n", features->shaderInt64                            );
    printf("\tshaderInt16                             = %u\n", features->shaderInt16                            );
    printf("\tshaderResourceResidency                 = %u\n", features->shaderResourceResidency                );
    printf("\tshaderResourceMinLOD                    = %u\n", features->shaderResourceMinLOD                   );
    printf("\talphaToOne                              = %u\n", features->alphaToOne                             );
    printf("\tsparseBinding                           = %u\n", features->sparseBinding                          );
    printf("\tsparseResidencyBuffer                   = %u\n", features->sparseResidencyBuffer                  );
    printf("\tsparseResidencyImage2D                  = %u\n", features->sparseResidencyImage2D                 );
    printf("\tsparseResidencyImage3D                  = %u\n", features->sparseResidencyImage3D                 );
    printf("\tsparseResidency2Samples                 = %u\n", features->sparseResidency2Samples                );
    printf("\tsparseResidency4Samples                 = %u\n", features->sparseResidency4Samples                );
    printf("\tsparseResidency8Samples                 = %u\n", features->sparseResidency8Samples                );
    printf("\tsparseResidency16Samples                = %u\n", features->sparseResidency16Samples               );
    printf("\tsparseResidencyAliased                  = %u\n", features->sparseResidencyAliased                 );
}

static void app_dump_sparse_props(const VkPhysicalDeviceSparseProperties *sparseProps)
{

    printf("\tVkPhysicalDeviceSparseProperties:\n");
    printf("\t---------------------------------\n");

    printf("\t\tresidencyStandard2DBlockShape     = %u\n", sparseProps->residencyStandard2DBlockShape    );
    printf("\t\tresidencyStandard2DMSBlockShape   = %u\n", sparseProps->residencyStandard2DMSBlockShape  );
    printf("\t\tresidencyStandard3DBlockShape     = %u\n", sparseProps->residencyStandard3DBlockShape    );
    printf("\t\tresidencyAlignedMipSize           = %u\n", sparseProps->residencyAlignedMipSize          );
    printf("\t\tresidencyNonResident              = %u\n", sparseProps->residencyNonResident             );
    printf("\t\tresidencyNonResidentStrict        = %u\n", sparseProps->residencyNonResidentStrict       );
}

static void app_dump_limits(const VkPhysicalDeviceLimits *limits)
{
    printf("\tVkPhysicalDeviceLimits:\n");
    printf("\t-----------------------\n");

    printf("\t\tmaxImageDimension1D                     = 0x%" PRIxLEAST32 "\n", limits->maxImageDimension1D                    );
    printf("\t\tmaxImageDimension2D                     = 0x%" PRIxLEAST32 "\n", limits->maxImageDimension2D                    );
    printf("\t\tmaxImageDimension3D                     = 0x%" PRIxLEAST32 "\n", limits->maxImageDimension3D                    );
    printf("\t\tmaxImageDimensionCube                   = 0x%" PRIxLEAST32 "\n", limits->maxImageDimensionCube                  );
    printf("\t\tmaxImageArrayLayers                     = 0x%" PRIxLEAST32 "\n", limits->maxImageArrayLayers                    );
    printf("\t\tsampleCounts                            = 0x%" PRIxLEAST32 "\n", limits->sampleCounts                           );
    printf("\t\tmaxTexelBufferSize                      = 0x%" PRIxLEAST32 "\n", limits->maxTexelBufferSize                     );
    printf("\t\tmaxUniformBufferRange                    = 0x%" PRIxLEAST32 "\n", limits->maxUniformBufferRange                 );
    printf("\t\tmaxStorageBufferRange                    = 0x%" PRIxLEAST32 "\n", limits->maxStorageBufferRange                 );
    printf("\t\tmaxPushConstantsSize                    = 0x%" PRIxLEAST32 "\n", limits->maxPushConstantsSize                   );
    printf("\t\tmaxMemoryAllocationCount                = 0x%" PRIxLEAST32 "\n", limits->maxMemoryAllocationCount               );
    printf("\t\tbufferImageGranularity                  = 0x%" PRIxLEAST64 "\n", limits->bufferImageGranularity                 );
    printf("\t\tsparseAddressSpaceSize                  = 0x%" PRIxLEAST64 "\n", limits->sparseAddressSpaceSize                 );
    printf("\t\tmaxBoundDescriptorSets                  = 0x%" PRIxLEAST32 "\n", limits->maxBoundDescriptorSets                 );
    printf("\t\tmaxPerStageDescriptorSamplers           = 0x%" PRIxLEAST32 "\n", limits->maxPerStageDescriptorSamplers          );
    printf("\t\tmaxPerStageDescriptorUniformBuffers     = 0x%" PRIxLEAST32 "\n", limits->maxPerStageDescriptorUniformBuffers    );
    printf("\t\tmaxPerStageDescriptorStorageBuffers     = 0x%" PRIxLEAST32 "\n", limits->maxPerStageDescriptorStorageBuffers    );
    printf("\t\tmaxPerStageDescriptorSampledImages      = 0x%" PRIxLEAST32 "\n", limits->maxPerStageDescriptorSampledImages     );
    printf("\t\tmaxPerStageDescriptorStorageImages      = 0x%" PRIxLEAST32 "\n", limits->maxPerStageDescriptorStorageImages     );
    printf("\t\tmaxDescriptorSetSamplers                = 0x%" PRIxLEAST32 "\n", limits->maxDescriptorSetSamplers               );
    printf("\t\tmaxDescriptorSetUniformBuffers          = 0x%" PRIxLEAST32 "\n", limits->maxDescriptorSetUniformBuffers         );
    printf("\t\tmaxDescriptorSetUniformBuffersDynamic   = 0x%" PRIxLEAST32 "\n", limits->maxDescriptorSetUniformBuffersDynamic  );
    printf("\t\tmaxDescriptorSetStorageBuffers          = 0x%" PRIxLEAST32 "\n", limits->maxDescriptorSetStorageBuffers         );
    printf("\t\tmaxDescriptorSetStorageBuffersDynamic   = 0x%" PRIxLEAST32 "\n", limits->maxDescriptorSetStorageBuffersDynamic  );
    printf("\t\tmaxDescriptorSetSampledImages           = 0x%" PRIxLEAST32 "\n", limits->maxDescriptorSetSampledImages          );
    printf("\t\tmaxDescriptorSetStorageImages           = 0x%" PRIxLEAST32 "\n", limits->maxDescriptorSetStorageImages          );
    printf("\t\tmaxVertexInputAttributes                = 0x%" PRIxLEAST32 "\n", limits->maxVertexInputAttributes               );
    printf("\t\tmaxVertexInputBindings                  = 0x%" PRIxLEAST32 "\n", limits->maxVertexInputBindings                 );
    printf("\t\tmaxVertexInputAttributeOffset           = 0x%" PRIxLEAST32 "\n", limits->maxVertexInputAttributeOffset          );
    printf("\t\tmaxVertexInputBindingStride             = 0x%" PRIxLEAST32 "\n", limits->maxVertexInputBindingStride            );
    printf("\t\tmaxVertexOutputComponents               = 0x%" PRIxLEAST32 "\n", limits->maxVertexOutputComponents              );
    printf("\t\tmaxTessellationGenLevel                         = 0x%" PRIxLEAST32 "\n", limits->maxTessellationGenLevel                        );
    printf("\t\tmaxTessellationPatchSize                        = 0x%" PRIxLEAST32 "\n", limits->maxTessellationPatchSize                       );
    printf("\t\tmaxTessellationControlPerVertexInputComponents  = 0x%" PRIxLEAST32 "\n", limits->maxTessellationControlPerVertexInputComponents );
    printf("\t\tmaxTessellationControlPerVertexOutputComponents = 0x%" PRIxLEAST32 "\n", limits->maxTessellationControlPerVertexOutputComponents);
    printf("\t\tmaxTessellationControlPerPatchOutputComponents  = 0x%" PRIxLEAST32 "\n", limits->maxTessellationControlPerPatchOutputComponents );
    printf("\t\tmaxTessellationControlTotalOutputComponents     = 0x%" PRIxLEAST32 "\n", limits->maxTessellationControlTotalOutputComponents    );
    printf("\t\tmaxTessellationEvaluationInputComponents        = 0x%" PRIxLEAST32 "\n", limits->maxTessellationEvaluationInputComponents       );
    printf("\t\tmaxTessellationEvaluationOutputComponents       = 0x%" PRIxLEAST32 "\n", limits->maxTessellationEvaluationOutputComponents      );
    printf("\t\tmaxGeometryShaderInvocations            = 0x%" PRIxLEAST32 "\n", limits->maxGeometryShaderInvocations           );
    printf("\t\tmaxGeometryInputComponents              = 0x%" PRIxLEAST32 "\n", limits->maxGeometryInputComponents             );
    printf("\t\tmaxGeometryOutputComponents             = 0x%" PRIxLEAST32 "\n", limits->maxGeometryOutputComponents            );
    printf("\t\tmaxGeometryOutputVertices               = 0x%" PRIxLEAST32 "\n", limits->maxGeometryOutputVertices              );
    printf("\t\tmaxGeometryTotalOutputComponents        = 0x%" PRIxLEAST32 "\n", limits->maxGeometryTotalOutputComponents       );
    printf("\t\tmaxFragmentInputComponents              = 0x%" PRIxLEAST32 "\n", limits->maxFragmentInputComponents             );
    printf("\t\tmaxFragmentOutputAttachments                = 0x%" PRIxLEAST32 "\n", limits->maxFragmentOutputAttachments               );
    printf("\t\tmaxFragmentDualSourceAttachments            = 0x%" PRIxLEAST32 "\n", limits->maxFragmentDualSourceAttachments           );
    printf("\t\tmaxFragmentCombinedOutputResources      = 0x%" PRIxLEAST32 "\n", limits->maxFragmentCombinedOutputResources     );
    printf("\t\tmaxComputeSharedMemorySize              = 0x%" PRIxLEAST32 "\n", limits->maxComputeSharedMemorySize             );
    printf("\t\tmaxComputeWorkGroupCount[0]             = 0x%" PRIxLEAST32 "\n", limits->maxComputeWorkGroupCount[0]            );
    printf("\t\tmaxComputeWorkGroupCount[1]             = 0x%" PRIxLEAST32 "\n", limits->maxComputeWorkGroupCount[1]            );
    printf("\t\tmaxComputeWorkGroupCount[2]             = 0x%" PRIxLEAST32 "\n", limits->maxComputeWorkGroupCount[2]            );
    printf("\t\tmaxComputeWorkGroupInvocations          = 0x%" PRIxLEAST32 "\n", limits->maxComputeWorkGroupInvocations         );
    printf("\t\tmaxComputeWorkGroupSize[0]              = 0x%" PRIxLEAST32 "\n", limits->maxComputeWorkGroupSize[0]             );
    printf("\t\tmaxComputeWorkGroupSize[1]              = 0x%" PRIxLEAST32 "\n", limits->maxComputeWorkGroupSize[1]             );
    printf("\t\tmaxComputeWorkGroupSize[2]              = 0x%" PRIxLEAST32 "\n", limits->maxComputeWorkGroupSize[2]             );
    printf("\t\tsubPixelPrecisionBits                   = 0x%" PRIxLEAST32 "\n", limits->subPixelPrecisionBits                  );
    printf("\t\tsubTexelPrecisionBits                   = 0x%" PRIxLEAST32 "\n", limits->subTexelPrecisionBits                  );
    printf("\t\tmipmapPrecisionBits                     = 0x%" PRIxLEAST32 "\n", limits->mipmapPrecisionBits                    );
    printf("\t\tmaxDrawIndexedIndexValue                = 0x%" PRIxLEAST32 "\n", limits->maxDrawIndexedIndexValue               );
    printf("\t\tmaxDrawIndirectInstanceCount            = 0x%" PRIxLEAST32 "\n", limits->maxDrawIndirectInstanceCount           );
    printf("\t\tprimitiveRestartForPatches              = 0x%" PRIxLEAST32 "\n", limits->primitiveRestartForPatches             );
    printf("\t\tmaxSamplerLodBias                       = %f\n",                 limits->maxSamplerLodBias                      );
    printf("\t\tmaxSamplerAnisotropy                    = %f\n",                 limits->maxSamplerAnisotropy                   );
    printf("\t\tmaxViewports                            = 0x%" PRIxLEAST32 "\n", limits->maxViewports                           );
    printf("\t\tmaxViewportDimensions[0]                = 0x%" PRIxLEAST32 "\n", limits->maxViewportDimensions[0]               );
    printf("\t\tmaxViewportDimensions[1]                = 0x%" PRIxLEAST32 "\n", limits->maxViewportDimensions[1]               );
    printf("\t\tviewportBoundsRange[0]                  = %f\n",                 limits->viewportBoundsRange[0]                 );
    printf("\t\tviewportBoundsRange[1]                  = %f\n",                 limits->viewportBoundsRange[1]                 );
    printf("\t\tviewportSubPixelBits                    = 0x%" PRIxLEAST32 "\n", limits->viewportSubPixelBits                   );
    printf("\t\tminMemoryMapAlignment                   = 0x%" PRIxLEAST32 "\n", limits->minMemoryMapAlignment                  );
    printf("\t\tminTexelBufferOffsetAlignment           = 0x%" PRIxLEAST32 "\n", limits->minTexelBufferOffsetAlignment          );
    printf("\t\tminUniformBufferOffsetAlignment         = 0x%" PRIxLEAST32 "\n", limits->minUniformBufferOffsetAlignment        );
    printf("\t\tminStorageBufferOffsetAlignment         = 0x%" PRIxLEAST32 "\n", limits->minStorageBufferOffsetAlignment        );
    printf("\t\tminTexelOffset                          = 0x%" PRIxLEAST32 "\n", limits->minTexelOffset                         );
    printf("\t\tmaxTexelOffset                          = 0x%" PRIxLEAST32 "\n", limits->maxTexelOffset                         );
    printf("\t\tminTexelGatherOffset                    = 0x%" PRIxLEAST32 "\n", limits->minTexelGatherOffset                   );
    printf("\t\tmaxTexelGatherOffset                    = 0x%" PRIxLEAST32 "\n", limits->maxTexelGatherOffset                   );
    printf("\t\tminInterpolationOffset                  = %f\n",                 limits->minInterpolationOffset                 );
    printf("\t\tmaxInterpolationOffset                  = %f\n",                 limits->maxInterpolationOffset                 );
    printf("\t\tsubPixelInterpolationOffsetBits         = 0x%" PRIxLEAST32 "\n", limits->subPixelInterpolationOffsetBits        );
    printf("\t\tmaxFramebufferWidth                     = 0x%" PRIxLEAST32 "\n", limits->maxFramebufferWidth                    );
    printf("\t\tmaxFramebufferHeight                    = 0x%" PRIxLEAST32 "\n", limits->maxFramebufferHeight                   );
    printf("\t\tmaxFramebufferLayers                    = 0x%" PRIxLEAST32 "\n", limits->maxFramebufferLayers                   );
    printf("\t\tmaxFramebufferColorSamples              = 0x%" PRIxLEAST32 "\n", limits->maxFramebufferColorSamples             );
    printf("\t\tmaxFramebufferDepthSamples              = 0x%" PRIxLEAST32 "\n", limits->maxFramebufferDepthSamples             );
    printf("\t\tmaxFramebufferStencilSamples            = 0x%" PRIxLEAST32 "\n", limits->maxFramebufferStencilSamples           );
    printf("\t\tmaxColorAttachments                     = 0x%" PRIxLEAST32 "\n", limits->maxColorAttachments                    );
    printf("\t\tmaxSampledImageColorSamples             = 0x%" PRIxLEAST32 "\n", limits->maxSampledImageColorSamples            );
    printf("\t\tmaxSampledImageDepthSamples             = 0x%" PRIxLEAST32 "\n", limits->maxSampledImageDepthSamples            );
    printf("\t\tmaxSampledImageIntegerSamples           = 0x%" PRIxLEAST32 "\n", limits->maxSampledImageIntegerSamples          );
    printf("\t\tmaxStorageImageSamples                  = 0x%" PRIxLEAST32 "\n", limits->maxStorageImageSamples                 );
    printf("\t\tmaxSampleMaskWords                      = 0x%" PRIxLEAST32 "\n", limits->maxSampleMaskWords                     );
    printf("\t\ttimestampFrequency                      = 0x%" PRIxLEAST64 "\n", limits->timestampFrequency                     );
    printf("\t\tmaxClipDistances                        = 0x%" PRIxLEAST32 "\n", limits->maxClipDistances                       );
    printf("\t\tmaxCullDistances                        = 0x%" PRIxLEAST32 "\n", limits->maxCullDistances                       );
    printf("\t\tmaxCombinedClipAndCullDistances         = 0x%" PRIxLEAST32 "\n", limits->maxCombinedClipAndCullDistances        );
    printf("\t\tpointSizeRange[0]                       = %f\n",                 limits->pointSizeRange[0]                      );
    printf("\t\tpointSizeRange[1]                       = %f\n",                 limits->pointSizeRange[1]                      );
    printf("\t\tlineWidthRange[0]                       = %f\n",                 limits->lineWidthRange[0]                      );
    printf("\t\tlineWidthRange[1]                       = %f\n",                 limits->lineWidthRange[1]                      );
    printf("\t\tpointSizeGranularity                    = %f\n",                 limits->pointSizeGranularity                   );
    printf("\t\tlineWidthGranularity                    = %f\n",                 limits->lineWidthGranularity                   );
}

static void app_gpu_dump_props(const struct app_gpu *gpu)
{
    const VkPhysicalDeviceProperties *props = &gpu->props;

    printf("VkPhysicalDeviceProperties:\n");
    printf("===========================\n");
    printf("\tapiVersion     = %u\n",                props->apiVersion);
    printf("\tdriverVersion  = %u\n",                props->driverVersion);
    printf("\tvendorId       = 0x%04x\n",            props->vendorId);
    printf("\tdeviceId       = 0x%04x\n",            props->deviceId);
    printf("\tdeviceType     = %s\n",                vk_physical_device_type_string(props->deviceType));
    printf("\tdeviceName     = %s\n",                props->deviceName);

    app_dump_limits(&gpu->props.limits);
    app_dump_sparse_props(&gpu->props.sparseProperties);

    fflush(stdout);
}

static void app_dump_extensions(
        const char *indent,
        const char *layer_name,
        const uint32_t extension_count,
        const VkExtensionProperties *extension_properties)
{
    uint32_t i;
    if (layer_name && (strlen(layer_name) > 0)) {
        printf("%s%s Extensions", indent, layer_name);
    } else {
        printf("Extensions");
    }
    printf("\tcount = %d\n", extension_count);
    for (i=0; i< extension_count; i++) {
        VkExtensionProperties const *ext_prop = &extension_properties[i];

        if (i>0)
            printf("\n"); // separator between extensions

        printf("%s\t", indent);
        printf("%-32s: extension revision %2d",
                       ext_prop->extName, ext_prop->specVersion);
    }
    printf("\n");
    fflush(stdout);
}

static void app_gpu_dump_queue_props(const struct app_gpu *gpu, uint32_t id)
{
    const VkQueueFamilyProperties *props = &gpu->queue_props[id];

    printf("VkQueueFamilyProperties[%d]:\n", id);
    printf("============================\n");
    printf("\tqueueFlags         = %c%c%c%c\n",
            (props->queueFlags & VK_QUEUE_GRAPHICS_BIT) ? 'G' : '.',
            (props->queueFlags & VK_QUEUE_COMPUTE_BIT)  ? 'C' : '.',
            (props->queueFlags & VK_QUEUE_DMA_BIT)      ? 'D' : '.',
            (props->queueFlags & VK_QUEUE_EXTENDED_BIT) ? 'X' : '.');
    printf("\tqueueCount         = %u\n",   props->queueCount);
    printf("\ttimestampValidBits = %u\n",   props->timestampValidBits);
    fflush(stdout);
}

static void app_gpu_dump_memory_props(const struct app_gpu *gpu)
{
    const VkPhysicalDeviceMemoryProperties *props = &gpu->memory_props;

    printf("VkPhysicalDeviceMemoryProperties:\n");
    printf("=================================\n");
    printf("\tmemoryTypeCount       = %u\n", props->memoryTypeCount);
    for (uint32_t i = 0; i < props->memoryTypeCount; i++) {
        printf("\tmemoryTypes[%u] : \n", i);
        printf("\t\tpropertyFlags = %u\n", props->memoryTypes[i].propertyFlags);
        printf("\t\theapIndex     = %u\n", props->memoryTypes[i].heapIndex);
    }
    printf("\tmemoryHeapCount       = %u\n",                props->memoryHeapCount);
    for (uint32_t i = 0; i < props->memoryHeapCount; i++) {
        printf("\tmemoryHeaps[%u] : \n", i);
        printf("\t\tsize          = " PRINTF_SIZE_T_SPECIFIER "\n", props->memoryHeaps[i].size);
    }
    fflush(stdout);
}

static void app_gpu_dump(const struct app_gpu *gpu)
{
    uint32_t i;

    printf("Device Extensions and layers:\n");
    printf("=============================\n");
    printf("GPU%u\n", gpu->id);
    app_gpu_dump_props(gpu);
    printf("\n");
    app_dump_extensions("", "Device", gpu->device_extension_count, gpu->device_extensions);
    printf("\n");
    printf("Layers\tcount = %d\n", gpu->device_layer_count);
    for (uint32_t i = 0; i < gpu->device_layer_count; i++) {
        uint32_t major, minor, patch;
        char spec_version[64], layer_version[64];
        struct layer_extension_list const *layer_info = &gpu->device_layers[i];

        extract_version(layer_info->layer_properties.specVersion, &major, &minor, &patch);
        snprintf(spec_version, sizeof(spec_version), "%d.%d.%d", major, minor, patch);
        extract_version(layer_info->layer_properties.implVersion, &major, &minor, &patch);
        snprintf(layer_version, sizeof(layer_version), "%d.%d.%d", major, minor, patch);
        printf("\t%s (%s) Vulkan version %s, layer version %s\n",
               layer_info->layer_properties.layerName,
               (char*) layer_info->layer_properties.description,
               spec_version, layer_version);

        app_dump_extensions("\t",
                            layer_info->layer_properties.layerName,
                            layer_info->extension_count,
                            layer_info->extension_properties);
        fflush(stdout);
    }
    printf("\n");
    for (i = 0; i < gpu->queue_count; i++) {
        app_gpu_dump_queue_props(gpu, i);
        printf("\n");
    }
    app_gpu_dump_memory_props(gpu);
    printf("\n");
    app_gpu_dump_features(gpu);
    printf("\n");
    app_dev_dump(&gpu->dev);
}

int main(int argc, char **argv)
{
    unsigned int major, minor, patch;
    struct app_gpu gpus[MAX_GPUS];
    VkPhysicalDevice objs[MAX_GPUS];
    uint32_t gpu_count, i;
    VkResult err;
    struct app_instance inst;

    major = VK_API_VERSION >> 22;
    minor = (VK_API_VERSION >> 12) & 0x4ff;
    patch = VK_API_VERSION & 0xfff;
    printf("===========\n");
	printf("VULKAN INFO\n");
    printf("===========\n\n");
    printf("Vulkan API Version: %d %d %d\n\n", major, minor, patch);

    app_create_instance(&inst);

    printf("Instance Extensions and layers:\n");
    printf("===============================\n");
    app_dump_extensions("", "Instance", inst.global_extension_count, inst.global_extensions);

    printf("Instance Layers\tcount = %d\n", inst.global_layer_count);
    for (uint32_t i = 0; i < inst.global_layer_count; i++) {
        uint32_t major, minor, patch;
        char spec_version[64], layer_version[64];
        VkLayerProperties const *layer_prop = &inst.global_layers[i].layer_properties;

        extract_version(layer_prop->specVersion, &major, &minor, &patch);
        snprintf(spec_version, sizeof(spec_version), "%d.%d.%d", major, minor, patch);
        extract_version(layer_prop->implVersion, &major, &minor, &patch);
        snprintf(layer_version, sizeof(layer_version), "%d.%d.%d", major, minor, patch);
        printf("\t%s (%s) Vulkan version %s, layer version %s\n",
               layer_prop->layerName, (char*) layer_prop->description, spec_version, layer_version);

        app_dump_extensions("\t",
                            inst.global_layers[i].layer_properties.layerName,
                            inst.global_layers[i].extension_count,
                            inst.global_layers[i].extension_properties);
    }

    err = vkEnumeratePhysicalDevices(inst.instance, &gpu_count, NULL);
    if (err)
        ERR_EXIT(err);
    if (gpu_count > MAX_GPUS) {
        printf("Too many GPUS found \n");
        ERR_EXIT(-1);
    }
    err = vkEnumeratePhysicalDevices(inst.instance, &gpu_count, objs);
    if (err)
        ERR_EXIT(err);

    for (i = 0; i < gpu_count; i++) {
        app_gpu_init(&gpus[i], i, objs[i]);
        app_gpu_dump(&gpus[i]);
        printf("\n\n");
    }

    for (i = 0; i < gpu_count; i++)
        app_gpu_destroy(&gpus[i]);

    app_destroy_instance(&inst);

    return 0;
}

#ifdef _WIN32

// Create a console window with a large scrollback size to which to send stdout.
// Returns true if console window was successfully created, false otherwise.
bool SetStdOutToNewConsole()
{
    // don't do anything if we already have a console
    if (GetStdHandle(STD_OUTPUT_HANDLE))
        return false;

    // allocate a console for this app
    AllocConsole();

    // redirect unbuffered STDOUT to the console
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    int fileDescriptor = _open_osfhandle((intptr_t)consoleHandle, _O_TEXT);
    FILE *fp = _fdopen( fileDescriptor, "w" );
    *stdout = *fp;
    setvbuf( stdout, NULL, _IONBF, 0 );

    // make the console window bigger
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    SMALL_RECT r;
    COORD bufferSize;
    if (!GetConsoleScreenBufferInfo(consoleHandle, &csbi))
        return false;
    bufferSize.X = csbi.dwSize.X+30;
    bufferSize.Y = 20000;
    if (!SetConsoleScreenBufferSize(consoleHandle, bufferSize))
        return false;
    r.Left = r.Top = 0;
    r.Right = csbi.dwSize.X-1+30;
    r.Bottom = 50;
    if (!SetConsoleWindowInfo(consoleHandle, true, &r))
        return false;

    // change the console window title
    if (!SetConsoleTitle(TEXT(APP_SHORT_NAME)))
        return false;

    return true;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
    char *argv = pCmdLine;
    consoleCreated = SetStdOutToNewConsole();
    main(1, &argv);
    fflush(stdout);
    if (consoleCreated)
        Sleep(INFINITE);
}
#endif
