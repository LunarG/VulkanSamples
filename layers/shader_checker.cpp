/*
 *
 * Copyright (C) 2015 Valve Corporation
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
 * Author: Chia-I Wu <olv@lunarg.com>
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 */
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include "vk_loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vulkan/vk_layer.h"
#include "vk_layer_utils.h"
#include "vk_layer_config.h"
#include "vk_layer_table.h"
#include "vk_enum_string_helper.h"
#include "shader_checker.h"
#include "vk_layer_extension_utils.h"

#include "spirv/spirv.hpp"

// fwd decls
struct shader_module;
struct render_pass;

struct layer_data {
    debug_report_data *report_data;
    std::vector<VkDbgMsgCallback> logging_callback;
    VkLayerDispatchTable* device_dispatch_table;
    VkLayerInstanceDispatchTable* instance_dispatch_table;

    std::unordered_map<VkShaderModule, shader_module *> shader_module_map;
    std::unordered_map<VkDescriptorSetLayout, std::unordered_set<uint32_t>*> descriptor_set_layout_map;
    std::unordered_map<VkPipelineLayout, std::vector<std::unordered_set<uint32_t>*>*> pipeline_layout_map;
    std::unordered_map<VkRenderPass, render_pass *> render_pass_map;

    layer_data() :
        report_data(nullptr),
        device_dispatch_table(nullptr),
        instance_dispatch_table(nullptr)
    {};
};

static void
build_type_def_index(std::vector<unsigned> const &words, std::unordered_map<unsigned, unsigned> &type_def_index);

struct shader_module {
    /* the spirv image itself */
    std::vector<uint32_t> words;
    /* a mapping of <id> to the first word of its def. this is useful because walking type
     * trees requires jumping all over the instruction stream.
     */
    std::unordered_map<unsigned, unsigned> type_def_index;

    shader_module(VkShaderModuleCreateInfo const *pCreateInfo) :
        words((uint32_t *)pCreateInfo->pCode, (uint32_t *)pCreateInfo->pCode + pCreateInfo->codeSize / sizeof(uint32_t)),
        type_def_index() {

        build_type_def_index(words, type_def_index);
    }
};

struct render_pass {
    std::vector<std::vector<VkFormat>> subpass_color_formats;

    render_pass(VkRenderPassCreateInfo const *pCreateInfo)
    {
        uint32_t i;

        subpass_color_formats.reserve(pCreateInfo->subpassCount);
        for (i = 0; i < pCreateInfo->subpassCount; i++) {
            const VkSubpassDescription *subpass = &pCreateInfo->pSubpasses[i];
            std::vector<VkFormat> color_formats;
            uint32_t j;

            color_formats.reserve(subpass->colorAttachmentCount);
            for (j = 0; j < subpass->colorAttachmentCount; j++) {
                const uint32_t att = subpass->pColorAttachments[j].attachment;
                const VkFormat format = pCreateInfo->pAttachments[att].format;

                color_formats.push_back(pCreateInfo->pAttachments[att].format);
            }

            subpass_color_formats.push_back(color_formats);
        }
    }
};

static std::unordered_map<void *, layer_data *> layer_data_map;

template layer_data *get_my_data_ptr<layer_data>(
        void *data_key,
        std::unordered_map<void *, layer_data *> &data_map);

static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(g_initOnce);
// TODO : This can be much smarter, using separate locks for separate global data
static int globalLockInitialized = 0;
static loader_platform_thread_mutex globalLock;

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorSetLayout(
    VkDevice device,
    const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDescriptorSetLayout* pSetLayout)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    /* stash a copy of the layout bindings */
    VkResult result = my_data->device_dispatch_table->CreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);

    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        auto& bindings = my_data->descriptor_set_layout_map[*pSetLayout];
        bindings = new std::unordered_set<uint32_t>();
        bindings->reserve(pCreateInfo->bindingCount);
        for (uint32_t i = 0; i < pCreateInfo->bindingCount; i++)
            bindings->insert(pCreateInfo->pBinding[i].binding);

        loader_platform_thread_unlock_mutex(&globalLock);
    }

    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineLayout(
    VkDevice                                    device,
    const VkPipelineLayoutCreateInfo*           pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkPipelineLayout*                           pPipelineLayout)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = my_data->device_dispatch_table->CreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);

    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        auto& layouts = my_data->pipeline_layout_map[*pPipelineLayout];
        layouts = new std::vector<std::unordered_set<uint32_t>*>();
        layouts->reserve(pCreateInfo->setLayoutCount);
        for (unsigned i = 0; i < pCreateInfo->setLayoutCount; i++) {
            layouts->push_back(my_data->descriptor_set_layout_map[pCreateInfo->pSetLayouts[i]]);
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }

    return result;
}

static void
build_type_def_index(std::vector<unsigned> const &words, std::unordered_map<unsigned, unsigned> &type_def_index)
{
    unsigned int const *code = (unsigned int const *)&words[0];
    size_t size = words.size();

    unsigned word = 5;
    while (word < size) {
        unsigned opcode = code[word] & 0x0ffffu;
        unsigned oplen = (code[word] & 0xffff0000u) >> 16;

        switch (opcode) {
        case spv::OpTypeVoid:
        case spv::OpTypeBool:
        case spv::OpTypeInt:
        case spv::OpTypeFloat:
        case spv::OpTypeVector:
        case spv::OpTypeMatrix:
        case spv::OpTypeImage:
        case spv::OpTypeSampler:
        case spv::OpTypeSampledImage:
        case spv::OpTypeArray:
        case spv::OpTypeRuntimeArray:
        case spv::OpTypeStruct:
        case spv::OpTypeOpaque:
        case spv::OpTypePointer:
        case spv::OpTypeFunction:
        case spv::OpTypeEvent:
        case spv::OpTypeDeviceEvent:
        case spv::OpTypeReserveId:
        case spv::OpTypeQueue:
        case spv::OpTypePipe:
            type_def_index[code[word+1]] = word;
            break;

        default:
            /* We only care about type definitions */
            break;
        }

        word += oplen;
    }
}

