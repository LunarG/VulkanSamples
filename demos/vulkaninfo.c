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

#ifdef _WIN32
#include <Windows.h>
#include <fcntl.h>
#include <io.h>
#endif

#include <vulkan.h>

#define ERR(err) printf("%s:%d: failed with %s\n", \
    __FILE__, __LINE__, vk_result_string(err));

#ifdef _WIN32

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


    VkFormatProperties format_props[VK_NUM_FORMAT];
};

struct app_instance {
    VkInstance  instance;
    uint32_t global_extension_count;
    VkExtensionProperties *global_extensions;
};

struct app_gpu {
    uint32_t id;
    VkPhysicalDevice obj;

    VkPhysicalDeviceProperties props;
    VkPhysicalDevicePerformance perf;

    uint32_t queue_count;
    VkPhysicalDeviceQueueProperties *queue_props;
    VkDeviceQueueCreateInfo *queue_reqs;

    VkPhysicalDeviceMemoryProperties memory_props;

    uint32_t device_extension_count;
    VkExtensionProperties *device_extensions;

    struct app_dev dev;
};

static const char *vk_result_string(VkResult err)
{
    switch (err) {
#define STR(r) case r: return #r
    STR(VK_SUCCESS);
    STR(VK_UNSUPPORTED);
    STR(VK_NOT_READY);
    STR(VK_TIMEOUT);
    STR(VK_EVENT_SET);
    STR(VK_EVENT_RESET);
    STR(VK_ERROR_UNKNOWN);
    STR(VK_ERROR_UNAVAILABLE);
    STR(VK_ERROR_INITIALIZATION_FAILED);
    STR(VK_ERROR_OUT_OF_HOST_MEMORY);
    STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
    STR(VK_ERROR_DEVICE_ALREADY_CREATED);
    STR(VK_ERROR_DEVICE_LOST);
    STR(VK_ERROR_INVALID_POINTER);
    STR(VK_ERROR_INVALID_VALUE);
    STR(VK_ERROR_INVALID_HANDLE);
    STR(VK_ERROR_INVALID_ORDINAL);
    STR(VK_ERROR_INVALID_MEMORY_SIZE);
    STR(VK_ERROR_INVALID_EXTENSION);
    STR(VK_ERROR_INVALID_FLAGS);
    STR(VK_ERROR_INVALID_ALIGNMENT);
    STR(VK_ERROR_INVALID_FORMAT);
    STR(VK_ERROR_INVALID_IMAGE);
    STR(VK_ERROR_INVALID_DESCRIPTOR_SET_DATA);
    STR(VK_ERROR_INVALID_QUEUE_TYPE);
    STR(VK_ERROR_INVALID_OBJECT_TYPE);
    STR(VK_ERROR_UNSUPPORTED_SHADER_IL_VERSION);
    STR(VK_ERROR_BAD_SHADER_CODE);
    STR(VK_ERROR_BAD_PIPELINE_DATA);
    STR(VK_ERROR_NOT_MAPPABLE);
    STR(VK_ERROR_MEMORY_MAP_FAILED);
    STR(VK_ERROR_MEMORY_UNMAP_FAILED);
    STR(VK_ERROR_INCOMPATIBLE_DEVICE);
    STR(VK_ERROR_INCOMPATIBLE_DRIVER);
    STR(VK_ERROR_INCOMPLETE_COMMAND_BUFFER);
    STR(VK_ERROR_BUILDING_COMMAND_BUFFER);
    STR(VK_ERROR_MEMORY_NOT_BOUND);
    STR(VK_ERROR_INCOMPATIBLE_QUEUE);
    STR(VK_ERROR_NOT_SHAREABLE);
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
    STR(D24_UNORM);
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

    for (f = 0; f < VK_NUM_FORMAT; f++) {
        const VkFormat fmt = f;
        VkResult err;
        size_t size = sizeof(dev->format_props[f]);

        err = vkGetFormatInfo(dev->obj, fmt,
                               VK_FORMAT_INFO_TYPE_PROPERTIES,
                               &size, &dev->format_props[f]);
        if (err) {
            memset(&dev->format_props[f], 0,
                   sizeof(dev->format_props[f]));
        }
        else if (size != sizeof(dev->format_props[f])) {
            ERR_EXIT(VK_ERROR_UNKNOWN);
        }
    }
}

static void app_dev_init(struct app_dev *dev, struct app_gpu *gpu)
{
    VkDeviceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueRecordCount = 0,
        .pRequestedQueues = NULL,
        .extensionCount = 0,
        .pEnabledExtensions = NULL,
        .flags = 0,
    };
    VkResult U_ASSERT_ONLY err;
    // Extensions to enable
    VkExtensionProperties *enable_extension_list;
    static char *known_extensions[] = {
        "Validation",
    };
    size_t extSize = sizeof(uint32_t);
    uint32_t extCount = 0;
    err = vkGetPhysicalDeviceExtensionInfo(gpu->obj, VK_EXTENSION_INFO_TYPE_COUNT, 0, &extSize, &extCount);
    assert(!err);

    enable_extension_list = malloc(sizeof(VkExtensionProperties) * extCount);
    if (!enable_extension_list) {
        ERR_EXIT(VK_ERROR_OUT_OF_HOST_MEMORY);
    }

    VkExtensionProperties extProp;
    extSize = sizeof(VkExtensionProperties);
    gpu->device_extension_count = 0;
    bool32_t U_ASSERT_ONLY extFound = 0; // TODO : Need to enhance this if/when we enable multiple extensions
    for (uint32_t i = 0; i < ARRAY_SIZE(known_extensions); i++) {
        for (uint32_t j = 0; j < extCount; j++) {
            err = vkGetPhysicalDeviceExtensionInfo(
                      gpu->obj, VK_EXTENSION_INFO_TYPE_PROPERTIES,
                      j, &extSize,
                      &extProp);
            if (!strcmp(known_extensions[i], extProp.name)) {
                extFound = 1;
                memcpy(&enable_extension_list[gpu->device_extension_count], &extProp, sizeof(extProp));
                gpu->device_extension_count++;
            }
        }
    }
    assert(extFound);

    gpu->device_extensions = enable_extension_list;

    /* request all queues */
    info.queueRecordCount = gpu->queue_count;
    info.pRequestedQueues = gpu->queue_reqs;

    info.extensionCount = extCount;
    info.pEnabledExtensions = enable_extension_list;
    dev->gpu = gpu;
    err = vkCreateDevice(gpu->obj, &info, &dev->obj);
    if (err)
        ERR_EXIT(err);

}

