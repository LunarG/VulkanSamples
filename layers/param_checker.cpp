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

#include "loader_platform.h"
#include "vkLayer.h"
#include "layers_config.h"
#include "vk_enum_validate_helper.h"
#include "vk_struct_validate_helper.h"
//The following is #included again to catch certain OS-specific functions being used:
#include "loader_platform.h"

#include "layers_table.h"
#include "layer_data.h"
#include "layer_logging.h"

typedef struct _layer_data {
    debug_report_data *report_data;
    VkDbgMsgCallback logging_callback;
} layer_data;

static std::unordered_map<void*, layer_data*> layer_data_map;
static device_table_map pc_device_table_map;
static instance_table_map pc_instance_table_map;

#define PC_LAYER_EXT_ARRAY_SIZE 2
static const VkExtensionProperties pcExts[PC_LAYER_EXT_ARRAY_SIZE] = {
    {
        VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES,
        "ParamChecker",
        0x10,
        "Sample layer: ParamChecker",
    },
    {
        VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES,
        "Validation",
        0x10,
        "Sample layer: ParamChecker",
    }
};

// "my instance data"
debug_report_data *mid(VkInstance object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *data = get_my_data_ptr(get_dispatch_key(object), layer_data_map);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "MID: map: %p, object: %p, key: %p, data: %p\n", &layer_data_map, object, key, data);
#endif
    assert(data->report_data != NULL);

    return data->report_data;
}