bool
shader_is_spirv(VkShaderModuleCreateInfo const *pCreateInfo)
{
    uint32_t *words = (uint32_t *)pCreateInfo->pCode;
    size_t sizeInWords = pCreateInfo->codeSize / sizeof(uint32_t);

    /* Just validate that the header makes sense. */
    return sizeInWords >= 5 && words[0] == spv::MagicNumber && words[1] == spv::Version;
}

static void
init_shader_checker(layer_data *my_data)
{
    uint32_t report_flags = 0;
    uint32_t debug_action = 0;
    FILE *log_output = NULL;
    const char *option_str;
    VkDbgMsgCallback callback;
    // initialize ShaderChecker options
    report_flags = getLayerOptionFlags("ShaderCheckerReportFlags", 0);
    getLayerOptionEnum("ShaderCheckerDebugAction", (uint32_t *) &debug_action);

    if (debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        option_str = getLayerOption("ShaderCheckerLogFilename");
        log_output = getLayerLogOutput(option_str, "ShaderChecker");
        layer_create_msg_callback(my_data->report_data, report_flags, log_callback, (void *) log_output, &callback);
        my_data->logging_callback.push_back(callback);
    }

    if (debug_action & VK_DBG_LAYER_ACTION_DEBUG_OUTPUT) {
        layer_create_msg_callback(my_data->report_data, report_flags, win32_debug_output_msg, NULL, &callback);
        my_data->logging_callback.push_back(callback);
    }

    if (!globalLockInitialized)
    {
        loader_platform_thread_create_mutex(&globalLock);
        globalLockInitialized = 1;
    }
}