static void app_dev_destroy(struct app_dev *dev)
{
    vkDestroyDevice(dev->obj);
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
        .extensionCount = 0,
        .pEnabledExtensions = NULL,
    };
    VkResult U_ASSERT_ONLY err;
    // Global Extensions to enable
    static char *known_extensions[] = {
        "VK_WSI_LunarG",
    };
    size_t extSize = sizeof(uint32_t);
    uint32_t extCount = 0;
    VkExtensionProperties extProp;
    VkExtensionProperties *enable_extension_list;
    uint32_t global_extension_count = 0;

    err = vkGetGlobalExtensionInfo(VK_EXTENSION_INFO_TYPE_COUNT, 0, &extSize, &extCount);
    assert(!err);

    enable_extension_list = malloc(sizeof(VkExtensionProperties) * extCount);
    if (!enable_extension_list) {
        ERR_EXIT(VK_ERROR_OUT_OF_HOST_MEMORY);
    }

    extSize = sizeof(VkExtensionProperties);
    bool32_t U_ASSERT_ONLY extFound = 0; // TODO : Need to enhance this if/when we enable multiple extensions
    for (uint32_t i = 0; i < ARRAY_SIZE(known_extensions); i++) {
        for (uint32_t j = 0; j < extCount; j++) {
            err = vkGetGlobalExtensionInfo(VK_EXTENSION_INFO_TYPE_PROPERTIES, j, &extSize, &extProp);
            if (!strcmp(known_extensions[i], extProp.name)) {
                extFound = 1;
                memcpy(&enable_extension_list[global_extension_count], &extProp, sizeof(extProp));
                global_extension_count++;
            }
        }
    }
    assert(extFound);

    inst_info.extensionCount = global_extension_count;
    inst_info.pEnabledExtensions = enable_extension_list;

    err = vkCreateInstance(&inst_info, &inst->instance);
    if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
        printf("Cannot create Vulkan instance.\n");
        ERR_EXIT(err);
    } else if (err) {
        ERR_EXIT(err);
    }

    inst->global_extension_count = global_extension_count;
    inst->global_extensions = enable_extension_list;
}