// "my device data"
debug_report_data *mdd(VkObject object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *data = get_my_data_ptr(key, layer_data_map);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "MDD: map: %p, object: %p, key: %p, data: %p\n", &layer_data_map, object, key, data);
#endif
    assert(data->report_data != NULL);
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
        if(log_output == NULL)
        {
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

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionCount(uint32_t* pCount)
{
    *pCount = PC_LAYER_EXT_ARRAY_SIZE;
    return VK_SUCCESS;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionProperties(
    uint32_t extensionIndex,
    VkExtensionProperties* pProperties)
{
    /* This entrypoint is NOT going to init it's own dispatch table since loader calls here early */
    if (extensionIndex >= PC_LAYER_EXT_ARRAY_SIZE)
        return VK_ERROR_INVALID_VALUE;
    memcpy(pProperties, &pcExts[extensionIndex], sizeof(VkExtensionProperties));

    return VK_SUCCESS;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceExtensionCount(
    VkPhysicalDevice gpu,
    uint32_t* pCount)
{
    *pCount = PC_LAYER_EXT_ARRAY_SIZE;
    return VK_SUCCESS;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceExtensionProperties(
    VkPhysicalDevice gpu,
    uint32_t extensionIndex,
    VkExtensionProperties* pProperties)
{
    /* This entrypoint is NOT going to init it's own dispatch table since loader calls here early */
    if (extensionIndex >= PC_LAYER_EXT_ARRAY_SIZE)
        return VK_ERROR_INVALID_VALUE;
    memcpy(pProperties, &pcExts[extensionIndex], sizeof(VkExtensionProperties));

    return VK_SUCCESS;
}

// Version: 0.111.0

static
std::string EnumeratorString(VkResult const& enumerator)
{
    switch(enumerator)
    {
        case VK_ERROR_MEMORY_NOT_BOUND:
        {
            return "VK_ERROR_MEMORY_NOT_BOUND";
            break;
        }
        case VK_ERROR_BUILDING_COMMAND_BUFFER:
        {
            return "VK_ERROR_BUILDING_COMMAND_BUFFER";
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
        case VK_ERROR_BAD_PIPELINE_DATA:
        {
            return "VK_ERROR_BAD_PIPELINE_DATA";
            break;
        }
        case VK_ERROR_INVALID_OBJECT_TYPE:
        {
            return "VK_ERROR_INVALID_OBJECT_TYPE";
            break;
        }
        case VK_ERROR_INVALID_QUEUE_TYPE:
        {
            return "VK_ERROR_INVALID_QUEUE_TYPE";
            break;
        }
        case VK_ERROR_BAD_SHADER_CODE:
        {
            return "VK_ERROR_BAD_SHADER_CODE";
            break;
        }
        case VK_ERROR_INVALID_IMAGE:
        {
            return "VK_ERROR_INVALID_IMAGE";
            break;
        }
        case VK_ERROR_UNSUPPORTED_SHADER_IL_VERSION:
        {
            return "VK_ERROR_UNSUPPORTED_SHADER_IL_VERSION";
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
        case VK_ERROR_INVALID_POINTER:
        {
            return "VK_ERROR_INVALID_POINTER";
            break;
        }
        case VK_ERROR_INVALID_VALUE:
        {
            return "VK_ERROR_INVALID_VALUE";
            break;
        }
        case VK_ERROR_UNAVAILABLE:
        {
            return "VK_ERROR_UNAVAILABLE";
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
        case VK_ERROR_NOT_MAPPABLE:
        {
            return "VK_ERROR_NOT_MAPPABLE";
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
        case VK_ERROR_INVALID_FLAGS:
        {
            return "VK_ERROR_INVALID_FLAGS";
            break;
        }
        case VK_EVENT_RESET:
        {
            return "VK_EVENT_RESET";
            break;
        }
        case VK_ERROR_INVALID_DESCRIPTOR_SET_DATA:
        {
            return "VK_ERROR_INVALID_DESCRIPTOR_SET_DATA";
            break;
        }
        case VK_UNSUPPORTED:
        {
            return "VK_UNSUPPORTED";
            break;
        }
        case VK_ERROR_INVALID_HANDLE:
        {
            return "VK_ERROR_INVALID_HANDLE";
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
        case VK_ERROR_INCOMPATIBLE_QUEUE:
        {
            return "VK_ERROR_INCOMPATIBLE_QUEUE";
            break;
        }
        case VK_ERROR_INVALID_EXTENSION:
        {
            return "VK_ERROR_INVALID_EXTENSION";
            break;
        }
        case VK_ERROR_DEVICE_ALREADY_CREATED:
        {
            return "VK_ERROR_DEVICE_ALREADY_CREATED";
            break;
        }
        case VK_ERROR_DEVICE_LOST:
        {
            return "VK_ERROR_DEVICE_LOST";
            break;
        }
        case VK_ERROR_INVALID_ORDINAL:
        {
            return "VK_ERROR_INVALID_ORDINAL";
            break;
        }
        case VK_ERROR_INVALID_MEMORY_SIZE:
        {
            return "VK_ERROR_INVALID_MEMORY_SIZE";
            break;
        }
        case VK_ERROR_INCOMPLETE_COMMAND_BUFFER:
        {
            return "VK_ERROR_INCOMPLETE_COMMAND_BUFFER";
            break;
        }
        case VK_ERROR_INVALID_ALIGNMENT:
        {
            return "VK_ERROR_INVALID_ALIGNMENT";
            break;
        }
        case VK_ERROR_INVALID_FORMAT:
        {
            return "VK_ERROR_INVALID_FORMAT";
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
    VkMemoryPropertyFlagBits allFlags = (VkMemoryPropertyFlagBits)(
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
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
        VK_BUFFER_USAGE_GENERAL);
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
    if(enumerator & VK_BUFFER_USAGE_GENERAL)
    {
        strings.push_back("VK_BUFFER_USAGE_GENERAL");
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
    VkBufferCreateFlagBits allFlags = (VkBufferCreateFlagBits)(VK_BUFFER_CREATE_SPARSE_BIT);
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
bool ValidateEnumerator(VkImageUsageFlagBits const& enumerator)
{
    VkImageUsageFlagBits allFlags = (VkImageUsageFlagBits)(VK_IMAGE_USAGE_DEPTH_STENCIL_BIT |
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT |
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT |
        VK_IMAGE_USAGE_GENERAL);
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
    if(enumerator & VK_IMAGE_USAGE_GENERAL)
    {
        strings.push_back("VK_IMAGE_USAGE_GENERAL");
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
        VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT |
        VK_IMAGE_CREATE_SPARSE_BIT |
        VK_IMAGE_CREATE_INVARIANT_DATA_BIT);
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
    if(enumerator & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT)
    {
        strings.push_back("VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT");
    }
    if(enumerator & VK_IMAGE_CREATE_SPARSE_BIT)
    {
        strings.push_back("VK_IMAGE_CREATE_SPARSE_BIT");
    }
    if(enumerator & VK_IMAGE_CREATE_INVARIANT_DATA_BIT)
    {
        strings.push_back("VK_IMAGE_CREATE_INVARIANT_DATA_BIT");
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
bool ValidateEnumerator(VkDepthStencilViewCreateFlagBits const& enumerator)
{
    VkDepthStencilViewCreateFlagBits allFlags = (VkDepthStencilViewCreateFlagBits)(VK_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_STENCIL_BIT |
        VK_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_DEPTH_BIT);
    if(enumerator & (~allFlags))
    {
        return false;
    }

    return true;
}

static
std::string EnumeratorString(VkDepthStencilViewCreateFlagBits const& enumerator)
{
    if(!ValidateEnumerator(enumerator))
    {
        return "unrecognized enumerator";
    }

    std::vector<std::string> strings;
    if(enumerator & VK_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_STENCIL_BIT)
    {
        strings.push_back("VK_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_STENCIL_BIT");
    }
    if(enumerator & VK_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_DEPTH_BIT)
    {
        strings.push_back("VK_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_DEPTH_BIT");
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
    VkPipelineCreateFlagBits allFlags = (VkPipelineCreateFlagBits)(VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT |
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
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
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
bool ValidateEnumerator(VkCmdBufferOptimizeFlagBits const& enumerator)
{
    VkCmdBufferOptimizeFlagBits allFlags = (VkCmdBufferOptimizeFlagBits)(VK_CMD_BUFFER_OPTIMIZE_DESCRIPTOR_SET_SWITCH_BIT |
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

VK_LAYER_EXPORT VkResult VKAPI vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    VkInstance* pInstance)
{
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(pc_instance_table_map, *pInstance);
    VkResult result = pTable->CreateInstance(pCreateInfo, pInstance);

    if (result == VK_SUCCESS) {
        layer_data *data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
        data->report_data = debug_report_create_instance(pTable, *pInstance, pCreateInfo->extensionCount,
            pCreateInfo->pEnabledExtensions);

        InitParamChecker(data);
    }

        return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyInstance(
    VkInstance instance)
{
    // Grab the key before the instance is destroyed.
    dispatch_key key = get_dispatch_key(instance);
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(pc_instance_table_map, instance);
    VkResult result = pTable->DestroyInstance(instance);

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

    return result;
}

void PreEnumeratePhysicalDevices(
    VkInstance instance,
    VkPhysicalDevice* pPhysicalDevices)
{
    if(instance == nullptr)
    {
        log_msg(mid(instance), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkEnumeratePhysicalDevices parameter, VkInstance instance, is null pointer");
        return;
    }

    if(pPhysicalDevices == nullptr)
    {
        log_msg(mid(instance), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkEnumeratePhysicalDevices parameter, VkPhysicalDevice* pPhysicalDevices, is null pointer");
        return;
    }
    if((*pPhysicalDevices) == nullptr)
    {
        log_msg(mid(instance), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkEnumeratePhysicalDevices parameter, VkPhysicalDevice* pPhysicalDevices, is null pointer");
        return;
    }
}

void PostEnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t* pPhysicalDeviceCount,
    VkPhysicalDevice* pPhysicalDevices,
    VkResult result)
{
    if(instance == nullptr)
    {
        log_msg(mid(instance), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkEnumeratePhysicalDevices parameter, VkInstance instance, is null pointer");
        return;
    }

    if(pPhysicalDeviceCount == nullptr)
    {
        log_msg(mid(instance), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkEnumeratePhysicalDevices parameter, uint32_t* pPhysicalDeviceCount, is null pointer");
        return;
    }

    if(pPhysicalDevices == nullptr)
    {
        log_msg(mid(instance), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkEnumeratePhysicalDevices parameter, VkPhysicalDevice* pPhysicalDevices, is null pointer");
        return;
    }
    if((*pPhysicalDevices) == nullptr)
    {
        log_msg(mid(instance), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkEnumeratePhysicalDevices parameter, VkPhysicalDevice* pPhysicalDevices, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkEnumeratePhysicalDevices parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mid(instance), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t* pPhysicalDeviceCount,
    VkPhysicalDevice* pPhysicalDevices)
{
    PreEnumeratePhysicalDevices(instance, pPhysicalDevices);
    VkResult result = get_dispatch_table(pc_instance_table_map, instance)->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);

    PostEnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices, result);

    return result;
}

void PreGetPhysicalDeviceFeatures(
    VkPhysicalDevice physicalDevice)
{
    if(physicalDevice == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceFeatures parameter, VkPhysicalDevice physicalDevice, is null pointer");
        return;
    }
}

void PostGetPhysicalDeviceFeatures(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceFeatures* pFeatures,
    VkResult result)
{
    if(physicalDevice == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceFeatures parameter, VkPhysicalDevice physicalDevice, is null pointer");
        return;
    }

    if(pFeatures == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceFeatures parameter, VkPhysicalDeviceFeatures* pFeatures, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkGetPhysicalDeviceFeatures parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceFeatures(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceFeatures* pFeatures)
{
    PreGetPhysicalDeviceFeatures(physicalDevice);
    VkResult result = get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceFeatures(physicalDevice, pFeatures);

    PostGetPhysicalDeviceFeatures(physicalDevice, pFeatures, result);

    return result;
}

void PreGetPhysicalDeviceFormatInfo(
    VkPhysicalDevice physicalDevice)
{
    if(physicalDevice == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceFormatInfo parameter, VkPhysicalDevice physicalDevice, is null pointer");
        return;
    }
}

void PostGetPhysicalDeviceFormatInfo(
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkFormatProperties* pFormatInfo,
    VkResult result)
{
    if(physicalDevice == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceFormatInfo parameter, VkPhysicalDevice physicalDevice, is null pointer");
        return;
    }

    if(format < VK_FORMAT_BEGIN_RANGE ||
        format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceFormatInfo parameter, VkFormat format, is unrecognized enumerator");
        return;
    }

    if(pFormatInfo == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceFormatInfo parameter, VkFormatProperties* pFormatInfo, is null pointer");
        return;
    }
    if(!ValidateEnumerator((VkFormatFeatureFlagBits)pFormatInfo->linearTilingFeatures))
    {
        std::string reason = "vkGetPhysicalDeviceFormatInfo parameter, VkFormatFeatureFlags pFormatInfo->linearTilingFeatures, is " + EnumeratorString((VkFormatFeatureFlagBits)pFormatInfo->linearTilingFeatures);
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
    if(!ValidateEnumerator((VkFormatFeatureFlagBits)pFormatInfo->optimalTilingFeatures))
    {
        std::string reason = "vkGetPhysicalDeviceFormatInfo parameter, VkFormatFeatureFlags pFormatInfo->optimalTilingFeatures, is " + EnumeratorString((VkFormatFeatureFlagBits)pFormatInfo->optimalTilingFeatures);
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkGetPhysicalDeviceFormatInfo parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceFormatInfo(
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkFormatProperties* pFormatInfo)
{
    PreGetPhysicalDeviceFormatInfo(physicalDevice);
    VkResult result = get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceFormatInfo(physicalDevice, format, pFormatInfo);

    PostGetPhysicalDeviceFormatInfo(physicalDevice, format, pFormatInfo, result);

    return result;
}

void PreGetPhysicalDeviceLimits(
    VkPhysicalDevice physicalDevice)
{
    if(physicalDevice == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceLimits parameter, VkPhysicalDevice physicalDevice, is null pointer");
        return;
    }
}

void PostGetPhysicalDeviceLimits(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceLimits* pLimits,
    VkResult result)
{
    if(physicalDevice == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceLimits parameter, VkPhysicalDevice physicalDevice, is null pointer");
        return;
    }

    if(pLimits == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceLimits parameter, VkPhysicalDeviceLimits* pLimits, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkGetPhysicalDeviceLimits parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceLimits(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceLimits* pLimits)
{
    PreGetPhysicalDeviceLimits(physicalDevice);
    VkResult result = get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceLimits(physicalDevice, pLimits);

    PostGetPhysicalDeviceLimits(physicalDevice, pLimits, result);

    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    VkDevice* pDevice)
{
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

VK_LAYER_EXPORT VkResult VKAPI vkDestroyDevice(
    VkDevice device)
{
    layer_debug_report_destroy_device(device);

    dispatch_key key = get_dispatch_key(device);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "Device: %p, key: %p\n", device, key);
#endif

    VkResult result = get_dispatch_table(pc_device_table_map, device)->DestroyDevice(device);
    pc_device_table_map.erase(key);
    assert(pc_device_table_map.size() == 0 && "Should not have any instance mappings hanging around");

    return result;
}

void PreGetPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice)
{
    if(physicalDevice == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceProperties parameter, VkPhysicalDevice physicalDevice, is null pointer");
        return;
    }
}

void PostGetPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties* pProperties,
    VkResult result)
{
    if(physicalDevice == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceProperties parameter, VkPhysicalDevice physicalDevice, is null pointer");
        return;
    }

    if(pProperties == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceProperties parameter, VkPhysicalDeviceProperties* pProperties, is null pointer");
        return;
    }
    if(pProperties->deviceType < VK_PHYSICAL_DEVICE_TYPE_BEGIN_RANGE ||
        pProperties->deviceType > VK_PHYSICAL_DEVICE_TYPE_END_RANGE)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceProperties parameter, VkPhysicalDeviceType pProperties->deviceType, is unrecognized enumerator");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkGetPhysicalDeviceProperties parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties* pProperties)
{
    PreGetPhysicalDeviceProperties(physicalDevice);
    VkResult result = get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceProperties(physicalDevice, pProperties);

    PostGetPhysicalDeviceProperties(physicalDevice, pProperties, result);

    return result;
}

void PreGetPhysicalDevicePerformance(
    VkPhysicalDevice physicalDevice)
{
    if(physicalDevice == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDevicePerformance parameter, VkPhysicalDevice physicalDevice, is null pointer");
        return;
    }
}

void PostGetPhysicalDevicePerformance(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDevicePerformance* pPerformance,
    VkResult result)
{
    if(physicalDevice == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDevicePerformance parameter, VkPhysicalDevice physicalDevice, is null pointer");
        return;
    }

    if(pPerformance == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDevicePerformance parameter, VkPhysicalDevicePerformance* pPerformance, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkGetPhysicalDevicePerformance parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDevicePerformance(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDevicePerformance* pPerformance)
{
    PreGetPhysicalDevicePerformance(physicalDevice);
    VkResult result = get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDevicePerformance(physicalDevice, pPerformance);

    PostGetPhysicalDevicePerformance(physicalDevice, pPerformance, result);

    return result;
}

void PreGetPhysicalDeviceQueueCount(
    VkPhysicalDevice physicalDevice)
{
    if(physicalDevice == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceQueueCount parameter, VkPhysicalDevice physicalDevice, is null pointer");
        return;
    }
}

void PostGetPhysicalDeviceQueueCount(
    VkPhysicalDevice physicalDevice,
    uint32_t* pCount,
    VkResult result)
{
    if(physicalDevice == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceQueueCount parameter, VkPhysicalDevice physicalDevice, is null pointer");
        return;
    }

    if(pCount == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceQueueCount parameter, uint32_t* pCount, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkGetPhysicalDeviceQueueCount parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceQueueCount(
    VkPhysicalDevice physicalDevice,
    uint32_t* pCount)
{
    PreGetPhysicalDeviceQueueCount(physicalDevice);
    VkResult result = get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceQueueCount(physicalDevice, pCount);

    PostGetPhysicalDeviceQueueCount(physicalDevice, pCount, result);

    return result;
}

void PreGetPhysicalDeviceQueueProperties(
    VkPhysicalDevice physicalDevice)
{
    if(physicalDevice == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceQueueProperties parameter, VkPhysicalDevice physicalDevice, is null pointer");
        return;
    }
}

void PostGetPhysicalDeviceQueueProperties(
    VkPhysicalDevice physicalDevice,
    uint32_t count,
    VkPhysicalDeviceQueueProperties* pQueueProperties,
    VkResult result)
{
    if(physicalDevice == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceQueueProperties parameter, VkPhysicalDevice physicalDevice, is null pointer");
        return;
    }


    if(pQueueProperties == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceQueueProperties parameter, VkPhysicalDeviceQueueProperties* pQueueProperties, is null pointer");
        return;
    }
    if(!ValidateEnumerator((VkQueueFlagBits)pQueueProperties->queueFlags))
    {
        std::string reason = "vkGetPhysicalDeviceQueueProperties parameter, VkQueueFlags pQueueProperties->queueFlags, is " + EnumeratorString((VkQueueFlagBits)pQueueProperties->queueFlags);
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkGetPhysicalDeviceQueueProperties parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceQueueProperties(
    VkPhysicalDevice physicalDevice,
    uint32_t count,
    VkPhysicalDeviceQueueProperties* pQueueProperties)
{
    PreGetPhysicalDeviceQueueProperties(physicalDevice);
    VkResult result = get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceQueueProperties(physicalDevice, count, pQueueProperties);

    PostGetPhysicalDeviceQueueProperties(physicalDevice, count, pQueueProperties, result);

    return result;
}

void PreGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice physicalDevice)
{
    if(physicalDevice == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceMemoryProperties parameter, VkPhysicalDevice physicalDevice, is null pointer");
        return;
    }
}

void PostGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties* pMemoryProperies,
    VkResult result)
{
    if(physicalDevice == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceMemoryProperties parameter, VkPhysicalDevice physicalDevice, is null pointer");
        return;
    }

    if(pMemoryProperies == nullptr)
    {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetPhysicalDeviceMemoryProperties parameter, VkPhysicalDeviceMemoryProperties* pMemoryProperies, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkGetPhysicalDeviceMemoryProperties parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties* pMemoryProperies)
{
    PreGetPhysicalDeviceMemoryProperties(physicalDevice);
    VkResult result = get_dispatch_table(pc_instance_table_map, physicalDevice)->GetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperies);

    PostGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperies, result);

    return result;
}

void PreGetDeviceQueue(
    VkDevice device,
    VkQueue* pQueue)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetDeviceQueue parameter, VkDevice device, is null pointer");
        return;
    }

    if(pQueue == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetDeviceQueue parameter, VkQueue* pQueue, is null pointer");
        return;
    }
    if((*pQueue) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetDeviceQueue parameter, VkQueue* pQueue, is null pointer");
        return;
    }
}

void PostGetDeviceQueue(
    VkDevice device,
    uint32_t queueNodeIndex,
    uint32_t queueIndex,
    VkQueue* pQueue,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetDeviceQueue parameter, VkDevice device, is null pointer");
        return;
    }



    if(pQueue == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetDeviceQueue parameter, VkQueue* pQueue, is null pointer");
        return;
    }
    if((*pQueue) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetDeviceQueue parameter, VkQueue* pQueue, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkGetDeviceQueue parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkGetDeviceQueue(
    VkDevice device,
    uint32_t queueNodeIndex,
    uint32_t queueIndex,
    VkQueue* pQueue)
{
    PreGetDeviceQueue(device, pQueue);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->GetDeviceQueue(device, queueNodeIndex, queueIndex, pQueue);

    PostGetDeviceQueue(device, queueNodeIndex, queueIndex, pQueue, result);

    return result;
}

void PreQueueSubmit(
    VkQueue queue,
    const VkCmdBuffer* pCmdBuffers)
{
    if(queue == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueSubmit parameter, VkQueue queue, is null pointer");
        return;
    }

    if(pCmdBuffers == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueSubmit parameter, const VkCmdBuffer* pCmdBuffers, is null pointer");
        return;
    }
    if((*pCmdBuffers) == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueSubmit parameter, const VkCmdBuffer* pCmdBuffers, is null pointer");
        return;
    }
}

void PostQueueSubmit(
    VkQueue queue,
    uint32_t cmdBufferCount,
    VkFence fence,
    VkResult result)
{
    if(queue == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueSubmit parameter, VkQueue queue, is null pointer");
        return;
    }


    if(fence == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueSubmit parameter, VkFence fence, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkQueueSubmit parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreQueueWaitIdle(
    VkQueue queue)
{
    if(queue == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueWaitIdle parameter, VkQueue queue, is null pointer");
        return;
    }
}

void PostQueueWaitIdle(
    VkQueue queue,
    VkResult result)
{
    if(queue == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueWaitIdle parameter, VkQueue queue, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkQueueWaitIdle parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueWaitIdle(
    VkQueue queue)
{
    PreQueueWaitIdle(queue);
    VkResult result = get_dispatch_table(pc_device_table_map, queue)->QueueWaitIdle(queue);

    PostQueueWaitIdle(queue, result);

    return result;
}

void PreDeviceWaitIdle(
    VkDevice device)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkDeviceWaitIdle parameter, VkDevice device, is null pointer");
        return;
    }
}

void PostDeviceWaitIdle(
    VkDevice device,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkDeviceWaitIdle parameter, VkDevice device, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkDeviceWaitIdle parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkDeviceWaitIdle(
    VkDevice device)
{
    PreDeviceWaitIdle(device);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->DeviceWaitIdle(device);

    PostDeviceWaitIdle(device, result);

    return result;
}

void PreAllocMemory(
    VkDevice device,
    const VkMemoryAllocInfo* pAllocInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkAllocMemory parameter, VkDevice device, is null pointer");
        return;
    }

    if(pAllocInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkAllocMemory parameter, const VkMemoryAllocInfo* pAllocInfo, is null pointer");
        return;
    }
    if(pAllocInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pAllocInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkAllocMemory parameter, VkStructureType pAllocInfo->sType, is unrecognized enumerator");
        return;
    }
}

void PostAllocMemory(
    VkDevice device,
    VkDeviceMemory* pMem,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkAllocMemory parameter, VkDevice device, is null pointer");
        return;
    }

    if(pMem == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkAllocMemory parameter, VkDeviceMemory* pMem, is null pointer");
        return;
    }
    if((*pMem) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkAllocMemory parameter, VkDeviceMemory* pMem, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkAllocMemory parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreFreeMemory(
    VkDevice device)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkFreeMemory parameter, VkDevice device, is null pointer");
        return;
    }
}

void PostFreeMemory(
    VkDevice device,
    VkDeviceMemory mem,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkFreeMemory parameter, VkDevice device, is null pointer");
        return;
    }

    if(mem == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkFreeMemory parameter, VkDeviceMemory mem, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkFreeMemory parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkFreeMemory(
    VkDevice device,
    VkDeviceMemory mem)
{
    PreFreeMemory(device);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->FreeMemory(device, mem);

    PostFreeMemory(device, mem, result);

    return result;
}

void PreMapMemory(
    VkDevice device)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkMapMemory parameter, VkDevice device, is null pointer");
        return;
    }
}

void PostMapMemory(
    VkDevice device,
    VkDeviceMemory mem,
    VkDeviceSize offset,
    VkDeviceSize size,
    VkMemoryMapFlags flags,
    void** ppData,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkMapMemory parameter, VkDevice device, is null pointer");
        return;
    }

    if(mem == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkMapMemory parameter, VkDeviceMemory mem, is null pointer");
        return;
    }




    if(ppData == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkMapMemory parameter, void** ppData, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkMapMemory parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkMapMemory(
    VkDevice device,
    VkDeviceMemory mem,
    VkDeviceSize offset,
    VkDeviceSize size,
    VkMemoryMapFlags flags,
    void** ppData)
{
    PreMapMemory(device);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->MapMemory(device, mem, offset, size, flags, ppData);

    PostMapMemory(device, mem, offset, size, flags, ppData, result);

    return result;
}

void PreUnmapMemory(
    VkDevice device)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUnmapMemory parameter, VkDevice device, is null pointer");
        return;
    }
}

void PostUnmapMemory(
    VkDevice device,
    VkDeviceMemory mem,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUnmapMemory parameter, VkDevice device, is null pointer");
        return;
    }

    if(mem == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUnmapMemory parameter, VkDeviceMemory mem, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkUnmapMemory parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkUnmapMemory(
    VkDevice device,
    VkDeviceMemory mem)
{
    PreUnmapMemory(device);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->UnmapMemory(device, mem);

    PostUnmapMemory(device, mem, result);

    return result;
}

void PreFlushMappedMemoryRanges(
    VkDevice device,
    const VkMappedMemoryRange* pMemRanges)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkFlushMappedMemoryRanges parameter, VkDevice device, is null pointer");
        return;
    }

    if(pMemRanges == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkFlushMappedMemoryRanges parameter, const VkMappedMemoryRange* pMemRanges, is null pointer");
        return;
    }
    if(pMemRanges->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pMemRanges->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkFlushMappedMemoryRanges parameter, VkStructureType pMemRanges->sType, is unrecognized enumerator");
        return;
    }
    if(pMemRanges->mem == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkFlushMappedMemoryRanges parameter, VkDeviceMemory pMemRanges->mem, is null pointer");
        return;
    }
}

void PostFlushMappedMemoryRanges(
    VkDevice device,
    uint32_t memRangeCount,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkFlushMappedMemoryRanges parameter, VkDevice device, is null pointer");
        return;
    }


    if(result != VK_SUCCESS)
    {
        std::string reason = "vkFlushMappedMemoryRanges parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreInvalidateMappedMemoryRanges(
    VkDevice device,
    const VkMappedMemoryRange* pMemRanges)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkInvalidateMappedMemoryRanges parameter, VkDevice device, is null pointer");
        return;
    }

    if(pMemRanges == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkInvalidateMappedMemoryRanges parameter, const VkMappedMemoryRange* pMemRanges, is null pointer");
        return;
    }
    if(pMemRanges->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pMemRanges->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkInvalidateMappedMemoryRanges parameter, VkStructureType pMemRanges->sType, is unrecognized enumerator");
        return;
    }
    if(pMemRanges->mem == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkInvalidateMappedMemoryRanges parameter, VkDeviceMemory pMemRanges->mem, is null pointer");
        return;
    }
}

void PostInvalidateMappedMemoryRanges(
    VkDevice device,
    uint32_t memRangeCount,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkInvalidateMappedMemoryRanges parameter, VkDevice device, is null pointer");
        return;
    }


    if(result != VK_SUCCESS)
    {
        std::string reason = "vkInvalidateMappedMemoryRanges parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreDestroyObject(
    VkDevice device)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkDestroyObject parameter, VkDevice device, is null pointer");
        return;
    }
}

void PostDestroyObject(
    VkDevice device,
    VkObjectType objType,
    VkObject object,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkDestroyObject parameter, VkDevice device, is null pointer");
        return;
    }

    if(objType < VK_OBJECT_TYPE_BEGIN_RANGE ||
        objType > VK_OBJECT_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkDestroyObject parameter, VkObjectType objType, is unrecognized enumerator");
        return;
    }

    if(object == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkDestroyObject parameter, VkObject object, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkDestroyObject parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyObject(
    VkDevice device,
    VkObjectType objType,
    VkObject object)
{
    PreDestroyObject(device);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->DestroyObject(device, objType, object);

    PostDestroyObject(device, objType, object, result);

    return result;
}

void PreBindObjectMemory(
    VkDevice device)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkBindObjectMemory parameter, VkDevice device, is null pointer");
        return;
    }
}

void PostBindObjectMemory(
    VkDevice device,
    VkObjectType objType,
    VkObject object,
    VkDeviceMemory mem,
    VkDeviceSize memOffset,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkBindObjectMemory parameter, VkDevice device, is null pointer");
        return;
    }

    if(objType < VK_OBJECT_TYPE_BEGIN_RANGE ||
        objType > VK_OBJECT_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkBindObjectMemory parameter, VkObjectType objType, is unrecognized enumerator");
        return;
    }

    if(object == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkBindObjectMemory parameter, VkObject object, is null pointer");
        return;
    }

    if(mem == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkBindObjectMemory parameter, VkDeviceMemory mem, is null pointer");
        return;
    }


    if(result != VK_SUCCESS)
    {
        std::string reason = "vkBindObjectMemory parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkBindObjectMemory(
    VkDevice device,
    VkObjectType objType,
    VkObject object,
    VkDeviceMemory mem,
    VkDeviceSize memOffset)
{
    PreBindObjectMemory(device);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->BindObjectMemory(device, objType, object, mem, memOffset);

    PostBindObjectMemory(device, objType, object, mem, memOffset, result);

    return result;
}

void PreGetObjectMemoryRequirements(
    VkDevice device)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetObjectMemoryRequirements parameter, VkDevice device, is null pointer");
        return;
    }
}

void PostGetObjectMemoryRequirements(
    VkDevice device,
    VkObjectType objType,
    VkObject object,
    VkMemoryRequirements* pMemoryRequirements,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetObjectMemoryRequirements parameter, VkDevice device, is null pointer");
        return;
    }

    if(objType < VK_OBJECT_TYPE_BEGIN_RANGE ||
        objType > VK_OBJECT_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetObjectMemoryRequirements parameter, VkObjectType objType, is unrecognized enumerator");
        return;
    }

    if(object == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetObjectMemoryRequirements parameter, VkObject object, is null pointer");
        return;
    }

    if(pMemoryRequirements == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetObjectMemoryRequirements parameter, VkMemoryRequirements* pMemoryRequirements, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkGetObjectMemoryRequirements parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkGetObjectMemoryRequirements(
    VkDevice device,
    VkObjectType objType,
    VkObject object,
    VkMemoryRequirements* pMemoryRequirements)
{
    PreGetObjectMemoryRequirements(device);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->GetObjectMemoryRequirements(device, objType, object, pMemoryRequirements);

    PostGetObjectMemoryRequirements(device, objType, object, pMemoryRequirements, result);

    return result;
}

void PreQueueBindSparseBufferMemory(
    VkQueue queue)
{
    if(queue == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueBindSparseBufferMemory parameter, VkQueue queue, is null pointer");
        return;
    }
}

void PostQueueBindSparseBufferMemory(
    VkQueue queue,
    VkBuffer buffer,
    VkDeviceSize rangeOffset,
    VkDeviceSize rangeSize,
    VkDeviceMemory mem,
    VkDeviceSize memOffset,
    VkResult result)
{
    if(queue == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueBindSparseBufferMemory parameter, VkQueue queue, is null pointer");
        return;
    }

    if(buffer == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueBindSparseBufferMemory parameter, VkBuffer buffer, is null pointer");
        return;
    }



    if(mem == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueBindSparseBufferMemory parameter, VkDeviceMemory mem, is null pointer");
        return;
    }


    if(result != VK_SUCCESS)
    {
        std::string reason = "vkQueueBindSparseBufferMemory parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueBindSparseBufferMemory(
    VkQueue queue,
    VkBuffer buffer,
    VkDeviceSize rangeOffset,
    VkDeviceSize rangeSize,
    VkDeviceMemory mem,
    VkDeviceSize memOffset)
{
    PreQueueBindSparseBufferMemory(queue);
    VkResult result = get_dispatch_table(pc_device_table_map, queue)->QueueBindSparseBufferMemory(queue, buffer, rangeOffset, rangeSize, mem, memOffset);

    PostQueueBindSparseBufferMemory(queue, buffer, rangeOffset, rangeSize, mem, memOffset, result);

    return result;
}

void PreQueueBindSparseImageMemory(
    VkQueue queue,
    const VkImageMemoryBindInfo* pBindInfo)
{
    if(queue == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueBindSparseImageMemory parameter, VkQueue queue, is null pointer");
        return;
    }

    if(pBindInfo == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueBindSparseImageMemory parameter, const VkImageMemoryBindInfo* pBindInfo, is null pointer");
        return;
    }
    if(pBindInfo->subresource.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pBindInfo->subresource.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueBindSparseImageMemory parameter, VkImageAspect pBindInfo->subresource.aspect, is unrecognized enumerator");
        return;
    }
}

void PostQueueBindSparseImageMemory(
    VkQueue queue,
    VkImage image,
    VkDeviceMemory mem,
    VkDeviceSize memOffset,
    VkResult result)
{
    if(queue == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueBindSparseImageMemory parameter, VkQueue queue, is null pointer");
        return;
    }

    if(image == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueBindSparseImageMemory parameter, VkImage image, is null pointer");
        return;
    }

    if(mem == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueBindSparseImageMemory parameter, VkDeviceMemory mem, is null pointer");
        return;
    }


    if(result != VK_SUCCESS)
    {
        std::string reason = "vkQueueBindSparseImageMemory parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueBindSparseImageMemory(
    VkQueue queue,
    VkImage image,
    const VkImageMemoryBindInfo* pBindInfo,
    VkDeviceMemory mem,
    VkDeviceSize memOffset)
{
    PreQueueBindSparseImageMemory(queue, pBindInfo);
    VkResult result = get_dispatch_table(pc_device_table_map, queue)->QueueBindSparseImageMemory(queue, image, pBindInfo, mem, memOffset);

    PostQueueBindSparseImageMemory(queue, image, mem, memOffset, result);

    return result;
}

void PreCreateFence(
    VkDevice device,
    const VkFenceCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateFence parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateFence parameter, const VkFenceCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateFence parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(!ValidateEnumerator((VkFenceCreateFlagBits)pCreateInfo->flags))
    {
        std::string reason = "vkCreateFence parameter, VkFenceCreateFlags pCreateInfo->flags, is " + EnumeratorString((VkFenceCreateFlagBits)pCreateInfo->flags);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

void PostCreateFence(
    VkDevice device,
    VkFence* pFence,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateFence parameter, VkDevice device, is null pointer");
        return;
    }

    if(pFence == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateFence parameter, VkFence* pFence, is null pointer");
        return;
    }
    if((*pFence) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateFence parameter, VkFence* pFence, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateFence parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreResetFences(
    VkDevice device,
    const VkFence* pFences)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkResetFences parameter, VkDevice device, is null pointer");
        return;
    }

    if(pFences == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkResetFences parameter, const VkFence* pFences, is null pointer");
        return;
    }
    if((*pFences) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkResetFences parameter, const VkFence* pFences, is null pointer");
        return;
    }
}

void PostResetFences(
    VkDevice device,
    uint32_t fenceCount,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkResetFences parameter, VkDevice device, is null pointer");
        return;
    }


    if(result != VK_SUCCESS)
    {
        std::string reason = "vkResetFences parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreGetFenceStatus(
    VkDevice device)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetFenceStatus parameter, VkDevice device, is null pointer");
        return;
    }
}

void PostGetFenceStatus(
    VkDevice device,
    VkFence fence,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetFenceStatus parameter, VkDevice device, is null pointer");
        return;
    }

    if(fence == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetFenceStatus parameter, VkFence fence, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkGetFenceStatus parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkGetFenceStatus(
    VkDevice device,
    VkFence fence)
{
    PreGetFenceStatus(device);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->GetFenceStatus(device, fence);

    PostGetFenceStatus(device, fence, result);

    return result;
}

void PreWaitForFences(
    VkDevice device,
    const VkFence* pFences)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkWaitForFences parameter, VkDevice device, is null pointer");
        return;
    }

    if(pFences == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkWaitForFences parameter, const VkFence* pFences, is null pointer");
        return;
    }
    if((*pFences) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkWaitForFences parameter, const VkFence* pFences, is null pointer");
        return;
    }
}

void PostWaitForFences(
    VkDevice device,
    uint32_t fenceCount,
    bool32_t waitAll,
    uint64_t timeout,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkWaitForFences parameter, VkDevice device, is null pointer");
        return;
    }




    if(result != VK_SUCCESS)
    {
        std::string reason = "vkWaitForFences parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkWaitForFences(
    VkDevice device,
    uint32_t fenceCount,
    const VkFence* pFences,
    bool32_t waitAll,
    uint64_t timeout)
{
    PreWaitForFences(device, pFences);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->WaitForFences(device, fenceCount, pFences, waitAll, timeout);

    PostWaitForFences(device, fenceCount, waitAll, timeout, result);

    return result;
}

void PreCreateSemaphore(
    VkDevice device,
    const VkSemaphoreCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSemaphore parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSemaphore parameter, const VkSemaphoreCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSemaphore parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
}

void PostCreateSemaphore(
    VkDevice device,
    VkSemaphore* pSemaphore,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSemaphore parameter, VkDevice device, is null pointer");
        return;
    }

    if(pSemaphore == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSemaphore parameter, VkSemaphore* pSemaphore, is null pointer");
        return;
    }
    if((*pSemaphore) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSemaphore parameter, VkSemaphore* pSemaphore, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateSemaphore parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreQueueSignalSemaphore(
    VkQueue queue)
{
    if(queue == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueSignalSemaphore parameter, VkQueue queue, is null pointer");
        return;
    }
}

void PostQueueSignalSemaphore(
    VkQueue queue,
    VkSemaphore semaphore,
    VkResult result)
{
    if(queue == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueSignalSemaphore parameter, VkQueue queue, is null pointer");
        return;
    }

    if(semaphore == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueSignalSemaphore parameter, VkSemaphore semaphore, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkQueueSignalSemaphore parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueSignalSemaphore(
    VkQueue queue,
    VkSemaphore semaphore)
{
    PreQueueSignalSemaphore(queue);
    VkResult result = get_dispatch_table(pc_device_table_map, queue)->QueueSignalSemaphore(queue, semaphore);

    PostQueueSignalSemaphore(queue, semaphore, result);

    return result;
}

void PreQueueWaitSemaphore(
    VkQueue queue)
{
    if(queue == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueWaitSemaphore parameter, VkQueue queue, is null pointer");
        return;
    }
}

void PostQueueWaitSemaphore(
    VkQueue queue,
    VkSemaphore semaphore,
    VkResult result)
{
    if(queue == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueWaitSemaphore parameter, VkQueue queue, is null pointer");
        return;
    }

    if(semaphore == nullptr)
    {
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkQueueWaitSemaphore parameter, VkSemaphore semaphore, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkQueueWaitSemaphore parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(queue), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueWaitSemaphore(
    VkQueue queue,
    VkSemaphore semaphore)
{
    PreQueueWaitSemaphore(queue);
    VkResult result = get_dispatch_table(pc_device_table_map, queue)->QueueWaitSemaphore(queue, semaphore);

    PostQueueWaitSemaphore(queue, semaphore, result);

    return result;
}

void PreCreateEvent(
    VkDevice device,
    const VkEventCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateEvent parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateEvent parameter, const VkEventCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateEvent parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
}

void PostCreateEvent(
    VkDevice device,
    VkEvent* pEvent,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateEvent parameter, VkDevice device, is null pointer");
        return;
    }

    if(pEvent == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateEvent parameter, VkEvent* pEvent, is null pointer");
        return;
    }
    if((*pEvent) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateEvent parameter, VkEvent* pEvent, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateEvent parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreGetEventStatus(
    VkDevice device)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetEventStatus parameter, VkDevice device, is null pointer");
        return;
    }
}

void PostGetEventStatus(
    VkDevice device,
    VkEvent event,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetEventStatus parameter, VkDevice device, is null pointer");
        return;
    }

    if(event == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetEventStatus parameter, VkEvent event, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkGetEventStatus parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkGetEventStatus(
    VkDevice device,
    VkEvent event)
{
    PreGetEventStatus(device);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->GetEventStatus(device, event);

    PostGetEventStatus(device, event, result);

    return result;
}

void PreSetEvent(
    VkDevice device)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkSetEvent parameter, VkDevice device, is null pointer");
        return;
    }
}

void PostSetEvent(
    VkDevice device,
    VkEvent event,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkSetEvent parameter, VkDevice device, is null pointer");
        return;
    }

    if(event == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkSetEvent parameter, VkEvent event, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkSetEvent parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkSetEvent(
    VkDevice device,
    VkEvent event)
{
    PreSetEvent(device);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->SetEvent(device, event);

    PostSetEvent(device, event, result);

    return result;
}

void PreResetEvent(
    VkDevice device)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkResetEvent parameter, VkDevice device, is null pointer");
        return;
    }
}

void PostResetEvent(
    VkDevice device,
    VkEvent event,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkResetEvent parameter, VkDevice device, is null pointer");
        return;
    }

    if(event == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkResetEvent parameter, VkEvent event, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkResetEvent parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkResetEvent(
    VkDevice device,
    VkEvent event)
{
    PreResetEvent(device);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->ResetEvent(device, event);

    PostResetEvent(device, event, result);

    return result;
}

void PreCreateQueryPool(
    VkDevice device,
    const VkQueryPoolCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateQueryPool parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateQueryPool parameter, const VkQueryPoolCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateQueryPool parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->queryType < VK_QUERY_TYPE_BEGIN_RANGE ||
        pCreateInfo->queryType > VK_QUERY_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateQueryPool parameter, VkQueryType pCreateInfo->queryType, is unrecognized enumerator");
        return;
    }
    if(!ValidateEnumerator((VkQueryPipelineStatisticFlagBits)pCreateInfo->pipelineStatistics))
    {
        std::string reason = "vkCreateQueryPool parameter, VkQueryPipelineStatisticFlags pCreateInfo->pipelineStatistics, is " + EnumeratorString((VkQueryPipelineStatisticFlagBits)pCreateInfo->pipelineStatistics);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

void PostCreateQueryPool(
    VkDevice device,
    VkQueryPool* pQueryPool,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateQueryPool parameter, VkDevice device, is null pointer");
        return;
    }

    if(pQueryPool == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateQueryPool parameter, VkQueryPool* pQueryPool, is null pointer");
        return;
    }
    if((*pQueryPool) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateQueryPool parameter, VkQueryPool* pQueryPool, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateQueryPool parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreGetQueryPoolResults(
    VkDevice device)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetQueryPoolResults parameter, VkDevice device, is null pointer");
        return;
    }
}

void PostGetQueryPoolResults(
    VkDevice device,
    VkQueryPool queryPool,
    uint32_t startQuery,
    uint32_t queryCount,
    size_t* pDataSize,
    void* pData,
    VkQueryResultFlags flags,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetQueryPoolResults parameter, VkDevice device, is null pointer");
        return;
    }

    if(queryPool == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetQueryPoolResults parameter, VkQueryPool queryPool, is null pointer");
        return;
    }



    if(pDataSize == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetQueryPoolResults parameter, size_t* pDataSize, is null pointer");
        return;
    }

    if(pData == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetQueryPoolResults parameter, void* pData, is null pointer");
        return;
    }

    if(!ValidateEnumerator((VkQueryResultFlagBits)flags))
    {
        std::string reason = "vkGetQueryPoolResults parameter, VkQueryResultFlags flags, is " + EnumeratorString((VkQueryResultFlagBits)flags);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkGetQueryPoolResults parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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
    PreGetQueryPoolResults(device);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->GetQueryPoolResults(device, queryPool, startQuery, queryCount, pDataSize, pData, flags);

    PostGetQueryPoolResults(device, queryPool, startQuery, queryCount, pDataSize, pData, flags, result);

    return result;
}

void PreCreateBuffer(
    VkDevice device,
    const VkBufferCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateBuffer parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateBuffer parameter, const VkBufferCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateBuffer parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(!ValidateEnumerator((VkBufferUsageFlagBits)pCreateInfo->usage))
    {
        std::string reason = "vkCreateBuffer parameter, VkBufferUsageFlags pCreateInfo->usage, is " + EnumeratorString((VkBufferUsageFlagBits)pCreateInfo->usage);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
    if(!ValidateEnumerator((VkBufferCreateFlagBits)pCreateInfo->flags))
    {
        std::string reason = "vkCreateBuffer parameter, VkBufferCreateFlags pCreateInfo->flags, is " + EnumeratorString((VkBufferCreateFlagBits)pCreateInfo->flags);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

void PostCreateBuffer(
    VkDevice device,
    VkBuffer* pBuffer,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateBuffer parameter, VkDevice device, is null pointer");
        return;
    }

    if(pBuffer == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateBuffer parameter, VkBuffer* pBuffer, is null pointer");
        return;
    }
    if((*pBuffer) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateBuffer parameter, VkBuffer* pBuffer, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateBuffer parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreCreateBufferView(
    VkDevice device,
    const VkBufferViewCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateBufferView parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateBufferView parameter, const VkBufferViewCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateBufferView parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->buffer == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateBufferView parameter, VkBuffer pCreateInfo->buffer, is null pointer");
        return;
    }
    if(pCreateInfo->viewType < VK_BUFFER_VIEW_TYPE_BEGIN_RANGE ||
        pCreateInfo->viewType > VK_BUFFER_VIEW_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateBufferView parameter, VkBufferViewType pCreateInfo->viewType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfo->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateBufferView parameter, VkFormat pCreateInfo->format, is unrecognized enumerator");
        return;
    }
}

void PostCreateBufferView(
    VkDevice device,
    VkBufferView* pView,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateBufferView parameter, VkDevice device, is null pointer");
        return;
    }

    if(pView == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateBufferView parameter, VkBufferView* pView, is null pointer");
        return;
    }
    if((*pView) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateBufferView parameter, VkBufferView* pView, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateBufferView parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreCreateImage(
    VkDevice device,
    const VkImageCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImage parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImage parameter, const VkImageCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImage parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->imageType < VK_IMAGE_TYPE_BEGIN_RANGE ||
        pCreateInfo->imageType > VK_IMAGE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImage parameter, VkImageType pCreateInfo->imageType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfo->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImage parameter, VkFormat pCreateInfo->format, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->tiling < VK_IMAGE_TILING_BEGIN_RANGE ||
        pCreateInfo->tiling > VK_IMAGE_TILING_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImage parameter, VkImageTiling pCreateInfo->tiling, is unrecognized enumerator");
        return;
    }
    if(!ValidateEnumerator((VkImageUsageFlagBits)pCreateInfo->usage))
    {
        std::string reason = "vkCreateImage parameter, VkImageUsageFlags pCreateInfo->usage, is " + EnumeratorString((VkImageUsageFlagBits)pCreateInfo->usage);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
    if(!ValidateEnumerator((VkImageCreateFlagBits)pCreateInfo->flags))
    {
        std::string reason = "vkCreateImage parameter, VkImageCreateFlags pCreateInfo->flags, is " + EnumeratorString((VkImageCreateFlagBits)pCreateInfo->flags);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

void PostCreateImage(
    VkDevice device,
    VkImage* pImage,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImage parameter, VkDevice device, is null pointer");
        return;
    }

    if(pImage == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImage parameter, VkImage* pImage, is null pointer");
        return;
    }
    if((*pImage) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImage parameter, VkImage* pImage, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateImage parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreGetImageSubresourceLayout(
    VkDevice device,
    const VkImageSubresource* pSubresource)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetImageSubresourceLayout parameter, VkDevice device, is null pointer");
        return;
    }

    if(pSubresource == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetImageSubresourceLayout parameter, const VkImageSubresource* pSubresource, is null pointer");
        return;
    }
    if(pSubresource->aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pSubresource->aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetImageSubresourceLayout parameter, VkImageAspect pSubresource->aspect, is unrecognized enumerator");
        return;
    }
}

void PostGetImageSubresourceLayout(
    VkDevice device,
    VkImage image,
    VkSubresourceLayout* pLayout,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetImageSubresourceLayout parameter, VkDevice device, is null pointer");
        return;
    }

    if(image == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetImageSubresourceLayout parameter, VkImage image, is null pointer");
        return;
    }

    if(pLayout == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkGetImageSubresourceLayout parameter, VkSubresourceLayout* pLayout, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkGetImageSubresourceLayout parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreCreateImageView(
    VkDevice device,
    const VkImageViewCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, const VkImageViewCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->image == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkImage pCreateInfo->image, is null pointer");
        return;
    }
    if(pCreateInfo->viewType < VK_IMAGE_VIEW_TYPE_BEGIN_RANGE ||
        pCreateInfo->viewType > VK_IMAGE_VIEW_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkImageViewType pCreateInfo->viewType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfo->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkFormat pCreateInfo->format, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->channels.r < VK_CHANNEL_SWIZZLE_BEGIN_RANGE ||
        pCreateInfo->channels.r > VK_CHANNEL_SWIZZLE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkChannelSwizzle pCreateInfo->channels.r, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->channels.g < VK_CHANNEL_SWIZZLE_BEGIN_RANGE ||
        pCreateInfo->channels.g > VK_CHANNEL_SWIZZLE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkChannelSwizzle pCreateInfo->channels.g, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->channels.b < VK_CHANNEL_SWIZZLE_BEGIN_RANGE ||
        pCreateInfo->channels.b > VK_CHANNEL_SWIZZLE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkChannelSwizzle pCreateInfo->channels.b, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->channels.a < VK_CHANNEL_SWIZZLE_BEGIN_RANGE ||
        pCreateInfo->channels.a > VK_CHANNEL_SWIZZLE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkChannelSwizzle pCreateInfo->channels.a, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->subresourceRange.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pCreateInfo->subresourceRange.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkImageAspect pCreateInfo->subresourceRange.aspect, is unrecognized enumerator");
        return;
    }
}

void PostCreateImageView(
    VkDevice device,
    VkImageView* pView,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkDevice device, is null pointer");
        return;
    }

    if(pView == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkImageView* pView, is null pointer");
        return;
    }
    if((*pView) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateImageView parameter, VkImageView* pView, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateImageView parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreCreateColorAttachmentView(
    VkDevice device,
    const VkColorAttachmentViewCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateColorAttachmentView parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateColorAttachmentView parameter, const VkColorAttachmentViewCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateColorAttachmentView parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->image == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateColorAttachmentView parameter, VkImage pCreateInfo->image, is null pointer");
        return;
    }
    if(pCreateInfo->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfo->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateColorAttachmentView parameter, VkFormat pCreateInfo->format, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->msaaResolveImage == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateColorAttachmentView parameter, VkImage pCreateInfo->msaaResolveImage, is null pointer");
        return;
    }
    if(pCreateInfo->msaaResolveSubResource.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pCreateInfo->msaaResolveSubResource.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateColorAttachmentView parameter, VkImageAspect pCreateInfo->msaaResolveSubResource.aspect, is unrecognized enumerator");
        return;
    }
}

void PostCreateColorAttachmentView(
    VkDevice device,
    VkColorAttachmentView* pView,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateColorAttachmentView parameter, VkDevice device, is null pointer");
        return;
    }

    if(pView == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateColorAttachmentView parameter, VkColorAttachmentView* pView, is null pointer");
        return;
    }
    if((*pView) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateColorAttachmentView parameter, VkColorAttachmentView* pView, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateColorAttachmentView parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateColorAttachmentView(
    VkDevice device,
    const VkColorAttachmentViewCreateInfo* pCreateInfo,
    VkColorAttachmentView* pView)
{
    PreCreateColorAttachmentView(device, pCreateInfo);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateColorAttachmentView(device, pCreateInfo, pView);

    PostCreateColorAttachmentView(device, pView, result);

    return result;
}

void PreCreateDepthStencilView(
    VkDevice device,
    const VkDepthStencilViewCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDepthStencilView parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDepthStencilView parameter, const VkDepthStencilViewCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDepthStencilView parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->image == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDepthStencilView parameter, VkImage pCreateInfo->image, is null pointer");
        return;
    }
    if(!ValidateEnumerator((VkDepthStencilViewCreateFlagBits)pCreateInfo->flags))
    {
        std::string reason = "vkCreateDepthStencilView parameter, VkDepthStencilViewCreateFlags pCreateInfo->flags, is " + EnumeratorString((VkDepthStencilViewCreateFlagBits)pCreateInfo->flags);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

void PostCreateDepthStencilView(
    VkDevice device,
    VkDepthStencilView* pView,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDepthStencilView parameter, VkDevice device, is null pointer");
        return;
    }

    if(pView == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDepthStencilView parameter, VkDepthStencilView* pView, is null pointer");
        return;
    }
    if((*pView) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDepthStencilView parameter, VkDepthStencilView* pView, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateDepthStencilView parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDepthStencilView(
    VkDevice device,
    const VkDepthStencilViewCreateInfo* pCreateInfo,
    VkDepthStencilView* pView)
{
    PreCreateDepthStencilView(device, pCreateInfo);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateDepthStencilView(device, pCreateInfo, pView);

    PostCreateDepthStencilView(device, pView, result);

    return result;
}

void PreCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateShaderModule parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateShaderModule parameter, const VkShaderCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateShaderModule parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pCode == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateShaderModule parameter, const void* pCreateInfo->pCode, is null pointer");
        return;
    }
    if(pCreateInfo->codeSize == 0)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateShaderModule parameter, size_t pCreateInfo->codeSize, is zero");
        return;
    }
}

void PostCreateShaderModule(
    VkDevice device,
    VkShaderModule* pShaderModule,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateShaderModule parameter, VkDevice device, is null pointer");
        return;
    }

    if(pShaderModule == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateShaderModule parameter, VkShader* pShader, is null pointer");
        return;
    }
    if((*pShaderModule) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateShaderModule parameter, VkShader* pShader, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateShaderModule parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    VkShaderModule* pShaderModule)
{
    PreCreateShaderModule(device, pCreateInfo);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateShaderModule(device, pCreateInfo, pShaderModule);

    PostCreateShaderModule(device, pShaderModule, result);

    return result;
}

void PreCreateShader(
    VkDevice device,
    const VkShaderCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateShader parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateShader parameter, const VkShaderCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateShader parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->module == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateShader parameter, VkShaderModule pCreateInfo->module, is null pointer");
        return;
    }
    if(pCreateInfo->pName == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateShader parameter, const char* pCreateInfo->name, is null pointer");
        return;
    }
}

void PostCreateShader(
    VkDevice device,
    VkShader* pShader,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateShader parameter, VkDevice device, is null pointer");
        return;
    }

    if(pShader == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateShader parameter, VkShader* pShader, is null pointer");
        return;
    }
    if((*pShader) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateShader parameter, VkShader* pShader, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateShader parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreCreateGraphicsPipeline(
    VkDevice device,
    const VkGraphicsPipelineCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const VkGraphicsPipelineCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pStages == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const VkPipelineShaderStageCreateInfo* pCreateInfo->pStages, is null pointer");
        return;
    }
    if(pCreateInfo->pStages->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->pStages->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkStructureType pCreateInfo->pStages->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pStages->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const void* pCreateInfo->pStages->pNext, is null pointer");
        return;
    }
    if(pCreateInfo->pStages->stage < VK_SHADER_STAGE_BEGIN_RANGE ||
        pCreateInfo->pStages->stage > VK_SHADER_STAGE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkShaderStage pCreateInfo->pStages->stage, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pStages->shader == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkShader pCreateInfo->pStages->shader, is null pointer");
        return;
    }
    if(pCreateInfo->pStages->pLinkConstBufferInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const VkLinkConstBuffer* pCreateInfo->pStages->pLinkConstBufferInfo, is null pointer");
        return;
    }
    if(pCreateInfo->pStages->pLinkConstBufferInfo->pBufferData == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const void* pCreateInfo->pStages->pLinkConstBufferInfo->pBufferData, is null pointer");
        return;
    }
    if(pCreateInfo->pStages->pSpecializationInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const VkSpecializationInfo* pCreateInfo->pStages->pSpecializationInfo, is null pointer");
        return;
    }
    if(pCreateInfo->pStages->pSpecializationInfo->pMap == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const VkSpecializationMapEntry* pCreateInfo->pStages->pSpecializationInfo->pMap, is null pointer");
        return;
    }
    if(pCreateInfo->pStages->pSpecializationInfo->pData == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const void* pCreateInfo->pStages->pSpecializationInfo->pData, is null pointer");
        return;
    }
    if(pCreateInfo->pVertexInputState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const VkPipelineVertexInputStateCreateInfo* pCreateInfo->pVertexInputState, is null pointer");
        return;
    }
    if(pCreateInfo->pVertexInputState->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->pVertexInputState->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkStructureType pCreateInfo->pVertexInputState->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pVertexInputState->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const void* pCreateInfo->pVertexInputState->pNext, is null pointer");
        return;
    }
    if(pCreateInfo->pVertexInputState->pVertexBindingDescriptions == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const VkVertexInputBindingDescription* pCreateInfo->pVertexInputState->pVertexBindingDescriptions, is null pointer");
        return;
    }
    if(pCreateInfo->pVertexInputState->pVertexBindingDescriptions->stepRate < VK_VERTEX_INPUT_STEP_RATE_BEGIN_RANGE ||
        pCreateInfo->pVertexInputState->pVertexBindingDescriptions->stepRate > VK_VERTEX_INPUT_STEP_RATE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkVertexInputStepRate pCreateInfo->pVertexInputState->pVertexBindingDescriptions->stepRate, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pVertexInputState->pVertexAttributeDescriptions == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const VkVertexInputAttributeDescription* pCreateInfo->pVertexInputState->pVertexAttributeDescriptions, is null pointer");
        return;
    }
    if(pCreateInfo->pVertexInputState->pVertexAttributeDescriptions->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfo->pVertexInputState->pVertexAttributeDescriptions->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkFormat pCreateInfo->pVertexInputState->pVertexAttributeDescriptions->format, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pIaState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const VkPipelineIaStateCreateInfo* pCreateInfo->pIaState, is null pointer");
        return;
    }
    if(pCreateInfo->pIaState->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->pIaState->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkStructureType pCreateInfo->pIaState->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pIaState->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const void* pCreateInfo->pIaState->pNext, is null pointer");
        return;
    }
    if(pCreateInfo->pIaState->topology < VK_PRIMITIVE_TOPOLOGY_BEGIN_RANGE ||
        pCreateInfo->pIaState->topology > VK_PRIMITIVE_TOPOLOGY_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkPrimitiveTopology pCreateInfo->pIaState->topology, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pTessState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const VkPipelineTessStateCreateInfo* pCreateInfo->pTessState, is null pointer");
        return;
    }
    if(pCreateInfo->pTessState->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->pTessState->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkStructureType pCreateInfo->pTessState->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pTessState->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const void* pCreateInfo->pTessState->pNext, is null pointer");
        return;
    }
    if(pCreateInfo->pVpState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const VkPipelineVpStateCreateInfo* pCreateInfo->pVpState, is null pointer");
        return;
    }
    if(pCreateInfo->pVpState->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->pVpState->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkStructureType pCreateInfo->pVpState->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pVpState->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const void* pCreateInfo->pVpState->pNext, is null pointer");
        return;
    }
    if(pCreateInfo->pVpState->clipOrigin < VK_COORDINATE_ORIGIN_BEGIN_RANGE ||
        pCreateInfo->pVpState->clipOrigin > VK_COORDINATE_ORIGIN_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkCoordinateOrigin pCreateInfo->pVpState->clipOrigin, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pVpState->depthMode < VK_DEPTH_MODE_BEGIN_RANGE ||
        pCreateInfo->pVpState->depthMode > VK_DEPTH_MODE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkDepthMode pCreateInfo->pVpState->depthMode, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pRsState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const VkPipelineRsStateCreateInfo* pCreateInfo->pRsState, is null pointer");
        return;
    }
    if(pCreateInfo->pRsState->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->pRsState->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkStructureType pCreateInfo->pRsState->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pRsState->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const void* pCreateInfo->pRsState->pNext, is null pointer");
        return;
    }
    if(pCreateInfo->pRsState->pointOrigin < VK_COORDINATE_ORIGIN_BEGIN_RANGE ||
        pCreateInfo->pRsState->pointOrigin > VK_COORDINATE_ORIGIN_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkCoordinateOrigin pCreateInfo->pRsState->pointOrigin, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pRsState->provokingVertex < VK_PROVOKING_VERTEX_BEGIN_RANGE ||
        pCreateInfo->pRsState->provokingVertex > VK_PROVOKING_VERTEX_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkProvokingVertex pCreateInfo->pRsState->provokingVertex, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pRsState->fillMode < VK_FILL_MODE_BEGIN_RANGE ||
        pCreateInfo->pRsState->fillMode > VK_FILL_MODE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkFillMode pCreateInfo->pRsState->fillMode, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pRsState->cullMode < VK_CULL_MODE_BEGIN_RANGE ||
        pCreateInfo->pRsState->cullMode > VK_CULL_MODE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkCullMode pCreateInfo->pRsState->cullMode, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pRsState->frontFace < VK_FRONT_FACE_BEGIN_RANGE ||
        pCreateInfo->pRsState->frontFace > VK_FRONT_FACE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkFrontFace pCreateInfo->pRsState->frontFace, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pMsState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const VkPipelineMsStateCreateInfo* pCreateInfo->pMsState, is null pointer");
        return;
    }
    if(pCreateInfo->pMsState->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->pMsState->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkStructureType pCreateInfo->pMsState->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pMsState->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const void* pCreateInfo->pMsState->pNext, is null pointer");
        return;
    }
    if(pCreateInfo->pDsState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const VkPipelineDsStateCreateInfo* pCreateInfo->pDsState, is null pointer");
        return;
    }
    if(pCreateInfo->pDsState->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->pDsState->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkStructureType pCreateInfo->pDsState->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const void* pCreateInfo->pDsState->pNext, is null pointer");
        return;
    }
    if(pCreateInfo->pDsState->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfo->pDsState->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkFormat pCreateInfo->pDsState->format, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->depthCompareOp < VK_COMPARE_OP_BEGIN_RANGE ||
        pCreateInfo->pDsState->depthCompareOp > VK_COMPARE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkCompareOp pCreateInfo->pDsState->depthCompareOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->front.stencilFailOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfo->pDsState->front.stencilFailOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkStencilOp pCreateInfo->pDsState->front.stencilFailOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->front.stencilPassOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfo->pDsState->front.stencilPassOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkStencilOp pCreateInfo->pDsState->front.stencilPassOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->front.stencilDepthFailOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfo->pDsState->front.stencilDepthFailOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkStencilOp pCreateInfo->pDsState->front.stencilDepthFailOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->front.stencilCompareOp < VK_COMPARE_OP_BEGIN_RANGE ||
        pCreateInfo->pDsState->front.stencilCompareOp > VK_COMPARE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkCompareOp pCreateInfo->pDsState->front.stencilCompareOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->back.stencilFailOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfo->pDsState->back.stencilFailOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkStencilOp pCreateInfo->pDsState->back.stencilFailOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->back.stencilPassOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfo->pDsState->back.stencilPassOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkStencilOp pCreateInfo->pDsState->back.stencilPassOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->back.stencilDepthFailOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfo->pDsState->back.stencilDepthFailOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkStencilOp pCreateInfo->pDsState->back.stencilDepthFailOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->back.stencilCompareOp < VK_COMPARE_OP_BEGIN_RANGE ||
        pCreateInfo->pDsState->back.stencilCompareOp > VK_COMPARE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkCompareOp pCreateInfo->pDsState->back.stencilCompareOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pCbState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const VkPipelineCbStateCreateInfo* pCreateInfo->pCbState, is null pointer");
        return;
    }
    if(pCreateInfo->pCbState->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->pCbState->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkStructureType pCreateInfo->pCbState->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pCbState->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const void* pCreateInfo->pCbState->pNext, is null pointer");
        return;
    }
    if(pCreateInfo->pCbState->logicOp < VK_LOGIC_OP_BEGIN_RANGE ||
        pCreateInfo->pCbState->logicOp > VK_LOGIC_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkLogicOp pCreateInfo->pCbState->logicOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pCbState->pAttachments == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, const VkPipelineCbAttachmentState* pCreateInfo->pCbState->pAttachments, is null pointer");
        return;
    }
    if(pCreateInfo->pCbState->pAttachments->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfo->pCbState->pAttachments->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkFormat pCreateInfo->pCbState->pAttachments->format, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pCbState->pAttachments->srcBlendColor < VK_BLEND_BEGIN_RANGE ||
        pCreateInfo->pCbState->pAttachments->srcBlendColor > VK_BLEND_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkBlend pCreateInfo->pCbState->pAttachments->srcBlendColor, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pCbState->pAttachments->destBlendColor < VK_BLEND_BEGIN_RANGE ||
        pCreateInfo->pCbState->pAttachments->destBlendColor > VK_BLEND_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkBlend pCreateInfo->pCbState->pAttachments->destBlendColor, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pCbState->pAttachments->blendOpColor < VK_BLEND_OP_BEGIN_RANGE ||
        pCreateInfo->pCbState->pAttachments->blendOpColor > VK_BLEND_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkBlendOp pCreateInfo->pCbState->pAttachments->blendOpColor, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pCbState->pAttachments->srcBlendAlpha < VK_BLEND_BEGIN_RANGE ||
        pCreateInfo->pCbState->pAttachments->srcBlendAlpha > VK_BLEND_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkBlend pCreateInfo->pCbState->pAttachments->srcBlendAlpha, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pCbState->pAttachments->destBlendAlpha < VK_BLEND_BEGIN_RANGE ||
        pCreateInfo->pCbState->pAttachments->destBlendAlpha > VK_BLEND_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkBlend pCreateInfo->pCbState->pAttachments->destBlendAlpha, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pCbState->pAttachments->blendOpAlpha < VK_BLEND_OP_BEGIN_RANGE ||
        pCreateInfo->pCbState->pAttachments->blendOpAlpha > VK_BLEND_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkBlendOp pCreateInfo->pCbState->pAttachments->blendOpAlpha, is unrecognized enumerator");
        return;
    }
    if(!ValidateEnumerator((VkChannelFlagBits)pCreateInfo->pCbState->pAttachments->channelWriteMask))
    {
        std::string reason = "vkCreateGraphicsPipeline parameter, VkChannelFlags pCreateInfo->pCbState->pAttachments->channelWriteMask, is " + EnumeratorString((VkChannelFlagBits)pCreateInfo->pCbState->pAttachments->channelWriteMask);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
    if(!ValidateEnumerator((VkPipelineCreateFlagBits)pCreateInfo->flags))
    {
        std::string reason = "vkCreateGraphicsPipeline parameter, VkPipelineCreateFlags pCreateInfo->flags, is " + EnumeratorString((VkPipelineCreateFlagBits)pCreateInfo->flags);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
    if(pCreateInfo->layout == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkPipelineLayout pCreateInfo->layout, is null pointer");
        return;
    }
}

void PostCreateGraphicsPipeline(
    VkDevice device,
    VkPipeline* pPipeline,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkDevice device, is null pointer");
        return;
    }

    if(pPipeline == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkPipeline* pPipeline, is null pointer");
        return;
    }
    if((*pPipeline) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipeline parameter, VkPipeline* pPipeline, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateGraphicsPipeline parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateGraphicsPipeline(
    VkDevice device,
    const VkGraphicsPipelineCreateInfo* pCreateInfo,
    VkPipeline* pPipeline)
{
    PreCreateGraphicsPipeline(device, pCreateInfo);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateGraphicsPipeline(device, pCreateInfo, pPipeline);

    PostCreateGraphicsPipeline(device, pPipeline, result);

    return result;
}

void PreCreateGraphicsPipelineDerivative(
    VkDevice device,
    const VkGraphicsPipelineCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const VkGraphicsPipelineCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pStages == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const VkPipelineShaderStageCreateInfo* pCreateInfo->pStages, is null pointer");
        return;
    }
    if(pCreateInfo->pStages->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->pStages->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkStructureType pCreateInfo->pStages->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pStages->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const void* pCreateInfo->pStages->pNext, is null pointer");
        return;
    }
    if(pCreateInfo->pStages->stage < VK_SHADER_STAGE_BEGIN_RANGE ||
        pCreateInfo->pStages->stage > VK_SHADER_STAGE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkShaderStage pCreateInfo->pStages->stage, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pStages->shader == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkShader pCreateInfo->pStages->shader, is null pointer");
        return;
    }
    if(pCreateInfo->pStages->pLinkConstBufferInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const VkLinkConstBuffer* pCreateInfo->pStages->pLinkConstBufferInfo, is null pointer");
        return;
    }
    if(pCreateInfo->pStages->pLinkConstBufferInfo->pBufferData == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const void* pCreateInfo->pStages->pLinkConstBufferInfo->pBufferData, is null pointer");
        return;
    }
    if(pCreateInfo->pStages->pSpecializationInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const VkSpecializationInfo* pCreateInfo->pStages->pSpecializationInfo, is null pointer");
        return;
    }
    if(pCreateInfo->pStages->pSpecializationInfo->pMap == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const VkSpecializationMapEntry* pCreateInfo->pStages->pSpecializationInfo->pMap, is null pointer");
        return;
    }
    if(pCreateInfo->pStages->pSpecializationInfo->pData == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const void* pCreateInfo->pStages->pSpecializationInfo->pData, is null pointer");
        return;
    }
    if(pCreateInfo->pVertexInputState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const VkPipelineVertexInputStateCreateInfo* pCreateInfo->pVertexInputState, is null pointer");
        return;
    }
    if(pCreateInfo->pVertexInputState->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->pVertexInputState->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkStructureType pCreateInfo->pVertexInputState->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pVertexInputState->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const void* pCreateInfo->pVertexInputState->pNext, is null pointer");
        return;
    }
    if(pCreateInfo->pVertexInputState->pVertexBindingDescriptions == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const VkVertexInputBindingDescription* pCreateInfo->pVertexInputState->pVertexBindingDescriptions, is null pointer");
        return;
    }
    if(pCreateInfo->pVertexInputState->pVertexBindingDescriptions->stepRate < VK_VERTEX_INPUT_STEP_RATE_BEGIN_RANGE ||
        pCreateInfo->pVertexInputState->pVertexBindingDescriptions->stepRate > VK_VERTEX_INPUT_STEP_RATE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkVertexInputStepRate pCreateInfo->pVertexInputState->pVertexBindingDescriptions->stepRate, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pVertexInputState->pVertexAttributeDescriptions == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const VkVertexInputAttributeDescription* pCreateInfo->pVertexInputState->pVertexAttributeDescriptions, is null pointer");
        return;
    }
    if(pCreateInfo->pVertexInputState->pVertexAttributeDescriptions->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfo->pVertexInputState->pVertexAttributeDescriptions->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkFormat pCreateInfo->pVertexInputState->pVertexAttributeDescriptions->format, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pIaState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const VkPipelineIaStateCreateInfo* pCreateInfo->pIaState, is null pointer");
        return;
    }
    if(pCreateInfo->pIaState->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->pIaState->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkStructureType pCreateInfo->pIaState->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pIaState->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const void* pCreateInfo->pIaState->pNext, is null pointer");
        return;
    }
    if(pCreateInfo->pIaState->topology < VK_PRIMITIVE_TOPOLOGY_BEGIN_RANGE ||
        pCreateInfo->pIaState->topology > VK_PRIMITIVE_TOPOLOGY_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkPrimitiveTopology pCreateInfo->pIaState->topology, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pTessState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const VkPipelineTessStateCreateInfo* pCreateInfo->pTessState, is null pointer");
        return;
    }
    if(pCreateInfo->pTessState->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->pTessState->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkStructureType pCreateInfo->pTessState->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pTessState->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const void* pCreateInfo->pTessState->pNext, is null pointer");
        return;
    }
    if(pCreateInfo->pVpState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const VkPipelineVpStateCreateInfo* pCreateInfo->pVpState, is null pointer");
        return;
    }
    if(pCreateInfo->pVpState->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->pVpState->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkStructureType pCreateInfo->pVpState->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pVpState->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const void* pCreateInfo->pVpState->pNext, is null pointer");
        return;
    }
    if(pCreateInfo->pVpState->clipOrigin < VK_COORDINATE_ORIGIN_BEGIN_RANGE ||
        pCreateInfo->pVpState->clipOrigin > VK_COORDINATE_ORIGIN_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkCoordinateOrigin pCreateInfo->pVpState->clipOrigin, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pVpState->depthMode < VK_DEPTH_MODE_BEGIN_RANGE ||
        pCreateInfo->pVpState->depthMode > VK_DEPTH_MODE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkDepthMode pCreateInfo->pVpState->depthMode, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pRsState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const VkPipelineRsStateCreateInfo* pCreateInfo->pRsState, is null pointer");
        return;
    }
    if(pCreateInfo->pRsState->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->pRsState->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkStructureType pCreateInfo->pRsState->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pRsState->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const void* pCreateInfo->pRsState->pNext, is null pointer");
        return;
    }
    if(pCreateInfo->pRsState->pointOrigin < VK_COORDINATE_ORIGIN_BEGIN_RANGE ||
        pCreateInfo->pRsState->pointOrigin > VK_COORDINATE_ORIGIN_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkCoordinateOrigin pCreateInfo->pRsState->pointOrigin, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pRsState->provokingVertex < VK_PROVOKING_VERTEX_BEGIN_RANGE ||
        pCreateInfo->pRsState->provokingVertex > VK_PROVOKING_VERTEX_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkProvokingVertex pCreateInfo->pRsState->provokingVertex, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pRsState->fillMode < VK_FILL_MODE_BEGIN_RANGE ||
        pCreateInfo->pRsState->fillMode > VK_FILL_MODE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkFillMode pCreateInfo->pRsState->fillMode, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pRsState->cullMode < VK_CULL_MODE_BEGIN_RANGE ||
        pCreateInfo->pRsState->cullMode > VK_CULL_MODE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkCullMode pCreateInfo->pRsState->cullMode, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pRsState->frontFace < VK_FRONT_FACE_BEGIN_RANGE ||
        pCreateInfo->pRsState->frontFace > VK_FRONT_FACE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkFrontFace pCreateInfo->pRsState->frontFace, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pMsState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const VkPipelineMsStateCreateInfo* pCreateInfo->pMsState, is null pointer");
        return;
    }
    if(pCreateInfo->pMsState->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->pMsState->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkStructureType pCreateInfo->pMsState->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pMsState->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const void* pCreateInfo->pMsState->pNext, is null pointer");
        return;
    }
    if(pCreateInfo->pDsState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const VkPipelineDsStateCreateInfo* pCreateInfo->pDsState, is null pointer");
        return;
    }
    if(pCreateInfo->pDsState->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->pDsState->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkStructureType pCreateInfo->pDsState->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const void* pCreateInfo->pDsState->pNext, is null pointer");
        return;
    }
    if(pCreateInfo->pDsState->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfo->pDsState->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkFormat pCreateInfo->pDsState->format, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->depthCompareOp < VK_COMPARE_OP_BEGIN_RANGE ||
        pCreateInfo->pDsState->depthCompareOp > VK_COMPARE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkCompareOp pCreateInfo->pDsState->depthCompareOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->front.stencilFailOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfo->pDsState->front.stencilFailOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkStencilOp pCreateInfo->pDsState->front.stencilFailOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->front.stencilPassOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfo->pDsState->front.stencilPassOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkStencilOp pCreateInfo->pDsState->front.stencilPassOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->front.stencilDepthFailOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfo->pDsState->front.stencilDepthFailOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkStencilOp pCreateInfo->pDsState->front.stencilDepthFailOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->front.stencilCompareOp < VK_COMPARE_OP_BEGIN_RANGE ||
        pCreateInfo->pDsState->front.stencilCompareOp > VK_COMPARE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkCompareOp pCreateInfo->pDsState->front.stencilCompareOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->back.stencilFailOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfo->pDsState->back.stencilFailOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkStencilOp pCreateInfo->pDsState->back.stencilFailOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->back.stencilPassOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfo->pDsState->back.stencilPassOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkStencilOp pCreateInfo->pDsState->back.stencilPassOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->back.stencilDepthFailOp < VK_STENCIL_OP_BEGIN_RANGE ||
        pCreateInfo->pDsState->back.stencilDepthFailOp > VK_STENCIL_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkStencilOp pCreateInfo->pDsState->back.stencilDepthFailOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDsState->back.stencilCompareOp < VK_COMPARE_OP_BEGIN_RANGE ||
        pCreateInfo->pDsState->back.stencilCompareOp > VK_COMPARE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkCompareOp pCreateInfo->pDsState->back.stencilCompareOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pCbState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const VkPipelineCbStateCreateInfo* pCreateInfo->pCbState, is null pointer");
        return;
    }
    if(pCreateInfo->pCbState->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->pCbState->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkStructureType pCreateInfo->pCbState->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pCbState->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const void* pCreateInfo->pCbState->pNext, is null pointer");
        return;
    }
    if(pCreateInfo->pCbState->logicOp < VK_LOGIC_OP_BEGIN_RANGE ||
        pCreateInfo->pCbState->logicOp > VK_LOGIC_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkLogicOp pCreateInfo->pCbState->logicOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pCbState->pAttachments == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, const VkPipelineCbAttachmentState* pCreateInfo->pCbState->pAttachments, is null pointer");
        return;
    }
    if(pCreateInfo->pCbState->pAttachments->format < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfo->pCbState->pAttachments->format > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkFormat pCreateInfo->pCbState->pAttachments->format, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pCbState->pAttachments->srcBlendColor < VK_BLEND_BEGIN_RANGE ||
        pCreateInfo->pCbState->pAttachments->srcBlendColor > VK_BLEND_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkBlend pCreateInfo->pCbState->pAttachments->srcBlendColor, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pCbState->pAttachments->destBlendColor < VK_BLEND_BEGIN_RANGE ||
        pCreateInfo->pCbState->pAttachments->destBlendColor > VK_BLEND_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkBlend pCreateInfo->pCbState->pAttachments->destBlendColor, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pCbState->pAttachments->blendOpColor < VK_BLEND_OP_BEGIN_RANGE ||
        pCreateInfo->pCbState->pAttachments->blendOpColor > VK_BLEND_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkBlendOp pCreateInfo->pCbState->pAttachments->blendOpColor, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pCbState->pAttachments->srcBlendAlpha < VK_BLEND_BEGIN_RANGE ||
        pCreateInfo->pCbState->pAttachments->srcBlendAlpha > VK_BLEND_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkBlend pCreateInfo->pCbState->pAttachments->srcBlendAlpha, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pCbState->pAttachments->destBlendAlpha < VK_BLEND_BEGIN_RANGE ||
        pCreateInfo->pCbState->pAttachments->destBlendAlpha > VK_BLEND_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkBlend pCreateInfo->pCbState->pAttachments->destBlendAlpha, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pCbState->pAttachments->blendOpAlpha < VK_BLEND_OP_BEGIN_RANGE ||
        pCreateInfo->pCbState->pAttachments->blendOpAlpha > VK_BLEND_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkBlendOp pCreateInfo->pCbState->pAttachments->blendOpAlpha, is unrecognized enumerator");
        return;
    }
    if(!ValidateEnumerator((VkChannelFlagBits)pCreateInfo->pCbState->pAttachments->channelWriteMask))
    {
        std::string reason = "vkCreateGraphicsPipelineDerivative parameter, VkChannelFlags pCreateInfo->pCbState->pAttachments->channelWriteMask, is " + EnumeratorString((VkChannelFlagBits)pCreateInfo->pCbState->pAttachments->channelWriteMask);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
    if(!ValidateEnumerator((VkPipelineCreateFlagBits)pCreateInfo->flags))
    {
        std::string reason = "vkCreateGraphicsPipelineDerivative parameter, VkPipelineCreateFlags pCreateInfo->flags, is " + EnumeratorString((VkPipelineCreateFlagBits)pCreateInfo->flags);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
    if(pCreateInfo->layout == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkPipelineLayout pCreateInfo->layout, is null pointer");
        return;
    }
}

void PostCreateGraphicsPipelineDerivative(
    VkDevice device,
    VkPipeline basePipeline,
    VkPipeline* pPipeline,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkDevice device, is null pointer");
        return;
    }

    if(basePipeline == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkPipeline basePipeline, is null pointer");
        return;
    }

    if(pPipeline == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkPipeline* pPipeline, is null pointer");
        return;
    }
    if((*pPipeline) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateGraphicsPipelineDerivative parameter, VkPipeline* pPipeline, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateGraphicsPipelineDerivative parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateGraphicsPipelineDerivative(
    VkDevice device,
    const VkGraphicsPipelineCreateInfo* pCreateInfo,
    VkPipeline basePipeline,
    VkPipeline* pPipeline)
{
    PreCreateGraphicsPipelineDerivative(device, pCreateInfo);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateGraphicsPipelineDerivative(device, pCreateInfo, basePipeline, pPipeline);

    PostCreateGraphicsPipelineDerivative(device, basePipeline, pPipeline, result);

    return result;
}

void PreCreateComputePipeline(
    VkDevice device,
    const VkComputePipelineCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateComputePipeline parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateComputePipeline parameter, const VkComputePipelineCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateComputePipeline parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->cs.sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->cs.sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateComputePipeline parameter, VkStructureType pCreateInfo->cs.sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->cs.pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateComputePipeline parameter, const void* pCreateInfo->cs.pNext, is null pointer");
        return;
    }
    if(pCreateInfo->cs.stage < VK_SHADER_STAGE_BEGIN_RANGE ||
        pCreateInfo->cs.stage > VK_SHADER_STAGE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateComputePipeline parameter, VkShaderStage pCreateInfo->cs.stage, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->cs.shader == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateComputePipeline parameter, VkShader pCreateInfo->cs.shader, is null pointer");
        return;
    }
    if(pCreateInfo->cs.pLinkConstBufferInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateComputePipeline parameter, const VkLinkConstBuffer* pCreateInfo->cs.pLinkConstBufferInfo, is null pointer");
        return;
    }
    if(pCreateInfo->cs.pLinkConstBufferInfo->pBufferData == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateComputePipeline parameter, const void* pCreateInfo->cs.pLinkConstBufferInfo->pBufferData, is null pointer");
        return;
    }
    if(pCreateInfo->cs.pSpecializationInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateComputePipeline parameter, const VkSpecializationInfo* pCreateInfo->cs.pSpecializationInfo, is null pointer");
        return;
    }
    if(pCreateInfo->cs.pSpecializationInfo->pMap == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateComputePipeline parameter, const VkSpecializationMapEntry* pCreateInfo->cs.pSpecializationInfo->pMap, is null pointer");
        return;
    }
    if(pCreateInfo->cs.pSpecializationInfo->pData == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateComputePipeline parameter, const void* pCreateInfo->cs.pSpecializationInfo->pData, is null pointer");
        return;
    }
    if(!ValidateEnumerator((VkPipelineCreateFlagBits)pCreateInfo->flags))
    {
        std::string reason = "vkCreateComputePipeline parameter, VkPipelineCreateFlags pCreateInfo->flags, is " + EnumeratorString((VkPipelineCreateFlagBits)pCreateInfo->flags);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
    if(pCreateInfo->layout == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateComputePipeline parameter, VkPipelineLayout pCreateInfo->layout, is null pointer");
        return;
    }
}

void PostCreateComputePipeline(
    VkDevice device,
    VkPipeline* pPipeline,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateComputePipeline parameter, VkDevice device, is null pointer");
        return;
    }

    if(pPipeline == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateComputePipeline parameter, VkPipeline* pPipeline, is null pointer");
        return;
    }
    if((*pPipeline) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateComputePipeline parameter, VkPipeline* pPipeline, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateComputePipeline parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateComputePipeline(
    VkDevice device,
    const VkComputePipelineCreateInfo* pCreateInfo,
    VkPipeline* pPipeline)
{
    PreCreateComputePipeline(device, pCreateInfo);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateComputePipeline(device, pCreateInfo, pPipeline);

    PostCreateComputePipeline(device, pPipeline, result);

    return result;
}

void PreStorePipeline(
    VkDevice device)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkStorePipeline parameter, VkDevice device, is null pointer");
        return;
    }
}

void PostStorePipeline(
    VkDevice device,
    VkPipeline pipeline,
    size_t* pDataSize,
    void* pData,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkStorePipeline parameter, VkDevice device, is null pointer");
        return;
    }

    if(pipeline == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkStorePipeline parameter, VkPipeline pipeline, is null pointer");
        return;
    }

    if(pDataSize == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkStorePipeline parameter, size_t* pDataSize, is null pointer");
        return;
    }

    if(pData == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkStorePipeline parameter, void* pData, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkStorePipeline parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkStorePipeline(
    VkDevice device,
    VkPipeline pipeline,
    size_t* pDataSize,
    void* pData)
{
    PreStorePipeline(device);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->StorePipeline(device, pipeline, pDataSize, pData);

    PostStorePipeline(device, pipeline, pDataSize, pData, result);

    return result;
}

void PreLoadPipeline(
    VkDevice device,
    const void* pData)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkLoadPipeline parameter, VkDevice device, is null pointer");
        return;
    }

    if(pData == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkLoadPipeline parameter, const void* pData, is null pointer");
        return;
    }
}

void PostLoadPipeline(
    VkDevice device,
    size_t dataSize,
    VkPipeline* pPipeline,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkLoadPipeline parameter, VkDevice device, is null pointer");
        return;
    }


    if(pPipeline == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkLoadPipeline parameter, VkPipeline* pPipeline, is null pointer");
        return;
    }
    if((*pPipeline) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkLoadPipeline parameter, VkPipeline* pPipeline, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkLoadPipeline parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkLoadPipeline(
    VkDevice device,
    size_t dataSize,
    const void* pData,
    VkPipeline* pPipeline)
{
    PreLoadPipeline(device, pData);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->LoadPipeline(device, dataSize, pData, pPipeline);

    PostLoadPipeline(device, dataSize, pPipeline, result);

    return result;
}

void PreLoadPipelineDerivative(
    VkDevice device,
    const void* pData)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkLoadPipelineDerivative parameter, VkDevice device, is null pointer");
        return;
    }

    if(pData == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkLoadPipelineDerivative parameter, const void* pData, is null pointer");
        return;
    }
}

void PostLoadPipelineDerivative(
    VkDevice device,
    size_t dataSize,
    VkPipeline basePipeline,
    VkPipeline* pPipeline,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkLoadPipelineDerivative parameter, VkDevice device, is null pointer");
        return;
    }


    if(basePipeline == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkLoadPipelineDerivative parameter, VkPipeline basePipeline, is null pointer");
        return;
    }

    if(pPipeline == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkLoadPipelineDerivative parameter, VkPipeline* pPipeline, is null pointer");
        return;
    }
    if((*pPipeline) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkLoadPipelineDerivative parameter, VkPipeline* pPipeline, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkLoadPipelineDerivative parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkLoadPipelineDerivative(
    VkDevice device,
    size_t dataSize,
    const void* pData,
    VkPipeline basePipeline,
    VkPipeline* pPipeline)
{
    PreLoadPipelineDerivative(device, pData);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->LoadPipelineDerivative(device, dataSize, pData, basePipeline, pPipeline);

    PostLoadPipelineDerivative(device, dataSize, basePipeline, pPipeline, result);

    return result;
}

void PreCreatePipelineLayout(
    VkDevice device,
    const VkPipelineLayoutCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreatePipelineLayout parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreatePipelineLayout parameter, const VkPipelineLayoutCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreatePipelineLayout parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pSetLayouts == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreatePipelineLayout parameter, const VkDescriptorSetLayout* pCreateInfo->pSetLayouts, is null pointer");
        return;
    }
    if((*pCreateInfo->pSetLayouts) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreatePipelineLayout parameter, const VkDescriptorSetLayout* pCreateInfo->pSetLayouts, is null pointer");
        return;
    }
}

void PostCreatePipelineLayout(
    VkDevice device,
    VkPipelineLayout* pPipelineLayout,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreatePipelineLayout parameter, VkDevice device, is null pointer");
        return;
    }

    if(pPipelineLayout == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreatePipelineLayout parameter, VkPipelineLayout* pPipelineLayout, is null pointer");
        return;
    }
    if((*pPipelineLayout) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreatePipelineLayout parameter, VkPipelineLayout* pPipelineLayout, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreatePipelineLayout parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreCreateSampler(
    VkDevice device,
    const VkSamplerCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, const VkSamplerCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->magFilter < VK_TEX_FILTER_BEGIN_RANGE ||
        pCreateInfo->magFilter > VK_TEX_FILTER_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkTexFilter pCreateInfo->magFilter, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->minFilter < VK_TEX_FILTER_BEGIN_RANGE ||
        pCreateInfo->minFilter > VK_TEX_FILTER_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkTexFilter pCreateInfo->minFilter, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->mipMode < VK_TEX_MIPMAP_MODE_BEGIN_RANGE ||
        pCreateInfo->mipMode > VK_TEX_MIPMAP_MODE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkTexMipmapMode pCreateInfo->mipMode, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->addressU < VK_TEX_ADDRESS_BEGIN_RANGE ||
        pCreateInfo->addressU > VK_TEX_ADDRESS_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkTexAddress pCreateInfo->addressU, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->addressV < VK_TEX_ADDRESS_BEGIN_RANGE ||
        pCreateInfo->addressV > VK_TEX_ADDRESS_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkTexAddress pCreateInfo->addressV, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->addressW < VK_TEX_ADDRESS_BEGIN_RANGE ||
        pCreateInfo->addressW > VK_TEX_ADDRESS_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkTexAddress pCreateInfo->addressW, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->compareOp < VK_COMPARE_OP_BEGIN_RANGE ||
        pCreateInfo->compareOp > VK_COMPARE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkCompareOp pCreateInfo->compareOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->borderColor < VK_BORDER_COLOR_BEGIN_RANGE ||
        pCreateInfo->borderColor > VK_BORDER_COLOR_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkBorderColor pCreateInfo->borderColor, is unrecognized enumerator");
        return;
    }
}

void PostCreateSampler(
    VkDevice device,
    VkSampler* pSampler,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkDevice device, is null pointer");
        return;
    }

    if(pSampler == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkSampler* pSampler, is null pointer");
        return;
    }
    if((*pSampler) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateSampler parameter, VkSampler* pSampler, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateSampler parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreCreateDescriptorSetLayout(
    VkDevice device,
    const VkDescriptorSetLayoutCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorSetLayout parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorSetLayout parameter, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorSetLayout parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pBinding == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorSetLayout parameter, const VkDescriptorSetLayoutBinding* pCreateInfo->pBinding, is null pointer");
        return;
    }
    if(pCreateInfo->pBinding->descriptorType < VK_DESCRIPTOR_TYPE_BEGIN_RANGE ||
        pCreateInfo->pBinding->descriptorType > VK_DESCRIPTOR_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorSetLayout parameter, VkDescriptorType pCreateInfo->pBinding->descriptorType, is unrecognized enumerator");
        return;
    }
    if(!ValidateEnumerator((VkShaderStageFlagBits)pCreateInfo->pBinding->stageFlags))
    {
        std::string reason = "vkCreateDescriptorSetLayout parameter, VkShaderStageFlags pCreateInfo->pBinding->stageFlags, is " + EnumeratorString((VkShaderStageFlagBits)pCreateInfo->pBinding->stageFlags);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
    if(pCreateInfo->pBinding->pImmutableSamplers == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorSetLayout parameter, const VkSampler* pCreateInfo->pBinding->pImmutableSamplers, is null pointer");
        return;
    }
    if((*pCreateInfo->pBinding->pImmutableSamplers) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorSetLayout parameter, const VkSampler* pCreateInfo->pBinding->pImmutableSamplers, is null pointer");
        return;
    }
}

void PostCreateDescriptorSetLayout(
    VkDevice device,
    VkDescriptorSetLayout* pSetLayout,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorSetLayout parameter, VkDevice device, is null pointer");
        return;
    }

    if(pSetLayout == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorSetLayout parameter, VkDescriptorSetLayout* pSetLayout, is null pointer");
        return;
    }
    if((*pSetLayout) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorSetLayout parameter, VkDescriptorSetLayout* pSetLayout, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateDescriptorSetLayout parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreCreateDescriptorPool(
    VkDevice device,
    const VkDescriptorPoolCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorPool parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorPool parameter, const VkDescriptorPoolCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorPool parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pTypeCount == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorPool parameter, const VkDescriptorTypeCount* pCreateInfo->pTypeCount, is null pointer");
        return;
    }
    if(pCreateInfo->pTypeCount->type < VK_DESCRIPTOR_TYPE_BEGIN_RANGE ||
        pCreateInfo->pTypeCount->type > VK_DESCRIPTOR_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorPool parameter, VkDescriptorType pCreateInfo->pTypeCount->type, is unrecognized enumerator");
        return;
    }
}

void PostCreateDescriptorPool(
    VkDevice device,
    VkDescriptorPoolUsage poolUsage,
    uint32_t maxSets,
    VkDescriptorPool* pDescriptorPool,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorPool parameter, VkDevice device, is null pointer");
        return;
    }

    if(poolUsage < VK_DESCRIPTOR_POOL_USAGE_BEGIN_RANGE ||
        poolUsage > VK_DESCRIPTOR_POOL_USAGE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorPool parameter, VkDescriptorPoolUsage poolUsage, is unrecognized enumerator");
        return;
    }


    if(pDescriptorPool == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorPool parameter, VkDescriptorPool* pDescriptorPool, is null pointer");
        return;
    }
    if((*pDescriptorPool) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDescriptorPool parameter, VkDescriptorPool* pDescriptorPool, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateDescriptorPool parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreResetDescriptorPool(
    VkDevice device)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkResetDescriptorPool parameter, VkDevice device, is null pointer");
        return;
    }
}

void PostResetDescriptorPool(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkResetDescriptorPool parameter, VkDevice device, is null pointer");
        return;
    }

    if(descriptorPool == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkResetDescriptorPool parameter, VkDescriptorPool descriptorPool, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkResetDescriptorPool parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkResetDescriptorPool(
    VkDevice device,
    VkDescriptorPool descriptorPool)
{
    PreResetDescriptorPool(device);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->ResetDescriptorPool(device, descriptorPool);

    PostResetDescriptorPool(device, descriptorPool, result);

    return result;
}

void PreAllocDescriptorSets(
    VkDevice device,
    const VkDescriptorSetLayout* pSetLayouts)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkAllocDescriptorSets parameter, VkDevice device, is null pointer");
        return;
    }

    if(pSetLayouts == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkAllocDescriptorSets parameter, const VkDescriptorSetLayout* pSetLayouts, is null pointer");
        return;
    }
    if((*pSetLayouts) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkAllocDescriptorSets parameter, const VkDescriptorSetLayout* pSetLayouts, is null pointer");
        return;
    }
}

void PostAllocDescriptorSets(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    VkDescriptorSetUsage setUsage,
    uint32_t count,
    VkDescriptorSet* pDescriptorSets,
    uint32_t* pCount,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkAllocDescriptorSets parameter, VkDevice device, is null pointer");
        return;
    }

    if(descriptorPool == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkAllocDescriptorSets parameter, VkDescriptorPool descriptorPool, is null pointer");
        return;
    }

    if(setUsage < VK_DESCRIPTOR_SET_USAGE_BEGIN_RANGE ||
        setUsage > VK_DESCRIPTOR_SET_USAGE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkAllocDescriptorSets parameter, VkDescriptorSetUsage setUsage, is unrecognized enumerator");
        return;
    }


    if(pDescriptorSets == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkAllocDescriptorSets parameter, VkDescriptorSet* pDescriptorSets, is null pointer");
        return;
    }
    if((*pDescriptorSets) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkAllocDescriptorSets parameter, VkDescriptorSet* pDescriptorSets, is null pointer");
        return;
    }

    if(pCount == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkAllocDescriptorSets parameter, uint32_t* pCount, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkAllocDescriptorSets parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkAllocDescriptorSets(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    VkDescriptorSetUsage setUsage,
    uint32_t count,
    const VkDescriptorSetLayout* pSetLayouts,
    VkDescriptorSet* pDescriptorSets,
    uint32_t* pCount)
{
    PreAllocDescriptorSets(device, pSetLayouts);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->AllocDescriptorSets(device, descriptorPool, setUsage, count, pSetLayouts, pDescriptorSets, pCount);

    PostAllocDescriptorSets(device, descriptorPool, setUsage, count, pDescriptorSets, pCount, result);

    return result;
}

void PreUpdateDescriptorSets(
    VkDevice device,
    const VkWriteDescriptorSet* pDescriptorWrites,
    const VkCopyDescriptorSet* pDescriptorCopies)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkDevice device, is null pointer");
        return;
    }

    if(pDescriptorWrites == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, const VkWriteDescriptorSet* pDescriptorWrites, is null pointer");
        return;
    }
    if(pDescriptorWrites->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pDescriptorWrites->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkStructureType pDescriptorWrites->sType, is unrecognized enumerator");
        return;
    }
    if(pDescriptorWrites->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, const void* pDescriptorWrites->pNext, is null pointer");
        return;
    }
    if(pDescriptorWrites->destSet == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkDescriptorSet pDescriptorWrites->destSet, is null pointer");
        return;
    }
    if(pDescriptorWrites->descriptorType < VK_DESCRIPTOR_TYPE_BEGIN_RANGE ||
        pDescriptorWrites->descriptorType > VK_DESCRIPTOR_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkDescriptorType pDescriptorWrites->descriptorType, is unrecognized enumerator");
        return;
    }
    if(pDescriptorWrites->pDescriptors == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, const VkDescriptorInfo* pDescriptorWrites->pDescriptors, is null pointer");
        return;
    }
    if(pDescriptorWrites->pDescriptors->bufferView == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkBufferView pDescriptorWrites->pDescriptors->bufferView, is null pointer");
        return;
    }
    if(pDescriptorWrites->pDescriptors->sampler == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkSampler pDescriptorWrites->pDescriptors->sampler, is null pointer");
        return;
    }
    if(pDescriptorWrites->pDescriptors->imageView == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkImageView pDescriptorWrites->pDescriptors->imageView, is null pointer");
        return;
    }
    if(pDescriptorWrites->pDescriptors->imageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        pDescriptorWrites->pDescriptors->imageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkImageLayout pDescriptorWrites->pDescriptors->imageLayout, is unrecognized enumerator");
        return;
    }

    if(pDescriptorCopies == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, const VkCopyDescriptorSet* pDescriptorCopies, is null pointer");
        return;
    }
    if(pDescriptorCopies->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pDescriptorCopies->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkStructureType pDescriptorCopies->sType, is unrecognized enumerator");
        return;
    }
    if(pDescriptorCopies->pNext == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, const void* pDescriptorCopies->pNext, is null pointer");
        return;
    }
    if(pDescriptorCopies->srcSet == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkDescriptorSet pDescriptorCopies->srcSet, is null pointer");
        return;
    }
    if(pDescriptorCopies->destSet == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkDescriptorSet pDescriptorCopies->destSet, is null pointer");
        return;
    }
}

void PostUpdateDescriptorSets(
    VkDevice device,
    uint32_t writeCount,
    uint32_t copyCount,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkUpdateDescriptorSets parameter, VkDevice device, is null pointer");
        return;
    }



    if(result != VK_SUCCESS)
    {
        std::string reason = "vkUpdateDescriptorSets parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkUpdateDescriptorSets(
    VkDevice device,
    uint32_t writeCount,
    const VkWriteDescriptorSet* pDescriptorWrites,
    uint32_t copyCount,
    const VkCopyDescriptorSet* pDescriptorCopies)
{
    PreUpdateDescriptorSets(device, pDescriptorWrites, pDescriptorCopies);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->UpdateDescriptorSets(device, writeCount, pDescriptorWrites, copyCount, pDescriptorCopies);

    PostUpdateDescriptorSets(device, writeCount, copyCount, result);

    return result;
}

void PreCreateDynamicViewportState(
    VkDevice device,
    const VkDynamicVpStateCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicViewportState parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicViewportState parameter, const VkDynamicVpStateCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicViewportState parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pViewports == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicViewportState parameter, const VkViewport* pCreateInfo->pViewports, is null pointer");
        return;
    }
    if(pCreateInfo->pScissors == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicViewportState parameter, const VkRect* pCreateInfo->pScissors, is null pointer");
        return;
    }
}

void PostCreateDynamicViewportState(
    VkDevice device,
    VkDynamicVpState* pState,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicViewportState parameter, VkDevice device, is null pointer");
        return;
    }

    if(pState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicViewportState parameter, VkDynamicVpState* pState, is null pointer");
        return;
    }
    if((*pState) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicViewportState parameter, VkDynamicVpState* pState, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateDynamicViewportState parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicViewportState(
    VkDevice device,
    const VkDynamicVpStateCreateInfo* pCreateInfo,
    VkDynamicVpState* pState)
{
    PreCreateDynamicViewportState(device, pCreateInfo);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateDynamicViewportState(device, pCreateInfo, pState);

    PostCreateDynamicViewportState(device, pState, result);

    return result;
}

void PreCreateDynamicRasterState(
    VkDevice device,
    const VkDynamicRsStateCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicRasterState parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicRasterState parameter, const VkDynamicRsStateCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicRasterState parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
}

void PostCreateDynamicRasterState(
    VkDevice device,
    VkDynamicRsState* pState,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicRasterState parameter, VkDevice device, is null pointer");
        return;
    }

    if(pState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicRasterState parameter, VkDynamicRsState* pState, is null pointer");
        return;
    }
    if((*pState) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicRasterState parameter, VkDynamicRsState* pState, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateDynamicRasterState parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicRasterState(
    VkDevice device,
    const VkDynamicRsStateCreateInfo* pCreateInfo,
    VkDynamicRsState* pState)
{
    PreCreateDynamicRasterState(device, pCreateInfo);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateDynamicRasterState(device, pCreateInfo, pState);

    PostCreateDynamicRasterState(device, pState, result);

    return result;
}

void PreCreateDynamicColorBlendState(
    VkDevice device,
    const VkDynamicCbStateCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicColorBlendState parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicColorBlendState parameter, const VkDynamicCbStateCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicColorBlendState parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
}

void PostCreateDynamicColorBlendState(
    VkDevice device,
    VkDynamicCbState* pState,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicColorBlendState parameter, VkDevice device, is null pointer");
        return;
    }

    if(pState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicColorBlendState parameter, VkDynamicCbState* pState, is null pointer");
        return;
    }
    if((*pState) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicColorBlendState parameter, VkDynamicCbState* pState, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateDynamicColorBlendState parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicColorBlendState(
    VkDevice device,
    const VkDynamicCbStateCreateInfo* pCreateInfo,
    VkDynamicCbState* pState)
{
    PreCreateDynamicColorBlendState(device, pCreateInfo);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateDynamicColorBlendState(device, pCreateInfo, pState);

    PostCreateDynamicColorBlendState(device, pState, result);

    return result;
}

void PreCreateDynamicDepthStencilState(
    VkDevice device,
    const VkDynamicDsStateCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicDepthStencilState parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicDepthStencilState parameter, const VkDynamicDsStateCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicDepthStencilState parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
}

void PostCreateDynamicDepthStencilState(
    VkDevice device,
    VkDynamicDsState* pState,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicDepthStencilState parameter, VkDevice device, is null pointer");
        return;
    }

    if(pState == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicDepthStencilState parameter, VkDynamicDsState* pState, is null pointer");
        return;
    }
    if((*pState) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateDynamicDepthStencilState parameter, VkDynamicDsState* pState, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateDynamicDepthStencilState parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicDepthStencilState(
    VkDevice device,
    const VkDynamicDsStateCreateInfo* pCreateInfo,
    VkDynamicDsState* pState)
{
    PreCreateDynamicDepthStencilState(device, pCreateInfo);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateDynamicDepthStencilState(device, pCreateInfo, pState);

    PostCreateDynamicDepthStencilState(device, pState, result);

    return result;
}

void PreCreateCommandBuffer(
    VkDevice device,
    const VkCmdBufferCreateInfo* pCreateInfo,
    VkCmdBuffer* pCmdBuffer)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateCommandBuffer parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateCommandBuffer parameter, const VkCmdBufferCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateCommandBuffer parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }

    if(pCmdBuffer == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateCommandBuffer parameter, VkCmdBuffer* pCmdBuffer, is null pointer");
        return;
    }
    if((*pCmdBuffer) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateCommandBuffer parameter, VkCmdBuffer* pCmdBuffer, is null pointer");
        return;
    }
}

void PostCreateCommandBuffer(
    VkDevice device,
    VkCmdBuffer* pCmdBuffer,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateCommandBuffer parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCmdBuffer == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateCommandBuffer parameter, VkCmdBuffer* pCmdBuffer, is null pointer");
        return;
    }
    if((*pCmdBuffer) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateCommandBuffer parameter, VkCmdBuffer* pCmdBuffer, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateCommandBuffer parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateCommandBuffer(
    VkDevice device,
    const VkCmdBufferCreateInfo* pCreateInfo,
    VkCmdBuffer* pCmdBuffer)
{
    PreCreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    VkResult result = get_dispatch_table(pc_device_table_map, device)->CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);

    PostCreateCommandBuffer(device, pCmdBuffer, result);

    return result;
}

void PreBeginCommandBuffer(
    VkCmdBuffer cmdBuffer,
    const VkCmdBufferBeginInfo* pBeginInfo)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkBeginCommandBuffer parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(pBeginInfo == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkBeginCommandBuffer parameter, const VkCmdBufferBeginInfo* pBeginInfo, is null pointer");
        return;
    }
    if(pBeginInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pBeginInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkBeginCommandBuffer parameter, VkStructureType pBeginInfo->sType, is unrecognized enumerator");
        return;
    }
    if(!ValidateEnumerator((VkCmdBufferOptimizeFlagBits)pBeginInfo->flags))
    {
        std::string reason = "vkBeginCommandBuffer parameter, VkCmdBufferOptimizeFlags pBeginInfo->flags, is " + EnumeratorString((VkCmdBufferOptimizeFlagBits)pBeginInfo->flags);
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

void PostBeginCommandBuffer(
    VkCmdBuffer cmdBuffer,
    VkResult result)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkBeginCommandBuffer parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkBeginCommandBuffer parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreEndCommandBuffer(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkEndCommandBuffer parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostEndCommandBuffer(
    VkCmdBuffer cmdBuffer,
    VkResult result)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkEndCommandBuffer parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkEndCommandBuffer parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkEndCommandBuffer(
    VkCmdBuffer cmdBuffer)
{
    PreEndCommandBuffer(cmdBuffer);
    VkResult result = get_dispatch_table(pc_device_table_map, cmdBuffer)->EndCommandBuffer(cmdBuffer);

    PostEndCommandBuffer(cmdBuffer, result);

    return result;
}

void PreResetCommandBuffer(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkResetCommandBuffer parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostResetCommandBuffer(
    VkCmdBuffer cmdBuffer,
    VkResult result)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkResetCommandBuffer parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkResetCommandBuffer parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkResetCommandBuffer(
    VkCmdBuffer cmdBuffer)
{
    PreResetCommandBuffer(cmdBuffer);
    VkResult result = get_dispatch_table(pc_device_table_map, cmdBuffer)->ResetCommandBuffer(cmdBuffer);

    PostResetCommandBuffer(cmdBuffer, result);

    return result;
}

void PreCmdBindPipeline(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindPipeline parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostCmdBindPipeline(
    VkCmdBuffer cmdBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipeline pipeline)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindPipeline parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(pipelineBindPoint < VK_PIPELINE_BIND_POINT_BEGIN_RANGE ||
        pipelineBindPoint > VK_PIPELINE_BIND_POINT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindPipeline parameter, VkPipelineBindPoint pipelineBindPoint, is unrecognized enumerator");
        return;
    }

    if(pipeline == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindPipeline parameter, VkPipeline pipeline, is null pointer");
        return;
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdBindPipeline(
    VkCmdBuffer cmdBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipeline pipeline)
{
    PreCmdBindPipeline(cmdBuffer);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);

    PostCmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
}

void PreCmdBindDynamicStateObject(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindDynamicStateObject parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostCmdBindDynamicStateObject(
    VkCmdBuffer cmdBuffer,
    VkStateBindPoint stateBindPoint,
    VkDynamicStateObject dynamicState)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindDynamicStateObject parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(stateBindPoint < VK_STATE_BIND_POINT_BEGIN_RANGE ||
        stateBindPoint > VK_STATE_BIND_POINT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindDynamicStateObject parameter, VkStateBindPoint stateBindPoint, is unrecognized enumerator");
        return;
    }

    if(dynamicState == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindDynamicStateObject parameter, VkDynamicStateObject dynamicState, is null pointer");
        return;
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDynamicStateObject(
    VkCmdBuffer cmdBuffer,
    VkStateBindPoint stateBindPoint,
    VkDynamicStateObject dynamicState)
{
    PreCmdBindDynamicStateObject(cmdBuffer);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdBindDynamicStateObject(cmdBuffer, stateBindPoint, dynamicState);

    PostCmdBindDynamicStateObject(cmdBuffer, stateBindPoint, dynamicState);
}

void PreCmdBindDescriptorSets(
    VkCmdBuffer cmdBuffer,
    const VkDescriptorSet* pDescriptorSets,
    const uint32_t* pDynamicOffsets)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindDescriptorSets parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(pDescriptorSets == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindDescriptorSets parameter, const VkDescriptorSet* pDescriptorSets, is null pointer");
        return;
    }
    if((*pDescriptorSets) == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindDescriptorSets parameter, const VkDescriptorSet* pDescriptorSets, is null pointer");
        return;
    }

    if(pDynamicOffsets == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindDescriptorSets parameter, const uint32_t* pDynamicOffsets, is null pointer");
        return;
    }
}

void PostCmdBindDescriptorSets(
    VkCmdBuffer cmdBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipelineLayout layout,
    uint32_t firstSet,
    uint32_t setCount,
    uint32_t dynamicOffsetCount)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindDescriptorSets parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(pipelineBindPoint < VK_PIPELINE_BIND_POINT_BEGIN_RANGE ||
        pipelineBindPoint > VK_PIPELINE_BIND_POINT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindDescriptorSets parameter, VkPipelineBindPoint pipelineBindPoint, is unrecognized enumerator");
        return;
    }

    if(layout == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindDescriptorSets parameter, VkPipelineLayout layout, is null pointer");
        return;
    }



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

void PreCmdBindIndexBuffer(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindIndexBuffer parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostCmdBindIndexBuffer(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    VkIndexType indexType)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindIndexBuffer parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(buffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindIndexBuffer parameter, VkBuffer buffer, is null pointer");
        return;
    }


    if(indexType < VK_INDEX_TYPE_BEGIN_RANGE ||
        indexType > VK_INDEX_TYPE_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindIndexBuffer parameter, VkIndexType indexType, is unrecognized enumerator");
        return;
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdBindIndexBuffer(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    VkIndexType indexType)
{
    PreCmdBindIndexBuffer(cmdBuffer);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);

    PostCmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
}

void PreCmdBindVertexBuffers(
    VkCmdBuffer cmdBuffer,
    const VkBuffer* pBuffers,
    const VkDeviceSize* pOffsets)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindVertexBuffers parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(pBuffers == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindVertexBuffers parameter, const VkBuffer* pBuffers, is null pointer");
        return;
    }
    if((*pBuffers) == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindVertexBuffers parameter, const VkBuffer* pBuffers, is null pointer");
        return;
    }

    if(pOffsets == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindVertexBuffers parameter, const VkDeviceSize* pOffsets, is null pointer");
        return;
    }
}

void PostCmdBindVertexBuffers(
    VkCmdBuffer cmdBuffer,
    uint32_t startBinding,
    uint32_t bindingCount)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBindVertexBuffers parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }


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

void PreCmdDraw(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdDraw parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostCmdDraw(
    VkCmdBuffer cmdBuffer,
    uint32_t firstVertex,
    uint32_t vertexCount,
    uint32_t firstInstance,
    uint32_t instanceCount)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdDraw parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }




}

VK_LAYER_EXPORT void VKAPI vkCmdDraw(
    VkCmdBuffer cmdBuffer,
    uint32_t firstVertex,
    uint32_t vertexCount,
    uint32_t firstInstance,
    uint32_t instanceCount)
{
    PreCmdDraw(cmdBuffer);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdDraw(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);

    PostCmdDraw(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
}

void PreCmdDrawIndexed(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdDrawIndexed parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostCmdDrawIndexed(
    VkCmdBuffer cmdBuffer,
    uint32_t firstIndex,
    uint32_t indexCount,
    int32_t vertexOffset,
    uint32_t firstInstance,
    uint32_t instanceCount)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdDrawIndexed parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }





}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexed(
    VkCmdBuffer cmdBuffer,
    uint32_t firstIndex,
    uint32_t indexCount,
    int32_t vertexOffset,
    uint32_t firstInstance,
    uint32_t instanceCount)
{
    PreCmdDrawIndexed(cmdBuffer);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdDrawIndexed(cmdBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);

    PostCmdDrawIndexed(cmdBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
}

void PreCmdDrawIndirect(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdDrawIndirect parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostCmdDrawIndirect(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    uint32_t count,
    uint32_t stride)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdDrawIndirect parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(buffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdDrawIndirect parameter, VkBuffer buffer, is null pointer");
        return;
    }



}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndirect(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    uint32_t count,
    uint32_t stride)
{
    PreCmdDrawIndirect(cmdBuffer);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);

    PostCmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
}

void PreCmdDrawIndexedIndirect(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdDrawIndexedIndirect parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostCmdDrawIndexedIndirect(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    uint32_t count,
    uint32_t stride)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdDrawIndexedIndirect parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(buffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdDrawIndexedIndirect parameter, VkBuffer buffer, is null pointer");
        return;
    }



}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexedIndirect(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    uint32_t count,
    uint32_t stride)
{
    PreCmdDrawIndexedIndirect(cmdBuffer);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);

    PostCmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
}

void PreCmdDispatch(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdDispatch parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostCmdDispatch(
    VkCmdBuffer cmdBuffer,
    uint32_t x,
    uint32_t y,
    uint32_t z)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdDispatch parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }



}

VK_LAYER_EXPORT void VKAPI vkCmdDispatch(
    VkCmdBuffer cmdBuffer,
    uint32_t x,
    uint32_t y,
    uint32_t z)
{
    PreCmdDispatch(cmdBuffer);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdDispatch(cmdBuffer, x, y, z);

    PostCmdDispatch(cmdBuffer, x, y, z);
}

void PreCmdDispatchIndirect(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdDispatchIndirect parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostCmdDispatchIndirect(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdDispatchIndirect parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(buffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdDispatchIndirect parameter, VkBuffer buffer, is null pointer");
        return;
    }

}

VK_LAYER_EXPORT void VKAPI vkCmdDispatchIndirect(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset)
{
    PreCmdDispatchIndirect(cmdBuffer);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdDispatchIndirect(cmdBuffer, buffer, offset);

    PostCmdDispatchIndirect(cmdBuffer, buffer, offset);
}

void PreCmdCopyBuffer(
    VkCmdBuffer cmdBuffer,
    const VkBufferCopy* pRegions)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyBuffer parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(pRegions == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyBuffer parameter, const VkBufferCopy* pRegions, is null pointer");
        return;
    }
}

void PostCmdCopyBuffer(
    VkCmdBuffer cmdBuffer,
    VkBuffer srcBuffer,
    VkBuffer destBuffer,
    uint32_t regionCount)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyBuffer parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(srcBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyBuffer parameter, VkBuffer srcBuffer, is null pointer");
        return;
    }

    if(destBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyBuffer parameter, VkBuffer destBuffer, is null pointer");
        return;
    }

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

void PreCmdCopyImage(
    VkCmdBuffer cmdBuffer,
    const VkImageCopy* pRegions)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyImage parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(pRegions == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyImage parameter, const VkImageCopy* pRegions, is null pointer");
        return;
    }
    if(pRegions->srcSubresource.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRegions->srcSubresource.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyImage parameter, VkImageAspect pRegions->srcSubresource.aspect, is unrecognized enumerator");
        return;
    }
    if(pRegions->destSubresource.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRegions->destSubresource.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyImage parameter, VkImageAspect pRegions->destSubresource.aspect, is unrecognized enumerator");
        return;
    }
}

void PostCmdCopyImage(
    VkCmdBuffer cmdBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkImage destImage,
    VkImageLayout destImageLayout,
    uint32_t regionCount)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyImage parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(srcImage == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyImage parameter, VkImage srcImage, is null pointer");
        return;
    }

    if(srcImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        srcImageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyImage parameter, VkImageLayout srcImageLayout, is unrecognized enumerator");
        return;
    }

    if(destImage == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyImage parameter, VkImage destImage, is null pointer");
        return;
    }

    if(destImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        destImageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyImage parameter, VkImageLayout destImageLayout, is unrecognized enumerator");
        return;
    }

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

void PreCmdBlitImage(
    VkCmdBuffer cmdBuffer,
    const VkImageBlit* pRegions)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBlitImage parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(pRegions == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBlitImage parameter, const VkImageBlit* pRegions, is null pointer");
        return;
    }
    if(pRegions->srcSubresource.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRegions->srcSubresource.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBlitImage parameter, VkImageAspect pRegions->srcSubresource.aspect, is unrecognized enumerator");
        return;
    }
    if(pRegions->destSubresource.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRegions->destSubresource.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBlitImage parameter, VkImageAspect pRegions->destSubresource.aspect, is unrecognized enumerator");
        return;
    }
}

void PostCmdBlitImage(
    VkCmdBuffer cmdBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkImage destImage,
    VkImageLayout destImageLayout,
    uint32_t regionCount,
    VkTexFilter filter)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBlitImage parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(srcImage == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBlitImage parameter, VkImage srcImage, is null pointer");
        return;
    }

    if(srcImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        srcImageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBlitImage parameter, VkImageLayout srcImageLayout, is unrecognized enumerator");
        return;
    }

    if(destImage == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBlitImage parameter, VkImage destImage, is null pointer");
        return;
    }

    if(destImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        destImageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBlitImage parameter, VkImageLayout destImageLayout, is unrecognized enumerator");
        return;
    }


    if(filter < VK_TEX_FILTER_BEGIN_RANGE ||
        filter > VK_TEX_FILTER_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBlitImage parameter, VkTexFilter filter, is unrecognized enumerator");
        return;
    }
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

void PreCmdCopyBufferToImage(
    VkCmdBuffer cmdBuffer,
    const VkBufferImageCopy* pRegions)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyBufferToImage parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(pRegions == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyBufferToImage parameter, const VkBufferImageCopy* pRegions, is null pointer");
        return;
    }
    if(pRegions->imageSubresource.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRegions->imageSubresource.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyBufferToImage parameter, VkImageAspect pRegions->imageSubresource.aspect, is unrecognized enumerator");
        return;
    }
}

void PostCmdCopyBufferToImage(
    VkCmdBuffer cmdBuffer,
    VkBuffer srcBuffer,
    VkImage destImage,
    VkImageLayout destImageLayout,
    uint32_t regionCount)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyBufferToImage parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(srcBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyBufferToImage parameter, VkBuffer srcBuffer, is null pointer");
        return;
    }

    if(destImage == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyBufferToImage parameter, VkImage destImage, is null pointer");
        return;
    }

    if(destImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        destImageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyBufferToImage parameter, VkImageLayout destImageLayout, is unrecognized enumerator");
        return;
    }

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

void PreCmdCopyImageToBuffer(
    VkCmdBuffer cmdBuffer,
    const VkBufferImageCopy* pRegions)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyImageToBuffer parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(pRegions == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyImageToBuffer parameter, const VkBufferImageCopy* pRegions, is null pointer");
        return;
    }
    if(pRegions->imageSubresource.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRegions->imageSubresource.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyImageToBuffer parameter, VkImageAspect pRegions->imageSubresource.aspect, is unrecognized enumerator");
        return;
    }
}

void PostCmdCopyImageToBuffer(
    VkCmdBuffer cmdBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkBuffer destBuffer,
    uint32_t regionCount)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyImageToBuffer parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(srcImage == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyImageToBuffer parameter, VkImage srcImage, is null pointer");
        return;
    }

    if(srcImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        srcImageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyImageToBuffer parameter, VkImageLayout srcImageLayout, is unrecognized enumerator");
        return;
    }

    if(destBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyImageToBuffer parameter, VkBuffer destBuffer, is null pointer");
        return;
    }

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

void PreCmdUpdateBuffer(
    VkCmdBuffer cmdBuffer,
    const uint32_t* pData)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdUpdateBuffer parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(pData == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdUpdateBuffer parameter, const uint32_t* pData, is null pointer");
        return;
    }
}

void PostCmdUpdateBuffer(
    VkCmdBuffer cmdBuffer,
    VkBuffer destBuffer,
    VkDeviceSize destOffset,
    VkDeviceSize dataSize)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdUpdateBuffer parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(destBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdUpdateBuffer parameter, VkBuffer destBuffer, is null pointer");
        return;
    }


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

void PreCmdFillBuffer(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdFillBuffer parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostCmdFillBuffer(
    VkCmdBuffer cmdBuffer,
    VkBuffer destBuffer,
    VkDeviceSize destOffset,
    VkDeviceSize fillSize,
    uint32_t data)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdFillBuffer parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(destBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdFillBuffer parameter, VkBuffer destBuffer, is null pointer");
        return;
    }



}

VK_LAYER_EXPORT void VKAPI vkCmdFillBuffer(
    VkCmdBuffer cmdBuffer,
    VkBuffer destBuffer,
    VkDeviceSize destOffset,
    VkDeviceSize fillSize,
    uint32_t data)
{
    PreCmdFillBuffer(cmdBuffer);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);

    PostCmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
}

void PreCmdClearColorImage(
    VkCmdBuffer cmdBuffer,
    const VkClearColor* pColor,
    const VkImageSubresourceRange* pRanges)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdClearColorImage parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(pColor == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdClearColorImage parameter, const VkClearColor* pColor, is null pointer");
        return;
    }

    if(pRanges == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdClearColorImage parameter, const VkImageSubresourceRange* pRanges, is null pointer");
        return;
    }
    if(pRanges->aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRanges->aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdClearColorImage parameter, VkImageAspect pRanges->aspect, is unrecognized enumerator");
        return;
    }
}

void PostCmdClearColorImage(
    VkCmdBuffer cmdBuffer,
    VkImage image,
    VkImageLayout imageLayout,
    uint32_t rangeCount)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdClearColorImage parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(image == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdClearColorImage parameter, VkImage image, is null pointer");
        return;
    }

    if(imageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        imageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdClearColorImage parameter, VkImageLayout imageLayout, is unrecognized enumerator");
        return;
    }

}

VK_LAYER_EXPORT void VKAPI vkCmdClearColorImage(
    VkCmdBuffer cmdBuffer,
    VkImage image,
    VkImageLayout imageLayout,
    const VkClearColor* pColor,
    uint32_t rangeCount,
    const VkImageSubresourceRange* pRanges)
{
    PreCmdClearColorImage(cmdBuffer, pColor, pRanges);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdClearColorImage(cmdBuffer, image, imageLayout, pColor, rangeCount, pRanges);

    PostCmdClearColorImage(cmdBuffer, image, imageLayout, rangeCount);
}

void PreCmdClearDepthStencil(
    VkCmdBuffer cmdBuffer,
    const VkImageSubresourceRange* pRanges)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdClearDepthStencil parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(pRanges == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdClearDepthStencil parameter, const VkImageSubresourceRange* pRanges, is null pointer");
        return;
    }
    if(pRanges->aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRanges->aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdClearDepthStencil parameter, VkImageAspect pRanges->aspect, is unrecognized enumerator");
        return;
    }
}

void PostCmdClearDepthStencil(
    VkCmdBuffer cmdBuffer,
    VkImage image,
    VkImageLayout imageLayout,
    float depth,
    uint32_t stencil,
    uint32_t rangeCount)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdClearDepthStencil parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(image == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdClearDepthStencil parameter, VkImage image, is null pointer");
        return;
    }

    if(imageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        imageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdClearDepthStencil parameter, VkImageLayout imageLayout, is unrecognized enumerator");
        return;
    }



}

VK_LAYER_EXPORT void VKAPI vkCmdClearDepthStencil(
    VkCmdBuffer cmdBuffer,
    VkImage image,
    VkImageLayout imageLayout,
    float depth,
    uint32_t stencil,
    uint32_t rangeCount,
    const VkImageSubresourceRange* pRanges)
{
    PreCmdClearDepthStencil(cmdBuffer, pRanges);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdClearDepthStencil(cmdBuffer, image, imageLayout, depth, stencil, rangeCount, pRanges);

    PostCmdClearDepthStencil(cmdBuffer, image, imageLayout, depth, stencil, rangeCount);
}

void PreCmdResolveImage(
    VkCmdBuffer cmdBuffer,
    const VkImageResolve* pRegions)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdResolveImage parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(pRegions == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdResolveImage parameter, const VkImageResolve* pRegions, is null pointer");
        return;
    }
    if(pRegions->srcSubresource.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRegions->srcSubresource.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdResolveImage parameter, VkImageAspect pRegions->srcSubresource.aspect, is unrecognized enumerator");
        return;
    }
    if(pRegions->destSubresource.aspect < VK_IMAGE_ASPECT_BEGIN_RANGE ||
        pRegions->destSubresource.aspect > VK_IMAGE_ASPECT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdResolveImage parameter, VkImageAspect pRegions->destSubresource.aspect, is unrecognized enumerator");
        return;
    }
}

void PostCmdResolveImage(
    VkCmdBuffer cmdBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkImage destImage,
    VkImageLayout destImageLayout,
    uint32_t regionCount)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdResolveImage parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(srcImage == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdResolveImage parameter, VkImage srcImage, is null pointer");
        return;
    }

    if(srcImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        srcImageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdResolveImage parameter, VkImageLayout srcImageLayout, is unrecognized enumerator");
        return;
    }

    if(destImage == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdResolveImage parameter, VkImage destImage, is null pointer");
        return;
    }

    if(destImageLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        destImageLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdResolveImage parameter, VkImageLayout destImageLayout, is unrecognized enumerator");
        return;
    }

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

void PreCmdSetEvent(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdSetEvent parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostCmdSetEvent(
    VkCmdBuffer cmdBuffer,
    VkEvent event,
    VkPipelineStageFlags stageMask)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdSetEvent parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(event == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdSetEvent parameter, VkEvent event, is null pointer");
        return;
    }

}

VK_LAYER_EXPORT void VKAPI vkCmdSetEvent(
    VkCmdBuffer cmdBuffer,
    VkEvent event,
    VkPipelineStageFlags stageMask)
{
    PreCmdSetEvent(cmdBuffer);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdSetEvent(cmdBuffer, event, stageMask);

    PostCmdSetEvent(cmdBuffer, event, stageMask);
}

void PreCmdResetEvent(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdResetEvent parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostCmdResetEvent(
    VkCmdBuffer cmdBuffer,
    VkEvent event,
    VkPipelineStageFlags stageMask)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdResetEvent parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(event == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdResetEvent parameter, VkEvent event, is null pointer");
        return;
    }

}

VK_LAYER_EXPORT void VKAPI vkCmdResetEvent(
    VkCmdBuffer cmdBuffer,
    VkEvent event,
    VkPipelineStageFlags stageMask)
{
    PreCmdResetEvent(cmdBuffer);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdResetEvent(cmdBuffer, event, stageMask);

    PostCmdResetEvent(cmdBuffer, event, stageMask);
}

void PreCmdWaitEvents(
    VkCmdBuffer cmdBuffer,
    const VkEvent* pEvents,
    const void** ppMemBarriers)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdWaitEvents parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(pEvents == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdWaitEvents parameter, const VkEvent* pEvents, is null pointer");
        return;
    }
    if((*pEvents) == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdWaitEvents parameter, const VkEvent* pEvents, is null pointer");
        return;
    }

    if(ppMemBarriers == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdWaitEvents parameter, const void** ppMemBarriers, is null pointer");
        return;
    }
}

void PostCmdWaitEvents(
    VkCmdBuffer cmdBuffer,
    uint32_t eventCount,
    VkPipelineStageFlags sourceStageMask,
    VkPipelineStageFlags destStageMask,
    uint32_t memBarrierCount)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdWaitEvents parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }




}

VK_LAYER_EXPORT void VKAPI vkCmdWaitEvents(
    VkCmdBuffer cmdBuffer,
    uint32_t eventCount,
    const VkEvent* pEvents,
    VkPipelineStageFlags sourceStageMask,
    VkPipelineStageFlags destStageMask,
    uint32_t memBarrierCount,
    const void** ppMemBarriers)
{
    PreCmdWaitEvents(cmdBuffer, pEvents, ppMemBarriers);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdWaitEvents(cmdBuffer, eventCount, pEvents, sourceStageMask, destStageMask, memBarrierCount, ppMemBarriers);

    PostCmdWaitEvents(cmdBuffer, eventCount, sourceStageMask, destStageMask, memBarrierCount);
}

void PreCmdPipelineBarrier(
    VkCmdBuffer cmdBuffer,
    const void** ppMemBarriers)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdPipelineBarrier parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(ppMemBarriers == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdPipelineBarrier parameter, const void** ppMemBarriers, is null pointer");
        return;
    }
}

void PostCmdPipelineBarrier(
    VkCmdBuffer cmdBuffer,
    VkPipelineStageFlags sourceStageMask,
    VkPipelineStageFlags destStageMask,
    bool32_t byRegion,
    uint32_t memBarrierCount)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdPipelineBarrier parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }




}

VK_LAYER_EXPORT void VKAPI vkCmdPipelineBarrier(
    VkCmdBuffer cmdBuffer,
    VkPipelineStageFlags sourceStageMask,
    VkPipelineStageFlags destStageMask,
    bool32_t byRegion,
    uint32_t memBarrierCount,
    const void** ppMemBarriers)
{
    PreCmdPipelineBarrier(cmdBuffer, ppMemBarriers);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdPipelineBarrier(cmdBuffer, sourceStageMask, destStageMask, byRegion, memBarrierCount, ppMemBarriers);

    PostCmdPipelineBarrier(cmdBuffer, sourceStageMask, destStageMask, byRegion, memBarrierCount);
}

void PreCmdBeginQuery(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBeginQuery parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostCmdBeginQuery(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t slot,
    VkQueryControlFlags flags)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBeginQuery parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(queryPool == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBeginQuery parameter, VkQueryPool queryPool, is null pointer");
        return;
    }


    if(!ValidateEnumerator((VkQueryControlFlagBits)flags))
    {
        std::string reason = "vkCmdBeginQuery parameter, VkQueryControlFlags flags, is " + EnumeratorString((VkQueryControlFlagBits)flags);
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginQuery(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t slot,
    VkQueryControlFlags flags)
{
    PreCmdBeginQuery(cmdBuffer);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdBeginQuery(cmdBuffer, queryPool, slot, flags);

    PostCmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

void PreCmdEndQuery(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdEndQuery parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostCmdEndQuery(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t slot)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdEndQuery parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(queryPool == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdEndQuery parameter, VkQueryPool queryPool, is null pointer");
        return;
    }

}

VK_LAYER_EXPORT void VKAPI vkCmdEndQuery(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t slot)
{
    PreCmdEndQuery(cmdBuffer);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdEndQuery(cmdBuffer, queryPool, slot);

    PostCmdEndQuery(cmdBuffer, queryPool, slot);
}

void PreCmdResetQueryPool(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdResetQueryPool parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostCmdResetQueryPool(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t startQuery,
    uint32_t queryCount)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdResetQueryPool parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(queryPool == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdResetQueryPool parameter, VkQueryPool queryPool, is null pointer");
        return;
    }


}

VK_LAYER_EXPORT void VKAPI vkCmdResetQueryPool(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t startQuery,
    uint32_t queryCount)
{
    PreCmdResetQueryPool(cmdBuffer);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);

    PostCmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

void PreCmdWriteTimestamp(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdWriteTimestamp parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostCmdWriteTimestamp(
    VkCmdBuffer cmdBuffer,
    VkTimestampType timestampType,
    VkBuffer destBuffer,
    VkDeviceSize destOffset)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdWriteTimestamp parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(timestampType < VK_TIMESTAMP_TYPE_BEGIN_RANGE ||
        timestampType > VK_TIMESTAMP_TYPE_END_RANGE)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdWriteTimestamp parameter, VkTimestampType timestampType, is unrecognized enumerator");
        return;
    }

    if(destBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdWriteTimestamp parameter, VkBuffer destBuffer, is null pointer");
        return;
    }

}

VK_LAYER_EXPORT void VKAPI vkCmdWriteTimestamp(
    VkCmdBuffer cmdBuffer,
    VkTimestampType timestampType,
    VkBuffer destBuffer,
    VkDeviceSize destOffset)
{
    PreCmdWriteTimestamp(cmdBuffer);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdWriteTimestamp(cmdBuffer, timestampType, destBuffer, destOffset);

    PostCmdWriteTimestamp(cmdBuffer, timestampType, destBuffer, destOffset);
}

void PreCmdCopyQueryPoolResults(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyQueryPoolResults parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostCmdCopyQueryPoolResults(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t startQuery,
    uint32_t queryCount,
    VkBuffer destBuffer,
    VkDeviceSize destOffset,
    VkDeviceSize destStride,
    VkQueryResultFlags flags)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyQueryPoolResults parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(queryPool == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyQueryPoolResults parameter, VkQueryPool queryPool, is null pointer");
        return;
    }



    if(destBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdCopyQueryPoolResults parameter, VkBuffer destBuffer, is null pointer");
        return;
    }



    if(!ValidateEnumerator((VkQueryResultFlagBits)flags))
    {
        std::string reason = "vkCmdCopyQueryPoolResults parameter, VkQueryResultFlags flags, is " + EnumeratorString((VkQueryResultFlagBits)flags);
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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
    PreCmdCopyQueryPoolResults(cmdBuffer);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdCopyQueryPoolResults(cmdBuffer, queryPool, startQuery, queryCount, destBuffer, destOffset, destStride, flags);

    PostCmdCopyQueryPoolResults(cmdBuffer, queryPool, startQuery, queryCount, destBuffer, destOffset, destStride, flags);
}

void PreCreateFramebuffer(
    VkDevice device,
    const VkFramebufferCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateFramebuffer parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateFramebuffer parameter, const VkFramebufferCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateFramebuffer parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pColorAttachments == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateFramebuffer parameter, const VkColorAttachmentBindInfo* pCreateInfo->pColorAttachments, is null pointer");
        return;
    }
    if(pCreateInfo->pColorAttachments->view == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateFramebuffer parameter, VkColorAttachmentView pCreateInfo->pColorAttachments->view, is null pointer");
        return;
    }
    if(pCreateInfo->pColorAttachments->layout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        pCreateInfo->pColorAttachments->layout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateFramebuffer parameter, VkImageLayout pCreateInfo->pColorAttachments->layout, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pDepthStencilAttachment == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateFramebuffer parameter, const VkDepthStencilBindInfo* pCreateInfo->pDepthStencilAttachment, is null pointer");
        return;
    }
    if(pCreateInfo->pDepthStencilAttachment->view == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateFramebuffer parameter, VkDepthStencilView pCreateInfo->pDepthStencilAttachment->view, is null pointer");
        return;
    }
    if(pCreateInfo->pDepthStencilAttachment->layout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        pCreateInfo->pDepthStencilAttachment->layout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateFramebuffer parameter, VkImageLayout pCreateInfo->pDepthStencilAttachment->layout, is unrecognized enumerator");
        return;
    }
}

void PostCreateFramebuffer(
    VkDevice device,
    VkFramebuffer* pFramebuffer,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateFramebuffer parameter, VkDevice device, is null pointer");
        return;
    }

    if(pFramebuffer == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateFramebuffer parameter, VkFramebuffer* pFramebuffer, is null pointer");
        return;
    }
    if((*pFramebuffer) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateFramebuffer parameter, VkFramebuffer* pFramebuffer, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateFramebuffer parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreCreateRenderPass(
    VkDevice device,
    const VkRenderPassCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkDevice device, is null pointer");
        return;
    }

    if(pCreateInfo == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, const VkRenderPassCreateInfo* pCreateInfo, is null pointer");
        return;
    }
    if(pCreateInfo->sType < VK_STRUCTURE_TYPE_BEGIN_RANGE ||
        pCreateInfo->sType > VK_STRUCTURE_TYPE_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkStructureType pCreateInfo->sType, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pColorFormats == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, const VkFormat* pCreateInfo->pColorFormats, is null pointer");
        return;
    }
    if((*pCreateInfo->pColorFormats) < VK_FORMAT_BEGIN_RANGE ||
        (*pCreateInfo->pColorFormats) > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, const VkFormat* pCreateInfo->pColorFormats, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pColorLayouts == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, const VkImageLayout* pCreateInfo->pColorLayouts, is null pointer");
        return;
    }
    if((*pCreateInfo->pColorLayouts) < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        (*pCreateInfo->pColorLayouts) > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, const VkImageLayout* pCreateInfo->pColorLayouts, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pColorLoadOps == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, const VkAttachmentLoadOp* pCreateInfo->pColorLoadOps, is null pointer");
        return;
    }
    if((*pCreateInfo->pColorLoadOps) < VK_ATTACHMENT_LOAD_OP_BEGIN_RANGE ||
        (*pCreateInfo->pColorLoadOps) > VK_ATTACHMENT_LOAD_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, const VkAttachmentLoadOp* pCreateInfo->pColorLoadOps, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pColorStoreOps == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, const VkAttachmentStoreOp* pCreateInfo->pColorStoreOps, is null pointer");
        return;
    }
    if((*pCreateInfo->pColorStoreOps) < VK_ATTACHMENT_STORE_OP_BEGIN_RANGE ||
        (*pCreateInfo->pColorStoreOps) > VK_ATTACHMENT_STORE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, const VkAttachmentStoreOp* pCreateInfo->pColorStoreOps, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->pColorLoadClearValues == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, const VkClearColor* pCreateInfo->pColorLoadClearValues, is null pointer");
        return;
    }
    if(pCreateInfo->depthStencilFormat < VK_FORMAT_BEGIN_RANGE ||
        pCreateInfo->depthStencilFormat > VK_FORMAT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkFormat pCreateInfo->depthStencilFormat, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->depthStencilLayout < VK_IMAGE_LAYOUT_BEGIN_RANGE ||
        pCreateInfo->depthStencilLayout > VK_IMAGE_LAYOUT_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkImageLayout pCreateInfo->depthStencilLayout, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->depthLoadOp < VK_ATTACHMENT_LOAD_OP_BEGIN_RANGE ||
        pCreateInfo->depthLoadOp > VK_ATTACHMENT_LOAD_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkAttachmentLoadOp pCreateInfo->depthLoadOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->depthStoreOp < VK_ATTACHMENT_STORE_OP_BEGIN_RANGE ||
        pCreateInfo->depthStoreOp > VK_ATTACHMENT_STORE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkAttachmentStoreOp pCreateInfo->depthStoreOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->stencilLoadOp < VK_ATTACHMENT_LOAD_OP_BEGIN_RANGE ||
        pCreateInfo->stencilLoadOp > VK_ATTACHMENT_LOAD_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkAttachmentLoadOp pCreateInfo->stencilLoadOp, is unrecognized enumerator");
        return;
    }
    if(pCreateInfo->stencilStoreOp < VK_ATTACHMENT_STORE_OP_BEGIN_RANGE ||
        pCreateInfo->stencilStoreOp > VK_ATTACHMENT_STORE_OP_END_RANGE)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkAttachmentStoreOp pCreateInfo->stencilStoreOp, is unrecognized enumerator");
        return;
    }
}

void PostCreateRenderPass(
    VkDevice device,
    VkRenderPass* pRenderPass,
    VkResult result)
{
    if(device == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkDevice device, is null pointer");
        return;
    }

    if(pRenderPass == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkRenderPass* pRenderPass, is null pointer");
        return;
    }
    if((*pRenderPass) == nullptr)
    {
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCreateRenderPass parameter, VkRenderPass* pRenderPass, is null pointer");
        return;
    }

    if(result != VK_SUCCESS)
    {
        std::string reason = "vkCreateRenderPass parameter, VkResult result, is " + EnumeratorString(result);
        log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK", reason.c_str());
        return;
    }
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

void PreCmdBeginRenderPass(
    VkCmdBuffer cmdBuffer,
    const VkRenderPassBegin* pRenderPassBegin)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBeginRenderPass parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(pRenderPassBegin == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBeginRenderPass parameter, const VkRenderPassBegin* pRenderPassBegin, is null pointer");
        return;
    }
    if(pRenderPassBegin->renderPass == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBeginRenderPass parameter, VkRenderPass pRenderPassBegin->renderPass, is null pointer");
        return;
    }
    if(pRenderPassBegin->framebuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBeginRenderPass parameter, VkFramebuffer pRenderPassBegin->framebuffer, is null pointer");
        return;
    }
}

void PostCmdBeginRenderPass(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdBeginRenderPass parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginRenderPass(
    VkCmdBuffer cmdBuffer,
    const VkRenderPassBegin* pRenderPassBegin)
{
    PreCmdBeginRenderPass(cmdBuffer, pRenderPassBegin);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdBeginRenderPass(cmdBuffer, pRenderPassBegin);

    PostCmdBeginRenderPass(cmdBuffer);
}

void PreCmdEndRenderPass(
    VkCmdBuffer cmdBuffer)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdEndRenderPass parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }
}

void PostCmdEndRenderPass(
    VkCmdBuffer cmdBuffer,
    VkRenderPass renderPass)
{
    if(cmdBuffer == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdEndRenderPass parameter, VkCmdBuffer cmdBuffer, is null pointer");
        return;
    }

    if(renderPass == nullptr)
    {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "PARAMCHECK",
        "vkCmdEndRenderPass parameter, VkRenderPass renderPass, is null pointer");
        return;
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdEndRenderPass(
    VkCmdBuffer cmdBuffer,
    VkRenderPass renderPass)
{
    PreCmdEndRenderPass(cmdBuffer);
    get_dispatch_table(pc_device_table_map, cmdBuffer)->CmdEndRenderPass(cmdBuffer, renderPass);

    PostCmdEndRenderPass(cmdBuffer, renderPass);
}

VK_LAYER_EXPORT void* VKAPI vkGetDeviceProcAddr(VkDevice device, const char* funcName)
{
    if (device == NULL) {
        return NULL;
    }

    /* loader uses this to force layer initialization; device object is wrapped */
    if (!strcmp(funcName, "vkGetDeviceProcAddr")) {
        initDeviceTable(pc_device_table_map, (const VkBaseLayerObject *) device);
        return (void*) vkGetDeviceProcAddr;
    }

    if (!strcmp(funcName, "vkCreateDevice"))
        return (void*) vkCreateDevice;
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (void*) vkDestroyDevice;
    if (!strcmp(funcName, "vkGetDeviceQueue"))
        return (void*) vkGetDeviceQueue;
    if (!strcmp(funcName, "vkQueueSubmit"))
        return (void*) vkQueueSubmit;
    if (!strcmp(funcName, "vkQueueWaitIdle"))
        return (void*) vkQueueWaitIdle;
    if (!strcmp(funcName, "vkDeviceWaitIdle"))
        return (void*) vkDeviceWaitIdle;
    if (!strcmp(funcName, "vkAllocMemory"))
        return (void*) vkAllocMemory;
    if (!strcmp(funcName, "vkFreeMemory"))
        return (void*) vkFreeMemory;
    if (!strcmp(funcName, "vkMapMemory"))
        return (void*) vkMapMemory;
    if (!strcmp(funcName, "vkUnmapMemory"))
        return (void*) vkUnmapMemory;
    if (!strcmp(funcName, "vkFlushMappedMemoryRanges"))
        return (void*) vkFlushMappedMemoryRanges;
    if (!strcmp(funcName, "vkInvalidateMappedMemoryRanges"))
        return (void*) vkInvalidateMappedMemoryRanges;
    if (!strcmp(funcName, "vkDestroyObject"))
        return (void*) vkDestroyObject;
    if (!strcmp(funcName, "vkGetObjectMemoryRequirements"))
        return (void*) vkGetObjectMemoryRequirements;
    if (!strcmp(funcName, "vkCreateFence"))
        return (void*) vkCreateFence;
    if (!strcmp(funcName, "vkResetFences"))
        return (void*) vkResetFences;
    if (!strcmp(funcName, "vkGetFenceStatus"))
        return (void*) vkGetFenceStatus;
    if (!strcmp(funcName, "vkWaitForFences"))
        return (void*) vkWaitForFences;
    if (!strcmp(funcName, "vkCreateSemaphore"))
        return (void*) vkCreateSemaphore;
    if (!strcmp(funcName, "vkQueueSignalSemaphore"))
        return (void*) vkQueueSignalSemaphore;
    if (!strcmp(funcName, "vkQueueWaitSemaphore"))
        return (void*) vkQueueWaitSemaphore;
    if (!strcmp(funcName, "vkCreateEvent"))
        return (void*) vkCreateEvent;
    if (!strcmp(funcName, "vkGetEventStatus"))
        return (void*) vkGetEventStatus;
    if (!strcmp(funcName, "vkSetEvent"))
        return (void*) vkSetEvent;
    if (!strcmp(funcName, "vkResetEvent"))
        return (void*) vkResetEvent;
    if (!strcmp(funcName, "vkCreateQueryPool"))
        return (void*) vkCreateQueryPool;
    if (!strcmp(funcName, "vkGetQueryPoolResults"))
        return (void*) vkGetQueryPoolResults;
    if (!strcmp(funcName, "vkCreateBuffer"))
        return (void*) vkCreateBuffer;
    if (!strcmp(funcName, "vkCreateBufferView"))
        return (void*) vkCreateBufferView;
    if (!strcmp(funcName, "vkCreateImage"))
        return (void*) vkCreateImage;
    if (!strcmp(funcName, "vkGetImageSubresourceLayout"))
        return (void*) vkGetImageSubresourceLayout;
    if (!strcmp(funcName, "vkCreateImageView"))
        return (void*) vkCreateImageView;
    if (!strcmp(funcName, "vkCreateColorAttachmentView"))
        return (void*) vkCreateColorAttachmentView;
    if (!strcmp(funcName, "vkCreateDepthStencilView"))
        return (void*) vkCreateDepthStencilView;
    if (!strcmp(funcName, "vkCreateShader"))
        return (void*) vkCreateShader;
    if (!strcmp(funcName, "vkCreateGraphicsPipeline"))
        return (void*) vkCreateGraphicsPipeline;
    if (!strcmp(funcName, "vkCreateGraphicsPipelineDerivative"))
        return (void*) vkCreateGraphicsPipelineDerivative;
    if (!strcmp(funcName, "vkCreateComputePipeline"))
        return (void*) vkCreateComputePipeline;
    if (!strcmp(funcName, "vkStorePipeline"))
        return (void*) vkStorePipeline;
    if (!strcmp(funcName, "vkLoadPipeline"))
        return (void*) vkLoadPipeline;
    if (!strcmp(funcName, "vkLoadPipelineDerivative"))
        return (void*) vkLoadPipelineDerivative;
    if (!strcmp(funcName, "vkCreatePipelineLayout"))
        return (void*) vkCreatePipelineLayout;
    if (!strcmp(funcName, "vkCreateSampler"))
        return (void*) vkCreateSampler;
    if (!strcmp(funcName, "vkCreateDescriptorSetLayout"))
        return (void*) vkCreateDescriptorSetLayout;
    if (!strcmp(funcName, "vkCreateDescriptorPool"))
        return (void*) vkCreateDescriptorPool;
    if (!strcmp(funcName, "vkResetDescriptorPool"))
        return (void*) vkResetDescriptorPool;
    if (!strcmp(funcName, "vkAllocDescriptorSets"))
        return (void*) vkAllocDescriptorSets;
    if (!strcmp(funcName, "vkCreateDynamicViewportState"))
        return (void*) vkCreateDynamicViewportState;
    if (!strcmp(funcName, "vkCreateDynamicRasterState"))
        return (void*) vkCreateDynamicRasterState;
    if (!strcmp(funcName, "vkCreateDynamicColorBlendState"))
        return (void*) vkCreateDynamicColorBlendState;
    if (!strcmp(funcName, "vkCreateDynamicDepthStencilState"))
        return (void*) vkCreateDynamicDepthStencilState;
    if (!strcmp(funcName, "vkCreateCommandBuffer"))
        return (void*) vkCreateCommandBuffer;
    if (!strcmp(funcName, "vkBeginCommandBuffer"))
        return (void*) vkBeginCommandBuffer;
    if (!strcmp(funcName, "vkEndCommandBuffer"))
        return (void*) vkEndCommandBuffer;
    if (!strcmp(funcName, "vkResetCommandBuffer"))
        return (void*) vkResetCommandBuffer;
    if (!strcmp(funcName, "vkCmdBindPipeline"))
        return (void*) vkCmdBindPipeline;
    if (!strcmp(funcName, "vkCmdBindDynamicStateObject"))
        return (void*) vkCmdBindDynamicStateObject;
    if (!strcmp(funcName, "vkCmdBindDescriptorSets"))
        return (void*) vkCmdBindDescriptorSets;
    if (!strcmp(funcName, "vkCmdBindVertexBuffers"))
        return (void*) vkCmdBindVertexBuffers;
    if (!strcmp(funcName, "vkCmdBindIndexBuffer"))
        return (void*) vkCmdBindIndexBuffer;
    if (!strcmp(funcName, "vkCmdDraw"))
        return (void*) vkCmdDraw;
    if (!strcmp(funcName, "vkCmdDrawIndexed"))
        return (void*) vkCmdDrawIndexed;
    if (!strcmp(funcName, "vkCmdDrawIndirect"))
        return (void*) vkCmdDrawIndirect;
    if (!strcmp(funcName, "vkCmdDrawIndexedIndirect"))
        return (void*) vkCmdDrawIndexedIndirect;
    if (!strcmp(funcName, "vkCmdDispatch"))
        return (void*) vkCmdDispatch;
    if (!strcmp(funcName, "vkCmdDispatchIndirect"))
        return (void*) vkCmdDispatchIndirect;
    if (!strcmp(funcName, "vkCmdCopyBuffer"))
        return (void*) vkCmdCopyBuffer;
    if (!strcmp(funcName, "vkCmdCopyImage"))
        return (void*) vkCmdCopyImage;
    if (!strcmp(funcName, "vkCmdBlitImage"))
        return (void*) vkCmdBlitImage;
    if (!strcmp(funcName, "vkCmdCopyBufferToImage"))
        return (void*) vkCmdCopyBufferToImage;
    if (!strcmp(funcName, "vkCmdCopyImageToBuffer"))
        return (void*) vkCmdCopyImageToBuffer;
    if (!strcmp(funcName, "vkCmdUpdateBuffer"))
        return (void*) vkCmdUpdateBuffer;
    if (!strcmp(funcName, "vkCmdFillBuffer"))
        return (void*) vkCmdFillBuffer;
    if (!strcmp(funcName, "vkCmdClearColorImage"))
        return (void*) vkCmdClearColorImage;
    if (!strcmp(funcName, "vkCmdClearDepthStencil"))
        return (void*) vkCmdClearDepthStencil;
    if (!strcmp(funcName, "vkCmdResolveImage"))
        return (void*) vkCmdResolveImage;
    if (!strcmp(funcName, "vkCmdSetEvent"))
        return (void*) vkCmdSetEvent;
    if (!strcmp(funcName, "vkCmdResetEvent"))
        return (void*) vkCmdResetEvent;
    if (!strcmp(funcName, "vkCmdWaitEvents"))
        return (void*) vkCmdWaitEvents;
    if (!strcmp(funcName, "vkCmdPipelineBarrier"))
        return (void*) vkCmdPipelineBarrier;
    if (!strcmp(funcName, "vkCmdBeginQuery"))
        return (void*) vkCmdBeginQuery;
    if (!strcmp(funcName, "vkCmdEndQuery"))
        return (void*) vkCmdEndQuery;
    if (!strcmp(funcName, "vkCmdResetQueryPool"))
        return (void*) vkCmdResetQueryPool;
    if (!strcmp(funcName, "vkCmdWriteTimestamp"))
        return (void*) vkCmdWriteTimestamp;
    if (!strcmp(funcName, "vkCmdCopyQueryPoolResults"))
        return (void*) vkCmdCopyQueryPoolResults;
    if (!strcmp(funcName, "vkCreateFramebuffer"))
        return (void*) vkCreateFramebuffer;
    if (!strcmp(funcName, "vkCreateRenderPass"))
        return (void*) vkCreateRenderPass;
    if (!strcmp(funcName, "vkCmdBeginRenderPass"))
        return (void*) vkCmdBeginRenderPass;
    if (!strcmp(funcName, "vkCmdEndRenderPass"))
        return (void*) vkCmdEndRenderPass;
    if (!strcmp(funcName, "vkGetGlobalExtensionCount"))
        return (void*) vkGetGlobalExtensionCount;
    if (!strcmp(funcName, "vkGetGlobalExtensionProperties"))
        return (void*) vkGetGlobalExtensionProperties;

    {
        if (get_dispatch_table(pc_device_table_map, device)->GetDeviceProcAddr == NULL)
            return NULL;
        return get_dispatch_table(pc_device_table_map, device)->GetDeviceProcAddr(device, funcName);
    }
}

VK_LAYER_EXPORT void* VKAPI vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    if (instance == NULL) {
        return NULL;
    }

    /* loader uses this to force layer initialization; instance object is wrapped */
    if (!strcmp(funcName, "vkGetInstanceProcAddr")) {
        initInstanceTable(pc_instance_table_map, (const VkBaseLayerObject *) instance);
        return (void *) vkGetInstanceProcAddr;
    }

    if (!strcmp(funcName, "vkCreateInstance"))
        return (void*) vkCreateInstance;
    if (!strcmp(funcName, "vkDestroyInstance"))
        return (void*) vkDestroyInstance;
    if (!strcmp(funcName, "vkEnumeratePhysicalDevices"))
        return (void*) vkEnumeratePhysicalDevices;
    if (!strcmp(funcName, "vkGetPhysicalDeviceExtensionCount"))
        return (void*) vkGetPhysicalDeviceExtensionCount;
    if (!strcmp(funcName, "vkGetPhysicalDeviceExtensionProperties"))
        return (void*) vkGetPhysicalDeviceExtensionProperties;
    if (!strcmp(funcName, "vkGetPhysicalDeviceProperties"))
        return (void*) vkGetPhysicalDeviceProperties;
    if (!strcmp(funcName, "vkGetPhysicalDeviceFeatures"))
        return (void*) vkGetPhysicalDeviceFeatures;
    if (!strcmp(funcName, "vkGetPhysicalDeviceFormatInfo"))
        return (void*) vkGetPhysicalDeviceFormatInfo;
    if (!strcmp(funcName, "vkGetPhysicalDeviceLimits"))
        return (void*) vkGetPhysicalDeviceLimits;

    layer_data *data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    void* fptr = debug_report_get_instance_proc_addr(data->report_data, funcName);
    if(fptr)
        return fptr;

    {
        if (get_dispatch_table(pc_instance_table_map, instance)->GetInstanceProcAddr == NULL)
            return NULL;
        return get_dispatch_table(pc_instance_table_map, instance)->GetInstanceProcAddr(instance, funcName);
    }
}