static const VkLayerProperties shader_checker_global_layers[] = {
    {
        "ShaderChecker",
        VK_API_VERSION,
        VK_MAKE_VERSION(0, 1, 0),
        "Validation layer: ShaderChecker",
    }
};

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
        const char *pLayerName,
        uint32_t *pCount,
        VkExtensionProperties* pProperties)
{
    /* shader checker does not have any global extensions */
    return util_GetExtensionProperties(0, NULL, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(
        uint32_t *pCount,
        VkLayerProperties*    pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(shader_checker_global_layers),
                                   shader_checker_global_layers,
                                   pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(
        VkPhysicalDevice                            physicalDevice,
        const char*                                 pLayerName,
        uint32_t*                                   pCount,
        VkExtensionProperties*                      pProperties)
{
    /* Shader checker does not have any physical device extensions */
    if (pLayerName == NULL) {
        dispatch_key key = get_dispatch_key(physicalDevice);
        layer_data *my_data = get_my_data_ptr(key, layer_data_map);
        return my_data->instance_dispatch_table->EnumerateDeviceExtensionProperties(
                                                    physicalDevice,
                                                    NULL,
                                                    pCount,
                                                    pProperties);
    } else {
        return util_GetExtensionProperties(0, NULL, pCount, pProperties);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(
        VkPhysicalDevice                            physicalDevice,
        uint32_t*                                   pCount,
        VkLayerProperties*                          pProperties)
{
    /* Shader checker physical device layers are the same as global */
    return util_GetLayerProperties(ARRAY_SIZE(shader_checker_global_layers),
                                   shader_checker_global_layers,
                                   pCount, pProperties);
}

static char const *
storage_class_name(unsigned sc)
{
    switch (sc) {
    case spv::StorageClassInput: return "input";
    case spv::StorageClassOutput: return "output";
    case spv::StorageClassUniformConstant: return "const uniform";
    case spv::StorageClassUniform: return "uniform";
    case spv::StorageClassWorkgroup: return "workgroup local";
    case spv::StorageClassCrossWorkgroup: return "workgroup global";
    case spv::StorageClassPrivate: return "private global";
    case spv::StorageClassFunction: return "function";
    case spv::StorageClassGeneric: return "generic";
    case spv::StorageClassAtomicCounter: return "atomic counter";
    case spv::StorageClassImage: return "image";
    default: return "unknown";
    }
}

/* returns ptr to null terminator */
static char *
describe_type(char *dst, shader_module const *src, unsigned type)
{
    auto type_def_it = src->type_def_index.find(type);

    if (type_def_it == src->type_def_index.end()) {
        return dst + sprintf(dst, "undef");
    }

    unsigned int const *code = (unsigned int const *)&src->words[type_def_it->second];
    unsigned opcode = code[0] & 0x0ffffu;
    switch (opcode) {
        case spv::OpTypeBool:
            return dst + sprintf(dst, "bool");
        case spv::OpTypeInt:
            return dst + sprintf(dst, "%cint%d", code[3] ? 's' : 'u', code[2]);
        case spv::OpTypeFloat:
            return dst + sprintf(dst, "float%d", code[2]);
        case spv::OpTypeVector:
            dst += sprintf(dst, "vec%d of ", code[3]);
            return describe_type(dst, src, code[2]);
        case spv::OpTypeMatrix:
            dst += sprintf(dst, "mat%d of ", code[3]);
            return describe_type(dst, src, code[2]);
        case spv::OpTypeArray:
            dst += sprintf(dst, "arr[%d] of ", code[3]);
            return describe_type(dst, src, code[2]);
        case spv::OpTypePointer:
            dst += sprintf(dst, "ptr to %s ", storage_class_name(code[2]));
            return describe_type(dst, src, code[3]);
        case spv::OpTypeStruct:
            {
                unsigned oplen = code[0] >> 16;
                dst += sprintf(dst, "struct of (");
                for (unsigned i = 2; i < oplen; i++) {
                    dst = describe_type(dst, src, code[i]);
                    dst += sprintf(dst, i == oplen-1 ? ")" : ", ");
                }
                return dst;
            }
        case spv::OpTypeSampler:
            return dst + sprintf(dst, "sampler");
        default:
            return dst + sprintf(dst, "oddtype");
    }
}

static bool
types_match(shader_module const *a, shader_module const *b, unsigned a_type, unsigned b_type, bool b_arrayed)
{
    auto a_type_def_it = a->type_def_index.find(a_type);
    auto b_type_def_it = b->type_def_index.find(b_type);

    if (a_type_def_it == a->type_def_index.end()) {
        return false;
    }

    if (b_type_def_it == b->type_def_index.end()) {
        return false;
    }

    /* walk two type trees together, and complain about differences */
    unsigned int const *a_code = (unsigned int const *)&a->words[a_type_def_it->second];
    unsigned int const *b_code = (unsigned int const *)&b->words[b_type_def_it->second];

    unsigned a_opcode = a_code[0] & 0x0ffffu;
    unsigned b_opcode = b_code[0] & 0x0ffffu;

    if (b_arrayed && b_opcode == spv::OpTypeArray) {
        /* we probably just found the extra level of arrayness in b_type: compare the type inside it to a_type */
        return types_match(a, b, a_type, b_code[2], false);
    }

    if (a_opcode != b_opcode) {
        return false;
    }

    switch (a_opcode) {
        /* if b_arrayed and we hit a leaf type, then we can't match -- there's nowhere for the extra OpTypeArray to be! */
        case spv::OpTypeBool:
            return true && !b_arrayed;
        case spv::OpTypeInt:
            /* match on width, signedness */
            return a_code[2] == b_code[2] && a_code[3] == b_code[3] && !b_arrayed;
        case spv::OpTypeFloat:
            /* match on width */
            return a_code[2] == b_code[2] && !b_arrayed;
        case spv::OpTypeVector:
        case spv::OpTypeMatrix:
        case spv::OpTypeArray:
            /* match on element type, count. these all have the same layout. we don't get here if
             * b_arrayed -- that is handled above. */
            return !b_arrayed && types_match(a, b, a_code[2], b_code[2], b_arrayed) && a_code[3] == b_code[3];
        case spv::OpTypeStruct:
            /* match on all element types */
            {
                if (b_arrayed) {
                    /* for the purposes of matching different levels of arrayness, structs are leaves. */
                    return false;
                }

                unsigned a_len = a_code[0] >> 16;
                unsigned b_len = b_code[0] >> 16;

                if (a_len != b_len) {
                    return false;   /* structs cannot match if member counts differ */
                }

                for (unsigned i = 2; i < a_len; i++) {
                    if (!types_match(a, b, a_code[i], b_code[i], b_arrayed)) {
                        return false;
                    }
                }

                return true;
            }
        case spv::OpTypePointer:
            /* match on pointee type. storage class is expected to differ */
            return types_match(a, b, a_code[3], b_code[3], b_arrayed);

        default:
            /* remaining types are CLisms, or may not appear in the interfaces we
             * are interested in. Just claim no match.
             */
            return false;

    }
}

static int
value_or_default(std::unordered_map<unsigned, unsigned> const &map, unsigned id, int def)
{
    auto it = map.find(id);
    if (it == map.end())
        return def;
    else
        return it->second;
}

struct interface_var {
    uint32_t id;
    uint32_t type_id;
    /* TODO: collect the name, too? Isn't required to be present. */
};

static void
collect_interface_by_location(layer_data *my_data, VkDevice dev,
                              shader_module const *src, spv::StorageClass sinterface,
                              std::map<uint32_t, interface_var> &out,
                              std::map<uint32_t, interface_var> &builtins_out)
{
    unsigned int const *code = (unsigned int const *)&src->words[0];
    size_t size = src->words.size();

    std::unordered_map<unsigned, unsigned> var_locations;
    std::unordered_map<unsigned, unsigned> var_builtins;

    unsigned word = 5;
    while (word < size) {

        unsigned opcode = code[word] & 0x0ffffu;
        unsigned oplen = (code[word] & 0xffff0000u) >> 16;

        /* We consider two interface models: SSO rendezvous-by-location, and
         * builtins. Complain about anything that fits neither model.
         */
        if (opcode == spv::OpDecorate) {
            if (code[word+2] == spv::DecorationLocation) {
                var_locations[code[word+1]] = code[word+3];
            }

            if (code[word+2] == spv::DecorationBuiltIn) {
                var_builtins[code[word+1]] = code[word+3];
            }
        }

        /* TODO: handle grouped decorations */
        /* TODO: handle index=1 dual source outputs from FS -- two vars will
         * have the same location, and we DONT want to clobber. */

        if (opcode == spv::OpVariable && code[word+3] == sinterface) {
            int location = value_or_default(var_locations, code[word+2], -1);
            int builtin = value_or_default(var_builtins, code[word+2], -1);

            if (location == -1 && builtin == -1) {
                /* No location defined, and not bound to an API builtin.
                 * The spec says nothing about how this case works (or doesn't)
                 * for interface matching.
                 */
                log_msg(my_data->report_data, VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_DEVICE, /*dev*/0, 0, SHADER_CHECKER_INCONSISTENT_SPIRV, "SC",
                        "var %d (type %d) in %s interface has no Location or Builtin decoration",
                        code[word+2], code[word+1], storage_class_name(sinterface));
            }
            else if (location != -1) {
                /* A user-defined interface variable, with a location. */
                interface_var v;
                v.id = code[word+2];
                v.type_id = code[word+1];
                out[location] = v;
            }
            else {
                /* A builtin interface variable */
                interface_var v;
                v.id = code[word+2];
                v.type_id = code[word+1];
                builtins_out[builtin] = v;
            }
        }

        word += oplen;
    }
}

static void
collect_interface_by_descriptor_slot(layer_data *my_data, VkDevice dev,
                              shader_module const *src, spv::StorageClass sinterface,
                              std::map<std::pair<unsigned, unsigned>, interface_var> &out)
{
    unsigned int const *code = (unsigned int const *)&src->words[0];
    size_t size = src->words.size();

    std::unordered_map<unsigned, unsigned> var_sets;
    std::unordered_map<unsigned, unsigned> var_bindings;

    unsigned word = 5;
    while (word < size) {

        unsigned opcode = code[word] & 0x0ffffu;
        unsigned oplen = (code[word] & 0xffff0000u) >> 16;

        /* We consider two interface models: SSO rendezvous-by-location, and
         * builtins. Complain about anything that fits neither model.
         */
        if (opcode == spv::OpDecorate) {
            if (code[word+2] == spv::DecorationDescriptorSet) {
                var_sets[code[word+1]] = code[word+3];
            }

            if (code[word+2] == spv::DecorationBinding) {
                var_bindings[code[word+1]] = code[word+3];
            }
        }

        if (opcode == spv::OpVariable && (code[word+3] == spv::StorageClassUniform ||
                                          code[word+3] == spv::StorageClassUniformConstant)) {
            unsigned set = value_or_default(var_sets, code[word+2], 0);
            unsigned binding = value_or_default(var_bindings, code[word+2], 0);

            auto existing_it = out.find(std::make_pair(set, binding));
            if (existing_it != out.end()) {
                /* conflict within spv image */
                log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE, /*dev*/0, 0,
                        SHADER_CHECKER_INCONSISTENT_SPIRV, "SC",
                        "var %d (type %d) in %s interface in descriptor slot (%u,%u) conflicts with existing definition",
                        code[word+2], code[word+1], storage_class_name(sinterface),
                        existing_it->first.first, existing_it->first.second);
            }

            interface_var v;
            v.id = code[word+2];
            v.type_id = code[word+1];
            out[std::make_pair(set, binding)] = v;
        }

        word += oplen;
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateShaderModule(
        VkDevice device,
        const VkShaderModuleCreateInfo *pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkShaderModule *pShaderModule)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;
    if (!shader_is_spirv(pCreateInfo)) {
        skip_call |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE,
                /* dev */ 0, 0, SHADER_CHECKER_NON_SPIRV_SHADER, "SC",
                "Shader is not SPIR-V");
    }

    if (skip_call)
        return VK_ERROR_VALIDATION_FAILED;

    VkResult res = my_data->device_dispatch_table->CreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);

    if (res == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        my_data->shader_module_map[*pShaderModule] = new shader_module(pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return res;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateRenderPass(
        VkDevice device,
        const VkRenderPassCreateInfo *pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkRenderPass *pRenderPass)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult res = my_data->device_dispatch_table->CreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);

    loader_platform_thread_lock_mutex(&globalLock);
    my_data->render_pass_map[*pRenderPass] = new render_pass(pCreateInfo);
    loader_platform_thread_unlock_mutex(&globalLock);
    return res;
}

static bool
validate_interface_between_stages(layer_data *my_data, VkDevice dev,
                                  shader_module const *producer, char const *producer_name,
                                  shader_module const *consumer, char const *consumer_name,
                                  bool consumer_arrayed_input)
{
    std::map<uint32_t, interface_var> outputs;
    std::map<uint32_t, interface_var> inputs;

    std::map<uint32_t, interface_var> builtin_outputs;
    std::map<uint32_t, interface_var> builtin_inputs;

    bool pass = true;

    collect_interface_by_location(my_data, dev, producer, spv::StorageClassOutput, outputs, builtin_outputs);
    collect_interface_by_location(my_data, dev, consumer, spv::StorageClassInput, inputs, builtin_inputs);

    auto a_it = outputs.begin();
    auto b_it = inputs.begin();

    /* maps sorted by key (location); walk them together to find mismatches */
    while ((outputs.size() > 0 && a_it != outputs.end()) || ( inputs.size() && b_it != inputs.end())) {
        bool a_at_end = outputs.size() == 0 || a_it == outputs.end();
        bool b_at_end = inputs.size() == 0  || b_it == inputs.end();
        auto a_first = a_at_end ? 0 : a_it->first;
        auto b_first = b_at_end ? 0 : b_it->first;

        if (b_at_end || ((!a_at_end) && (a_first < b_first))) {
            if (log_msg(my_data->report_data, VK_DBG_REPORT_PERF_WARN_BIT, VK_OBJECT_TYPE_DEVICE, /*dev*/0, 0, SHADER_CHECKER_OUTPUT_NOT_CONSUMED, "SC",
                    "%s writes to output location %d which is not consumed by %s", producer_name, a_first, consumer_name)) {
                pass = false;
            }
            a_it++;
        }
        else if (a_at_end || a_first > b_first) {
            if (log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE, /*dev*/0, 0, SHADER_CHECKER_INPUT_NOT_PRODUCED, "SC",
                    "%s consumes input location %d which is not written by %s", consumer_name, b_first, producer_name)) {
                pass = false;
            }
            b_it++;
        }
        else {
            if (types_match(producer, consumer, a_it->second.type_id, b_it->second.type_id, consumer_arrayed_input)) {
                /* OK! */
            }
            else {
                char producer_type[1024];
                char consumer_type[1024];
                describe_type(producer_type, producer, a_it->second.type_id);
                describe_type(consumer_type, consumer, b_it->second.type_id);

                if (log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE, /*dev*/0, 0, SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC",
                        "Type mismatch on location %d: '%s' vs '%s'", a_it->first, producer_type, consumer_type)) {
                pass = false;
                }
            }
            a_it++;
            b_it++;
        }
    }

    return pass;
}


enum FORMAT_TYPE {
    FORMAT_TYPE_UNDEFINED,
    FORMAT_TYPE_FLOAT,  /* UNORM, SNORM, FLOAT, USCALED, SSCALED, SRGB -- anything we consider float in the shader */
    FORMAT_TYPE_SINT,
    FORMAT_TYPE_UINT,
};


static unsigned
get_format_type(VkFormat fmt) {
    switch (fmt) {
    case VK_FORMAT_UNDEFINED:
        return FORMAT_TYPE_UNDEFINED;
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8G8_SINT:
    case VK_FORMAT_R8G8B8_SINT:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16G16_SINT:
    case VK_FORMAT_R16G16B16_SINT:
    case VK_FORMAT_R16G16B16A16_SINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32A32_SINT:
    case VK_FORMAT_B8G8R8_SINT:
    case VK_FORMAT_B8G8R8A8_SINT:
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        return FORMAT_TYPE_SINT;
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16G16_UINT:
    case VK_FORMAT_R16G16B16_UINT:
    case VK_FORMAT_R16G16B16A16_UINT:
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_B8G8R8_UINT:
    case VK_FORMAT_B8G8R8A8_UINT:
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
    case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        return FORMAT_TYPE_UINT;
    default:
        return FORMAT_TYPE_FLOAT;
    }
}


/* characterizes a SPIR-V type appearing in an interface to a FF stage,
 * for comparison to a VkFormat's characterization above. */
static unsigned
get_fundamental_type(shader_module const *src, unsigned type)
{
    auto type_def_it = src->type_def_index.find(type);

    if (type_def_it == src->type_def_index.end()) {
        return FORMAT_TYPE_UNDEFINED;
    }

    unsigned int const *code = (unsigned int const *)&src->words[type_def_it->second];
    unsigned opcode = code[0] & 0x0ffffu;
    switch (opcode) {
        case spv::OpTypeInt:
            return code[3] ? FORMAT_TYPE_SINT : FORMAT_TYPE_UINT;
        case spv::OpTypeFloat:
            return FORMAT_TYPE_FLOAT;
        case spv::OpTypeVector:
            return get_fundamental_type(src, code[2]);
        case spv::OpTypeMatrix:
            return get_fundamental_type(src, code[2]);
        case spv::OpTypeArray:
            return get_fundamental_type(src, code[2]);
        case spv::OpTypePointer:
            return get_fundamental_type(src, code[3]);
        default:
            return FORMAT_TYPE_UNDEFINED;
    }
}


static bool
validate_vi_consistency(layer_data *my_data, VkDevice dev, VkPipelineVertexInputStateCreateInfo const *vi)
{
    /* walk the binding descriptions, which describe the step rate and stride of each vertex buffer.
     * each binding should be specified only once.
     */
    std::unordered_map<uint32_t, VkVertexInputBindingDescription const *> bindings;
    bool pass = true;

    for (unsigned i = 0; i < vi->vertexBindingDescriptionCount; i++) {
        auto desc = &vi->pVertexBindingDescriptions[i];
        auto & binding = bindings[desc->binding];
        if (binding) {
            if (log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE, /*dev*/0, 0, SHADER_CHECKER_INCONSISTENT_VI, "SC",
                    "Duplicate vertex input binding descriptions for binding %d", desc->binding)) {
                pass = false;
            }
        }
        else {
            binding = desc;
        }
    }

    return pass;
}


static bool
validate_vi_against_vs_inputs(layer_data *my_data, VkDevice dev, VkPipelineVertexInputStateCreateInfo const *vi, shader_module const *vs)
{
    std::map<uint32_t, interface_var> inputs;
    /* we collect builtin inputs, but they will never appear in the VI state --
     * the vs builtin inputs are generated in the pipeline, not sourced from buffers (VertexID, etc)
     */
    std::map<uint32_t, interface_var> builtin_inputs;
    bool pass = true;

    collect_interface_by_location(my_data, dev, vs, spv::StorageClassInput, inputs, builtin_inputs);

    /* Build index by location */
    std::map<uint32_t, VkVertexInputAttributeDescription const *> attribs;
    if (vi) {
        for (unsigned i = 0; i < vi->vertexAttributeDescriptionCount; i++)
            attribs[vi->pVertexAttributeDescriptions[i].location] = &vi->pVertexAttributeDescriptions[i];
    }

    auto it_a = attribs.begin();
    auto it_b = inputs.begin();

    while ((attribs.size() > 0 && it_a != attribs.end()) || (inputs.size() > 0 && it_b != inputs.end())) {
        bool a_at_end = attribs.size() == 0 || it_a == attribs.end();
        bool b_at_end = inputs.size() == 0  || it_b == inputs.end();
        auto a_first = a_at_end ? 0 : it_a->first;
        auto b_first = b_at_end ? 0 : it_b->first;
        if (b_at_end || a_first < b_first) {
            if (log_msg(my_data->report_data, VK_DBG_REPORT_PERF_WARN_BIT, VK_OBJECT_TYPE_DEVICE, /*dev*/0, 0, SHADER_CHECKER_OUTPUT_NOT_CONSUMED, "SC",
                    "Vertex attribute at location %d not consumed by VS", a_first)) {
                pass = false;
            }
            it_a++;
        }
        else if (a_at_end || b_first < a_first) {
            if (log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE, /*dev*/0, 0, SHADER_CHECKER_INPUT_NOT_PRODUCED, "SC",
                    "VS consumes input at location %d but not provided", b_first)) {
                pass = false;
            }
            it_b++;
        }
        else {
            unsigned attrib_type = get_format_type(it_a->second->format);
            unsigned input_type = get_fundamental_type(vs, it_b->second.type_id);

            /* type checking */
            if (attrib_type != FORMAT_TYPE_UNDEFINED && input_type != FORMAT_TYPE_UNDEFINED && attrib_type != input_type) {
                char vs_type[1024];
                describe_type(vs_type, vs, it_b->second.type_id);
                if (log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE, /*dev*/0, 0, SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC",
                        "Attribute type of `%s` at location %d does not match VS input type of `%s`",
                        string_VkFormat(it_a->second->format), a_first, vs_type)) {
                    pass = false;
                }
            }

            /* OK! */
            it_a++;
            it_b++;
        }
    }

    return pass;
}