static void app_destroy_instance(struct app_instance *inst)
{
    free(inst->global_extensions);
    vkDestroyInstance(inst->instance);
}


static void app_gpu_init(struct app_gpu *gpu, uint32_t id, VkPhysicalDevice obj)
{
    size_t size;
    VkResult err;
    uint32_t i;

    memset(gpu, 0, sizeof(*gpu));

    gpu->id = id;
    gpu->obj = obj;
    size = sizeof(gpu->props);
    err = vkGetPhysicalDeviceInfo(gpu->obj,
                        VK_PHYSICAL_DEVICE_INFO_TYPE_PROPERTIES,
                        &size, &gpu->props);
    if (err || size != sizeof(gpu->props))
        ERR_EXIT(err);

    size = sizeof(gpu->perf);
    err = vkGetPhysicalDeviceInfo(gpu->obj,
                        VK_PHYSICAL_DEVICE_INFO_TYPE_PERFORMANCE,
                        &size, &gpu->perf);
    if (err || size != sizeof(gpu->perf))
        ERR_EXIT(err);

    /* get queue count */
    err = vkGetPhysicalDeviceInfo(gpu->obj,
                        VK_PHYSICAL_DEVICE_INFO_TYPE_QUEUE_PROPERTIES,
                        &size, NULL);
    if (err || size % sizeof(gpu->queue_props[0]))
        ERR_EXIT(err);
    gpu->queue_count = (uint32_t) (size / sizeof(gpu->queue_props[0]));

    gpu->queue_props =
            malloc(sizeof(gpu->queue_props[0]) * gpu->queue_count);
    size = sizeof(gpu->queue_props[0]) * gpu->queue_count;
    if (!gpu->queue_props)
        ERR_EXIT(VK_ERROR_OUT_OF_HOST_MEMORY);
    err = vkGetPhysicalDeviceInfo(gpu->obj,
                        VK_PHYSICAL_DEVICE_INFO_TYPE_QUEUE_PROPERTIES,
                        &size, gpu->queue_props);
    if (err || size != sizeof(gpu->queue_props[0]) * gpu->queue_count)
        ERR_EXIT(err);

    /* set up queue requests */
    size = sizeof(*gpu->queue_reqs) * gpu->queue_count;
    gpu->queue_reqs = malloc(sizeof(*gpu->queue_reqs) * gpu->queue_count);
    if (!gpu->queue_reqs)
        ERR_EXIT(VK_ERROR_OUT_OF_HOST_MEMORY);
    for (i = 0; i < gpu->queue_count; i++) {
        gpu->queue_reqs[i].queueNodeIndex = i;
        gpu->queue_reqs[i].queueCount = gpu->queue_props[i].queueCount;
    }

    size = sizeof(gpu->memory_props);
    err = vkGetPhysicalDeviceInfo(gpu->obj,
                        VK_PHYSICAL_DEVICE_INFO_TYPE_MEMORY_PROPERTIES,
                        &size, &gpu->memory_props);
    if (err || size != sizeof(gpu->memory_props))
        ERR_EXIT(err);

    app_dev_init(&gpu->dev, gpu);
    app_dev_init_formats(&gpu->dev);
}

static void app_gpu_destroy(struct app_gpu *gpu)
{
    app_dev_destroy(&gpu->dev);
    free(gpu->device_extensions);
    free(gpu->queue_reqs);
    free(gpu->queue_props);
}

static void app_dev_dump_format_props(const struct app_dev *dev, VkFormat fmt)
{
    const VkFormatProperties *props = &dev->format_props[fmt];
    struct {
        const char *name;
        VkFlags flags;
    } tilings[2];
    uint32_t i;

    if (!props->linearTilingFeatures && !props->optimalTilingFeatures)
        return;

    tilings[0].name = "linear";
    tilings[0].flags = props->linearTilingFeatures;
    tilings[1].name = "optimal";
    tilings[1].flags = props->optimalTilingFeatures;

    printf("FORMAT_%s\n", vk_format_string(fmt));
    for (i = 0; i < ARRAY_SIZE(tilings); i++) {
        if (!tilings[i].flags)
            continue;

        printf("\t%s tiling image =%s%s%s\n", tilings[i].name,
                (tilings[i].flags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)      ? " sampled" : "",
                (tilings[i].flags & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)   ? " storage" : "",
                (tilings[i].flags & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT) ? " atomic" : "");
        printf("\t%s tiling texel =%s%s%s\n", tilings[i].name,
                (tilings[i].flags & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT)      ? " TBO" : "",
                (tilings[i].flags & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT)   ? " IBO" : "",
                (tilings[i].flags & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT) ? " atomic" : "");
        printf("\t%s tiling attachment =%s%s%s\n", tilings[i].name,
                (tilings[i].flags & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) ? " color" : "",
                (tilings[i].flags & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT) ? " blend" : "",
                (tilings[i].flags & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)       ? " depth/stencil" : "");
        printf("\t%s tiling vertex = %u\n", tilings[i].name,
                (bool) (tilings[i].flags & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT));
        printf("\t%s tiling conversion = %u\n", tilings[i].name,
                (bool) (tilings[i].flags & VK_FORMAT_FEATURE_CONVERSION_BIT));
    }
}


