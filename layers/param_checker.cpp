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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "vk_loader_platform.h"
#include "vk_layer.h"
#include "vk_layer_config.h"
#include "vk_enum_validate_helper.h"
#include "vk_struct_validate_helper.h"
//The following is #included again to catch certain OS-specific functions being used:
#include "vk_loader_platform.h"

#include "vk_layer_table.h"
#include "vk_layer_data.h"
#include "vk_layer_logging.h"
#include "vk_layer_extension_utils.h"

typedef struct _layer_data {
    debug_report_data *report_data;
    VkDbgMsgCallback logging_callback;
} layer_data;

static std::unordered_map<void*, layer_data*> layer_data_map;
static device_table_map pc_device_table_map;
static instance_table_map pc_instance_table_map;

// "my instance data"
debug_report_data *mid(VkInstance object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *data = get_my_data_ptr(key, layer_data_map);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "MID: map: %p, object: %p, key: %p, data: %p\n", &layer_data_map, object, key, data);
#endif
    assert(data != NULL);

    return data->report_data;
}

// "my device data"
debug_report_data *mdd(void* object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *data = get_my_data_ptr(key, layer_data_map);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "MDD: map: %p, object: %p, key: %p, data: %p\n", &layer_data_map, object, key, data);
#endif
    assert(data != NULL);
    return data->report_data;
}