static bool
validate_fs_outputs_against_render_pass(layer_data *my_data, VkDevice dev, shader_module const *fs, render_pass const *rp, uint32_t subpass)
{
    const std::vector<VkFormat> &color_formats = rp->subpass_color_formats[subpass];
    std::map<uint32_t, interface_var> outputs;
    std::map<uint32_t, interface_var> builtin_outputs;
    bool pass = true;

    /* TODO: dual source blend index (spv::DecIndex, zero if not provided) */

    collect_interface_by_location(my_data, dev, fs, spv::StorageClassOutput, outputs, builtin_outputs);

    /* Check for legacy gl_FragColor broadcast: In this case, we should have no user-defined outputs,
     * and all color attachment should be UNORM/SNORM/FLOAT.
     */
#if 0
    if (builtin_outputs.find(spv::BuiltInFragColor) != builtin_outputs.end()) {
        if (outputs.size()) {
            if (log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE, /*dev*/0, 0, SHADER_CHECKER_FS_MIXED_BROADCAST, "SC",
                    "Should not have user-defined FS outputs when using broadcast")) {
                pass = false;
            }
        }

        for (unsigned i = 0; i < color_formats.size(); i++) {
            unsigned attachmentType = get_format_type(color_formats[i]);
            if (attachmentType == FORMAT_TYPE_SINT || attachmentType == FORMAT_TYPE_UINT) {
                if (log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE, /*dev*/0, 0, SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC",
                        "CB format should not be SINT or UINT when using broadcast")) {
                    pass = false;
                }
            }
        }

        return pass;
    }