static void
app_dev_dump(const struct app_dev *dev)
{
    VkFormat fmt;

    for (fmt = 0; fmt < VK_NUM_FORMAT; fmt++) {
        app_dev_dump_format_props(dev, fmt);
    }
}

static void app_gpu_dump_multi_compat(const struct app_gpu *gpu, const struct app_gpu *other,
        const VkPhysicalDeviceCompatibilityInfo *info)
{
    printf("VkPhysicalDeviceCompatibilityInfo[GPU%d]\n", other->id);

#define TEST(info, b) printf(#b " = %u\n", (bool) (info->compatibilityFlags & VK_PHYSICAL_DEVICE_COMPATIBILITY_ ##b## _BIT))
    TEST(info, FEATURES);
    TEST(info, IQ_MATCH);
    TEST(info, PEER_TRANSFER);
    TEST(info, SHARED_MEMORY);
    TEST(info, SHARED_SYNC);
    TEST(info, SHARED_DEVICE0_DISPLAY);
    TEST(info, SHARED_DEVICE1_DISPLAY);
#undef TEST
}

static void app_gpu_multi_compat(struct app_gpu *gpus, uint32_t gpu_count)
{
        VkResult err;
        uint32_t i, j;

        for (i = 0; i < gpu_count; i++) {
                for (j = 0; j < gpu_count; j++) {
                        VkPhysicalDeviceCompatibilityInfo info;

                        if (i == j)
                                continue;

                        err = vkGetMultiDeviceCompatibility(gpus[i].obj,
                                        gpus[j].obj, &info);
                        if (err)
                                ERR_EXIT(err);

                        app_gpu_dump_multi_compat(&gpus[i], &gpus[j], &info);
                }
        }
}

#ifdef _WIN32
#define PRINTF_SIZE_T_SPECIFIER    "%Iu"
#else
#define PRINTF_SIZE_T_SPECIFIER    "%zu"
#endif

static void app_gpu_dump_props(const struct app_gpu *gpu)
{
    const VkPhysicalDeviceProperties *props = &gpu->props;

    printf("VkPhysicalDeviceProperties\n");
    printf("\tapiVersion = %u\n",                   props->apiVersion);
    printf("\tdriverVersion = %u\n",                props->driverVersion);
    printf("\tvendorId = 0x%04x\n",                 props->vendorId);
    printf("\tdeviceId = 0x%04x\n",                 props->deviceId);
    printf("\tdeviceType = %s\n",                   vk_physical_device_type_string(props->deviceType));
    printf("\tdeviceName = %s\n",                   props->deviceName);
    printf("\tmaxInlineMemoryUpdateSize = " PRINTF_SIZE_T_SPECIFIER "\n",   props->maxInlineMemoryUpdateSize);
    printf("\tmaxBoundDescriptorSets = %u\n",       props->maxBoundDescriptorSets);
    printf("\tmaxThreadGroupSize = %u\n",           props->maxThreadGroupSize);
    printf("\ttimestampFrequency = %lu\n",          props->timestampFrequency);
    printf("\tmultiColorAttachmentClears = %u\n",   props->multiColorAttachmentClears);
}

static void app_gpu_dump_perf(const struct app_gpu *gpu)
{
    const VkPhysicalDevicePerformance *perf = &gpu->perf;

    printf("VkPhysicalDevicePerformance\n");
    printf("\tmaxGpuClock = %f\n",      perf->maxDeviceClock);
    printf("\taluPerClock = %f\n",      perf->aluPerClock);
    printf("\ttexPerClock = %f\n",      perf->texPerClock);
    printf("\tprimsPerClock = %f\n",    perf->primsPerClock);
    printf("\tpixelsPerClock = %f\n",   perf->pixelsPerClock);
}