static void InitParamChecker(layer_data *data)
{
    uint32_t report_flags = getLayerOptionFlags("ParamCheckerReportFlags", 0);

    uint32_t debug_action = 0;
    getLayerOptionEnum("ParamCheckerDebugAction", (uint32_t *) &debug_action);
    if(debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        FILE *log_output = NULL;
        const char* option_str = getLayerOption("ParamCheckerLogFilename");
        if(option_str)
        {
            log_output = fopen(option_str, "w");
        }
        if (log_output == NULL) {
            if (option_str)
                std::cout << std::endl << "ParamChecker ERROR: Bad output filename specified: " << option_str << ". Writing to STDOUT instead" << std::endl << std::endl;
            log_output = stdout;
        }

        layer_create_msg_callback(data->report_data, report_flags, log_callback, (void*)log_output, &data->logging_callback);
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgCreateMsgCallback(
    VkInstance instance,
    VkFlags msgFlags,
    const PFN_vkDbgMsgCallback pfnMsgCallback,
    void* pUserData,
    VkDbgMsgCallback* pMsgCallback)
{
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(pc_instance_table_map, instance);
    VkResult result =  pTable->DbgCreateMsgCallback(instance, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);

    if (result == VK_SUCCESS)
    {
        layer_data *data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
        result = layer_create_msg_callback(data->report_data, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
    }

    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgDestroyMsgCallback(
    VkInstance instance,
    VkDbgMsgCallback msgCallback)
{
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(pc_instance_table_map, instance);
    VkResult result =  pTable->DbgDestroyMsgCallback(instance, msgCallback);

    layer_data *data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    layer_destroy_msg_callback(data->report_data, msgCallback);

    return result;
}

static const VkLayerProperties pc_global_layers[] = {
    {
        "ParamChecker",
        VK_API_VERSION,
        VK_MAKE_VERSION(0, 1, 0),
        "Validation layer: ParamChecker",
    }
};

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionProperties(
        const char *pLayerName,
        uint32_t *pCount,
        VkExtensionProperties* pProperties)
{
    /* ParamChecker does not have any global extensions */
    return util_GetExtensionProperties(0, NULL, pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalLayerProperties(
        uint32_t *pCount,
        VkLayerProperties*    pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(pc_global_layers),
                                   pc_global_layers,
                                   pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceExtensionProperties(
        VkPhysicalDevice                            physicalDevice,
        const char*                                 pLayerName,
        uint32_t*                                   pCount,
        VkExtensionProperties*                      pProperties)
{
    /* ParamChecker does not have any physical device extensions */
    return util_GetExtensionProperties(0, NULL, pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceLayerProperties(
        VkPhysicalDevice                            physicalDevice,
        uint32_t*                                   pCount,
        VkLayerProperties*                          pProperties)
{
    /* ParamChecker's physical device layers are the same as global */
    return util_GetLayerProperties(ARRAY_SIZE(pc_global_layers), pc_global_layers,
                                   pCount, pProperties);
}

// Version: 0.138.2

static
std::string EnumeratorString(VkResult const& enumerator)
{
    switch(enumerator)
    {
        case VK_RESULT_MAX_ENUM:
        {
            return "VK_RESULT_MAX_ENUM";
            break;
        }
        case VK_ERROR_INVALID_LAYER:
        {
            return "VK_ERROR_INVALID_LAYER";
            break;
        }
        case VK_ERROR_INCOMPATIBLE_DRIVER:
        {
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
            break;
        }
        case VK_ERROR_MEMORY_UNMAP_FAILED:
        {
            return "VK_ERROR_MEMORY_UNMAP_FAILED";
            break;
        }
        case VK_ERROR_MEMORY_MAP_FAILED:
        {
            return "VK_ERROR_MEMORY_MAP_FAILED";
            break;
        }
        case VK_INCOMPLETE:
        {
            return "VK_INCOMPLETE";
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        {
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
            break;
        }
        case VK_ERROR_UNKNOWN:
        {
            return "VK_ERROR_UNKNOWN";
            break;
        }
        case VK_ERROR_INITIALIZATION_FAILED:
        {
            return "VK_ERROR_INITIALIZATION_FAILED";
            break;
        }
        case VK_NOT_READY:
        {
            return "VK_NOT_READY";
            break;
        }
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        {
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
            break;
        }
        case VK_EVENT_SET:
        {
            return "VK_EVENT_SET";
            break;
        }
        case VK_TIMEOUT:
        {
            return "VK_TIMEOUT";
            break;
        }
        case VK_EVENT_RESET:
        {
            return "VK_EVENT_RESET";
            break;
        }
        case VK_UNSUPPORTED:
        {
            return "VK_UNSUPPORTED";
            break;
        }
        case VK_ERROR_INCOMPATIBLE_DEVICE:
        {
            return "VK_ERROR_INCOMPATIBLE_DEVICE";
            break;
        }
        case VK_SUCCESS:
        {
            return "VK_SUCCESS";
            break;
        }
        case VK_ERROR_INVALID_EXTENSION:
        {
            return "VK_ERROR_INVALID_EXTENSION";
            break;
        }
        case VK_ERROR_DEVICE_LOST:
        {
            return "VK_ERROR_DEVICE_LOST";
            break;
        }
        default:
        {
            return "unrecognized enumerator";
            break;
        }
    }
}

static
bool ValidateEnumerator(VkFormatFeatureFlagBits const& enumerator)
{
    VkFormatFeatureFlagBits allFlags = (VkFormatFeatureFlagBits)(VK_FORMAT_FEATURE_CONVERSION_BIT |
        VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT |
        VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT |
        VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT |
        VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT |
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
        VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT |
        VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT |
        VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT |
        VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT |
        VK_FORMAT_FEATURE_BLIT_SOURCE_BIT |
        VK_FORMAT_FEATURE_BLIT_DESTINATION_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkFormatFeatureFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_FORMAT_FEATURE_CONVERSION_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_CONVERSION_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_BLIT_SOURCE_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_BLIT_SOURCE_BIT");
    }
    if(enumerator & VK_FORMAT_FEATURE_BLIT_DESTINATION_BIT)
    {
        strings.push_back("VK_FORMAT_FEATURE_BLIT_DESTINATION_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkImageUsageFlagBits const& enumerator)
{
    VkImageUsageFlagBits allFlags = (VkImageUsageFlagBits)(VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_DEPTH_STENCIL_BIT |
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT |
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkImageUsageFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
    {
        strings.push_back("VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT");
    }
    if(enumerator & VK_IMAGE_USAGE_DEPTH_STENCIL_BIT)
    {
        strings.push_back("VK_IMAGE_USAGE_DEPTH_STENCIL_BIT");
    }
    if(enumerator & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        strings.push_back("VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT");
    }
    if(enumerator & VK_IMAGE_USAGE_STORAGE_BIT)
    {
        strings.push_back("VK_IMAGE_USAGE_STORAGE_BIT");
    }
    if(enumerator & VK_IMAGE_USAGE_SAMPLED_BIT)
    {
        strings.push_back("VK_IMAGE_USAGE_SAMPLED_BIT");
    }
    if(enumerator & VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT)
    {
        strings.push_back("VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT");
    }
    if(enumerator & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
    {
        strings.push_back("VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT");
    }
    if(enumerator & VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT)
    {
        strings.push_back("VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkQueueFlagBits const& enumerator)
{
    VkQueueFlagBits allFlags = (VkQueueFlagBits)(VK_QUEUE_EXTENDED_BIT |
        VK_QUEUE_DMA_BIT |
        VK_QUEUE_COMPUTE_BIT |
        VK_QUEUE_SPARSE_MEMMGR_BIT |
        VK_QUEUE_GRAPHICS_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkQueueFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_QUEUE_EXTENDED_BIT)
    {
        strings.push_back("VK_QUEUE_EXTENDED_BIT");
    }
    if(enumerator & VK_QUEUE_DMA_BIT)
    {
        strings.push_back("VK_QUEUE_DMA_BIT");
    }
    if(enumerator & VK_QUEUE_COMPUTE_BIT)
    {
        strings.push_back("VK_QUEUE_COMPUTE_BIT");
    }
    if(enumerator & VK_QUEUE_SPARSE_MEMMGR_BIT)
    {
        strings.push_back("VK_QUEUE_SPARSE_MEMMGR_BIT");
    }
    if(enumerator & VK_QUEUE_GRAPHICS_BIT)
    {
        strings.push_back("VK_QUEUE_GRAPHICS_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkMemoryPropertyFlagBits const& enumerator)
{
    VkMemoryPropertyFlagBits allFlags = (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT |
        VK_MEMORY_PROPERTY_HOST_WRITE_COMBINED_BIT |
        VK_MEMORY_PROPERTY_HOST_NON_COHERENT_BIT |
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_UNCACHED_BIT |
        VK_MEMORY_PROPERTY_DEVICE_ONLY);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkMemoryPropertyFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
    {
        strings.push_back("VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT");
    }
    if(enumerator & VK_MEMORY_PROPERTY_HOST_WRITE_COMBINED_BIT)
    {
        strings.push_back("VK_MEMORY_PROPERTY_HOST_WRITE_COMBINED_BIT");
    }
    if(enumerator & VK_MEMORY_PROPERTY_HOST_NON_COHERENT_BIT)
    {
        strings.push_back("VK_MEMORY_PROPERTY_HOST_NON_COHERENT_BIT");
    }
    if(enumerator & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        strings.push_back("VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT");
    }
    if(enumerator & VK_MEMORY_PROPERTY_HOST_UNCACHED_BIT)
    {
        strings.push_back("VK_MEMORY_PROPERTY_HOST_UNCACHED_BIT");
    }
    if(enumerator & VK_MEMORY_PROPERTY_DEVICE_ONLY)
    {
        strings.push_back("VK_MEMORY_PROPERTY_DEVICE_ONLY");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkMemoryHeapFlagBits const& enumerator)
{
    VkMemoryHeapFlagBits allFlags = (VkMemoryHeapFlagBits)(VK_MEMORY_HEAP_HOST_LOCAL);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkMemoryHeapFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_MEMORY_HEAP_HOST_LOCAL)
    {
        strings.push_back("VK_MEMORY_HEAP_HOST_LOCAL");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkSparseImageFormatFlagBits const& enumerator)
{
    VkSparseImageFormatFlagBits allFlags = (VkSparseImageFormatFlagBits)(VK_SPARSE_IMAGE_FMT_NONSTD_BLOCK_SIZE_BIT |
        VK_SPARSE_IMAGE_FMT_ALIGNED_MIP_SIZE_BIT |
        VK_SPARSE_IMAGE_FMT_SINGLE_MIPTAIL_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkSparseImageFormatFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_SPARSE_IMAGE_FMT_NONSTD_BLOCK_SIZE_BIT)
    {
        strings.push_back("VK_SPARSE_IMAGE_FMT_NONSTD_BLOCK_SIZE_BIT");
    }
    if(enumerator & VK_SPARSE_IMAGE_FMT_ALIGNED_MIP_SIZE_BIT)
    {
        strings.push_back("VK_SPARSE_IMAGE_FMT_ALIGNED_MIP_SIZE_BIT");
    }
    if(enumerator & VK_SPARSE_IMAGE_FMT_SINGLE_MIPTAIL_BIT)
    {
        strings.push_back("VK_SPARSE_IMAGE_FMT_SINGLE_MIPTAIL_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkSparseMemoryBindFlagBits const& enumerator)
{
    VkSparseMemoryBindFlagBits allFlags = (VkSparseMemoryBindFlagBits)(VK_SPARSE_MEMORY_BIND_REPLICATE_64KIB_BLOCK_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkSparseMemoryBindFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_SPARSE_MEMORY_BIND_REPLICATE_64KIB_BLOCK_BIT)
    {
        strings.push_back("VK_SPARSE_MEMORY_BIND_REPLICATE_64KIB_BLOCK_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkFenceCreateFlagBits const& enumerator)
{
    VkFenceCreateFlagBits allFlags = (VkFenceCreateFlagBits)(VK_FENCE_CREATE_SIGNALED_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkFenceCreateFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_FENCE_CREATE_SIGNALED_BIT)
    {
        strings.push_back("VK_FENCE_CREATE_SIGNALED_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkQueryPipelineStatisticFlagBits const& enumerator)
{
    VkQueryPipelineStatisticFlagBits allFlags = (VkQueryPipelineStatisticFlagBits)(VK_QUERY_PIPELINE_STATISTIC_CS_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_IA_VERTICES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_IA_PRIMITIVES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_C_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_VS_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_GS_PRIMITIVES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_FS_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_C_PRIMITIVES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_TCS_PATCHES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_GS_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_TES_INVOCATIONS_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkQueryPipelineStatisticFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_CS_INVOCATIONS_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_CS_INVOCATIONS_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_IA_VERTICES_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_IA_VERTICES_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_IA_PRIMITIVES_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_IA_PRIMITIVES_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_C_INVOCATIONS_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_C_INVOCATIONS_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_VS_INVOCATIONS_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_VS_INVOCATIONS_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_GS_PRIMITIVES_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_GS_PRIMITIVES_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_FS_INVOCATIONS_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_FS_INVOCATIONS_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_C_PRIMITIVES_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_C_PRIMITIVES_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_TCS_PATCHES_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_TCS_PATCHES_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_GS_INVOCATIONS_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_GS_INVOCATIONS_BIT");
    }
    if(enumerator & VK_QUERY_PIPELINE_STATISTIC_TES_INVOCATIONS_BIT)
    {
        strings.push_back("VK_QUERY_PIPELINE_STATISTIC_TES_INVOCATIONS_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkQueryResultFlagBits const& enumerator)
{
    VkQueryResultFlagBits allFlags = (VkQueryResultFlagBits)(VK_QUERY_RESULT_PARTIAL_BIT |
        VK_QUERY_RESULT_WITH_AVAILABILITY_BIT |
        VK_QUERY_RESULT_WAIT_BIT |
        VK_QUERY_RESULT_64_BIT |
        VK_QUERY_RESULT_DEFAULT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkQueryResultFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_QUERY_RESULT_PARTIAL_BIT)
    {
        strings.push_back("VK_QUERY_RESULT_PARTIAL_BIT");
    }
    if(enumerator & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT)
    {
        strings.push_back("VK_QUERY_RESULT_WITH_AVAILABILITY_BIT");
    }
    if(enumerator & VK_QUERY_RESULT_WAIT_BIT)
    {
        strings.push_back("VK_QUERY_RESULT_WAIT_BIT");
    }
    if(enumerator & VK_QUERY_RESULT_64_BIT)
    {
        strings.push_back("VK_QUERY_RESULT_64_BIT");
    }
    if(enumerator & VK_QUERY_RESULT_DEFAULT)
    {
        strings.push_back("VK_QUERY_RESULT_DEFAULT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkBufferUsageFlagBits const& enumerator)
{
    VkBufferUsageFlagBits allFlags = (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
        VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT |
        VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SOURCE_BIT |
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkBufferUsageFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
    {
        strings.push_back("VK_BUFFER_USAGE_VERTEX_BUFFER_BIT");
    }
    if(enumerator & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
    {
        strings.push_back("VK_BUFFER_USAGE_INDEX_BUFFER_BIT");
    }
    if(enumerator & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
    {
        strings.push_back("VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT");
    }
    if(enumerator & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
    {
        strings.push_back("VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT");
    }
    if(enumerator & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
    {
        strings.push_back("VK_BUFFER_USAGE_STORAGE_BUFFER_BIT");
    }
    if(enumerator & VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT)
    {
        strings.push_back("VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT");
    }
    if(enumerator & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
    {
        strings.push_back("VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT");
    }
    if(enumerator & VK_BUFFER_USAGE_TRANSFER_SOURCE_BIT)
    {
        strings.push_back("VK_BUFFER_USAGE_TRANSFER_SOURCE_BIT");
    }
    if(enumerator & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
    {
        strings.push_back("VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkBufferCreateFlagBits const& enumerator)
{
    VkBufferCreateFlagBits allFlags = (VkBufferCreateFlagBits)(VK_BUFFER_CREATE_SPARSE_ALIASED_BIT |
        VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT |
        VK_BUFFER_CREATE_SPARSE_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkBufferCreateFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_BUFFER_CREATE_SPARSE_ALIASED_BIT)
    {
        strings.push_back("VK_BUFFER_CREATE_SPARSE_ALIASED_BIT");
    }
    if(enumerator & VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT)
    {
        strings.push_back("VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT");
    }
    if(enumerator & VK_BUFFER_CREATE_SPARSE_BIT)
    {
        strings.push_back("VK_BUFFER_CREATE_SPARSE_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkImageCreateFlagBits const& enumerator)
{
    VkImageCreateFlagBits allFlags = (VkImageCreateFlagBits)(VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT |
        VK_IMAGE_CREATE_INVARIANT_DATA_BIT |
        VK_IMAGE_CREATE_SPARSE_ALIASED_BIT |
        VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT |
        VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT |
        VK_IMAGE_CREATE_SPARSE_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkImageCreateFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
    {
        strings.push_back("VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT");
    }
    if(enumerator & VK_IMAGE_CREATE_INVARIANT_DATA_BIT)
    {
        strings.push_back("VK_IMAGE_CREATE_INVARIANT_DATA_BIT");
    }
    if(enumerator & VK_IMAGE_CREATE_SPARSE_ALIASED_BIT)
    {
        strings.push_back("VK_IMAGE_CREATE_SPARSE_ALIASED_BIT");
    }
    if(enumerator & VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT)
    {
        strings.push_back("VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT");
    }
    if(enumerator & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT)
    {
        strings.push_back("VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT");
    }
    if(enumerator & VK_IMAGE_CREATE_SPARSE_BIT)
    {
        strings.push_back("VK_IMAGE_CREATE_SPARSE_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkImageViewCreateFlagBits const& enumerator)
{
    VkImageViewCreateFlagBits allFlags = (VkImageViewCreateFlagBits)(VK_IMAGE_VIEW_CREATE_READ_ONLY_DEPTH_BIT |
        VK_IMAGE_VIEW_CREATE_READ_ONLY_STENCIL_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkImageViewCreateFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_IMAGE_VIEW_CREATE_READ_ONLY_DEPTH_BIT)
    {
        strings.push_back("VK_IMAGE_VIEW_CREATE_READ_ONLY_DEPTH_BIT");
    }
    if(enumerator & VK_IMAGE_VIEW_CREATE_READ_ONLY_STENCIL_BIT)
    {
        strings.push_back("VK_IMAGE_VIEW_CREATE_READ_ONLY_STENCIL_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkChannelFlagBits const& enumerator)
{
    VkChannelFlagBits allFlags = (VkChannelFlagBits)(VK_CHANNEL_A_BIT |
        VK_CHANNEL_B_BIT |
        VK_CHANNEL_G_BIT |
        VK_CHANNEL_R_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkChannelFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_CHANNEL_A_BIT)
    {
        strings.push_back("VK_CHANNEL_A_BIT");
    }
    if(enumerator & VK_CHANNEL_B_BIT)
    {
        strings.push_back("VK_CHANNEL_B_BIT");
    }
    if(enumerator & VK_CHANNEL_G_BIT)
    {
        strings.push_back("VK_CHANNEL_G_BIT");
    }
    if(enumerator & VK_CHANNEL_R_BIT)
    {
        strings.push_back("VK_CHANNEL_R_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkPipelineCreateFlagBits const& enumerator)
{
    VkPipelineCreateFlagBits allFlags = (VkPipelineCreateFlagBits)(VK_PIPELINE_CREATE_DERIVATIVE_BIT |
        VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT |
        VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkPipelineCreateFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_PIPELINE_CREATE_DERIVATIVE_BIT)
    {
        strings.push_back("VK_PIPELINE_CREATE_DERIVATIVE_BIT");
    }
    if(enumerator & VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT)
    {
        strings.push_back("VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT");
    }
    if(enumerator & VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT)
    {
        strings.push_back("VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkShaderStageFlagBits const& enumerator)
{
    VkShaderStageFlagBits allFlags = (VkShaderStageFlagBits)(VK_SHADER_STAGE_ALL |
        VK_SHADER_STAGE_FRAGMENT_BIT |
        VK_SHADER_STAGE_GEOMETRY_BIT |
        VK_SHADER_STAGE_COMPUTE_BIT |
        VK_SHADER_STAGE_TESS_EVALUATION_BIT |
        VK_SHADER_STAGE_TESS_CONTROL_BIT |
        VK_SHADER_STAGE_VERTEX_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkShaderStageFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_SHADER_STAGE_ALL)
    {
        strings.push_back("VK_SHADER_STAGE_ALL");
    }
    if(enumerator & VK_SHADER_STAGE_FRAGMENT_BIT)
    {
        strings.push_back("VK_SHADER_STAGE_FRAGMENT_BIT");
    }
    if(enumerator & VK_SHADER_STAGE_GEOMETRY_BIT)
    {
        strings.push_back("VK_SHADER_STAGE_GEOMETRY_BIT");
    }
    if(enumerator & VK_SHADER_STAGE_COMPUTE_BIT)
    {
        strings.push_back("VK_SHADER_STAGE_COMPUTE_BIT");
    }
    if(enumerator & VK_SHADER_STAGE_TESS_EVALUATION_BIT)
    {
        strings.push_back("VK_SHADER_STAGE_TESS_EVALUATION_BIT");
    }
    if(enumerator & VK_SHADER_STAGE_TESS_CONTROL_BIT)
    {
        strings.push_back("VK_SHADER_STAGE_TESS_CONTROL_BIT");
    }
    if(enumerator & VK_SHADER_STAGE_VERTEX_BIT)
    {
        strings.push_back("VK_SHADER_STAGE_VERTEX_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkSubpassDescriptionFlagBits const& enumerator)
{
    VkSubpassDescriptionFlagBits allFlags = (VkSubpassDescriptionFlagBits)(VK_SUBPASS_DESCRIPTION_NO_OVERDRAW_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkSubpassDescriptionFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_SUBPASS_DESCRIPTION_NO_OVERDRAW_BIT)
    {
        strings.push_back("VK_SUBPASS_DESCRIPTION_NO_OVERDRAW_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkPipelineStageFlagBits const& enumerator)
{
    VkPipelineStageFlagBits allFlags = (VkPipelineStageFlagBits)(VK_PIPELINE_STAGE_ALL_GRAPHICS |
        VK_PIPELINE_STAGE_HOST_BIT |
        VK_PIPELINE_STAGE_TRANSFER_BIT |
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_ALL_GPU_COMMANDS |
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT |
        VK_PIPELINE_STAGE_TRANSITION_BIT |
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
        VK_PIPELINE_STAGE_TESS_CONTROL_SHADER_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
        VK_PIPELINE_STAGE_TESS_EVALUATION_SHADER_BIT |
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
        VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkPipelineStageFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_PIPELINE_STAGE_ALL_GRAPHICS)
    {
        strings.push_back("VK_PIPELINE_STAGE_ALL_GRAPHICS");
    }
    if(enumerator & VK_PIPELINE_STAGE_HOST_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_HOST_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_TRANSFER_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_TRANSFER_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_ALL_GPU_COMMANDS)
    {
        strings.push_back("VK_PIPELINE_STAGE_ALL_GPU_COMMANDS");
    }
    if(enumerator & VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_TRANSITION_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_TRANSITION_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_TESS_CONTROL_SHADER_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_TESS_CONTROL_SHADER_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_VERTEX_SHADER_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_VERTEX_SHADER_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_TESS_EVALUATION_SHADER_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_TESS_EVALUATION_SHADER_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_VERTEX_INPUT_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_VERTEX_INPUT_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT");
    }
    if(enumerator & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
    {
        strings.push_back("VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkMemoryOutputFlagBits const& enumerator)
{
    VkMemoryOutputFlagBits allFlags = (VkMemoryOutputFlagBits)(VK_MEMORY_OUTPUT_DEPTH_STENCIL_ATTACHMENT_BIT |
        VK_MEMORY_OUTPUT_TRANSFER_BIT |
        VK_MEMORY_OUTPUT_COLOR_ATTACHMENT_BIT |
        VK_MEMORY_OUTPUT_SHADER_WRITE_BIT |
        VK_MEMORY_OUTPUT_HOST_WRITE_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkMemoryOutputFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_MEMORY_OUTPUT_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        strings.push_back("VK_MEMORY_OUTPUT_DEPTH_STENCIL_ATTACHMENT_BIT");
    }
    if(enumerator & VK_MEMORY_OUTPUT_TRANSFER_BIT)
    {
        strings.push_back("VK_MEMORY_OUTPUT_TRANSFER_BIT");
    }
    if(enumerator & VK_MEMORY_OUTPUT_COLOR_ATTACHMENT_BIT)
    {
        strings.push_back("VK_MEMORY_OUTPUT_COLOR_ATTACHMENT_BIT");
    }
    if(enumerator & VK_MEMORY_OUTPUT_SHADER_WRITE_BIT)
    {
        strings.push_back("VK_MEMORY_OUTPUT_SHADER_WRITE_BIT");
    }
    if(enumerator & VK_MEMORY_OUTPUT_HOST_WRITE_BIT)
    {
        strings.push_back("VK_MEMORY_OUTPUT_HOST_WRITE_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkMemoryInputFlagBits const& enumerator)
{
    VkMemoryInputFlagBits allFlags = (VkMemoryInputFlagBits)(VK_MEMORY_INPUT_TRANSFER_BIT |
        VK_MEMORY_INPUT_DEPTH_STENCIL_ATTACHMENT_BIT |
        VK_MEMORY_INPUT_COLOR_ATTACHMENT_BIT |
        VK_MEMORY_INPUT_SHADER_READ_BIT |
        VK_MEMORY_INPUT_UNIFORM_READ_BIT |
        VK_MEMORY_INPUT_INDEX_FETCH_BIT |
        VK_MEMORY_INPUT_INDIRECT_COMMAND_BIT |
        VK_MEMORY_INPUT_INPUT_ATTACHMENT_BIT |
        VK_MEMORY_INPUT_VERTEX_ATTRIBUTE_FETCH_BIT |
        VK_MEMORY_INPUT_HOST_READ_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkMemoryInputFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_MEMORY_INPUT_TRANSFER_BIT)
    {
        strings.push_back("VK_MEMORY_INPUT_TRANSFER_BIT");
    }
    if(enumerator & VK_MEMORY_INPUT_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        strings.push_back("VK_MEMORY_INPUT_DEPTH_STENCIL_ATTACHMENT_BIT");
    }
    if(enumerator & VK_MEMORY_INPUT_COLOR_ATTACHMENT_BIT)
    {
        strings.push_back("VK_MEMORY_INPUT_COLOR_ATTACHMENT_BIT");
    }
    if(enumerator & VK_MEMORY_INPUT_SHADER_READ_BIT)
    {
        strings.push_back("VK_MEMORY_INPUT_SHADER_READ_BIT");
    }
    if(enumerator & VK_MEMORY_INPUT_UNIFORM_READ_BIT)
    {
        strings.push_back("VK_MEMORY_INPUT_UNIFORM_READ_BIT");
    }
    if(enumerator & VK_MEMORY_INPUT_INDEX_FETCH_BIT)
    {
        strings.push_back("VK_MEMORY_INPUT_INDEX_FETCH_BIT");
    }
    if(enumerator & VK_MEMORY_INPUT_INDIRECT_COMMAND_BIT)
    {
        strings.push_back("VK_MEMORY_INPUT_INDIRECT_COMMAND_BIT");
    }
    if(enumerator & VK_MEMORY_INPUT_INPUT_ATTACHMENT_BIT)
    {
        strings.push_back("VK_MEMORY_INPUT_INPUT_ATTACHMENT_BIT");
    }
    if(enumerator & VK_MEMORY_INPUT_VERTEX_ATTRIBUTE_FETCH_BIT)
    {
        strings.push_back("VK_MEMORY_INPUT_VERTEX_ATTRIBUTE_FETCH_BIT");
    }
    if(enumerator & VK_MEMORY_INPUT_HOST_READ_BIT)
    {
        strings.push_back("VK_MEMORY_INPUT_HOST_READ_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkCmdPoolCreateFlagBits const& enumerator)
{
    VkCmdPoolCreateFlagBits allFlags = (VkCmdPoolCreateFlagBits)(VK_CMD_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
        VK_CMD_POOL_CREATE_TRANSIENT_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkCmdPoolCreateFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_CMD_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
    {
        strings.push_back("VK_CMD_POOL_CREATE_RESET_COMMAND_BUFFER_BIT");
    }
    if(enumerator & VK_CMD_POOL_CREATE_TRANSIENT_BIT)
    {
        strings.push_back("VK_CMD_POOL_CREATE_TRANSIENT_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkCmdPoolResetFlagBits const& enumerator)
{
    VkCmdPoolResetFlagBits allFlags = (VkCmdPoolResetFlagBits)(VK_CMD_POOL_RESET_RELEASE_RESOURCES);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkCmdPoolResetFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_CMD_POOL_RESET_RELEASE_RESOURCES)
    {
        strings.push_back("VK_CMD_POOL_RESET_RELEASE_RESOURCES");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkCmdBufferOptimizeFlagBits const& enumerator)
{
    VkCmdBufferOptimizeFlagBits allFlags = (VkCmdBufferOptimizeFlagBits)(VK_CMD_BUFFER_OPTIMIZE_NO_SIMULTANEOUS_USE_BIT |
        VK_CMD_BUFFER_OPTIMIZE_DESCRIPTOR_SET_SWITCH_BIT |
        VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT |
        VK_CMD_BUFFER_OPTIMIZE_PIPELINE_SWITCH_BIT |
        VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkCmdBufferOptimizeFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_CMD_BUFFER_OPTIMIZE_NO_SIMULTANEOUS_USE_BIT)
    {
        strings.push_back("VK_CMD_BUFFER_OPTIMIZE_NO_SIMULTANEOUS_USE_BIT");
    }
    if(enumerator & VK_CMD_BUFFER_OPTIMIZE_DESCRIPTOR_SET_SWITCH_BIT)
    {
        strings.push_back("VK_CMD_BUFFER_OPTIMIZE_DESCRIPTOR_SET_SWITCH_BIT");
    }
    if(enumerator & VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT)
    {
        strings.push_back("VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT");
    }
    if(enumerator & VK_CMD_BUFFER_OPTIMIZE_PIPELINE_SWITCH_BIT)
    {
        strings.push_back("VK_CMD_BUFFER_OPTIMIZE_PIPELINE_SWITCH_BIT");
    }
    if(enumerator & VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT)
    {
        strings.push_back("VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkCmdBufferResetFlagBits const& enumerator)
{
    VkCmdBufferResetFlagBits allFlags = (VkCmdBufferResetFlagBits)(VK_CMD_BUFFER_RESET_RELEASE_RESOURCES);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkCmdBufferResetFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_CMD_BUFFER_RESET_RELEASE_RESOURCES)
    {
        strings.push_back("VK_CMD_BUFFER_RESET_RELEASE_RESOURCES");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkImageAspectFlagBits const& enumerator)
{
    VkImageAspectFlagBits allFlags = (VkImageAspectFlagBits)(VK_IMAGE_ASPECT_METADATA_BIT |
        VK_IMAGE_ASPECT_STENCIL_BIT |
        VK_IMAGE_ASPECT_DEPTH_BIT |
        VK_IMAGE_ASPECT_COLOR_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkImageAspectFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_IMAGE_ASPECT_METADATA_BIT)
    {
        strings.push_back("VK_IMAGE_ASPECT_METADATA_BIT");
    }
    if(enumerator & VK_IMAGE_ASPECT_STENCIL_BIT)
    {
        strings.push_back("VK_IMAGE_ASPECT_STENCIL_BIT");
    }
    if(enumerator & VK_IMAGE_ASPECT_DEPTH_BIT)
    {
        strings.push_back("VK_IMAGE_ASPECT_DEPTH_BIT");
    }
    if(enumerator & VK_IMAGE_ASPECT_COLOR_BIT)
    {
        strings.push_back("VK_IMAGE_ASPECT_COLOR_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

static
bool ValidateEnumerator(VkQueryControlFlagBits const& enumerator)
{
    VkQueryControlFlagBits allFlags = (VkQueryControlFlagBits)(VK_QUERY_CONTROL_CONSERVATIVE_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkQueryControlFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_QUERY_CONTROL_CONSERVATIVE_BIT)
    {
        strings.push_back("VK_QUERY_CONTROL_CONSERVATIVE_BIT");
    }

    std::string enumeratorString;
    for(auto const& string : strings)
    {
        enumeratorString += string;

        if(string != strings.back())
        {
            enumeratorString += '|';
        }
    }

    return enumeratorString;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    VkInstance* pInstance)
{
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(pc_instance_table_map, *pInstance);
    VkResult result = pTable->CreateInstance(pCreateInfo, pInstance);

    if (result == VK_SUCCESS) {
        layer_data *data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
        data->report_data = debug_report_create_instance(pTable, *pInstance, pCreateInfo->extensionCount,
            pCreateInfo->ppEnabledExtensionNames);

        InitParamChecker(data);
    }

        return result;
}

VK_LAYER_EXPORT void VKAPI vkDestroyInstance(
    VkInstance instance)
{
    // Grab the key before the instance is destroyed.
    dispatch_key key = get_dispatch_key(instance);
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(pc_instance_table_map, instance);
    pTable->DestroyInstance(instance);

    // Clean up logging callback, if any
    layer_data *data = get_my_data_ptr(key, layer_data_map);
    if(data->logging_callback)
    {
        layer_destroy_msg_callback(data->report_data, data->logging_callback);
    }

    layer_debug_report_destroy_instance(mid(instance));
    layer_data_map.erase(pTable);

    pc_instance_table_map.erase(key);
    assert(pc_instance_table_map.size() == 0 && "Should not have any instance mappings hanging around");
}

bool PostEnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t* pPhysicalDeviceCount,
    VkPhysicalDevice* pPhysicalDevices,
    VkResult result)
{

    if(pPhysicalDeviceCount != nullptr)
    {
    }

    if(pPhysicalDevices != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkEnumeratePhysicalDevices parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mid(instance), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t* pPhysicalDeviceCount,
    VkPhysicalDevice* pPhysicalDevices)
{
    VkResult result = get_dispatch_table(pc_instance_table_map, instance)->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);

    PostEnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices, result);

    return result;
}

bool PostGetPhysicalDeviceFeatures(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceFeatures* pFeatures,
    VkResult result)
{

    if(pFeatures != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetPhysicalDeviceFeatures parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceFeatures(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceFeatures* pFeatures)
{
    VkResult result = get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceFeatures(physicalDevice, pFeatures);

    PostGetPhysicalDeviceFeatures(physicalDevice, pFeatures, result);

    return result;
}

bool PostGetPhysicalDeviceFormatProperties(
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkFormatProperties* pFormatProperties,
    VkResult result)
{

    if(format < VK_FORMAT_BEGIN_RANGE ||
        format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceFormatProperties parameter, VkFormat format, is an unrecognized enumerator");
        return false;
    }

    if(pFormatProperties != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetPhysicalDeviceFormatProperties parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceFormatProperties(
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkFormatProperties* pFormatProperties)
{
    VkResult result = get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties);

    PostGetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties, result);

    return result;
}

bool PostGetPhysicalDeviceImageFormatProperties(
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkImageType type,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkImageCreateFlags flags,
    VkImageFormatProperties* pImageFormatProperties,
    VkResult result)
{

    if(format < VK_FORMAT_BEGIN_RANGE ||
        format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceImageFormatProperties parameter, VkFormat format, is an unrecognized enumerator");
        return false;
    }

    if(type < VK_IMAGE_TYPE_BEGIN_RANGE ||
        type > VK_IMAGE_TYPE_END_RANGE)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceImageFormatProperties parameter, VkImageType type, is an unrecognized enumerator");
        return false;
    }

    if(tiling < VK_IMAGE_TILING_BEGIN_RANGE ||
        tiling > VK_IMAGE_TILING_END_RANGE)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceImageFormatProperties parameter, VkImageTiling tiling, is an unrecognized enumerator");
        return false;
    }


    if(pImageFormatProperties != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetPhysicalDeviceImageFormatProperties parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceImageFormatProperties(
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkImageType type,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkImageCreateFlags flags,
    VkImageFormatProperties* pImageFormatProperties)
{
    VkResult result = get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);

    PostGetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties, result);

    return result;
}

bool PostGetPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties* pProperties,
    VkResult result)
{

    if(pProperties != nullptr)
    {
    if(pProperties->deviceType < VK_PHYSICAL_DEVICE_TYPE_BEGIN_RANGE ||
        pProperties->deviceType > VK_PHYSICAL_DEVICE_TYPE_END_RANGE)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceProperties parameter, VkPhysicalDeviceType pProperties->deviceType, is an unrecognized enumerator");
        return false;
    }
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetPhysicalDeviceProperties parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties* pProperties)
{
    VkResult result = get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceProperties(physicalDevice, pProperties);

    PostGetPhysicalDeviceProperties(physicalDevice, pProperties, result);

    return result;
}

bool PostGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice physicalDevice,
    uint32_t* pCount,
    VkQueueFamilyProperties* pQueueProperties,
    VkResult result)
{

    if(pQueueProperties == nullptr && pCount != nullptr)
    {
    }

    if(pQueueProperties != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetPhysicalDeviceQueueFamilyProperties parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceQueueProperties(
    VkPhysicalDevice physicalDevice,
    uint32_t* pCount,
    VkQueueFamilyProperties* pQueueProperties)
{
    VkResult result = get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceQueueFamilyProperties(physicalDevice, pCount, pQueueProperties);

    PostGetPhysicalDeviceQueueFamilyProperties(physicalDevice, pCount, pQueueProperties, result);

    return result;
}

bool PostGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties* pMemoryProperties,
    VkResult result)
{

    if(pMemoryProperties != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetPhysicalDeviceMemoryProperties parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties* pMemoryProperties)
{
    VkResult result = get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);

    PostGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties, result);

    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    VkDevice* pDevice)
{
    /*
     * NOTE: The loader fills in the ICD's device object in *pDevice.
     * Use that object to get the dispatch table.
     *
     * NOTE: We do not validate physicalDevice or any dispatchable
     * object as the first parameter. We couldn't get here if it was wrong!
     */
    VkLayerDispatchTable *pTable = get_dispatch_table(pc_device_table_map, *pDevice);
    VkResult result = pTable->CreateDevice(physicalDevice, pCreateInfo, pDevice);
    if(result == VK_SUCCESS)
    {
        layer_data *instance_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
        layer_data *device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
        device_data->report_data = layer_debug_report_create_device(instance_data->report_data, *pDevice);
    }

    return result;
}

VK_LAYER_EXPORT void VKAPI vkDestroyDevice(
    VkDevice device)
{
    layer_debug_report_destroy_device(device);

    dispatch_key key = get_dispatch_key(device);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "Device: %p, key: %p\n", device, key);
#endif

    get_dispatch_table(pc_device_table_map, device)->DestroyDevice(device);
    pc_device_table_map.erase(key);
    assert(pc_device_table_map.size() == 0 && "Should not have any instance mappings hanging around");
}

bool PostGetDeviceQueue(
    VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex,
    VkQueue* pQueue,
    VkResult result)
{



    if(pQueue != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetDeviceQueue parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetDeviceQueue(
    VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex,
    VkQueue* pQueue)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);

    PostGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue, result);

    return result;
}

bool PreQueueSubmit(
    VkQueue queue,
    const VkCmdBuffer* pCmdBuffers)
{
    if(pCmdBuffers != nullptr)
    {
    }

    return true;
}

bool PostQueueSubmit(
    VkQueue queue,
    uint32_t cmdBufferCount,
    VkFence fence,
    VkResult result)
{



    if(result < VK_SUCCESS)
    {
        std::string reason = "vkQueueSubmit parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueSubmit(
    VkQueue queue,
    uint32_t cmdBufferCount,
    const VkCmdBuffer* pCmdBuffers,
    VkFence fence)
{
    PreQueueSubmit(queue, pCmdBuffers);

    VkResult result = get_dispatch_table(pc_device_table_map, queue)->QueueSubmit(queue, cmdBufferCount, pCmdBuffers, fence);

    PostQueueSubmit(queue, cmdBufferCount, fence, result);

    return result;
}

bool PostQueueWaitIdle(
    VkQueue queue,
    VkResult result)
{

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkQueueWaitIdle parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueWaitIdle(
    VkQueue queue)
{
    VkResult result = get_dispatch_table(pc_device_table_map, queue)->QueueWaitIdle(queue);

    PostQueueWaitIdle(queue, result);

    return result;
}

bool PostDeviceWaitIdle(
    VkDevice device,
    VkResult result)
{

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkDeviceWaitIdle parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkDeviceWaitIdle(
    VkDevice device)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->DeviceWaitIdle(device);

    PostDeviceWaitIdle(device, result);

    return result;
}

bool PreAllocMemory(
    VkDevice device,
    const VkMemoryAllocInfo* pAllocInfo)
{
    if(pAllocInfo != nullptr)
    {
    if(pAllocInfo->sType != VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkAllocMemory parameter, VkStructureType pAllocInfo->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostAllocMemory(
    VkDevice device,
    VkDeviceMemory* pMem,
    VkResult result)
{

    if(pMem != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkAllocMemory parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkAllocMemory(
    VkDevice device,
    const VkMemoryAllocInfo* pAllocInfo,
    VkDeviceMemory* pMem)
{
    PreAllocMemory(device, pAllocInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->AllocMemory(device, pAllocInfo, pMem);

    PostAllocMemory(device, pMem, result);

    return result;
}

bool PostMapMemory(
    VkDevice device,
    VkDeviceMemory mem,
    VkDeviceSize offset,
    VkDeviceSize size,
    VkMemoryMapFlags flags,
    void** ppData,
    VkResult result)
{





    if(ppData != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkMapMemory parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkMapMemory(
    VkDevice device,
    VkDeviceMemory mem,
    VkDeviceSize offset,
    VkDeviceSize size,
    VkMemoryMapFlags flags,
    void** ppData)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->MapMemory(device, mem, offset, size, flags, ppData);

    PostMapMemory(device, mem, offset, size, flags, ppData, result);

    return result;
}

bool PreFlushMappedMemoryRanges(
    VkDevice device,
    const VkMappedMemoryRange* pMemRanges)
{
    if(pMemRanges != nullptr)
    {
    if(pMemRanges->sType != VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkFlushMappedMemoryRanges parameter, VkStructureType pMemRanges->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostFlushMappedMemoryRanges(
    VkDevice device,
    uint32_t memRangeCount,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkFlushMappedMemoryRanges parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkFlushMappedMemoryRanges(
    VkDevice device,
    uint32_t memRangeCount,
    const VkMappedMemoryRange* pMemRanges)
{
    PreFlushMappedMemoryRanges(device, pMemRanges);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->FlushMappedMemoryRanges(device, memRangeCount, pMemRanges);

    PostFlushMappedMemoryRanges(device, memRangeCount, result);

    return result;
}

bool PreInvalidateMappedMemoryRanges(
    VkDevice device,
    const VkMappedMemoryRange* pMemRanges)
{
    if(pMemRanges != nullptr)
    {
    if(pMemRanges->sType != VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkInvalidateMappedMemoryRanges parameter, VkStructureType pMemRanges->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostInvalidateMappedMemoryRanges(
    VkDevice device,
    uint32_t memRangeCount,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkInvalidateMappedMemoryRanges parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkInvalidateMappedMemoryRanges(
    VkDevice device,
    uint32_t memRangeCount,
    const VkMappedMemoryRange* pMemRanges)
{
    PreInvalidateMappedMemoryRanges(device, pMemRanges);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->InvalidateMappedMemoryRanges(device, memRangeCount, pMemRanges);

    PostInvalidateMappedMemoryRanges(device, memRangeCount, result);

    return result;
}

bool PostGetDeviceMemoryCommitment(
    VkDevice device,
    VkDeviceMemory memory,
    VkDeviceSize* pCommittedMemoryInBytes,
    VkResult result)
{


    if(pCommittedMemoryInBytes != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetDeviceMemoryCommitment parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetDeviceMemoryCommitment(
    VkDevice device,
    VkDeviceMemory memory,
    VkDeviceSize* pCommittedMemoryInBytes)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->GetDeviceMemoryCommitment(device, memory, pCommittedMemoryInBytes);

    PostGetDeviceMemoryCommitment(device, memory, pCommittedMemoryInBytes, result);

    return result;
}

bool PostBindBufferMemory(
    VkDevice device,
    VkBuffer buffer,
    VkDeviceMemory mem,
    VkDeviceSize memOffset,
    VkResult result)
{




    if(result < VK_SUCCESS)
    {
        std::string reason = "vkBindBufferMemory parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkBindBufferMemory(
    VkDevice device,
    VkBuffer buffer,
    VkDeviceMemory mem,
    VkDeviceSize memOffset)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->BindBufferMemory(device, buffer, mem, memOffset);

    PostBindBufferMemory(device, buffer, mem, memOffset, result);

    return result;
}

bool PostBindImageMemory(
    VkDevice device,
    VkImage image,
    VkDeviceMemory mem,
    VkDeviceSize memOffset,
    VkResult result)
{




    if(result < VK_SUCCESS)
    {
        std::string reason = "vkBindImageMemory parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkBindImageMemory(
    VkDevice device,
    VkImage image,
    VkDeviceMemory mem,
    VkDeviceSize memOffset)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->BindImageMemory(device, image, mem, memOffset);

    PostBindImageMemory(device, image, mem, memOffset, result);

    return result;
}

bool PostGetBufferMemoryRequirements(
    VkDevice device,
    VkBuffer buffer,
    VkMemoryRequirements* pMemoryRequirements,
    VkResult result)
{


    if(pMemoryRequirements != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetBufferMemoryRequirements parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetBufferMemoryRequirements(
    VkDevice device,
    VkBuffer buffer,
    VkMemoryRequirements* pMemoryRequirements)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->GetBufferMemoryRequirements(device, buffer, pMemoryRequirements);

    PostGetBufferMemoryRequirements(device, buffer, pMemoryRequirements, result);

    return result;
}

bool PostGetImageMemoryRequirements(
    VkDevice device,
    VkImage image,
    VkMemoryRequirements* pMemoryRequirements,
    VkResult result)
{


    if(pMemoryRequirements != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetImageMemoryRequirements parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetImageMemoryRequirements(
    VkDevice device,
    VkImage image,
    VkMemoryRequirements* pMemoryRequirements)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->GetImageMemoryRequirements(device, image, pMemoryRequirements);

    PostGetImageMemoryRequirements(device, image, pMemoryRequirements, result);

    return result;
}

bool PostGetImageSparseMemoryRequirements(
    VkDevice device,
    VkImage image,
    uint32_t* pNumRequirements,
    VkSparseImageMemoryRequirements* pSparseMemoryRequirements,
    VkResult result)
{


    if(pNumRequirements != nullptr)
    {
    }

    if(pSparseMemoryRequirements != nullptr)
    {
    if(pSparseMemoryRequirements->formatProps.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pSparseMemoryRequirements->formatProps.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkGetImageSparseMemoryRequirements parameter, VkImageAspect pSparseMemoryRequirements->formatProps.aspect, is an unrecognized enumerator");
        return false;
    }
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetImageSparseMemoryRequirements parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetImageSparseMemoryRequirements(
    VkDevice device,
    VkImage image,
    uint32_t* pNumRequirements,
    VkSparseImageMemoryRequirements* pSparseMemoryRequirements)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->GetImageSparseMemoryRequirements(device, image, pNumRequirements, pSparseMemoryRequirements);

    PostGetImageSparseMemoryRequirements(device, image, pNumRequirements, pSparseMemoryRequirements, result);

    return result;
}

bool PostGetPhysicalDeviceSparseImageFormatProperties(
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkImageType type,
    uint32_t samples,
    VkImageUsageFlags usage,
    VkImageTiling tiling,
    uint32_t* pNumProperties,
    VkSparseImageFormatProperties* pProperties,
    VkResult result)
{

    if(format < VK_FORMAT_BEGIN_RANGE ||
        format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceSparseImageFormatProperties parameter, VkFormat format, is an unrecognized enumerator");
        return false;
    }

    if(type < VK_IMAGE_TYPE_BEGIN_RANGE ||
        type > VK_IMAGE_TYPE_END_RANGE)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceSparseImageFormatProperties parameter, VkImageType type, is an unrecognized enumerator");
        return false;
    }



    if(tiling < VK_IMAGE_TILING_BEGIN_RANGE ||
        tiling > VK_IMAGE_TILING_END_RANGE)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceSparseImageFormatProperties parameter, VkImageTiling tiling, is an unrecognized enumerator");
        return false;
    }

    if(pNumProperties != nullptr)
    {
    }

    if(pProperties != nullptr)
    {
    if(pProperties->aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pProperties->aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceSparseImageFormatProperties parameter, VkImageAspect pProperties->aspect, is an unrecognized enumerator");
        return false;
    }
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetPhysicalDeviceSparseImageFormatProperties parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceSparseImageFormatProperties(
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkImageType type,
    uint32_t samples,
    VkImageUsageFlags usage,
    VkImageTiling tiling,
    uint32_t* pNumProperties,
    VkSparseImageFormatProperties* pProperties)
{
    VkResult result = get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pNumProperties, pProperties);

    PostGetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pNumProperties, pProperties, result);

    return result;
}

bool PreQueueBindSparseBufferMemory(
    VkQueue queue,
    const VkSparseMemoryBindInfo* pBindInfo)
{
    if(pBindInfo != nullptr)
    {
    }

    return true;
}

bool PostQueueBindSparseBufferMemory(
    VkQueue queue,
    VkBuffer buffer,
    uint32_t numBindings,
    VkResult result)
{



    if(result < VK_SUCCESS)
    {
        std::string reason = "vkQueueBindSparseBufferMemory parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueBindSparseBufferMemory(
    VkQueue queue,
    VkBuffer buffer,
    uint32_t numBindings,
    const VkSparseMemoryBindInfo* pBindInfo)
{
    PreQueueBindSparseBufferMemory(queue, pBindInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, queue)->QueueBindSparseBufferMemory(queue, buffer, numBindings, pBindInfo);

    PostQueueBindSparseBufferMemory(queue, buffer, numBindings, result);

    return result;
}

bool PreQueueBindSparseImageOpaqueMemory(
    VkQueue queue,
    const VkSparseMemoryBindInfo* pBindInfo)
{
    if(pBindInfo != nullptr)
    {
    }

    return true;
}

bool PostQueueBindSparseImageOpaqueMemory(
    VkQueue queue,
    VkImage image,
    uint32_t numBindings,
    VkResult result)
{



    if(result < VK_SUCCESS)
    {
        std::string reason = "vkQueueBindSparseImageOpaqueMemory parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueBindSparseImageOpaqueMemory(
    VkQueue queue,
    VkImage image,
    uint32_t numBindings,
    const VkSparseMemoryBindInfo* pBindInfo)
{
    PreQueueBindSparseImageOpaqueMemory(queue, pBindInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, queue)->QueueBindSparseImageOpaqueMemory(queue, image, numBindings, pBindInfo);

    PostQueueBindSparseImageOpaqueMemory(queue, image, numBindings, result);

    return result;
}

bool PreQueueBindSparseImageMemory(
    VkQueue queue,
    const VkSparseImageMemoryBindInfo* pBindInfo)
{
    if(pBindInfo != nullptr)
    {
    if(pBindInfo->subresource.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pBindInfo->subresource.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkQueueBindSparseImageMemory parameter, VkImageAspect pBindInfo->subresource.aspect, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostQueueBindSparseImageMemory(
    VkQueue queue,
    VkImage image,
    uint32_t numBindings,
    VkResult result)
{



    if(result < VK_SUCCESS)
    {
        std::string reason = "vkQueueBindSparseImageMemory parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueBindSparseImageMemory(
    VkQueue queue,
    VkImage image,
    uint32_t numBindings,
    const VkSparseImageMemoryBindInfo* pBindInfo)
{
    PreQueueBindSparseImageMemory(queue, pBindInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, queue)->QueueBindSparseImageMemory(queue, image, numBindings, pBindInfo);

    PostQueueBindSparseImageMemory(queue, image, numBindings, result);

    return result;
}

bool PreCreateFence(
    VkDevice device,
    const VkFenceCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_FENCE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateFence parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateFence(
    VkDevice device,
    VkFence* pFence,
    VkResult result)
{

    if(pFence != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateFence parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateFence(
    VkDevice device,
    const VkFenceCreateInfo* pCreateInfo,
    VkFence* pFence)
{
    PreCreateFence(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateFence(device, pCreateInfo, pFence);

    PostCreateFence(device, pFence, result);

    return result;
}

bool PreResetFences(
    VkDevice device,
    const VkFence* pFences)
{
    if(pFences != nullptr)
    {
    }

    return true;
}

bool PostResetFences(
    VkDevice device,
    uint32_t fenceCount,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkResetFences parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetFences(
    VkDevice device,
    uint32_t fenceCount,
    const VkFence* pFences)
{
    PreResetFences(device, pFences);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->ResetFences(device, fenceCount, pFences);

    PostResetFences(device, fenceCount, result);

    return result;
}

bool PostGetFenceStatus(
    VkDevice device,
    VkFence fence,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetFenceStatus parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetFenceStatus(
    VkDevice device,
    VkFence fence)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->GetFenceStatus(device, fence);

    PostGetFenceStatus(device, fence, result);

    return result;
}

bool PreWaitForFences(
    VkDevice device,
    const VkFence* pFences)
{
    if(pFences != nullptr)
    {
    }

    return true;
}

bool PostWaitForFences(
    VkDevice device,
    uint32_t fenceCount,
    VkBool32 waitAll,
    uint64_t timeout,
    VkResult result)
{




    if(result < VK_SUCCESS)
    {
        std::string reason = "vkWaitForFences parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkWaitForFences(
    VkDevice device,
    uint32_t fenceCount,
    const VkFence* pFences,
    VkBool32 waitAll,
    uint64_t timeout)
{
    PreWaitForFences(device, pFences);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->WaitForFences(device, fenceCount, pFences, waitAll, timeout);

    PostWaitForFences(device, fenceCount, waitAll, timeout, result);

    return result;
}

bool PreCreateSemaphore(
    VkDevice device,
    const VkSemaphoreCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateSemaphore parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateSemaphore(
    VkDevice device,
    VkSemaphore* pSemaphore,
    VkResult result)
{

    if(pSemaphore != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateSemaphore parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateSemaphore(
    VkDevice device,
    const VkSemaphoreCreateInfo* pCreateInfo,
    VkSemaphore* pSemaphore)
{
    PreCreateSemaphore(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateSemaphore(device, pCreateInfo, pSemaphore);

    PostCreateSemaphore(device, pSemaphore, result);

    return result;
}

bool PostQueueSignalSemaphore(
    VkQueue queue,
    VkSemaphore semaphore,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkQueueSignalSemaphore parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueSignalSemaphore(
    VkQueue queue,
    VkSemaphore semaphore)
{
    VkResult result = get_dispatch_table(pc_device_table_map, queue)->QueueSignalSemaphore(queue, semaphore);

    PostQueueSignalSemaphore(queue, semaphore, result);

    return result;
}

bool PostQueueWaitSemaphore(
    VkQueue queue,
    VkSemaphore semaphore,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkQueueWaitSemaphore parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueWaitSemaphore(
    VkQueue queue,
    VkSemaphore semaphore)
{
    VkResult result = get_dispatch_table(pc_device_table_map, queue)->QueueWaitSemaphore(queue, semaphore);

    PostQueueWaitSemaphore(queue, semaphore, result);

    return result;
}

bool PreCreateEvent(
    VkDevice device,
    const VkEventCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_EVENT_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateEvent parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateEvent(
    VkDevice device,
    VkEvent* pEvent,
    VkResult result)
{

    if(pEvent != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateEvent parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateEvent(
    VkDevice device,
    const VkEventCreateInfo* pCreateInfo,
    VkEvent* pEvent)
{
    PreCreateEvent(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateEvent(device, pCreateInfo, pEvent);

    PostCreateEvent(device, pEvent, result);

    return result;
}

bool PostGetEventStatus(
    VkDevice device,
    VkEvent event,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetEventStatus parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetEventStatus(
    VkDevice device,
    VkEvent event)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->GetEventStatus(device, event);

    PostGetEventStatus(device, event, result);

    return result;
}

bool PostSetEvent(
    VkDevice device,
    VkEvent event,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkSetEvent parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkSetEvent(
    VkDevice device,
    VkEvent event)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->SetEvent(device, event);

    PostSetEvent(device, event, result);

    return result;
}

bool PostResetEvent(
    VkDevice device,
    VkEvent event,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkResetEvent parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetEvent(
    VkDevice device,
    VkEvent event)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->ResetEvent(device, event);

    PostResetEvent(device, event, result);

    return result;
}

bool PreCreateQueryPool(
    VkDevice device,
    const VkQueryPoolCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateQueryPool parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->queryType < VK_QUERY_TYPE_BEGIN_RANGE ||
        pCreateInfo->queryType > VK_QUERY_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateQueryPool parameter, VkQueryType pCreateInfo->queryType, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateQueryPool(
    VkDevice device,
    VkQueryPool* pQueryPool,
    VkResult result)
{

    if(pQueryPool != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateQueryPool parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateQueryPool(
    VkDevice device,
    const VkQueryPoolCreateInfo* pCreateInfo,
    VkQueryPool* pQueryPool)
{
    PreCreateQueryPool(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateQueryPool(device, pCreateInfo, pQueryPool);

    PostCreateQueryPool(device, pQueryPool, result);

    return result;
}

bool PostGetQueryPoolResults(
    VkDevice device,
    VkQueryPool queryPool,
    uint32_t startQuery,
    uint32_t queryCount,
    size_t* pDataSize,
    void* pData,
    VkQueryResultFlags flags,
    VkResult result)
{




    if(pDataSize != nullptr)
    {
    }

    if(pData != nullptr)
    {
    }


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetQueryPoolResults parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetQueryPoolResults(
    VkDevice device,
    VkQueryPool queryPool,
    uint32_t startQuery,
    uint32_t queryCount,
    size_t* pDataSize,
    void* pData,
    VkQueryResultFlags flags)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->GetQueryPoolResults(device, queryPool, startQuery, queryCount, pDataSize, pData, flags);

    PostGetQueryPoolResults(device, queryPool, startQuery, queryCount, pDataSize, pData, flags, result);

    return result;
}

bool PreCreateBuffer(
    VkDevice device,
    const VkBufferCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateBuffer parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->sharingMode < VK_SHARING_MODE_BEGIN_RANGE ||
        pCreateInfo->sharingMode > VK_SHARING_MODE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateBuffer parameter, VkSharingMode pCreateInfo->sharingMode, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->pQueueFamilyIndices != nullptr)
    {
    }
    }

    return true;
}

bool PostCreateBuffer(
    VkDevice device,
    VkBuffer* pBuffer,
    VkResult result)
{

    if(pBuffer != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateBuffer parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateBuffer(
    VkDevice device,
    const VkBufferCreateInfo* pCreateInfo,
    VkBuffer* pBuffer)
{
    PreCreateBuffer(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateBuffer(device, pCreateInfo, pBuffer);

    PostCreateBuffer(device, pBuffer, result);

    return result;
}

bool PreCreateBufferView(
    VkDevice device,
    const VkBufferViewCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateBufferView parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->viewType < VK_BUFFER_VIEW_TYPE_BEGIN_RANGE ||
        pCreateInfo->viewType > VK_BUFFER_VIEW_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateBufferView parameter, VkBufferViewType pCreateInfo->viewType, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfo->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateBufferView parameter, VkFormat pCreateInfo->format, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateBufferView(
    VkDevice device,
    VkBufferView* pView,
    VkResult result)
{

    if(pView != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateBufferView parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateBufferView(
    VkDevice device,
    const VkBufferViewCreateInfo* pCreateInfo,
    VkBufferView* pView)
{
    PreCreateBufferView(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateBufferView(device, pCreateInfo, pView);

    PostCreateBufferView(device, pView, result);

    return result;
}

bool PreCreateImage(
    VkDevice device,
    const VkImageCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateImage parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->imageType < VK_IMAGE_TYPE_BEGIN_RANGE ||
        pCreateInfo->imageType > VK_IMAGE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateImage parameter, VkImageType pCreateInfo->imageType, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfo->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateImage parameter, VkFormat pCreateInfo->format, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->tiling < VK_IMAGE_TILING_BEGIN_RANGE ||
        pCreateInfo->tiling > VK_IMAGE_TILING_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateImage parameter, VkImageTiling pCreateInfo->tiling, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->sharingMode < VK_SHARING_MODE_BEGIN_RANGE ||
        pCreateInfo->sharingMode > VK_SHARING_MODE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateImage parameter, VkSharingMode pCreateInfo->sharingMode, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->pQueueFamilyIndices != nullptr)
    {
    }
    }

    return true;
}

bool PostCreateImage(
    VkDevice device,
    VkImage* pImage,
    VkResult result)
{

    if(pImage != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateImage parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImage(
    VkDevice device,
    const VkImageCreateInfo* pCreateInfo,
    VkImage* pImage)
{
    PreCreateImage(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateImage(device, pCreateInfo, pImage);

    PostCreateImage(device, pImage, result);

    return result;
}

bool PreGetImageSubresourceLayout(
    VkDevice device,
    const VkImageSubresource* pSubresource)
{
    if(pSubresource != nullptr)
    {
    if(pSubresource->aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pSubresource->aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkGetImageSubresourceLayout parameter, VkImageAspect pSubresource->aspect, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostGetImageSubresourceLayout(
    VkDevice device,
    VkImage image,
    VkSubresourceLayout* pLayout,
    VkResult result)
{


    if(pLayout != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetImageSubresourceLayout parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetImageSubresourceLayout(
    VkDevice device,
    VkImage image,
    const VkImageSubresource* pSubresource,
    VkSubresourceLayout* pLayout)
{
    PreGetImageSubresourceLayout(device, pSubresource);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->GetImageSubresourceLayout(device, image, pSubresource, pLayout);

    PostGetImageSubresourceLayout(device, image, pLayout, result);

    return result;
}

bool PreCreateImageView(
    VkDevice device,
    const VkImageViewCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->viewType < VK_IMAGE_VIEW_TYPE_BEGIN_RANGE ||
        pCreateInfo->viewType > VK_IMAGE_VIEW_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkImageViewType pCreateInfo->viewType, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfo->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkFormat pCreateInfo->format, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->channels.r < VK_CHANNEL_SWIZZLE_BEGIN_RANGE ||
        pCreateInfo->channels.r > VK_CHANNEL_SWIZZLE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkChannelSwizzle pCreateInfo->channels.r, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->channels.g < VK_CHANNEL_SWIZZLE_BEGIN_RANGE ||
        pCreateInfo->channels.g > VK_CHANNEL_SWIZZLE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkChannelSwizzle pCreateInfo->channels.g, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->channels.b < VK_CHANNEL_SWIZZLE_BEGIN_RANGE ||
        pCreateInfo->channels.b > VK_CHANNEL_SWIZZLE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkChannelSwizzle pCreateInfo->channels.b, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->channels.a < VK_CHANNEL_SWIZZLE_BEGIN_RANGE ||
        pCreateInfo->channels.a > VK_CHANNEL_SWIZZLE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkChannelSwizzle pCreateInfo->channels.a, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->subresourceRange.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pCreateInfo->subresourceRange.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkImageAspect pCreateInfo->subresourceRange.aspect, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateImageView(
    VkDevice device,
    VkImageView* pView,
    VkResult result)
{

    if(pView != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateImageView parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImageView(
    VkDevice device,
    const VkImageViewCreateInfo* pCreateInfo,
    VkImageView* pView)
{
    PreCreateImageView(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateImageView(device, pCreateInfo, pView);

    PostCreateImageView(device, pView, result);

    return result;
}

bool PreCreateShader(
    VkDevice device,
    const VkShaderCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_SHADER_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateShader parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->pName != nullptr)
    {
    }
    }

    return true;
}

bool PostCreateShader(
    VkDevice device,
    VkShader* pShader,
    VkResult result)
{

    if(pShader != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateShader parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateShader(
    VkDevice device,
    const VkShaderCreateInfo* pCreateInfo,
    VkShader* pShader)
{
    PreCreateShader(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateShader(device, pCreateInfo, pShader);

    PostCreateShader(device, pShader, result);

    return result;
}

bool PreCreatePipelineCache(
    VkDevice device,
    const VkPipelineCacheCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreatePipelineCache parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->initialData != nullptr)
    {
    }
    }

    return true;
}

bool PostCreatePipelineCache(
    VkDevice device,
    VkPipelineCache* pPipelineCache,
    VkResult result)
{

    if(pPipelineCache != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreatePipelineCache parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreatePipelineCache(
    VkDevice device,
    const VkPipelineCacheCreateInfo* pCreateInfo,
    VkPipelineCache* pPipelineCache)
{
    PreCreatePipelineCache(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreatePipelineCache(device, pCreateInfo, pPipelineCache);

    PostCreatePipelineCache(device, pPipelineCache, result);

    return result;
}

bool PostGetPipelineCacheSize(
    VkDevice device,
    VkPipelineCache pipelineCache)
{


    return true;
}

VK_LAYER_EXPORT size_t VKAPI vkGetPipelineCacheSize(
    VkDevice device,
    VkPipelineCache pipelineCache)
{
    size_t result = get_dispatch_table(pc_device_table_map, device)->GetPipelineCacheSize(device, pipelineCache);

    PostGetPipelineCacheSize(device, pipelineCache);

    return result;
}

bool PostGetPipelineCacheData(
    VkDevice device,
    VkPipelineCache pipelineCache,
    void* pData,
    VkResult result)
{


    if(pData != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetPipelineCacheData parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPipelineCacheData(
    VkDevice device,
    VkPipelineCache pipelineCache,
    void* pData)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->GetPipelineCacheData(device, pipelineCache, pData);

    PostGetPipelineCacheData(device, pipelineCache, pData, result);

    return result;
}

bool PreMergePipelineCaches(
    VkDevice device,
    const VkPipelineCache* pSrcCaches)
{
    if(pSrcCaches != nullptr)
    {
    }

    return true;
}

bool PostMergePipelineCaches(
    VkDevice device,
    VkPipelineCache destCache,
    uint32_t srcCacheCount,
    VkResult result)
{



    if(result < VK_SUCCESS)
    {
        std::string reason = "vkMergePipelineCaches parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkMergePipelineCaches(
    VkDevice device,
    VkPipelineCache destCache,
    uint32_t srcCacheCount,
    const VkPipelineCache* pSrcCaches)
{
    PreMergePipelineCaches(device, pSrcCaches);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->MergePipelineCaches(device, destCache, srcCacheCount, pSrcCaches);

    PostMergePipelineCaches(device, destCache, srcCacheCount, result);

    return result;
}

bool PreCreateGraphicsPipelines(
    VkDevice device,
    const VkGraphicsPipelineCreateInfo* pCreateInfos)
{
    if(pCreateInfos != nullptr)
    {
    if(pCreateInfos->sType != VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfos->pStages != nullptr)
    {
    if(pCreateInfos->pStages->sType != VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->pStages->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfos->pStages->stage < VK_SHADER_STAGE_BEGIN_RANGE ||
        pCreateInfos->pStages->stage > VK_SHADER_STAGE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkShaderStage pCreateInfos->pStages->stage, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pStages->pSpecializationInfo != nullptr)
    {
    if(pCreateInfos->pStages->pSpecializationInfo->pMap != nullptr)
    {
    }
    if(pCreateInfos->pStages->pSpecializationInfo->pData != nullptr)
    {
    }
    }
    }
    if(pCreateInfos->pVertexInputState != nullptr)
    {
    if(pCreateInfos->pVertexInputState->sType != VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->pVertexInputState->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfos->pVertexInputState->pVertexBindingDescriptions != nullptr)
    {
    if(pCreateInfos->pVertexInputState->pVertexBindingDescriptions->stepRate < VK_VERTEX_INPUT_STEP_RATE_BEGIN_RANGE ||
        pCreateInfos->pVertexInputState->pVertexBindingDescriptions->stepRate > VK_VERTEX_INPUT_STEP_RATE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkVertexInputStepRate pCreateInfos->pVertexInputState->pVertexBindingDescriptions->stepRate, is an unrecognized enumerator");
        return false;
    }
    }
    if(pCreateInfos->pVertexInputState->pVertexAttributeDescriptions != nullptr)
    {
    if(pCreateInfos->pVertexInputState->pVertexAttributeDescriptions->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfos->pVertexInputState->pVertexAttributeDescriptions->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkFormat pCreateInfos->pVertexInputState->pVertexAttributeDescriptions->format, is an unrecognized enumerator");
        return false;
    }
    }
    }
    if(pCreateInfos->pInputAssemblyState != nullptr)
    {
    if(pCreateInfos->pInputAssemblyState->sType != VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->pInputAssemblyState->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfos->pInputAssemblyState->topology < VK_PRIMITIVE_TOPOLOGY_BEGIN_RANGE ||
        pCreateInfos->pInputAssemblyState->topology > VK_PRIMITIVE_TOPOLOGY_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkPrimitiveTopology pCreateInfos->pInputAssemblyState->topology, is an unrecognized enumerator");
        return false;
    }
    }
    if(pCreateInfos->pTessellationState != nullptr)
    {
    if(pCreateInfos->pTessellationState->sType != VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->pTessellationState->sType, is an invalid enumerator");
        return false;
    }
    }
    if(pCreateInfos->pViewportState != nullptr)
    {
    if(pCreateInfos->pViewportState->sType != VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->pViewportState->sType, is an invalid enumerator");
        return false;
    }
    }
    if(pCreateInfos->pRasterState != nullptr)
    {
    if(pCreateInfos->pRasterState->sType != VK_STRUCTURE_TYPE_PIPELINE_RASTER_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->pRasterState->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfos->pRasterState->fillMode < VK_FILL_MODE_BEGIN_RANGE ||
        pCreateInfos->pRasterState->fillMode > VK_FILL_MODE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkFillMode pCreateInfos->pRasterState->fillMode, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pRasterState->cullMode < VK_CULL_MODE_BEGIN_RANGE ||
        pCreateInfos->pRasterState->cullMode > VK_CULL_MODE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkCullMode pCreateInfos->pRasterState->cullMode, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pRasterState->frontFace < VK_FRONT_FACE_BEGIN_RANGE ||
        pCreateInfos->pRasterState->frontFace > VK_FRONT_FACE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkFrontFace pCreateInfos->pRasterState->frontFace, is an unrecognized enumerator");
        return false;
    }
    }
    if(pCreateInfos->pMultisampleState != nullptr)
    {
    if(pCreateInfos->pMultisampleState->sType != VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->pMultisampleState->sType, is an invalid enumerator");
        return false;
    }
    }
    if(pCreateInfos->pDepthStencilState != nullptr)
    {
    if(pCreateInfos->pDepthStencilState->sType != VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->pDepthStencilState->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfos->pDepthStencilState->depthCompareOp < VK_COMPARE_OP_BEGIN_RANGE ||
        pCreateInfos->pDepthStencilState->depthCompareOp > VK_COMPARE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkCompareOp pCreateInfos->pDepthStencilState->depthCompareOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pDepthStencilState->front.stencilFailOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfos->pDepthStencilState->front.stencilFailOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStencilOp pCreateInfos->pDepthStencilState->front.stencilFailOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pDepthStencilState->front.stencilPassOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfos->pDepthStencilState->front.stencilPassOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStencilOp pCreateInfos->pDepthStencilState->front.stencilPassOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pDepthStencilState->front.stencilDepthFailOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfos->pDepthStencilState->front.stencilDepthFailOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStencilOp pCreateInfos->pDepthStencilState->front.stencilDepthFailOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pDepthStencilState->front.stencilCompareOp < VK_COMPARE_OP_BEGIN_RANGE ||
        pCreateInfos->pDepthStencilState->front.stencilCompareOp > VK_COMPARE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkCompareOp pCreateInfos->pDepthStencilState->front.stencilCompareOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pDepthStencilState->back.stencilFailOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfos->pDepthStencilState->back.stencilFailOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStencilOp pCreateInfos->pDepthStencilState->back.stencilFailOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pDepthStencilState->back.stencilPassOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfos->pDepthStencilState->back.stencilPassOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStencilOp pCreateInfos->pDepthStencilState->back.stencilPassOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pDepthStencilState->back.stencilDepthFailOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfos->pDepthStencilState->back.stencilDepthFailOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStencilOp pCreateInfos->pDepthStencilState->back.stencilDepthFailOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pDepthStencilState->back.stencilCompareOp < VK_COMPARE_OP_BEGIN_RANGE ||
        pCreateInfos->pDepthStencilState->back.stencilCompareOp > VK_COMPARE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkCompareOp pCreateInfos->pDepthStencilState->back.stencilCompareOp, is an unrecognized enumerator");
        return false;
    }
    }
    if(pCreateInfos->pColorBlendState != nullptr)
    {
    if(pCreateInfos->pColorBlendState->sType != VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkStructureType pCreateInfos->pColorBlendState->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfos->pColorBlendState->logicOp < VK_LOGIC_OP_BEGIN_RANGE ||
        pCreateInfos->pColorBlendState->logicOp > VK_LOGIC_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkLogicOp pCreateInfos->pColorBlendState->logicOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pColorBlendState->pAttachments != nullptr)
    {
    if(pCreateInfos->pColorBlendState->pAttachments->srcBlendColor < VK_BLEND_BEGIN_RANGE ||
        pCreateInfos->pColorBlendState->pAttachments->srcBlendColor > VK_BLEND_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkBlend pCreateInfos->pColorBlendState->pAttachments->srcBlendColor, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pColorBlendState->pAttachments->destBlendColor < VK_BLEND_BEGIN_RANGE ||
        pCreateInfos->pColorBlendState->pAttachments->destBlendColor > VK_BLEND_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkBlend pCreateInfos->pColorBlendState->pAttachments->destBlendColor, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pColorBlendState->pAttachments->blendOpColor < VK_BLEND_OP_BEGIN_RANGE ||
        pCreateInfos->pColorBlendState->pAttachments->blendOpColor > VK_BLEND_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkBlendOp pCreateInfos->pColorBlendState->pAttachments->blendOpColor, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pColorBlendState->pAttachments->srcBlendAlpha < VK_BLEND_BEGIN_RANGE ||
        pCreateInfos->pColorBlendState->pAttachments->srcBlendAlpha > VK_BLEND_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkBlend pCreateInfos->pColorBlendState->pAttachments->srcBlendAlpha, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pColorBlendState->pAttachments->destBlendAlpha < VK_BLEND_BEGIN_RANGE ||
        pCreateInfos->pColorBlendState->pAttachments->destBlendAlpha > VK_BLEND_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkBlend pCreateInfos->pColorBlendState->pAttachments->destBlendAlpha, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->pColorBlendState->pAttachments->blendOpAlpha < VK_BLEND_OP_BEGIN_RANGE ||
        pCreateInfos->pColorBlendState->pAttachments->blendOpAlpha > VK_BLEND_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkBlendOp pCreateInfos->pColorBlendState->pAttachments->blendOpAlpha, is an unrecognized enumerator");
        return false;
    }
    }
    }
    if(pCreateInfos->renderPass == VK_NULL_HANDLE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelines parameter, VkRenderPass pCreateInfos->renderPass, is null pointer");
    }
    }

    return true;
}

bool PostCreateGraphicsPipelines(
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t count,
    VkPipeline* pPipelines,
    VkResult result)
{



    if(pPipelines != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateGraphicsPipelines parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateGraphicsPipelines(
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t count,
    const VkGraphicsPipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines)
{
    PreCreateGraphicsPipelines(device, pCreateInfos);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateGraphicsPipelines(device, pipelineCache, count, pCreateInfos, pPipelines);

    PostCreateGraphicsPipelines(device, pipelineCache, count, pPipelines, result);

    return result;
}

bool PreCreateComputePipelines(
    VkDevice device,
    const VkComputePipelineCreateInfo* pCreateInfos)
{
    if(pCreateInfos != nullptr)
    {
    if(pCreateInfos->sType != VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateComputePipelines parameter, VkStructureType pCreateInfos->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfos->stage.sType != VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateComputePipelines parameter, VkStructureType pCreateInfos->cs.sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfos->stage.stage < VK_SHADER_STAGE_BEGIN_RANGE ||
        pCreateInfos->stage.stage > VK_SHADER_STAGE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateComputePipelines parameter, VkShaderStage pCreateInfos->cs.stage, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfos->stage.pSpecializationInfo != nullptr)
    {
    if(pCreateInfos->stage.pSpecializationInfo->pMap != nullptr)
    {
    }
    if(pCreateInfos->stage.pSpecializationInfo->pData != nullptr)
    {
    }
    }
    }

    return true;
}

bool PostCreateComputePipelines(
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t count,
    VkPipeline* pPipelines,
    VkResult result)
{



    if(pPipelines != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateComputePipelines parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateComputePipelines(
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t count,
    const VkComputePipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines)
{
    PreCreateComputePipelines(device, pCreateInfos);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateComputePipelines(device, pipelineCache, count, pCreateInfos, pPipelines);

    PostCreateComputePipelines(device, pipelineCache, count, pPipelines, result);

    return result;
}

bool PreCreatePipelineLayout(
    VkDevice device,
    const VkPipelineLayoutCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreatePipelineLayout parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->pSetLayouts != nullptr)
    {
    }
    if(pCreateInfo->pPushConstantRanges != nullptr)
    {
    }
    }

    return true;
}

bool PostCreatePipelineLayout(
    VkDevice device,
    VkPipelineLayout* pPipelineLayout,
    VkResult result)
{

    if(pPipelineLayout != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreatePipelineLayout parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreatePipelineLayout(
    VkDevice device,
    const VkPipelineLayoutCreateInfo* pCreateInfo,
    VkPipelineLayout* pPipelineLayout)
{
    PreCreatePipelineLayout(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreatePipelineLayout(device, pCreateInfo, pPipelineLayout);

    PostCreatePipelineLayout(device, pPipelineLayout, result);

    return result;
}

bool PreCreateSampler(
    VkDevice device,
    const VkSamplerCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->magFilter < VK_TEX_FILTER_BEGIN_RANGE ||
        pCreateInfo->magFilter > VK_TEX_FILTER_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkTexFilter pCreateInfo->magFilter, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->minFilter < VK_TEX_FILTER_BEGIN_RANGE ||
        pCreateInfo->minFilter > VK_TEX_FILTER_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkTexFilter pCreateInfo->minFilter, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->mipMode < VK_TEX_MIPMAP_MODE_BEGIN_RANGE ||
        pCreateInfo->mipMode > VK_TEX_MIPMAP_MODE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkTexMipmapMode pCreateInfo->mipMode, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->addressU < VK_TEX_ADDRESS_BEGIN_RANGE ||
        pCreateInfo->addressU > VK_TEX_ADDRESS_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkTexAddress pCreateInfo->addressU, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->addressV < VK_TEX_ADDRESS_BEGIN_RANGE ||
        pCreateInfo->addressV > VK_TEX_ADDRESS_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkTexAddress pCreateInfo->addressV, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->addressW < VK_TEX_ADDRESS_BEGIN_RANGE ||
        pCreateInfo->addressW > VK_TEX_ADDRESS_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkTexAddress pCreateInfo->addressW, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->compareOp < VK_COMPARE_OP_BEGIN_RANGE ||
        pCreateInfo->compareOp > VK_COMPARE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkCompareOp pCreateInfo->compareOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->borderColor < VK_BORDER_COLOR_BEGIN_RANGE ||
        pCreateInfo->borderColor > VK_BORDER_COLOR_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkBorderColor pCreateInfo->borderColor, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateSampler(
    VkDevice device,
    VkSampler* pSampler,
    VkResult result)
{

    if(pSampler != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateSampler parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateSampler(
    VkDevice device,
    const VkSamplerCreateInfo* pCreateInfo,
    VkSampler* pSampler)
{
    PreCreateSampler(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateSampler(device, pCreateInfo, pSampler);

    PostCreateSampler(device, pSampler, result);

    return result;
}

bool PreCreateDescriptorSetLayout(
    VkDevice device,
    const VkDescriptorSetLayoutCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorSetLayout parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->pBinding != nullptr)
    {
    if(pCreateInfo->pBinding->descriptorType < VK_DESCRIPTOR_TYPE_BEGIN_RANGE ||
        pCreateInfo->pBinding->descriptorType > VK_DESCRIPTOR_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorSetLayout parameter, VkDescriptorType pCreateInfo->pBinding->descriptorType, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->pBinding->pImmutableSamplers != nullptr)
    {
    }
    }
    }

    return true;
}

bool PostCreateDescriptorSetLayout(
    VkDevice device,
    VkDescriptorSetLayout* pSetLayout,
    VkResult result)
{

    if(pSetLayout != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateDescriptorSetLayout parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDescriptorSetLayout(
    VkDevice device,
    const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
    VkDescriptorSetLayout* pSetLayout)
{
    PreCreateDescriptorSetLayout(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateDescriptorSetLayout(device, pCreateInfo, pSetLayout);

    PostCreateDescriptorSetLayout(device, pSetLayout, result);

    return result;
}

bool PreCreateDescriptorPool(
    VkDevice device,
    const VkDescriptorPoolCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorPool parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->pTypeCount != nullptr)
    {
    if(pCreateInfo->pTypeCount->type < VK_DESCRIPTOR_TYPE_BEGIN_RANGE ||
        pCreateInfo->pTypeCount->type > VK_DESCRIPTOR_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorPool parameter, VkDescriptorType pCreateInfo->pTypeCount->type, is an unrecognized enumerator");
        return false;
    }
    }
    }

    return true;
}

bool PostCreateDescriptorPool(
    VkDevice device,
    VkDescriptorPoolUsage poolUsage,
    uint32_t maxSets,
    VkDescriptorPool* pDescriptorPool,
    VkResult result)
{

    if(poolUsage < VK_DESCRIPTOR_POOL_USAGE_BEGIN_RANGE ||
        poolUsage > VK_DESCRIPTOR_POOL_USAGE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorPool parameter, VkDescriptorPoolUsage poolUsage, is an unrecognized enumerator");
        return false;
    }


    if(pDescriptorPool != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateDescriptorPool parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDescriptorPool(
    VkDevice device,
    VkDescriptorPoolUsage poolUsage,
    uint32_t maxSets,
    const VkDescriptorPoolCreateInfo* pCreateInfo,
    VkDescriptorPool* pDescriptorPool)
{
    PreCreateDescriptorPool(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateDescriptorPool(device, poolUsage, maxSets, pCreateInfo, pDescriptorPool);

    PostCreateDescriptorPool(device, poolUsage, maxSets, pDescriptorPool, result);

    return result;
}

bool PostResetDescriptorPool(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkResetDescriptorPool parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetDescriptorPool(
    VkDevice device,
    VkDescriptorPool descriptorPool)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->ResetDescriptorPool(device, descriptorPool);

    PostResetDescriptorPool(device, descriptorPool, result);

    return result;
}

bool PreAllocDescriptorSets(
    VkDevice device,
    const VkDescriptorSetLayout* pSetLayouts)
{
    if(pSetLayouts != nullptr)
    {
    }

    return true;
}

bool PostAllocDescriptorSets(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    VkDescriptorSetUsage setUsage,
    uint32_t count,
    VkDescriptorSet* pDescriptorSets,
    VkResult result)
{


    if(setUsage < VK_DESCRIPTOR_SET_USAGE_BEGIN_RANGE ||
        setUsage > VK_DESCRIPTOR_SET_USAGE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkAllocDescriptorSets parameter, VkDescriptorSetUsage setUsage, is an unrecognized enumerator");
        return false;
    }


    if(pDescriptorSets != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkAllocDescriptorSets parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkAllocDescriptorSets(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    VkDescriptorSetUsage setUsage,
    uint32_t count,
    const VkDescriptorSetLayout* pSetLayouts,
    VkDescriptorSet* pDescriptorSets)
{
    PreAllocDescriptorSets(device, pSetLayouts);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->AllocDescriptorSets(device, descriptorPool, setUsage, count, pSetLayouts, pDescriptorSets);

    PostAllocDescriptorSets(device, descriptorPool, setUsage, count, pDescriptorSets, result);

    return result;
}

bool PreFreeDescriptorSets(
    VkDevice device,
    const VkDescriptorSet* pDescriptorSets)
{
    if(pDescriptorSets != nullptr)
    {
    }

    return true;
}

bool PostFreeDescriptorSets(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    uint32_t count,
    VkResult result)
{



    if(result < VK_SUCCESS)
    {
        std::string reason = "vkFreeDescriptorSets parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkFreeDescriptorSets(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    uint32_t count,
    const VkDescriptorSet* pDescriptorSets)
{
    PreFreeDescriptorSets(device, pDescriptorSets);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->FreeDescriptorSets(device, descriptorPool, count, pDescriptorSets);

    PostFreeDescriptorSets(device, descriptorPool, count, result);

    return result;
}

bool PreUpdateDescriptorSets(
    VkDevice device,
    const VkWriteDescriptorSet* pDescriptorWrites,
    const VkCopyDescriptorSet* pDescriptorCopies)
{
    if(pDescriptorWrites != nullptr)
    {
    if(pDescriptorWrites->sType != VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkStructureType pDescriptorWrites->sType, is an invalid enumerator");
        return false;
    }
    if(pDescriptorWrites->descriptorType < VK_DESCRIPTOR_TYPE_BEGIN_RANGE ||
        pDescriptorWrites->descriptorType > VK_DESCRIPTOR_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkDescriptorType pDescriptorWrites->descriptorType, is an unrecognized enumerator");
        return false;
    }
    if(pDescriptorWrites->pDescriptors != nullptr)
    {
    if(pDescriptorWrites->pDescriptors->imageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        pDescriptorWrites->pDescriptors->imageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkImageLayout pDescriptorWrites->pDescriptors->imageLayout, is an unrecognized enumerator");
        return false;
    }
    }
    }

    if(pDescriptorCopies != nullptr)
    {
    if(pDescriptorCopies->sType != VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkStructureType pDescriptorCopies->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

VK_LAYER_EXPORT void VKAPI vkUpdateDescriptorSets(
    VkDevice device,
    uint32_t writeCount,
    const VkWriteDescriptorSet* pDescriptorWrites,
    uint32_t copyCount,
    const VkCopyDescriptorSet* pDescriptorCopies)
{
    PreUpdateDescriptorSets(device, pDescriptorWrites, pDescriptorCopies);

    get_dispatch_table(pc_device_table_map, device)->UpdateDescriptorSets(device, writeCount, pDescriptorWrites, copyCount, pDescriptorCopies);
}

bool PreCreateDynamicViewportState(
    VkDevice device,
    const VkDynamicViewportStateCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_DYNAMIC_VIEWPORT_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateDynamicViewportState parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->pViewports != nullptr)
    {
    }
    if(pCreateInfo->pScissors != nullptr)
    {
    }
    }

    return true;
}

bool PostCreateDynamicViewportState(
    VkDevice device,
    VkDynamicViewportState* pState,
    VkResult result)
{

    if(pState != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateDynamicViewportState parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicViewportState(
    VkDevice device,
    const VkDynamicViewportStateCreateInfo* pCreateInfo,
    VkDynamicViewportState* pState)
{
    PreCreateDynamicViewportState(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateDynamicViewportState(device, pCreateInfo, pState);

    PostCreateDynamicViewportState(device, pState, result);

    return result;
}

bool PreCreateDynamicLineWidthState(
    VkDevice device,
    const VkDynamicLineWidthStateCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_DYNAMIC_LINE_WIDTH_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateDynamicLineWidthState parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateDynamicLineWidthState(
    VkDevice device,
    VkDynamicLineWidthState* pState,
    VkResult result)
{

    if(pState != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateDynamicLineWidthState parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicLineWidthState(
    VkDevice device,
    const VkDynamicLineWidthStateCreateInfo* pCreateInfo,
    VkDynamicLineWidthState* pState)
{
    PreCreateDynamicLineWidthState(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateDynamicLineWidthState(device, pCreateInfo, pState);

    PostCreateDynamicLineWidthState(device, pState, result);

    return result;
}

bool PreCreateDynamicDepthBiasState(
    VkDevice device,
    const VkDynamicDepthBiasStateCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_DYNAMIC_DEPTH_BIAS_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateDynamicDepthBiasState parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateDynamicDepthBiasState(
    VkDevice device,
    VkDynamicDepthBiasState* pState,
    VkResult result)
{

    if(pState != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateDynamicDepthBiasState parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicDepthBiasState(
    VkDevice device,
    const VkDynamicDepthBiasStateCreateInfo* pCreateInfo,
    VkDynamicDepthBiasState* pState)
{
    PreCreateDynamicDepthBiasState(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateDynamicDepthBiasState(device, pCreateInfo, pState);

    PostCreateDynamicDepthBiasState(device, pState, result);

    return result;
}

bool PreCreateDynamicBlendState(
    VkDevice device,
    const VkDynamicBlendStateCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_DYNAMIC_BLEND_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateDynamicBlendState parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateDynamicBlendState(
    VkDevice device,
    VkDynamicBlendState* pState,
    VkResult result)
{

    if(pState != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateDynamicBlendState parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicBlendState(
    VkDevice device,
    const VkDynamicBlendStateCreateInfo* pCreateInfo,
    VkDynamicBlendState* pState)
{
    PreCreateDynamicBlendState(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateDynamicBlendState(device, pCreateInfo, pState);

    PostCreateDynamicBlendState(device, pState, result);

    return result;
}

bool PreCreateDynamicDepthBoundsState(
    VkDevice device,
    const VkDynamicDepthBoundsStateCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_DYNAMIC_DEPTH_BOUNDS_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateDynamicDepthBoundsState parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateDynamicDepthBoundsState(
    VkDevice device,
    VkDynamicDepthBoundsState* pState,
    VkResult result)
{

    if(pState != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateDynamicDepthBoundsState parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicDepthBoundsState(
    VkDevice device,
    const VkDynamicDepthBoundsStateCreateInfo* pCreateInfo,
    VkDynamicDepthBoundsState* pState)
{
    PreCreateDynamicDepthBoundsState(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateDynamicDepthBoundsState(device, pCreateInfo, pState);

    PostCreateDynamicDepthBoundsState(device, pState, result);

    return result;
}

bool PreCreateDynamicStencilState(
    VkDevice device,
    const VkDynamicStencilStateCreateInfo* pCreateInfoFront,
    const VkDynamicStencilStateCreateInfo* pCreateInfoBack)
{
    if(pCreateInfoFront != nullptr)
    {
    if(pCreateInfoFront->sType != VK_STRUCTURE_TYPE_DYNAMIC_STENCIL_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateDynamicStencilState parameter, VkStructureType pCreateInfoFront->sType, is an invalid enumerator");
        return false;
    }
    }

    if(pCreateInfoBack != nullptr)
    {
    if(pCreateInfoBack->sType != VK_STRUCTURE_TYPE_DYNAMIC_STENCIL_STATE_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateDynamicStencilState parameter, VkStructureType pCreateInfoBack->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateDynamicStencilState(
    VkDevice device,
    VkDynamicStencilState* pState,
    VkResult result)
{

    if(pState != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateDynamicStencilState parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicStencilState(
    VkDevice device,
    const VkDynamicStencilStateCreateInfo* pCreateInfoFront,
    const VkDynamicStencilStateCreateInfo* pCreateInfoBack,
    VkDynamicStencilState* pState)
{
    PreCreateDynamicStencilState(device, pCreateInfoFront, pCreateInfoBack);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateDynamicStencilState(device, pCreateInfoFront, pCreateInfoBack, pState);

    PostCreateDynamicStencilState(device, pState, result);

    return result;
}

bool PreCreateFramebuffer(
    VkDevice device,
    const VkFramebufferCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateFramebuffer parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->pAttachments != nullptr)
    {
    }
    }

    return true;
}

bool PostCreateFramebuffer(
    VkDevice device,
    VkFramebuffer* pFramebuffer,
    VkResult result)
{

    if(pFramebuffer != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateFramebuffer parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateFramebuffer(
    VkDevice device,
    const VkFramebufferCreateInfo* pCreateInfo,
    VkFramebuffer* pFramebuffer)
{
    PreCreateFramebuffer(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateFramebuffer(device, pCreateInfo, pFramebuffer);

    PostCreateFramebuffer(device, pFramebuffer, result);

    return result;
}

bool PreCreateRenderPass(
    VkDevice device,
    const VkRenderPassCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->pAttachments != nullptr)
    {
    if(pCreateInfo->pAttachments->sType != VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkStructureType pCreateInfo->pAttachments->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->pAttachments->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfo->pAttachments->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkFormat pCreateInfo->pAttachments->format, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->pAttachments->loadOp < VK_ATTACHMENT_LOAD_OP_BEGIN_RANGE ||
        pCreateInfo->pAttachments->loadOp > VK_ATTACHMENT_LOAD_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkAttachmentLoadOp pCreateInfo->pAttachments->loadOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->pAttachments->storeOp < VK_ATTACHMENT_STORE_OP_BEGIN_RANGE ||
        pCreateInfo->pAttachments->storeOp > VK_ATTACHMENT_STORE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkAttachmentStoreOp pCreateInfo->pAttachments->storeOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->pAttachments->stencilLoadOp < VK_ATTACHMENT_LOAD_OP_BEGIN_RANGE ||
        pCreateInfo->pAttachments->stencilLoadOp > VK_ATTACHMENT_LOAD_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkAttachmentLoadOp pCreateInfo->pAttachments->stencilLoadOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->pAttachments->stencilStoreOp < VK_ATTACHMENT_STORE_OP_BEGIN_RANGE ||
        pCreateInfo->pAttachments->stencilStoreOp > VK_ATTACHMENT_STORE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkAttachmentStoreOp pCreateInfo->pAttachments->stencilStoreOp, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->pAttachments->initialLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        pCreateInfo->pAttachments->initialLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkImageLayout pCreateInfo->pAttachments->initialLayout, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->pAttachments->finalLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        pCreateInfo->pAttachments->finalLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkImageLayout pCreateInfo->pAttachments->finalLayout, is an unrecognized enumerator");
        return false;
    }
    }
    if(pCreateInfo->pSubpasses != nullptr)
    {
    if(pCreateInfo->pSubpasses->sType != VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkStructureType pCreateInfo->pSubpasses->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->pSubpasses->pipelineBindPoint < VK_PIPELINE_BIND_POINT_BEGIN_RANGE ||
        pCreateInfo->pSubpasses->pipelineBindPoint > VK_PIPELINE_BIND_POINT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkPipelineBindPoint pCreateInfo->pSubpasses->pipelineBindPoint, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->pSubpasses->pInputAttachments != nullptr)
    {
    if(pCreateInfo->pSubpasses->pInputAttachments->layout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        pCreateInfo->pSubpasses->pInputAttachments->layout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkImageLayout pCreateInfo->pSubpasses->pInputAttachments->layout, is an unrecognized enumerator");
        return false;
    }
    }
    if(pCreateInfo->pSubpasses->pColorAttachments != nullptr)
    {
    if(pCreateInfo->pSubpasses->pColorAttachments->layout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        pCreateInfo->pSubpasses->pColorAttachments->layout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkImageLayout pCreateInfo->pSubpasses->pColorAttachments->layout, is an unrecognized enumerator");
        return false;
    }
    }
    if(pCreateInfo->pSubpasses->pResolveAttachments != nullptr)
    {
    if(pCreateInfo->pSubpasses->pResolveAttachments->layout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        pCreateInfo->pSubpasses->pResolveAttachments->layout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkImageLayout pCreateInfo->pSubpasses->pResolveAttachments->layout, is an unrecognized enumerator");
        return false;
    }
    }
    if(pCreateInfo->pSubpasses->depthStencilAttachment.layout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        pCreateInfo->pSubpasses->depthStencilAttachment.layout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkImageLayout pCreateInfo->pSubpasses->depthStencilAttachment.layout, is an unrecognized enumerator");
        return false;
    }
    if(pCreateInfo->pSubpasses->pPreserveAttachments != nullptr)
    {
    if(pCreateInfo->pSubpasses->pPreserveAttachments->layout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        pCreateInfo->pSubpasses->pPreserveAttachments->layout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkImageLayout pCreateInfo->pSubpasses->pPreserveAttachments->layout, is an unrecognized enumerator");
        return false;
    }
    }
    }
    if(pCreateInfo->pDependencies != nullptr)
    {
    if(pCreateInfo->pDependencies->sType != VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkStructureType pCreateInfo->pDependencies->sType, is an invalid enumerator");
        return false;
    }
    }
    }

    return true;
}

bool PostCreateRenderPass(
    VkDevice device,
    VkRenderPass* pRenderPass,
    VkResult result)
{

    if(pRenderPass != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateRenderPass parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateRenderPass(
    VkDevice device,
    const VkRenderPassCreateInfo* pCreateInfo,
    VkRenderPass* pRenderPass)
{
    PreCreateRenderPass(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateRenderPass(device, pCreateInfo, pRenderPass);

    PostCreateRenderPass(device, pRenderPass, result);

    return result;
}

bool PostGetRenderAreaGranularity(
    VkDevice device,
    VkRenderPass renderPass,
    VkExtent2D* pGranularity,
    VkResult result)
{


    if(pGranularity != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkGetRenderAreaGranularity parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetRenderAreaGranularity(
    VkDevice device,
    VkRenderPass renderPass,
    VkExtent2D* pGranularity)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->GetRenderAreaGranularity(device, renderPass, pGranularity);

    PostGetRenderAreaGranularity(device, renderPass, pGranularity, result);

    return result;
}

bool PreCreateCommandPool(
    VkDevice device,
    const VkCmdPoolCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_CMD_POOL_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateCommandPool parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateCommandPool(
    VkDevice device,
    VkCmdPool* pCmdPool,
    VkResult result)
{

    if(pCmdPool != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateCommandPool parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateCommandPool(
    VkDevice device,
    const VkCmdPoolCreateInfo* pCreateInfo,
    VkCmdPool* pCmdPool)
{
    PreCreateCommandPool(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateCommandPool(device, pCreateInfo, pCmdPool);

    PostCreateCommandPool(device, pCmdPool, result);

    return result;
}

bool PostResetCommandPool(
    VkDevice device,
    VkCmdPool cmdPool,
    VkCmdPoolResetFlags flags,
    VkResult result)
{



    if(result < VK_SUCCESS)
    {
        std::string reason = "vkResetCommandPool parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetCommandPool(
    VkDevice device,
    VkCmdPool cmdPool,
    VkCmdPoolResetFlags flags)
{
    VkResult result = get_dispatch_table(pc_device_table_map, device)->ResetCommandPool(device, cmdPool, flags);

    PostResetCommandPool(device, cmdPool, flags, result);

    return result;
}

bool PreCreateCommandBuffer(
    VkDevice device,
    const VkCmdBufferCreateInfo* pCreateInfo)
{
    if(pCreateInfo != nullptr)
    {
    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateCommandBuffer parameter, VkStructureType pCreateInfo->sType, is an invalid enumerator");
        return false;
    }
    if(pCreateInfo->level < VK_CMD_BUFFER_LEVEL_BEGIN_RANGE ||
        pCreateInfo->level > VK_CMD_BUFFER_LEVEL_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCreateCommandBuffer parameter, VkCmdBufferLevel pCreateInfo->level, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCreateCommandBuffer(
    VkDevice device,
    VkCmdBuffer* pCmdBuffer,
    VkResult result)
{

    if(pCmdBuffer != nullptr)
    {
    }

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkCreateCommandBuffer parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateCommandBuffer(
    VkDevice device,
    const VkCmdBufferCreateInfo* pCreateInfo,
    VkCmdBuffer* pCmdBuffer)
{
    PreCreateCommandBuffer(device, pCreateInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);

    PostCreateCommandBuffer(device, pCmdBuffer, result);

    return result;
}

bool PreBeginCommandBuffer(
    VkCmdBuffer cmdBuffer,
    const VkCmdBufferBeginInfo* pBeginInfo)
{
    if(pBeginInfo != nullptr)
    {
    if(pBeginInfo->sType != VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkBeginCommandBuffer parameter, VkStructureType pBeginInfo->sType, is an invalid enumerator");
        return false;
    }
    }

    return true;
}

bool PostBeginCommandBuffer(
    VkCmdBuffer cmdBuffer,
    VkResult result)
{

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkBeginCommandBuffer parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkBeginCommandBuffer(
    VkCmdBuffer cmdBuffer,
    const VkCmdBufferBeginInfo* pBeginInfo)
{
    PreBeginCommandBuffer(cmdBuffer, pBeginInfo);

    VkResult result = get_dispatch_table(pc_device_table_map, cmdBuffer)->BeginCommandBuffer(cmdBuffer, pBeginInfo);

    PostBeginCommandBuffer(cmdBuffer, result);

    return result;
}

bool PostEndCommandBuffer(
    VkCmdBuffer cmdBuffer,
    VkResult result)
{

    if(result < VK_SUCCESS)
    {
        std::string reason = "vkEndCommandBuffer parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkEndCommandBuffer(
    VkCmdBuffer cmdBuffer)
{
    VkResult result = get_dispatch_table(pc_device_table_map, cmdBuffer)->EndCommandBuffer(cmdBuffer);

    PostEndCommandBuffer(cmdBuffer, result);

    return result;
}

bool PostResetCommandBuffer(
    VkCmdBuffer cmdBuffer,
    VkCmdBufferResetFlags flags,
    VkResult result)
{


    if(result < VK_SUCCESS)
    {
        std::string reason = "vkResetCommandBuffer parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK", reason.c_str());
        return false;
    }

    return true;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetCommandBuffer(
    VkCmdBuffer cmdBuffer,
    VkCmdBufferResetFlags flags)
{
    VkResult result = get_dispatch_table(pc_device_table_map, cmdBuffer)->ResetCommandBuffer(cmdBuffer, flags);

    PostResetCommandBuffer(cmdBuffer, flags, result);

    return result;
}

bool PostCmdBindPipeline(
    VkCmdBuffer cmdBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipeline pipeline)
{

    if(pipelineBindPoint < VK_PIPELINE_BIND_POINT_BEGIN_RANGE ||
        pipelineBindPoint > VK_PIPELINE_BIND_POINT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdBindPipeline parameter, VkPipelineBindPoint pipelineBindPoint, is an unrecognized enumerator");
        return false;
    }


    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdBindPipeline(
    VkCmdBuffer cmdBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipeline pipeline)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);

    PostCmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
}

bool PostCmdBindDynamicViewportState(
    VkCmdBuffer cmdBuffer,
    VkDynamicViewportState dynamicViewportState)
{


    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDynamicViewportState(
    VkCmdBuffer cmdBuffer,
    VkDynamicViewportState dynamicViewportState)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdBindDynamicViewportState(cmdBuffer, dynamicViewportState);

    PostCmdBindDynamicViewportState(cmdBuffer, dynamicViewportState);
}

bool PostCmdBindDynamicLineWidthState(
    VkCmdBuffer cmdBuffer,
    VkDynamicLineWidthState dynamicLineWidthState)
{


    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDynamicLineWidthState(
    VkCmdBuffer cmdBuffer,
    VkDynamicLineWidthState dynamicLineWidthState)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdBindDynamicLineWidthState(cmdBuffer, dynamicLineWidthState);

    PostCmdBindDynamicLineWidthState(cmdBuffer, dynamicLineWidthState);
}

bool PostCmdBindDynamicDepthBiasState(
    VkCmdBuffer cmdBuffer,
    VkDynamicDepthBiasState dynamicDepthBiasState)
{


    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDynamicDepthBiasState(
    VkCmdBuffer cmdBuffer,
    VkDynamicDepthBiasState dynamicDepthBiasState)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdBindDynamicDepthBiasState(cmdBuffer, dynamicDepthBiasState);

    PostCmdBindDynamicDepthBiasState(cmdBuffer, dynamicDepthBiasState);
}

bool PostCmdBindDynamicBlendState(
    VkCmdBuffer cmdBuffer,
    VkDynamicBlendState dynamicBlendState)
{


    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDynamicBlendState(
    VkCmdBuffer cmdBuffer,
    VkDynamicBlendState dynamicBlendState)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdBindDynamicBlendState(cmdBuffer, dynamicBlendState);

    PostCmdBindDynamicBlendState(cmdBuffer, dynamicBlendState);
}

bool PostCmdBindDynamicDepthBoundsState(
    VkCmdBuffer cmdBuffer,
    VkDynamicDepthBoundsState dynamicDepthBoundsState)
{
    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDynamicDepthBoundsState(
    VkCmdBuffer cmdBuffer,
    VkDynamicDepthBoundsState dynamicDepthBoundsState)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdBindDynamicDepthBoundsState(cmdBuffer, dynamicDepthBoundsState);

    PostCmdBindDynamicDepthBoundsState(cmdBuffer, dynamicDepthBoundsState);
}

bool PostCmdBindDynamicStencilState(
    VkCmdBuffer cmdBuffer,
    VkDynamicStencilState dynamicStencilState)
{
    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDynamicStencilState(
    VkCmdBuffer cmdBuffer,
    VkDynamicStencilState dynamicStencilState)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdBindDynamicStencilState(cmdBuffer, dynamicStencilState);

    PostCmdBindDynamicStencilState(cmdBuffer, dynamicStencilState);
}

bool PreCmdBindDescriptorSets(
    VkCmdBuffer cmdBuffer,
    const VkDescriptorSet* pDescriptorSets,
    const uint32_t* pDynamicOffsets)
{
    if(pDescriptorSets != nullptr)
    {
    }

    if(pDynamicOffsets != nullptr)
    {
    }

    return true;
}

bool PostCmdBindDescriptorSets(
    VkCmdBuffer cmdBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipelineLayout layout,
    uint32_t firstSet,
    uint32_t setCount,
    uint32_t dynamicOffsetCount)
{

    if(pipelineBindPoint < VK_PIPELINE_BIND_POINT_BEGIN_RANGE ||
        pipelineBindPoint > VK_PIPELINE_BIND_POINT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdBindDescriptorSets parameter, VkPipelineBindPoint pipelineBindPoint, is an unrecognized enumerator");
        return false;
    }





    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDescriptorSets(
    VkCmdBuffer cmdBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipelineLayout layout,
    uint32_t firstSet,
    uint32_t setCount,
    const VkDescriptorSet* pDescriptorSets,
    uint32_t dynamicOffsetCount,
    const uint32_t* pDynamicOffsets)
{
    PreCmdBindDescriptorSets(cmdBuffer, pDescriptorSets, pDynamicOffsets);

    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdBindDescriptorSets(cmdBuffer, pipelineBindPoint, layout, firstSet, setCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);

    PostCmdBindDescriptorSets(cmdBuffer, pipelineBindPoint, layout, firstSet, setCount, dynamicOffsetCount);
}

bool PostCmdBindIndexBuffer(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    VkIndexType indexType)
{



    if(indexType < VK_INDEX_TYPE_BEGIN_RANGE ||
        indexType > VK_INDEX_TYPE_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdBindIndexBuffer parameter, VkIndexType indexType, is an unrecognized enumerator");
        return false;
    }

    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdBindIndexBuffer(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    VkIndexType indexType)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);

    PostCmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
}

bool PreCmdBindVertexBuffers(
    VkCmdBuffer cmdBuffer,
    const VkBuffer* pBuffers,
    const VkDeviceSize* pOffsets)
{
    if(pBuffers != nullptr)
    {
    }

    if(pOffsets != nullptr)
    {
    }

    return true;
}

bool PostCmdBindVertexBuffers(
    VkCmdBuffer cmdBuffer,
    uint32_t startBinding,
    uint32_t bindingCount)
{



    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdBindVertexBuffers(
    VkCmdBuffer cmdBuffer,
    uint32_t startBinding,
    uint32_t bindingCount,
    const VkBuffer* pBuffers,
    const VkDeviceSize* pOffsets)
{
    PreCmdBindVertexBuffers(cmdBuffer, pBuffers, pOffsets);

    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdBindVertexBuffers(cmdBuffer, startBinding, bindingCount, pBuffers, pOffsets);

    PostCmdBindVertexBuffers(cmdBuffer, startBinding, bindingCount);
}

bool PostCmdDraw(
    VkCmdBuffer cmdBuffer,
    uint32_t firstVertex,
    uint32_t vertexCount,
    uint32_t firstInstance,
    uint32_t instanceCount)
{





    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdDraw(
    VkCmdBuffer cmdBuffer,
    uint32_t firstVertex,
    uint32_t vertexCount,
    uint32_t firstInstance,
    uint32_t instanceCount)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdDraw(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);

    PostCmdDraw(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
}

bool PostCmdDrawIndexed(
    VkCmdBuffer cmdBuffer,
    uint32_t firstIndex,
    uint32_t indexCount,
    int32_t vertexOffset,
    uint32_t firstInstance,
    uint32_t instanceCount)
{






    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexed(
    VkCmdBuffer cmdBuffer,
    uint32_t firstIndex,
    uint32_t indexCount,
    int32_t vertexOffset,
    uint32_t firstInstance,
    uint32_t instanceCount)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdDrawIndexed(cmdBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);

    PostCmdDrawIndexed(cmdBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
}

bool PostCmdDrawIndirect(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    uint32_t count,
    uint32_t stride)
{





    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndirect(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    uint32_t count,
    uint32_t stride)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);

    PostCmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
}

bool PostCmdDrawIndexedIndirect(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    uint32_t count,
    uint32_t stride)
{





    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexedIndirect(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    uint32_t count,
    uint32_t stride)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);

    PostCmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
}

bool PostCmdDispatch(
    VkCmdBuffer cmdBuffer,
    uint32_t x,
    uint32_t y,
    uint32_t z)
{




    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatch(
    VkCmdBuffer cmdBuffer,
    uint32_t x,
    uint32_t y,
    uint32_t z)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdDispatch(cmdBuffer, x, y, z);

    PostCmdDispatch(cmdBuffer, x, y, z);
}

bool PostCmdDispatchIndirect(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset)
{



    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatchIndirect(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdDispatchIndirect(cmdBuffer, buffer, offset);

    PostCmdDispatchIndirect(cmdBuffer, buffer, offset);
}

bool PreCmdCopyBuffer(
    VkCmdBuffer cmdBuffer,
    const VkBufferCopy* pRegions)
{
    if(pRegions != nullptr)
    {
    }

    return true;
}

bool PostCmdCopyBuffer(
    VkCmdBuffer cmdBuffer,
    VkBuffer srcBuffer,
    VkBuffer destBuffer,
    uint32_t regionCount)
{




    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBuffer(
    VkCmdBuffer cmdBuffer,
    VkBuffer srcBuffer,
    VkBuffer destBuffer,
    uint32_t regionCount,
    const VkBufferCopy* pRegions)
{
    PreCmdCopyBuffer(cmdBuffer, pRegions);

    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount, pRegions);

    PostCmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount);
}

bool PreCmdCopyImage(
    VkCmdBuffer cmdBuffer,
    const VkImageCopy* pRegions)
{
    if(pRegions != nullptr)
    {
    if(pRegions->srcSubresource.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRegions->srcSubresource.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdCopyImage parameter, VkImageAspect pRegions->srcSubresource.aspect, is an unrecognized enumerator");
        return false;
    }
    if(pRegions->destSubresource.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRegions->destSubresource.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdCopyImage parameter, VkImageAspect pRegions->destSubresource.aspect, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCmdCopyImage(
    VkCmdBuffer cmdBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkImage destImage,
    VkImageLayout destImageLayout,
    uint32_t regionCount)
{


    if(srcImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        srcImageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdCopyImage parameter, VkImageLayout srcImageLayout, is an unrecognized enumerator");
        return false;
    }


    if(destImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        destImageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdCopyImage parameter, VkImageLayout destImageLayout, is an unrecognized enumerator");
        return false;
    }


    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImage(
    VkCmdBuffer cmdBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkImage destImage,
    VkImageLayout destImageLayout,
    uint32_t regionCount,
    const VkImageCopy* pRegions)
{
    PreCmdCopyImage(cmdBuffer, pRegions);

    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdCopyImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);

    PostCmdCopyImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount);
}

bool PreCmdBlitImage(
    VkCmdBuffer cmdBuffer,
    const VkImageBlit* pRegions)
{
    if(pRegions != nullptr)
    {
    if(pRegions->srcSubresource.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRegions->srcSubresource.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdBlitImage parameter, VkImageAspect pRegions->srcSubresource.aspect, is an unrecognized enumerator");
        return false;
    }
    if(pRegions->destSubresource.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRegions->destSubresource.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdBlitImage parameter, VkImageAspect pRegions->destSubresource.aspect, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCmdBlitImage(
    VkCmdBuffer cmdBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkImage destImage,
    VkImageLayout destImageLayout,
    uint32_t regionCount,
    VkTexFilter filter)
{


    if(srcImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        srcImageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdBlitImage parameter, VkImageLayout srcImageLayout, is an unrecognized enumerator");
        return false;
    }


    if(destImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        destImageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdBlitImage parameter, VkImageLayout destImageLayout, is an unrecognized enumerator");
        return false;
    }


    if(filter < VK_TEX_FILTER_BEGIN_RANGE ||
        filter > VK_TEX_FILTER_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdBlitImage parameter, VkTexFilter filter, is an unrecognized enumerator");
        return false;
    }

    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdBlitImage(
    VkCmdBuffer cmdBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkImage destImage,
    VkImageLayout destImageLayout,
    uint32_t regionCount,
    const VkImageBlit* pRegions,
    VkTexFilter filter)
{
    PreCmdBlitImage(cmdBuffer, pRegions);

    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdBlitImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions, filter);

    PostCmdBlitImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, filter);
}

bool PreCmdCopyBufferToImage(
    VkCmdBuffer cmdBuffer,
    const VkBufferImageCopy* pRegions)
{
    if(pRegions != nullptr)
    {
    if(pRegions->imageSubresource.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRegions->imageSubresource.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdCopyBufferToImage parameter, VkImageAspect pRegions->imageSubresource.aspect, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCmdCopyBufferToImage(
    VkCmdBuffer cmdBuffer,
    VkBuffer srcBuffer,
    VkImage destImage,
    VkImageLayout destImageLayout,
    uint32_t regionCount)
{



    if(destImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        destImageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdCopyBufferToImage parameter, VkImageLayout destImageLayout, is an unrecognized enumerator");
        return false;
    }


    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBufferToImage(
    VkCmdBuffer cmdBuffer,
    VkBuffer srcBuffer,
    VkImage destImage,
    VkImageLayout destImageLayout,
    uint32_t regionCount,
    const VkBufferImageCopy* pRegions)
{
    PreCmdCopyBufferToImage(cmdBuffer, pRegions);

    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdCopyBufferToImage(cmdBuffer, srcBuffer, destImage, destImageLayout, regionCount, pRegions);

    PostCmdCopyBufferToImage(cmdBuffer, srcBuffer, destImage, destImageLayout, regionCount);
}

bool PreCmdCopyImageToBuffer(
    VkCmdBuffer cmdBuffer,
    const VkBufferImageCopy* pRegions)
{
    if(pRegions != nullptr)
    {
    if(pRegions->imageSubresource.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRegions->imageSubresource.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdCopyImageToBuffer parameter, VkImageAspect pRegions->imageSubresource.aspect, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCmdCopyImageToBuffer(
    VkCmdBuffer cmdBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkBuffer destBuffer,
    uint32_t regionCount)
{


    if(srcImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        srcImageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdCopyImageToBuffer parameter, VkImageLayout srcImageLayout, is an unrecognized enumerator");
        return false;
    }



    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImageToBuffer(
    VkCmdBuffer cmdBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkBuffer destBuffer,
    uint32_t regionCount,
    const VkBufferImageCopy* pRegions)
{
    PreCmdCopyImageToBuffer(cmdBuffer, pRegions);

    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdCopyImageToBuffer(cmdBuffer, srcImage, srcImageLayout, destBuffer, regionCount, pRegions);

    PostCmdCopyImageToBuffer(cmdBuffer, srcImage, srcImageLayout, destBuffer, regionCount);
}

bool PreCmdUpdateBuffer(
    VkCmdBuffer cmdBuffer,
    const uint32_t* pData)
{
    if(pData != nullptr)
    {
    }

    return true;
}

bool PostCmdUpdateBuffer(
    VkCmdBuffer cmdBuffer,
    VkBuffer destBuffer,
    VkDeviceSize destOffset,
    VkDeviceSize dataSize)
{




    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdUpdateBuffer(
    VkCmdBuffer cmdBuffer,
    VkBuffer destBuffer,
    VkDeviceSize destOffset,
    VkDeviceSize dataSize,
    const uint32_t* pData)
{
    PreCmdUpdateBuffer(cmdBuffer, pData);

    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize, pData);

    PostCmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize);
}

bool PostCmdFillBuffer(
    VkCmdBuffer cmdBuffer,
    VkBuffer destBuffer,
    VkDeviceSize destOffset,
    VkDeviceSize fillSize,
    uint32_t data)
{





    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdFillBuffer(
    VkCmdBuffer cmdBuffer,
    VkBuffer destBuffer,
    VkDeviceSize destOffset,
    VkDeviceSize fillSize,
    uint32_t data)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);

    PostCmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
}

bool PreCmdClearColorImage(
    VkCmdBuffer cmdBuffer,
    const VkClearColorValue* pColor,
    const VkImageSubresourceRange* pRanges)
{
    if(pColor != nullptr)
    {
    }

    if(pRanges != nullptr)
    {
    if(pRanges->aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRanges->aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdClearColorImage parameter, VkImageAspect pRanges->aspect, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCmdClearColorImage(
    VkCmdBuffer cmdBuffer,
    VkImage image,
    VkImageLayout imageLayout,
    uint32_t rangeCount)
{


    if(imageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        imageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdClearColorImage parameter, VkImageLayout imageLayout, is an unrecognized enumerator");
        return false;
    }


    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdClearColorImage(
    VkCmdBuffer cmdBuffer,
    VkImage image,
    VkImageLayout imageLayout,
    const VkClearColorValue* pColor,
    uint32_t rangeCount,
    const VkImageSubresourceRange* pRanges)
{
    PreCmdClearColorImage(cmdBuffer, pColor, pRanges);

    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdClearColorImage(cmdBuffer, image, imageLayout, pColor, rangeCount, pRanges);

    PostCmdClearColorImage(cmdBuffer, image, imageLayout, rangeCount);
}

bool PreCmdClearDepthStencilImage(
    VkCmdBuffer cmdBuffer,
    const VkImageSubresourceRange* pRanges)
{
    if(pRanges != nullptr)
    {
    if(pRanges->aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRanges->aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdClearDepthStencilImage parameter, VkImageAspect pRanges->aspect, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCmdClearDepthStencilImage(
    VkCmdBuffer cmdBuffer,
    VkImage image,
    VkImageLayout imageLayout,
    float depth,
    uint32_t stencil,
    uint32_t rangeCount)
{


    if(imageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        imageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdClearDepthStencilImage parameter, VkImageLayout imageLayout, is an unrecognized enumerator");
        return false;
    }




    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdClearDepthStencilImage(
    VkCmdBuffer cmdBuffer,
    VkImage image,
    VkImageLayout imageLayout,
    float depth,
    uint32_t stencil,
    uint32_t rangeCount,
    const VkImageSubresourceRange* pRanges)
{
    PreCmdClearDepthStencilImage(cmdBuffer, pRanges);

    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdClearDepthStencilImage(cmdBuffer, image, imageLayout, depth, stencil, rangeCount, pRanges);

    PostCmdClearDepthStencilImage(cmdBuffer, image, imageLayout, depth, stencil, rangeCount);
}

bool PreCmdClearColorAttachment(
    VkCmdBuffer cmdBuffer,
    const VkClearColorValue* pColor,
    const VkRect3D* pRects)
{
    if(pColor != nullptr)
    {
    }

    if(pRects != nullptr)
    {
    }

    return true;
}

bool PostCmdClearColorAttachment(
    VkCmdBuffer cmdBuffer,
    uint32_t colorAttachment,
    VkImageLayout imageLayout,
    uint32_t rectCount)
{


    if(imageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        imageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdClearColorAttachment parameter, VkImageLayout imageLayout, is an unrecognized enumerator");
        return false;
    }


    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdClearColorAttachment(
    VkCmdBuffer cmdBuffer,
    uint32_t colorAttachment,
    VkImageLayout imageLayout,
    const VkClearColorValue* pColor,
    uint32_t rectCount,
    const VkRect3D* pRects)
{
    PreCmdClearColorAttachment(cmdBuffer, pColor, pRects);

    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdClearColorAttachment(cmdBuffer, colorAttachment, imageLayout, pColor, rectCount, pRects);

    PostCmdClearColorAttachment(cmdBuffer, colorAttachment, imageLayout, rectCount);
}

bool PreCmdClearDepthStencilAttachment(
    VkCmdBuffer cmdBuffer,
    const VkRect3D* pRects)
{
    if(pRects != nullptr)
    {
    }

    return true;
}

bool PostCmdClearDepthStencilAttachment(
    VkCmdBuffer cmdBuffer,
    VkImageAspectFlags imageAspectMask,
    VkImageLayout imageLayout,
    float depth,
    uint32_t stencil,
    uint32_t rectCount)
{


    if(imageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        imageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdClearDepthStencilAttachment parameter, VkImageLayout imageLayout, is an unrecognized enumerator");
        return false;
    }




    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdClearDepthStencilAttachment(
    VkCmdBuffer cmdBuffer,
    VkImageAspectFlags imageAspectMask,
    VkImageLayout imageLayout,
    float depth,
    uint32_t stencil,
    uint32_t rectCount,
    const VkRect3D* pRects)
{
    PreCmdClearDepthStencilAttachment(cmdBuffer, pRects);

    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdClearDepthStencilAttachment(cmdBuffer, imageAspectMask, imageLayout, depth, stencil, rectCount, pRects);

    PostCmdClearDepthStencilAttachment(cmdBuffer, imageAspectMask, imageLayout, depth, stencil, rectCount);
}

bool PreCmdResolveImage(
    VkCmdBuffer cmdBuffer,
    const VkImageResolve* pRegions)
{
    if(pRegions != nullptr)
    {
    if(pRegions->srcSubresource.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRegions->srcSubresource.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdResolveImage parameter, VkImageAspect pRegions->srcSubresource.aspect, is an unrecognized enumerator");
        return false;
    }
    if(pRegions->destSubresource.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRegions->destSubresource.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdResolveImage parameter, VkImageAspect pRegions->destSubresource.aspect, is an unrecognized enumerator");
        return false;
    }
    }

    return true;
}

bool PostCmdResolveImage(
    VkCmdBuffer cmdBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkImage destImage,
    VkImageLayout destImageLayout,
    uint32_t regionCount)
{


    if(srcImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        srcImageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdResolveImage parameter, VkImageLayout srcImageLayout, is an unrecognized enumerator");
        return false;
    }


    if(destImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        destImageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdResolveImage parameter, VkImageLayout destImageLayout, is an unrecognized enumerator");
        return false;
    }


    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdResolveImage(
    VkCmdBuffer cmdBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkImage destImage,
    VkImageLayout destImageLayout,
    uint32_t regionCount,
    const VkImageResolve* pRegions)
{
    PreCmdResolveImage(cmdBuffer, pRegions);

    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdResolveImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);

    PostCmdResolveImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount);
}

bool PostCmdSetEvent(
    VkCmdBuffer cmdBuffer,
    VkEvent event,
    VkPipelineStageFlags stageMask)
{



    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdSetEvent(
    VkCmdBuffer cmdBuffer,
    VkEvent event,
    VkPipelineStageFlags stageMask)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdSetEvent(cmdBuffer, event, stageMask);

    PostCmdSetEvent(cmdBuffer, event, stageMask);
}

bool PostCmdResetEvent(
    VkCmdBuffer cmdBuffer,
    VkEvent event,
    VkPipelineStageFlags stageMask)
{



    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdResetEvent(
    VkCmdBuffer cmdBuffer,
    VkEvent event,
    VkPipelineStageFlags stageMask)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdResetEvent(cmdBuffer, event, stageMask);

    PostCmdResetEvent(cmdBuffer, event, stageMask);
}

bool PreCmdWaitEvents(
    VkCmdBuffer cmdBuffer,
    const VkEvent* pEvents,
    const void* const* ppMemBarriers)
{
    if(pEvents != nullptr)
    {
    }

    if(ppMemBarriers != nullptr)
    {
    }

    return true;
}

bool PostCmdWaitEvents(
    VkCmdBuffer cmdBuffer,
    uint32_t eventCount,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags destStageMask,
    uint32_t memBarrierCount)
{





    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdWaitEvents(
    VkCmdBuffer cmdBuffer,
    uint32_t eventCount,
    const VkEvent* pEvents,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags destStageMask,
    uint32_t memBarrierCount,
    const void* const* ppMemBarriers)
{
    PreCmdWaitEvents(cmdBuffer, pEvents, ppMemBarriers);

    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdWaitEvents(cmdBuffer, eventCount, pEvents, srcStageMask, destStageMask, memBarrierCount, ppMemBarriers);

    PostCmdWaitEvents(cmdBuffer, eventCount, srcStageMask, destStageMask, memBarrierCount);
}

bool PreCmdPipelineBarrier(
    VkCmdBuffer cmdBuffer,
    const void* const* ppMemBarriers)
{
    if(ppMemBarriers != nullptr)
    {
    }

    return true;
}

bool PostCmdPipelineBarrier(
    VkCmdBuffer cmdBuffer,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags destStageMask,
    VkBool32 byRegion,
    uint32_t memBarrierCount)
{





    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdPipelineBarrier(
    VkCmdBuffer cmdBuffer,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags destStageMask,
    VkBool32 byRegion,
    uint32_t memBarrierCount,
    const void* const* ppMemBarriers)
{
    PreCmdPipelineBarrier(cmdBuffer, ppMemBarriers);

    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdPipelineBarrier(cmdBuffer, srcStageMask, destStageMask, byRegion, memBarrierCount, ppMemBarriers);

    PostCmdPipelineBarrier(cmdBuffer, srcStageMask, destStageMask, byRegion, memBarrierCount);
}

bool PostCmdBeginQuery(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t slot,
    VkQueryControlFlags flags)
{




    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginQuery(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t slot,
    VkQueryControlFlags flags)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdBeginQuery(cmdBuffer, queryPool, slot, flags);

    PostCmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

bool PostCmdEndQuery(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t slot)
{



    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdEndQuery(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t slot)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdEndQuery(cmdBuffer, queryPool, slot);

    PostCmdEndQuery(cmdBuffer, queryPool, slot);
}

bool PostCmdResetQueryPool(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t startQuery,
    uint32_t queryCount)
{




    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdResetQueryPool(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t startQuery,
    uint32_t queryCount)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);

    PostCmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

bool PostCmdWriteTimestamp(
    VkCmdBuffer cmdBuffer,
    VkTimestampType timestampType,
    VkBuffer destBuffer,
    VkDeviceSize destOffset)
{

    if(timestampType < VK_TIMESTAMP_TYPE_BEGIN_RANGE ||
        timestampType > VK_TIMESTAMP_TYPE_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdWriteTimestamp parameter, VkTimestampType timestampType, is an unrecognized enumerator");
        return false;
    }



    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdWriteTimestamp(
    VkCmdBuffer cmdBuffer,
    VkTimestampType timestampType,
    VkBuffer destBuffer,
    VkDeviceSize destOffset)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdWriteTimestamp(cmdBuffer, timestampType, destBuffer, destOffset);

    PostCmdWriteTimestamp(cmdBuffer, timestampType, destBuffer, destOffset);
}

bool PostCmdCopyQueryPoolResults(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t startQuery,
    uint32_t queryCount,
    VkBuffer destBuffer,
    VkDeviceSize destOffset,
    VkDeviceSize destStride,
    VkQueryResultFlags flags)
{








    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyQueryPoolResults(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t startQuery,
    uint32_t queryCount,
    VkBuffer destBuffer,
    VkDeviceSize destOffset,
    VkDeviceSize destStride,
    VkQueryResultFlags flags)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdCopyQueryPoolResults(cmdBuffer, queryPool, startQuery, queryCount, destBuffer, destOffset, destStride, flags);

    PostCmdCopyQueryPoolResults(cmdBuffer, queryPool, startQuery, queryCount, destBuffer, destOffset, destStride, flags);
}

bool PreCmdPushConstants(
    VkCmdBuffer cmdBuffer,
    const void* values)
{
    if(values != nullptr)
    {
    }

    return true;
}

bool PostCmdPushConstants(
    VkCmdBuffer cmdBuffer,
    VkPipelineLayout layout,
    VkShaderStageFlags stageFlags,
    uint32_t start,
    uint32_t length)
{





    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdPushConstants(
    VkCmdBuffer cmdBuffer,
    VkPipelineLayout layout,
    VkShaderStageFlags stageFlags,
    uint32_t start,
    uint32_t length,
    const void* values)
{
    PreCmdPushConstants(cmdBuffer, values);

    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdPushConstants(cmdBuffer, layout, stageFlags, start, length, values);

    PostCmdPushConstants(cmdBuffer, layout, stageFlags, start, length);
}

bool PreCmdBeginRenderPass(
    VkCmdBuffer cmdBuffer,
    const VkRenderPassBeginInfo* pRenderPassBegin)
{
    if(pRenderPassBegin != nullptr)
    {
    if(pRenderPassBegin->sType != VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdBeginRenderPass parameter, VkStructureType pRenderPassBegin->sType, is an invalid enumerator");
        return false;
    }
    if(pRenderPassBegin->pClearValues != nullptr)
    {
    }
    }

    return true;
}

bool PostCmdBeginRenderPass(
    VkCmdBuffer cmdBuffer,
    VkRenderPassContents contents)
{

    if(contents < VK_RENDER_PASS_CONTENTS_BEGIN_RANGE ||
        contents > VK_RENDER_PASS_CONTENTS_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdBeginRenderPass parameter, VkRenderPassContents contents, is an unrecognized enumerator");
        return false;
    }

    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginRenderPass(
    VkCmdBuffer cmdBuffer,
    const VkRenderPassBeginInfo* pRenderPassBegin,
    VkRenderPassContents contents)
{
    PreCmdBeginRenderPass(cmdBuffer, pRenderPassBegin);

    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdBeginRenderPass(cmdBuffer, pRenderPassBegin, contents);

    PostCmdBeginRenderPass(cmdBuffer, contents);
}

bool PostCmdNextSubpass(
    VkCmdBuffer cmdBuffer,
    VkRenderPassContents contents)
{

    if(contents < VK_RENDER_PASS_CONTENTS_BEGIN_RANGE ||
        contents > VK_RENDER_PASS_CONTENTS_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, 1, "PARAMCHECK",
        "vkCmdNextSubpass parameter, VkRenderPassContents contents, is an unrecognized enumerator");
        return false;
    }

    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdNextSubpass(
    VkCmdBuffer cmdBuffer,
    VkRenderPassContents contents)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdNextSubpass(cmdBuffer, contents);

    PostCmdNextSubpass(cmdBuffer, contents);
}

bool PostCmdEndRenderPass(
    VkCmdBuffer cmdBuffer)
{

    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdEndRenderPass(
    VkCmdBuffer cmdBuffer)
{
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdEndRenderPass(cmdBuffer);

    PostCmdEndRenderPass(cmdBuffer);
}

bool PreCmdExecuteCommands(
    VkCmdBuffer cmdBuffer,
    const VkCmdBuffer* pCmdBuffers)
{
    if(pCmdBuffers != nullptr)
    {
    }

    return true;
}

bool PostCmdExecuteCommands(
    VkCmdBuffer cmdBuffer,
    uint32_t cmdBuffersCount)
{


    return true;
}

VK_LAYER_EXPORT void VKAPI vkCmdExecuteCommands(
    VkCmdBuffer cmdBuffer,
    uint32_t cmdBuffersCount,
    const VkCmdBuffer* pCmdBuffers)
{
    PreCmdExecuteCommands(cmdBuffer, pCmdBuffers);

    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdExecuteCommands(cmdBuffer, cmdBuffersCount, pCmdBuffers);

    PostCmdExecuteCommands(cmdBuffer, cmdBuffersCount);
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI vkGetDeviceProcAddr(VkDevice device, const char* funcName)
{
    if (device == NULL) {
        return NULL;
    }

    /* loader uses this to force layer initialization; device object is wrapped */
    if (!strcmp(funcName, "vkGetDeviceProcAddr")) {
        initDeviceTable(pc_device_table_map, (const VkBaseLayerObject *) device);
        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;
    }

    if (!strcmp(funcName, "vkCreateDevice"))
        return (PFN_vkVoidFunction) vkCreateDevice;
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (PFN_vkVoidFunction) vkDestroyDevice;
    if (!strcmp(funcName, "vkGetDeviceQueue"))
        return (PFN_vkVoidFunction) vkGetDeviceQueue;
    if (!strcmp(funcName, "vkQueueSubmit"))
        return (PFN_vkVoidFunction) vkQueueSubmit;
    if (!strcmp(funcName, "vkQueueWaitIdle"))
        return (PFN_vkVoidFunction) vkQueueWaitIdle;
    if (!strcmp(funcName, "vkDeviceWaitIdle"))
        return (PFN_vkVoidFunction) vkDeviceWaitIdle;
    if (!strcmp(funcName, "vkAllocMemory"))
        return (PFN_vkVoidFunction) vkAllocMemory;
    if (!strcmp(funcName, "vkMapMemory"))
        return (PFN_vkVoidFunction) vkMapMemory;
    if (!strcmp(funcName, "vkFlushMappedMemoryRanges"))
        return (PFN_vkVoidFunction) vkFlushMappedMemoryRanges;
    if (!strcmp(funcName, "vkInvalidateMappedMemoryRanges"))
        return (PFN_vkVoidFunction) vkInvalidateMappedMemoryRanges;
    if (!strcmp(funcName, "vkCreateFence"))
        return (PFN_vkVoidFunction) vkCreateFence;
    if (!strcmp(funcName, "vkResetFences"))
        return (PFN_vkVoidFunction) vkResetFences;
    if (!strcmp(funcName, "vkGetFenceStatus"))
        return (PFN_vkVoidFunction) vkGetFenceStatus;
    if (!strcmp(funcName, "vkWaitForFences"))
        return (PFN_vkVoidFunction) vkWaitForFences;
    if (!strcmp(funcName, "vkCreateSemaphore"))
        return (PFN_vkVoidFunction) vkCreateSemaphore;
    if (!strcmp(funcName, "vkQueueSignalSemaphore"))
        return (PFN_vkVoidFunction) vkQueueSignalSemaphore;
    if (!strcmp(funcName, "vkQueueWaitSemaphore"))
        return (PFN_vkVoidFunction) vkQueueWaitSemaphore;
    if (!strcmp(funcName, "vkCreateEvent"))
        return (PFN_vkVoidFunction) vkCreateEvent;
    if (!strcmp(funcName, "vkGetEventStatus"))
        return (PFN_vkVoidFunction) vkGetEventStatus;
    if (!strcmp(funcName, "vkSetEvent"))
        return (PFN_vkVoidFunction) vkSetEvent;
    if (!strcmp(funcName, "vkResetEvent"))
        return (PFN_vkVoidFunction) vkResetEvent;
    if (!strcmp(funcName, "vkCreateQueryPool"))
        return (PFN_vkVoidFunction) vkCreateQueryPool;
    if (!strcmp(funcName, "vkGetQueryPoolResults"))
        return (PFN_vkVoidFunction) vkGetQueryPoolResults;
    if (!strcmp(funcName, "vkCreateBuffer"))
        return (PFN_vkVoidFunction) vkCreateBuffer;
    if (!strcmp(funcName, "vkCreateBufferView"))
        return (PFN_vkVoidFunction) vkCreateBufferView;
    if (!strcmp(funcName, "vkCreateImage"))
        return (PFN_vkVoidFunction) vkCreateImage;
    if (!strcmp(funcName, "vkGetImageSubresourceLayout"))
        return (PFN_vkVoidFunction) vkGetImageSubresourceLayout;
    if (!strcmp(funcName, "vkCreateImageView"))
        return (PFN_vkVoidFunction) vkCreateImageView;
    if (!strcmp(funcName, "vkCreateShader"))
        return (PFN_vkVoidFunction) vkCreateShader;
    if (!strcmp(funcName, "vkCreateGraphicsPipelines"))
        return (PFN_vkVoidFunction) vkCreateGraphicsPipelines;
    if (!strcmp(funcName, "vkCreateComputePipelines"))
        return (PFN_vkVoidFunction) vkCreateComputePipelines;
    if (!strcmp(funcName, "vkCreatePipelineLayout"))
        return (PFN_vkVoidFunction) vkCreatePipelineLayout;
    if (!strcmp(funcName, "vkCreateSampler"))
        return (PFN_vkVoidFunction) vkCreateSampler;
    if (!strcmp(funcName, "vkCreateDescriptorSetLayout"))
        return (PFN_vkVoidFunction) vkCreateDescriptorSetLayout;
    if (!strcmp(funcName, "vkCreateDescriptorPool"))
        return (PFN_vkVoidFunction) vkCreateDescriptorPool;
    if (!strcmp(funcName, "vkResetDescriptorPool"))
        return (PFN_vkVoidFunction) vkResetDescriptorPool;
    if (!strcmp(funcName, "vkAllocDescriptorSets"))
        return (PFN_vkVoidFunction) vkAllocDescriptorSets;
    if (!strcmp(funcName, "vkCreateDynamicViewportState"))
        return (PFN_vkVoidFunction) vkCreateDynamicViewportState;
    if (!strcmp(funcName, "vkCreateDynamicLineWidthState"))
        return (PFN_vkVoidFunction) vkCreateDynamicLineWidthState;
    if (!strcmp(funcName, "vkCreateDynamicDepthBiasState"))
        return (PFN_vkVoidFunction) vkCreateDynamicDepthBiasState;
    if (!strcmp(funcName, "vkCreateDynamicBlendState"))
        return (PFN_vkVoidFunction) vkCreateDynamicBlendState;
    if (!strcmp(funcName, "vkCreateDynamicDepthBoundsState"))
        return (PFN_vkVoidFunction) vkCreateDynamicDepthBoundsState;
    if (!strcmp(funcName, "vkCreateDynamicStencilState"))
        return (PFN_vkVoidFunction) vkCreateDynamicStencilState;
    if (!strcmp(funcName, "vkCreateCommandBuffer"))
        return (PFN_vkVoidFunction) vkCreateCommandBuffer;
    if (!strcmp(funcName, "vkBeginCommandBuffer"))
        return (PFN_vkVoidFunction) vkBeginCommandBuffer;
    if (!strcmp(funcName, "vkEndCommandBuffer"))
        return (PFN_vkVoidFunction) vkEndCommandBuffer;
    if (!strcmp(funcName, "vkResetCommandBuffer"))
        return (PFN_vkVoidFunction) vkResetCommandBuffer;
    if (!strcmp(funcName, "vkCmdBindPipeline"))
        return (PFN_vkVoidFunction) vkCmdBindPipeline;
    if (!strcmp(funcName, "vkCmdBindDescriptorSets"))
        return (PFN_vkVoidFunction) vkCmdBindDescriptorSets;
    if (!strcmp(funcName, "vkCmdBindVertexBuffers"))
        return (PFN_vkVoidFunction) vkCmdBindVertexBuffers;
    if (!strcmp(funcName, "vkCmdBindIndexBuffer"))
        return (PFN_vkVoidFunction) vkCmdBindIndexBuffer;
    if (!strcmp(funcName, "vkCmdDraw"))
        return (PFN_vkVoidFunction) vkCmdDraw;
    if (!strcmp(funcName, "vkCmdDrawIndexed"))
        return (PFN_vkVoidFunction) vkCmdDrawIndexed;
    if (!strcmp(funcName, "vkCmdDrawIndirect"))
        return (PFN_vkVoidFunction) vkCmdDrawIndirect;
    if (!strcmp(funcName, "vkCmdDrawIndexedIndirect"))
        return (PFN_vkVoidFunction) vkCmdDrawIndexedIndirect;
    if (!strcmp(funcName, "vkCmdDispatch"))
        return (PFN_vkVoidFunction) vkCmdDispatch;
    if (!strcmp(funcName, "vkCmdDispatchIndirect"))
        return (PFN_vkVoidFunction) vkCmdDispatchIndirect;
    if (!strcmp(funcName, "vkCmdCopyBuffer"))
        return (PFN_vkVoidFunction) vkCmdCopyBuffer;
    if (!strcmp(funcName, "vkCmdCopyImage"))
        return (PFN_vkVoidFunction) vkCmdCopyImage;
    if (!strcmp(funcName, "vkCmdBlitImage"))
        return (PFN_vkVoidFunction) vkCmdBlitImage;
    if (!strcmp(funcName, "vkCmdCopyBufferToImage"))
        return (PFN_vkVoidFunction) vkCmdCopyBufferToImage;
    if (!strcmp(funcName, "vkCmdCopyImageToBuffer"))
        return (PFN_vkVoidFunction) vkCmdCopyImageToBuffer;
    if (!strcmp(funcName, "vkCmdUpdateBuffer"))
        return (PFN_vkVoidFunction) vkCmdUpdateBuffer;
    if (!strcmp(funcName, "vkCmdFillBuffer"))
        return (PFN_vkVoidFunction) vkCmdFillBuffer;
    if (!strcmp(funcName, "vkCmdClearColorImage"))
        return (PFN_vkVoidFunction) vkCmdClearColorImage;
    if (!strcmp(funcName, "vkCmdResolveImage"))
        return (PFN_vkVoidFunction) vkCmdResolveImage;
    if (!strcmp(funcName, "vkCmdSetEvent"))
        return (PFN_vkVoidFunction) vkCmdSetEvent;
    if (!strcmp(funcName, "vkCmdResetEvent"))
        return (PFN_vkVoidFunction) vkCmdResetEvent;
    if (!strcmp(funcName, "vkCmdWaitEvents"))
        return (PFN_vkVoidFunction) vkCmdWaitEvents;
    if (!strcmp(funcName, "vkCmdPipelineBarrier"))
        return (PFN_vkVoidFunction) vkCmdPipelineBarrier;
    if (!strcmp(funcName, "vkCmdBeginQuery"))
        return (PFN_vkVoidFunction) vkCmdBeginQuery;
    if (!strcmp(funcName, "vkCmdEndQuery"))
        return (PFN_vkVoidFunction) vkCmdEndQuery;
    if (!strcmp(funcName, "vkCmdResetQueryPool"))
        return (PFN_vkVoidFunction) vkCmdResetQueryPool;
    if (!strcmp(funcName, "vkCmdWriteTimestamp"))
        return (PFN_vkVoidFunction) vkCmdWriteTimestamp;
    if (!strcmp(funcName, "vkCmdCopyQueryPoolResults"))
        return (PFN_vkVoidFunction) vkCmdCopyQueryPoolResults;
    if (!strcmp(funcName, "vkCreateFramebuffer"))
        return (PFN_vkVoidFunction) vkCreateFramebuffer;
    if (!strcmp(funcName, "vkCreateRenderPass"))
        return (PFN_vkVoidFunction) vkCreateRenderPass;
    if (!strcmp(funcName, "vkCmdBeginRenderPass"))
        return (PFN_vkVoidFunction) vkCmdBeginRenderPass;
    if (!strcmp(funcName, "vkCmdNextSubpass"))
        return (PFN_vkVoidFunction) vkCmdNextSubpass;

    {
        if (get_dispatch_table(pc_device_table_map, device)->GetDeviceProcAddr == NULL)
            return NULL;
        return get_dispatch_table(pc_device_table_map, device)->GetDeviceProcAddr(device, funcName);
    }
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    if (instance == NULL) {
        return NULL;
    }

    /* loader uses this to force layer initialization; instance object is wrapped */
    if (!strcmp(funcName, "vkGetInstanceProcAddr")) {
        initInstanceTable(pc_instance_table_map, (const VkBaseLayerObject *) instance);
        return (PFN_vkVoidFunction) vkGetInstanceProcAddr;
    }

    if (!strcmp(funcName, "vkCreateInstance"))
        return (PFN_vkVoidFunction) vkCreateInstance;
    if (!strcmp(funcName, "vkDestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(funcName, "vkEnumeratePhysicalDevices"))
        return (PFN_vkVoidFunction) vkEnumeratePhysicalDevices;
    if (!strcmp(funcName, "vkGetPhysicalDeviceProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceProperties;
    if (!strcmp(funcName, "vkGetPhysicalDeviceFeatures"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceFeatures;
    if (!strcmp(funcName, "vkGetPhysicalDeviceFormatProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceFormatProperties;
    if (!strcmp(funcName, "vkGetGlobalLayerProperties"))
        return (PFN_vkVoidFunction) vkGetGlobalLayerProperties;
    if (!strcmp(funcName, "vkGetGlobalExtensionProperties"))
        return (PFN_vkVoidFunction) vkGetGlobalExtensionProperties;
    if (!strcmp(funcName, "vkGetPhysicalDeviceLayerProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceLayerProperties;
    if (!strcmp(funcName, "vkGetPhysicalDeviceExtensionProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceExtensionProperties;

    layer_data *data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    PFN_vkVoidFunction fptr = debug_report_get_instance_proc_addr(data->report_data, funcName);
    if(fptr)
        return fptr;

    {
        if (get_dispatch_table(pc_instance_table_map, instance)->GetInstanceProcAddr == NULL)
            return NULL;
        return get_dispatch_table(pc_instance_table_map, instance)->GetInstanceProcAddr(instance, funcName);
    }
}