#endif

    auto it = outputs.begin();
    uint32_t attachment = 0;

    /* Walk attachment list and outputs together -- this is a little overpowered since attachments
     * are currently dense, but the parallel with matching between shader stages is nice.
     */

    /* TODO: Figure out compile error with cb->attachmentCount */
    while ((outputs.size() > 0 && it != outputs.end()) || attachment < color_formats.size()) {
        if (attachment == color_formats.size() || ( it != outputs.end() && it->first < attachment)) {
            if (log_msg(my_data->report_data, VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_DEVICE, /*dev*/0, 0, SHADER_CHECKER_OUTPUT_NOT_CONSUMED, "SC",
                    "FS writes to output location %d with no matching attachment", it->first)) {
                pass = false;
            }
            it++;
        }
        else if (it == outputs.end() || it->first > attachment) {
            if (log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE, /*dev*/0, 0, SHADER_CHECKER_INPUT_NOT_PRODUCED, "SC",
                    "Attachment %d not written by FS", attachment)) {
                pass = false;
            }
            attachment++;
        }
        else {
            unsigned output_type = get_fundamental_type(fs, it->second.type_id);
            unsigned att_type = get_format_type(color_formats[attachment]);

            /* type checking */
            if (att_type != FORMAT_TYPE_UNDEFINED && output_type != FORMAT_TYPE_UNDEFINED && att_type != output_type) {
                char fs_type[1024];
                describe_type(fs_type, fs, it->second.type_id);
                if (log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE, /*dev*/0, 0, SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC",
                        "Attachment %d of type `%s` does not match FS output type of `%s`",
                        attachment, string_VkFormat(color_formats[attachment]), fs_type)) {
                    pass = false;
                }
            }

            /* OK! */
            it++;
            attachment++;
        }
    }

    return pass;
}