static void app_gpu_dump_instance_extensions(const struct app_instance *inst)
{
    uint32_t i;
    printf("Extensions");
    printf("\tcount = %d\n",            inst->global_extension_count);
    printf("\t");
    for (i=0; i< inst->global_extension_count; i++) {
        if (i>0)
            printf(", "); // separator between extension names
        printf("%s",                    inst->global_extensions[i].name);
    }
    printf("\n");
}

static void app_gpu_dump_extensions(const struct app_gpu *gpu)
{
    uint32_t i;
    printf("Extensions");
    printf("\tcount = %d\n",            gpu->device_extension_count);
    printf("\t");
    for (i=0; i< gpu->device_extension_count; i++) {
        if (i>0)
            printf(", "); // separator between extension names
        printf("%s(%d): %s", gpu->device_extensions[i].name,
               gpu->device_extensions[i].version,
               gpu->device_extensions[i].description);
    }
    printf("\n");
}

static void app_gpu_dump_queue_props(const struct app_gpu *gpu, uint32_t id)
{
    const VkPhysicalDeviceQueueProperties *props = &gpu->queue_props[id];

    printf("VkPhysicalDeviceQueueProperties[%d]\n", id);
    printf("\tqueueFlags = %c%c%c%c\n",
            (props->queueFlags & VK_QUEUE_GRAPHICS_BIT) ? 'G' : '.',
            (props->queueFlags & VK_QUEUE_COMPUTE_BIT)  ? 'C' : '.',
            (props->queueFlags & VK_QUEUE_DMA_BIT)      ? 'D' : '.',
            (props->queueFlags & VK_QUEUE_EXTENDED_BIT) ? 'X' : '.');
    printf("\tqueueCount = %u\n",           props->queueCount);
    printf("\tmaxAtomicCounters = %u\n",    props->maxAtomicCounters);
    printf("\tsupportsTimestamps = %u\n",   props->supportsTimestamps);
}

static void app_gpu_dump_memory_props(const struct app_gpu *gpu)
{
    const VkPhysicalDeviceMemoryProperties *props = &gpu->memory_props;

    printf("VkPhysicalDeviceMemoryProperties\n");
    printf("\tsupportsMigration = %u\n",                props->supportsMigration);
    printf("\tsupportsPinning = %u\n",                  props->supportsPinning);
}

static void app_gpu_dump(const struct app_gpu *gpu)
{
    uint32_t i;

    printf("GPU%u\n", gpu->id);
    app_gpu_dump_props(gpu);
    printf("\n");
    app_gpu_dump_extensions(gpu);
    printf("\n");
    app_gpu_dump_perf(gpu);
    printf("\n");
    for (i = 0; i < gpu->queue_count; i++) {
        app_gpu_dump_queue_props(gpu, i);
        printf("\n");
    }
    app_gpu_dump_memory_props(gpu);
    printf("\n");
    app_dev_dump(&gpu->dev);
}

int main(int argc, char **argv)
{
    struct app_gpu gpus[MAX_GPUS];
    VkPhysicalDevice objs[MAX_GPUS];
    uint32_t gpu_count, i;
    VkResult err;
    struct app_instance inst;

    app_create_instance(&inst);
    app_gpu_dump_instance_extensions(&inst);

    err = vkEnumeratePhysicalDevices(inst.instance, &gpu_count, NULL);
    if (err)
        ERR_EXIT(err);
    if (gpu_count > MAX_GPUS) {
        printf("Too many GPUS found \n");
        ERR_EXIT(VK_ERROR_UNKNOWN);
    }
    err = vkEnumeratePhysicalDevices(inst.instance, &gpu_count, objs);
    if (err)
        ERR_EXIT(err);

    for (i = 0; i < gpu_count; i++) {
        app_gpu_init(&gpus[i], i, objs[i]);
        app_gpu_dump(&gpus[i]);
        printf("\n\n");
    }

    app_gpu_multi_compat(gpus, gpu_count);

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
    bufferSize.X = csbi.dwSize.X;
    bufferSize.Y = 1000;
    if (!SetConsoleScreenBufferSize(consoleHandle, bufferSize))
        return false;
    r.Left = r.Top = 0;
    r.Right = csbi.dwSize.X-1;
    r.Bottom = 60;
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