struct shader_stage_attributes {
    char const * const name;
    bool arrayed_input;
};


static shader_stage_attributes
shader_stage_attribs[] = {
    { "vertex shader", false },
    { "tessellation control shader", true },
    { "tessellation evaluation shader", false },
    { "geometry shader", true },
    { "fragment shader", false },
};


static bool
has_descriptor_binding(std::vector<std::unordered_set<uint32_t>*>* layout,
                       std::pair<unsigned, unsigned> slot)
{
    if (!layout)
        return false;

    if (slot.first >= layout->size())
        return false;

    auto set = (*layout)[slot.first];

    return (set->find(slot.second) != set->end());
}

static uint32_t get_shader_stage_id(VkShaderStageFlagBits stage)
{
    uint32_t bit_pos = u_ffs(stage);
    return bit_pos-1;
}

static bool
validate_graphics_pipeline(layer_data *my_data, VkDevice dev, VkGraphicsPipelineCreateInfo const *pCreateInfo)
{
    /* We seem to allow pipeline stages to be specified out of order, so collect and identify them
     * before trying to do anything more: */
    int vertex_stage = get_shader_stage_id(VK_SHADER_STAGE_VERTEX_BIT);
    int geometry_stage = get_shader_stage_id(VK_SHADER_STAGE_GEOMETRY_BIT);
    int fragment_stage = get_shader_stage_id(VK_SHADER_STAGE_FRAGMENT_BIT);

    shader_module **shaders = new shader_module*[fragment_stage + 1];  /* exclude CS */
    memset(shaders, 0, sizeof(shader_module *) * (fragment_stage +1));
    render_pass const *rp = 0;
    VkPipelineVertexInputStateCreateInfo const *vi = 0;
    bool pass = true;

    loader_platform_thread_lock_mutex(&globalLock);

    for (uint32_t i = 0; i < pCreateInfo->stageCount; i++) {
        VkPipelineShaderStageCreateInfo const *pStage = &pCreateInfo->pStages[i];
        if (pStage->sType == VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO) {

            if ((pStage->stage & (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
                                  | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)) == 0) {
                if (log_msg(my_data->report_data, VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_DEVICE, /*dev*/0, 0, SHADER_CHECKER_UNKNOWN_STAGE, "SC",
                        "Unknown shader stage %d", pStage->stage)) {
                    pass = false;
                }
            }
            else {
                shader_module *module = my_data->shader_module_map[pStage->module];
                shaders[get_shader_stage_id(pStage->stage)] = module;

                /* validate descriptor set layout against what the spirv module actually uses */
                std::map<std::pair<unsigned, unsigned>, interface_var> descriptor_uses;
                collect_interface_by_descriptor_slot(my_data, dev, module, spv::StorageClassUniform,
                        descriptor_uses);

                auto layout = pCreateInfo->layout != VK_NULL_HANDLE ?
                    my_data->pipeline_layout_map[pCreateInfo->layout] : nullptr;

                for (auto it = descriptor_uses.begin(); it != descriptor_uses.end(); it++) {

                    /* find the matching binding */
                    auto found = has_descriptor_binding(layout, it->first);

                    if (!found) {
                        char type_name[1024];
                        describe_type(type_name, module, it->second.type_id);
                        if (log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE, /*dev*/0, 0,
                                SHADER_CHECKER_MISSING_DESCRIPTOR, "SC",
                                "Shader uses descriptor slot %u.%u (used as type `%s`) but not declared in pipeline layout",
                                it->first.first, it->first.second, type_name)) {
                            pass = false;
                        }
                    }
                }
            }
        }
    }

    if (pCreateInfo->renderPass != VK_NULL_HANDLE)
        rp = my_data->render_pass_map[pCreateInfo->renderPass];

    vi = pCreateInfo->pVertexInputState;

    if (vi) {
        pass = validate_vi_consistency(my_data, dev, vi) && pass;
    }

    if (shaders[vertex_stage]) {
        pass = validate_vi_against_vs_inputs(my_data, dev, vi, shaders[vertex_stage]) && pass;
    }

    /* TODO: enforce rules about present combinations of shaders */
    int producer = get_shader_stage_id(VK_SHADER_STAGE_VERTEX_BIT);
    int consumer = get_shader_stage_id(VK_SHADER_STAGE_GEOMETRY_BIT);

    while (!shaders[producer] && producer != fragment_stage) {
        producer++;
        consumer++;
    }

    for (; producer != fragment_stage && consumer <= fragment_stage; consumer++) {
        assert(shaders[producer]);
        if (shaders[consumer]) {
            pass = validate_interface_between_stages(my_data, dev,
                                                     shaders[producer], shader_stage_attribs[producer].name,
                                                     shaders[consumer], shader_stage_attribs[consumer].name,
                                                     shader_stage_attribs[consumer].arrayed_input) && pass;

            producer = consumer;
        }
    }

    if (shaders[fragment_stage] && rp) {
        pass = validate_fs_outputs_against_render_pass(my_data, dev, shaders[fragment_stage], rp, pCreateInfo->subpass) && pass;
    }

    delete shaders;

    loader_platform_thread_unlock_mutex(&globalLock);
    return pass;
}

//TODO handle pipelineCache entry points
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkCreateGraphicsPipelines(VkDevice device,
                         VkPipelineCache pipelineCache,
                         uint32_t count,
                         const VkGraphicsPipelineCreateInfo *pCreateInfos,
                         const VkAllocationCallbacks* pAllocator,
                         VkPipeline *pPipelines)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool pass = true;
    for (uint32_t i = 0; i < count; i++) {
        pass = validate_graphics_pipeline(my_data, device, &pCreateInfos[i]) && pass;
    }

    if (pass) {
        /* The driver is allowed to crash if passed junk. Only actually create the
         * pipeline if we didn't run into any showstoppers above.
         */
        return my_data->device_dispatch_table->CreateGraphicsPipelines(device, pipelineCache, count, pCreateInfos, pAllocator, pPipelines);
    }
    else {
        return VK_ERROR_VALIDATION_FAILED;
    }
}


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
    VkResult result = my_device_data->device_dispatch_table->CreateDevice(gpu, pCreateInfo, pAllocator, pDevice);
    if (result == VK_SUCCESS) {
        layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(gpu), layer_data_map);
        my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
    }
    return result;
}

/* hook DextroyDevice to remove tableMap entry */
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_device_data = get_my_data_ptr(key, layer_data_map);
    my_device_data->device_dispatch_table->DestroyDevice(device, pAllocator);
    delete my_device_data->device_dispatch_table;
    layer_data_map.erase(key);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(
    const VkInstanceCreateInfo*                 pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkInstance*                                 pInstance)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    VkResult result = pTable->CreateInstance(pCreateInfo, pAllocator, pInstance);

    if (result == VK_SUCCESS) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
        my_data->report_data = debug_report_create_instance(
                                   pTable,
                                   *pInstance,
                                   pCreateInfo->enabledExtensionNameCount,
                                   pCreateInfo->ppEnabledExtensionNames);

        init_shader_checker(my_data);
    }
    return result;
}

/* hook DestroyInstance to remove tableInstanceMap entry */
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
    dispatch_key key = get_dispatch_key(instance);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    my_data->instance_dispatch_table->DestroyInstance(instance, pAllocator);

    // Clean up logging callback, if any
    while (my_data->logging_callback.size() > 0) {
        VkDbgMsgCallback callback = my_data->logging_callback.back();
        layer_destroy_msg_callback(my_data->report_data, callback);
        my_data->logging_callback.pop_back();
    }

    layer_debug_report_destroy_instance(my_data->report_data);
    delete my_data->instance_dispatch_table;
    layer_data_map.erase(key);
    if (layer_data_map.empty()) {
        // Release mutex when destroying last instance.
        loader_platform_thread_delete_mutex(&globalLock);
        globalLockInitialized = 0;
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkDbgCreateMsgCallback(
    VkInstance                          instance,
    VkFlags                             msgFlags,
    const PFN_vkDbgMsgCallback          pfnMsgCallback,
    void*                               pUserData,
    VkDbgMsgCallback*                   pMsgCallback)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkResult res = my_data->instance_dispatch_table->DbgCreateMsgCallback(instance, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
    if (VK_SUCCESS == res) {
        res = layer_create_msg_callback(my_data->report_data, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
    }
    return res;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkDbgDestroyMsgCallback(
    VkInstance                          instance,
    VkDbgMsgCallback                    msgCallback)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkResult res = my_data->instance_dispatch_table->DbgDestroyMsgCallback(instance, msgCallback);
    layer_destroy_msg_callback(my_data->report_data, msgCallback);
    return res;
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev, const char* funcName)
{
    if (dev == NULL)
        return NULL;

    layer_data *my_data;
    /* loader uses this to force layer initialization; device object is wrapped */
    if (!strcmp("vkGetDeviceProcAddr", funcName)) {
        VkBaseLayerObject* wrapped_dev = (VkBaseLayerObject*) dev;
        my_data = get_my_data_ptr(get_dispatch_key(wrapped_dev->baseObject), layer_data_map);
        my_data->device_dispatch_table = new VkLayerDispatchTable;
        layer_initialize_dispatch_table(my_data->device_dispatch_table, wrapped_dev);
        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;
    }
    my_data = get_my_data_ptr(get_dispatch_key(dev), layer_data_map);

#define ADD_HOOK(fn)    \
    if (!strncmp(#fn, funcName, sizeof(#fn))) \
        return (PFN_vkVoidFunction) fn

    ADD_HOOK(vkCreateDevice);
    ADD_HOOK(vkCreateShaderModule);
    ADD_HOOK(vkCreateRenderPass);
    ADD_HOOK(vkDestroyDevice);
    ADD_HOOK(vkCreateGraphicsPipelines);
    ADD_HOOK(vkCreateDescriptorSetLayout);
    ADD_HOOK(vkCreatePipelineLayout);
#undef ADD_HOOK

    VkLayerDispatchTable* pTable = my_data->device_dispatch_table;
    {
        if (pTable->GetDeviceProcAddr == NULL)
            return NULL;
        return pTable->GetDeviceProcAddr(dev, funcName);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    PFN_vkVoidFunction fptr;

    if (instance == NULL)
        return NULL;

    layer_data *my_data;
    if (!strcmp("vkGetInstanceProcAddr", funcName)) {
        VkBaseLayerObject* wrapped_inst = (VkBaseLayerObject*) instance;
        my_data = get_my_data_ptr(get_dispatch_key(wrapped_inst->baseObject), layer_data_map);
        my_data->instance_dispatch_table = new VkLayerInstanceDispatchTable;
        layer_init_instance_dispatch_table(my_data->instance_dispatch_table, wrapped_inst);
        return (PFN_vkVoidFunction) vkGetInstanceProcAddr;
    }
#define ADD_HOOK(fn)    \
    if (!strncmp(#fn, funcName, sizeof(#fn))) \
        return (PFN_vkVoidFunction) fn

    ADD_HOOK(vkCreateInstance);
    ADD_HOOK(vkDestroyInstance);
    ADD_HOOK(vkEnumerateInstanceExtensionProperties);
    ADD_HOOK(vkEnumerateDeviceExtensionProperties);
    ADD_HOOK(vkEnumerateInstanceLayerProperties);
    ADD_HOOK(vkEnumerateDeviceLayerProperties);
#undef ADD_HOOK


    my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    fptr = debug_report_get_instance_proc_addr(my_data->report_data, funcName);
    if (fptr)
        return fptr;

    {
        VkLayerInstanceDispatchTable* pTable = my_data->instance_dispatch_table;
        if (pTable->GetInstanceProcAddr == NULL)
            return NULL;
        return pTable->GetInstanceProcAddr(instance, funcName);
    }
}
