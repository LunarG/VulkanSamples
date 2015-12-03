/*
 *
 * Copyright (C) 2015 Valve Corporation
 * Copyright (C) 2015 Google, Inc.
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
 * Author: Cody Northrop <cnorthrop@google.com>
 * Author: Michael Lentine <mlentine@google.com>
 * Author: Tobin Ehlis <tobine@google.com>
 * Author: Chia-I Wu <olv@google.com>
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <string>
#include <iostream>
#include <algorithm>
#include <list>
#include <spirv.hpp>
#include <set>

#include "vk_loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vk_struct_string_helper_cpp.h"
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic warning "-Wwrite-strings"
#endif
#include "vk_struct_size_helper.h"
#include "draw_state.h"
#include "vk_layer_config.h"
#include "vulkan/vk_debug_marker_layer.h"
#include "vk_layer_table.h"
#include "vk_layer_debug_marker_table.h"
#include "vk_layer_data.h"
#include "vk_layer_logging.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_utils.h"

// This definition controls whether image layout transitions are enabled/disabled.
// disable until corner cases are fixed
#define DISABLE_IMAGE_LAYOUT_VALIDATION

using std::unordered_map;
using std::unordered_set;

struct devExts {
    VkBool32 debug_marker_enabled;
    VkBool32 wsi_enabled;
    unordered_map<VkSwapchainKHR, SWAPCHAIN_NODE*> swapchainMap;
};

// fwd decls
struct shader_module;
struct render_pass;

struct layer_data {
    debug_report_data* report_data;
    std::vector<VkDbgMsgCallback> logging_callback;
    VkLayerDispatchTable* device_dispatch_table;
    VkLayerInstanceDispatchTable* instance_dispatch_table;
    devExts device_extensions;
    // Layer specific data
    unordered_map<VkSampler,             unique_ptr<SAMPLER_NODE>>           sampleMap;
    unordered_map<VkImageView,           unique_ptr<VkImageViewCreateInfo>>  imageViewMap;
    unordered_map<VkImage,               unique_ptr<VkImageCreateInfo>>      imageMap;
    unordered_map<VkBufferView,          unique_ptr<VkBufferViewCreateInfo>> bufferViewMap;
    unordered_map<VkBuffer,              unique_ptr<VkBufferCreateInfo>>     bufferMap;
    unordered_map<VkPipeline,            PIPELINE_NODE*>                     pipelineMap;
    unordered_map<VkCommandPool,         list<VkCommandBuffer>>              commandPoolMap;
    unordered_map<VkDescriptorPool,      DESCRIPTOR_POOL_NODE*>              descriptorPoolMap;
    unordered_map<VkDescriptorSet,       SET_NODE*>                          setMap;
    unordered_map<VkDescriptorSetLayout, LAYOUT_NODE*>                       descriptorSetLayoutMap;
    unordered_map<VkPipelineLayout,      PIPELINE_LAYOUT_NODE>               pipelineLayoutMap;
    unordered_map<VkDeviceMemory,        VkImage>                            memImageMap;
    // Map for layout chains
    unordered_map<void*,                 GLOBAL_CB_NODE*>                    commandBufferMap;
    unordered_map<VkFramebuffer,         VkFramebufferCreateInfo*>           frameBufferMap;
    unordered_map<VkImage,               IMAGE_NODE*>                        imageLayoutMap;
    unordered_map<VkRenderPass,          RENDER_PASS_NODE*>                  renderPassMap;
    unordered_map<VkShaderModule, shader_module*>                            shaderModuleMap;
    // Current render pass
    VkRenderPassBeginInfo renderPassBeginInfo;
    uint32_t currentSubpass;

    layer_data() :
        report_data(nullptr),
        device_dispatch_table(nullptr),
        instance_dispatch_table(nullptr),
        device_extensions()
    {};
};
// Code imported from ShaderChecker
static void
build_type_def_index(std::vector<unsigned> const &words, std::unordered_map<unsigned, unsigned> &type_def_index);

struct shader_module {
    /* the spirv image itself */
    vector<uint32_t> words;
    /* a mapping of <id> to the first word of its def. this is useful because walking type
     * trees requires jumping all over the instruction stream.
     */
    unordered_map<unsigned, unsigned> type_def_index;

    shader_module(VkShaderModuleCreateInfo const *pCreateInfo) :
        words((uint32_t *)pCreateInfo->pCode, (uint32_t *)pCreateInfo->pCode + pCreateInfo->codeSize / sizeof(uint32_t)),
        type_def_index() {

        build_type_def_index(words, type_def_index);
    }
};

// TODO : Do we need to guard access to layer_data_map w/ lock?
static unordered_map<void*, layer_data*> layer_data_map;

static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(g_initOnce);
// TODO : This can be much smarter, using separate locks for separate global data
static int globalLockInitialized = 0;
static loader_platform_thread_mutex globalLock;
#define MAX_TID 513
static loader_platform_thread_id g_tidMapping[MAX_TID] = {0};
static uint32_t g_maxTID = 0;

template layer_data *get_my_data_ptr<layer_data>(
        void *data_key,
        std::unordered_map<void *, layer_data *> &data_map);

// Map actual TID to an index value and return that index
//  This keeps TIDs in range from 0-MAX_TID and simplifies compares between runs
static uint32_t getTIDIndex() {
    loader_platform_thread_id tid = loader_platform_get_thread_id();
    for (uint32_t i = 0; i < g_maxTID; i++) {
        if (tid == g_tidMapping[i])
            return i;
    }
    // Don't yet have mapping, set it and return newly set index
    uint32_t retVal = (uint32_t) g_maxTID;
    g_tidMapping[g_maxTID++] = tid;
    assert(g_maxTID < MAX_TID);
    return retVal;
}

// Return a string representation of CMD_TYPE enum
static string cmdTypeToString(CMD_TYPE cmd)
{
    switch (cmd)
    {
        case CMD_BINDPIPELINE:
            return "CMD_BINDPIPELINE";
        case CMD_BINDPIPELINEDELTA:
            return "CMD_BINDPIPELINEDELTA";
        case CMD_SETVIEWPORTSTATE:
            return "CMD_SETVIEWPORTSTATE";
        case CMD_SETLINEWIDTHSTATE:
            return "CMD_SETLINEWIDTHSTATE";
        case CMD_SETDEPTHBIASSTATE:
            return "CMD_SETDEPTHBIASSTATE";
        case CMD_SETBLENDSTATE:
            return "CMD_SETBLENDSTATE";
        case CMD_SETDEPTHBOUNDSSTATE:
            return "CMD_SETDEPTHBOUNDSSTATE";
        case CMD_SETSTENCILREADMASKSTATE:
            return "CMD_SETSTENCILREADMASKSTATE";
        case CMD_SETSTENCILWRITEMASKSTATE:
            return "CMD_SETSTENCILWRITEMASKSTATE";
        case CMD_SETSTENCILREFERENCESTATE:
            return "CMD_SETSTENCILREFERENCESTATE";
        case CMD_BINDDESCRIPTORSETS:
            return "CMD_BINDDESCRIPTORSETS";
        case CMD_BINDINDEXBUFFER:
            return "CMD_BINDINDEXBUFFER";
        case CMD_BINDVERTEXBUFFER:
            return "CMD_BINDVERTEXBUFFER";
        case CMD_DRAW:
            return "CMD_DRAW";
        case CMD_DRAWINDEXED:
            return "CMD_DRAWINDEXED";
        case CMD_DRAWINDIRECT:
            return "CMD_DRAWINDIRECT";
        case CMD_DRAWINDEXEDINDIRECT:
            return "CMD_DRAWINDEXEDINDIRECT";
        case CMD_DISPATCH:
            return "CMD_DISPATCH";
        case CMD_DISPATCHINDIRECT:
            return "CMD_DISPATCHINDIRECT";
        case CMD_COPYBUFFER:
            return "CMD_COPYBUFFER";
        case CMD_COPYIMAGE:
            return "CMD_COPYIMAGE";
        case CMD_BLITIMAGE:
            return "CMD_BLITIMAGE";
        case CMD_COPYBUFFERTOIMAGE:
            return "CMD_COPYBUFFERTOIMAGE";
        case CMD_COPYIMAGETOBUFFER:
            return "CMD_COPYIMAGETOBUFFER";
        case CMD_CLONEIMAGEDATA:
            return "CMD_CLONEIMAGEDATA";
        case CMD_UPDATEBUFFER:
            return "CMD_UPDATEBUFFER";
        case CMD_FILLBUFFER:
            return "CMD_FILLBUFFER";
        case CMD_CLEARCOLORIMAGE:
            return "CMD_CLEARCOLORIMAGE";
        case CMD_CLEARATTACHMENTS:
            return "CMD_CLEARCOLORATTACHMENT";
        case CMD_CLEARDEPTHSTENCILIMAGE:
            return "CMD_CLEARDEPTHSTENCILIMAGE";
        case CMD_RESOLVEIMAGE:
            return "CMD_RESOLVEIMAGE";
        case CMD_SETEVENT:
            return "CMD_SETEVENT";
        case CMD_RESETEVENT:
            return "CMD_RESETEVENT";
        case CMD_WAITEVENTS:
            return "CMD_WAITEVENTS";
        case CMD_PIPELINEBARRIER:
            return "CMD_PIPELINEBARRIER";
        case CMD_BEGINQUERY:
            return "CMD_BEGINQUERY";
        case CMD_ENDQUERY:
            return "CMD_ENDQUERY";
        case CMD_RESETQUERYPOOL:
            return "CMD_RESETQUERYPOOL";
        case CMD_COPYQUERYPOOLRESULTS:
            return "CMD_COPYQUERYPOOLRESULTS";
        case CMD_WRITETIMESTAMP:
            return "CMD_WRITETIMESTAMP";
        case CMD_INITATOMICCOUNTERS:
            return "CMD_INITATOMICCOUNTERS";
        case CMD_LOADATOMICCOUNTERS:
            return "CMD_LOADATOMICCOUNTERS";
        case CMD_SAVEATOMICCOUNTERS:
            return "CMD_SAVEATOMICCOUNTERS";
        case CMD_BEGINRENDERPASS:
            return "CMD_BEGINRENDERPASS";
        case CMD_ENDRENDERPASS:
            return "CMD_ENDRENDERPASS";
        case CMD_DBGMARKERBEGIN:
            return "CMD_DBGMARKERBEGIN";
        case CMD_DBGMARKEREND:
            return "CMD_DBGMARKEREND";
        default:
            return "UNKNOWN";
    }
}

// SPIRV utility functions
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


static unsigned
get_locations_consumed_by_type(shader_module const *src, unsigned type, bool strip_array_level)
{
    auto type_def_it = src->type_def_index.find(type);

    if (type_def_it == src->type_def_index.end()) {
        return 1;       /* This is actually broken SPIR-V... */
    }

    unsigned int const *code = (unsigned int const *)&src->words[type_def_it->second];
    unsigned opcode = code[0] & 0x0ffffu;

    switch (opcode) {
        case spv::OpTypePointer:
            /* see through the ptr -- this is only ever at the toplevel for graphics shaders;
             * we're never actually passing pointers around. */
            return get_locations_consumed_by_type(src, code[3], strip_array_level);
        case spv::OpTypeArray:
            if (strip_array_level) {
                return get_locations_consumed_by_type(src, code[2], false);
            }
            else {
                return code[3] * get_locations_consumed_by_type(src, code[2], false);
            }
        case spv::OpTypeMatrix:
            /* num locations is the dimension * element size */
            return code[3] * get_locations_consumed_by_type(src, code[2], false);
        default:
            /* everything else is just 1. */
            return 1;

        /* TODO: extend to handle 64bit scalar types, whose vectors may need
         * multiple locations. */
    }
}


struct interface_var {
    uint32_t id;
    uint32_t type_id;
    uint32_t offset;
    /* TODO: collect the name, too? Isn't required to be present. */
};

static void
collect_interface_by_location(layer_data *my_data, VkDevice dev,
                              shader_module const *src, spv::StorageClass sinterface,
                              std::map<uint32_t, interface_var> &out,
                              std::map<uint32_t, interface_var> &builtins_out,
                              bool is_array_of_verts)
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
            unsigned id = code[word+2];
            unsigned type = code[word+1];

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
                /* A user-defined interface variable, with a location. Where a variable
                 * occupied multiple locations, emit one result for each. */
                unsigned num_locations = get_locations_consumed_by_type(src, type,
                        is_array_of_verts);
                for (int offset = 0; offset < num_locations; offset++) {
                    interface_var v;
                    v.id = id;
                    v.type_id = type;
                    v.offset = offset;
                    out[location + offset] = v;
                }
            }
            else {
                /* A builtin interface variable */
                /* Note that since builtin interface variables do not consume numbered
                 * locations, there is no larger-than-vec4 consideration as above
                 */
                interface_var v;
                v.id = id;
                v.type_id = type;
                v.offset = 0;
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

        /* All variables in the Uniform or UniformConstant storage classes are required to be decorated with both
         * DecorationDescriptorSet and DecorationBinding.
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

    collect_interface_by_location(my_data, dev, producer, spv::StorageClassOutput, outputs, builtin_outputs, false);
    collect_interface_by_location(my_data, dev, consumer, spv::StorageClassInput, inputs, builtin_inputs,
            consumer_arrayed_input);

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

    collect_interface_by_location(my_data, dev, vs, spv::StorageClassInput, inputs, builtin_inputs, false);

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
validate_fs_outputs_against_render_pass(layer_data *my_data, VkDevice dev, shader_module const *fs, RENDER_PASS_NODE const *rp, uint32_t subpass)
{
    const std::vector<VkFormat> &color_formats = rp->subpassColorFormats[subpass];
    std::map<uint32_t, interface_var> outputs;
    std::map<uint32_t, interface_var> builtin_outputs;
    bool pass = true;

    /* TODO: dual source blend index (spv::DecIndex, zero if not provided) */

    collect_interface_by_location(my_data, dev, fs, spv::StorageClassOutput, outputs, builtin_outputs, false);

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

// For given pipelineLayout verify that the setLayout at slot.first
//  has the requested binding at slot.second
static bool
has_descriptor_binding(layer_data* my_data,
                       vector<VkDescriptorSetLayout>* pipelineLayout,
                       std::pair<unsigned, unsigned> slot)
{
    if (!pipelineLayout)
        return false;

    if (slot.first >= pipelineLayout->size())
        return false;

    auto set = my_data->descriptorSetLayoutMap[(*pipelineLayout)[slot.first]]->bindings;

    return (set.find(slot.second) != set.end());
}

static uint32_t get_shader_stage_id(VkShaderStageFlagBits stage)
{
    uint32_t bit_pos = u_ffs(stage);
    return bit_pos-1;
}

// Block of code at start here for managing/tracking Pipeline state that this layer cares about

static uint64_t g_drawCount[NUM_DRAW_TYPES] = {0, 0, 0, 0};

// TODO : Should be tracking lastBound per commandBuffer and when draws occur, report based on that cmd buffer lastBound
//   Then need to synchronize the accesses based on cmd buffer so that if I'm reading state on one cmd buffer, updates
//   to that same cmd buffer by separate thread are not changing state from underneath us
// Track the last cmd buffer touched by this thread

// Track the last global DrawState of interest touched by any thread
static PIPELINE_NODE*  g_lastBoundPipeline = NULL;
#define MAX_BINDING 0xFFFFFFFF // Default vtxBinding value in CB Node to identify if no vtxBinding set
// prototype
static GLOBAL_CB_NODE* getCBNode(layer_data*, const VkCommandBuffer);

static VkBool32 hasDrawCmd(GLOBAL_CB_NODE* pCB)
{
    for (uint32_t i=0; i<NUM_DRAW_TYPES; i++) {
        if (pCB->drawCount[i])
            return VK_TRUE;
    }
    return VK_FALSE;
}

// Check object status for selected flag state
static VkBool32 validate_status(layer_data* my_data, GLOBAL_CB_NODE* pNode, CBStatusFlags enable_mask, CBStatusFlags status_mask, CBStatusFlags status_flag, VkFlags msg_flags, DRAW_STATE_ERROR error_code, const char* fail_msg)
{
    // If non-zero enable mask is present, check it against status but if enable_mask
    //  is 0 then no enable required so we should always just check status
    if ((!enable_mask) || (enable_mask & pNode->status)) {
        if ((pNode->status & status_mask) != status_flag) {
            // TODO : How to pass dispatchable objects as srcObject? Here src obj should be cmd buffer
            return log_msg(my_data->report_data, msg_flags, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, error_code, "DS",
                    "CB object %#" PRIxLEAST64 ": %s", reinterpret_cast<uint64_t>(pNode->commandBuffer), fail_msg);
        }
    }
    return VK_FALSE;
}

// Retrieve pipeline node ptr for given pipeline object
static PIPELINE_NODE* getPipeline(layer_data* my_data, const VkPipeline pipeline)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (my_data->pipelineMap.find(pipeline) == my_data->pipelineMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return my_data->pipelineMap[pipeline];
}

// Return VK_TRUE if for a given PSO, the given state enum is dynamic, else return VK_FALSE
static VkBool32 isDynamic(const PIPELINE_NODE* pPipeline, const VkDynamicState state)
{
    if (pPipeline && pPipeline->graphicsPipelineCI.pDynamicState) {
        for (uint32_t i=0; i<pPipeline->graphicsPipelineCI.pDynamicState->dynamicStateCount; i++) {
            if (state == pPipeline->graphicsPipelineCI.pDynamicState->pDynamicStates[i])
                return VK_TRUE;
        }
    }
    return VK_FALSE;
}

// Validate state stored as flags at time of draw call
static VkBool32 validate_draw_state_flags(layer_data* my_data, GLOBAL_CB_NODE* pCB, VkBool32 indexedDraw) {
    VkBool32 result;
    result = validate_status(my_data, pCB, CBSTATUS_NONE, CBSTATUS_VIEWPORT_SET, CBSTATUS_VIEWPORT_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_VIEWPORT_NOT_BOUND, "Dynamic viewport state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_NONE, CBSTATUS_SCISSOR_SET, CBSTATUS_SCISSOR_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_SCISSOR_NOT_BOUND, "Dynamic scissor state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_NONE, CBSTATUS_LINE_WIDTH_SET, CBSTATUS_LINE_WIDTH_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_LINE_WIDTH_NOT_BOUND, "Dynamic line width state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_NONE, CBSTATUS_DEPTH_BIAS_SET, CBSTATUS_DEPTH_BIAS_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_DEPTH_BIAS_NOT_BOUND, "Dynamic depth bias state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_COLOR_BLEND_WRITE_ENABLE, CBSTATUS_BLEND_SET, CBSTATUS_BLEND_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_BLEND_NOT_BOUND, "Dynamic blend object state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_DEPTH_WRITE_ENABLE, CBSTATUS_DEPTH_BOUNDS_SET, CBSTATUS_DEPTH_BOUNDS_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_DEPTH_BOUNDS_NOT_BOUND, "Dynamic depth bounds state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_STENCIL_TEST_ENABLE, CBSTATUS_STENCIL_READ_MASK_SET, CBSTATUS_STENCIL_READ_MASK_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_STENCIL_NOT_BOUND, "Dynamic stencil read mask state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_STENCIL_TEST_ENABLE, CBSTATUS_STENCIL_WRITE_MASK_SET, CBSTATUS_STENCIL_WRITE_MASK_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_STENCIL_NOT_BOUND, "Dynamic stencil write mask state not set for this command buffer");
    result |= validate_status(my_data, pCB, CBSTATUS_STENCIL_TEST_ENABLE, CBSTATUS_STENCIL_REFERENCE_SET, CBSTATUS_STENCIL_REFERENCE_SET, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_STENCIL_NOT_BOUND, "Dynamic stencil reference state not set for this command buffer");
    if (indexedDraw)
        result |= validate_status(my_data, pCB, CBSTATUS_NONE, CBSTATUS_INDEX_BUFFER_BOUND, CBSTATUS_INDEX_BUFFER_BOUND, VK_DBG_REPORT_ERROR_BIT, DRAWSTATE_INDEX_BUFFER_NOT_BOUND, "Index buffer object not bound to this command buffer when Indexed Draw attempted");
    return result;
}

// For give SET_NODE, verify that its Set is compatible w/ the setLayout corresponding to pipelineLayout[layoutIndex]
static bool verify_set_layout_compatibility(layer_data* my_data, const SET_NODE* pSet, const VkPipelineLayout layout, const uint32_t layoutIndex, string& errorMsg)
{
    stringstream errorStr;
    if (my_data->pipelineLayoutMap.find(layout) == my_data->pipelineLayoutMap.end()) {
        errorStr << "invalid VkPipelineLayout (" << layout << ")";
        errorMsg = errorStr.str();
        return false;
    }
    PIPELINE_LAYOUT_NODE pl = my_data->pipelineLayoutMap[layout];
    if (layoutIndex >= pl.descriptorSetLayouts.size()) {
        errorStr << "VkPipelineLayout (" << layout << ") only contains " << pl.descriptorSetLayouts.size() << " setLayouts corresponding to sets 0-" << pl.descriptorSetLayouts.size()-1 << ", but you're attempting to bind set to index " << layoutIndex;
        errorMsg = errorStr.str();
        return false;
    }
    // Get the specific setLayout from PipelineLayout that overlaps this set
    LAYOUT_NODE* pLayoutNode = my_data->descriptorSetLayoutMap[pl.descriptorSetLayouts[layoutIndex]];
    if (pLayoutNode->layout == pSet->pLayout->layout) { // trivial pass case
        return true;
    }
    uint32_t descriptorCount = pLayoutNode->descriptorTypes.size();
    if (descriptorCount != pSet->pLayout->descriptorTypes.size()) {
        errorStr << "setLayout " << layoutIndex << " from pipelineLayout " << layout << " has " << descriptorCount << " descriptors, but corresponding set being bound has " << pSet->pLayout->descriptorTypes.size() << " descriptors.";
        errorMsg = errorStr.str();
        return false; // trivial fail case
    }
    // Now need to check set against corresponding pipelineLayout to verify compatibility
    for (uint32_t i=0; i<descriptorCount; ++i) {
        // Need to verify that layouts are identically defined
        //  TODO : Is below sufficient? Making sure that types & stageFlags match per descriptor
        //    do we also need to check immutable samplers?
        if (pLayoutNode->descriptorTypes[i] != pSet->pLayout->descriptorTypes[i]) {
            errorStr << "descriptor " << i << " for descriptorSet being bound is type '" << string_VkDescriptorType(pSet->pLayout->descriptorTypes[i]) << "' but corresponding descriptor from pipelineLayout is type '" << string_VkDescriptorType(pLayoutNode->descriptorTypes[i]) << "'";
            errorMsg = errorStr.str();
            return false;
        }
        if (pLayoutNode->stageFlags[i] != pSet->pLayout->stageFlags[i]) {
            errorStr << "stageFlags " << i << " for descriptorSet being bound is " << pSet->pLayout->stageFlags[i] << "' but corresponding descriptor from pipelineLayout has stageFlags " << pLayoutNode->stageFlags[i];
            errorMsg = errorStr.str();
            return false;
        }
    }
    return true;
}

// Validate that the shaders used by the given pipeline
//  As a side effect this function also records the sets that are actually used by the pipeline
static bool
validate_pipeline_shaders(layer_data *my_data, VkDevice dev, PIPELINE_NODE* pPipeline)
{
    VkGraphicsPipelineCreateInfo const *pCreateInfo = &pPipeline->graphicsPipelineCI;
    /* We seem to allow pipeline stages to be specified out of order, so collect and identify them
     * before trying to do anything more: */
    int vertex_stage = get_shader_stage_id(VK_SHADER_STAGE_VERTEX_BIT);
    int geometry_stage = get_shader_stage_id(VK_SHADER_STAGE_GEOMETRY_BIT);
    int fragment_stage = get_shader_stage_id(VK_SHADER_STAGE_FRAGMENT_BIT);

    shader_module **shaders = new shader_module*[fragment_stage + 1];  /* exclude CS */
    memset(shaders, 0, sizeof(shader_module *) * (fragment_stage +1));
    RENDER_PASS_NODE const *rp = 0;
    VkPipelineVertexInputStateCreateInfo const *vi = 0;
    bool pass = true;

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
                shader_module *module = my_data->shaderModuleMap[pStage->module];
                shaders[get_shader_stage_id(pStage->stage)] = module;

                /* validate descriptor set layout against what the spirv module actually uses */
                std::map<std::pair<unsigned, unsigned>, interface_var> descriptor_uses;
                collect_interface_by_descriptor_slot(my_data, dev, module, spv::StorageClassUniform,
                        descriptor_uses);

                auto layouts = pCreateInfo->layout != VK_NULL_HANDLE ?
                    &(my_data->pipelineLayoutMap[pCreateInfo->layout].descriptorSetLayouts) : nullptr;

                for (auto it = descriptor_uses.begin(); it != descriptor_uses.end(); it++) {
                    // As a side-effect of this function, capture which sets are used by the pipeline
                    pPipeline->active_sets.insert(it->first.first);

                    /* find the matching binding */
                    auto found = has_descriptor_binding(my_data, layouts, it->first);

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
        rp = my_data->renderPassMap[pCreateInfo->renderPass];

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

    return pass;
}

// Validate overall state at the time of a draw call
static VkBool32 validate_draw_state(layer_data* my_data, GLOBAL_CB_NODE* pCB, VkBool32 indexedDraw) {
    // First check flag states
    VkBool32 result = validate_draw_state_flags(my_data, pCB, indexedDraw);
    PIPELINE_NODE* pPipe = getPipeline(my_data, pCB->lastBoundPipeline);
    // Now complete other state checks
    // TODO : Currently only performing next check if *something* was bound (non-zero last bound)
    //  There is probably a better way to gate when this check happens, and to know if something *should* have been bound
    //  We should have that check separately and then gate this check based on that check
    if (pPipe && (pCB->lastBoundPipelineLayout)) {
        string errorString;
        for (auto setIndex : pPipe->active_sets) {
            // If valid set is not bound throw an error
            if ((pCB->boundDescriptorSets.size() <= setIndex) || (!pCB->boundDescriptorSets[setIndex])) {
                result |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_DESCRIPTOR_SET_NOT_BOUND, "DS",
                        "VkPipeline %#" PRIxLEAST64 " uses set #%u but that set is not bound.", (uint64_t)pPipe->pipeline, setIndex);
            } else if (!verify_set_layout_compatibility(my_data, my_data->setMap[pCB->boundDescriptorSets[setIndex]], pPipe->graphicsPipelineCI.layout, setIndex, errorString)) {
                // Set is bound but not compatible w/ overlapping pipelineLayout from PSO
                VkDescriptorSet setHandle = my_data->setMap[pCB->boundDescriptorSets[setIndex]]->set;
                result |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)setHandle, 0, DRAWSTATE_PIPELINE_LAYOUTS_INCOMPATIBLE, "DS",
                    "VkDescriptorSet (%#" PRIxLEAST64 ") bound as set #%u is not compatible with overlapping VkPipelineLayout %#" PRIxLEAST64 " due to: %s", (uint64_t)setHandle, setIndex, (uint64_t)pPipe->graphicsPipelineCI.layout, errorString.c_str());
            }
        }
    }
    // Verify Vtx binding
    if (MAX_BINDING != pCB->lastVtxBinding) {
        if (pCB->lastVtxBinding >= pPipe->vtxBindingCount) {
            if (0 == pPipe->vtxBindingCount) {
                result |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS, "DS",
                        "Vtx Buffer Index %u was bound, but no vtx buffers are attached to PSO.", pCB->lastVtxBinding);
            }
            else {
                result |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS, "DS",
                        "Vtx binding Index of %u exceeds PSO pVertexBindingDescriptions max array index of %u.", pCB->lastVtxBinding, (pPipe->vtxBindingCount - 1));
            }
        }
    }
    // If Viewport or scissors are dynamic, verify that dynamic count matches PSO count
    VkBool32 dynViewport = isDynamic(pPipe, VK_DYNAMIC_STATE_VIEWPORT);
    VkBool32 dynScissor = isDynamic(pPipe, VK_DYNAMIC_STATE_SCISSOR);
    if (dynViewport) {
        if (pCB->viewports.size() != pPipe->graphicsPipelineCI.pViewportState->viewportCount) {
            result |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                "Dynamic viewportCount from vkCmdSetViewport() is " PRINTF_SIZE_T_SPECIFIER ", but PSO viewportCount is %u. These counts must match.",
                 pCB->viewports.size(), pPipe->graphicsPipelineCI.pViewportState->viewportCount);
        }
    }
    if (dynScissor) {
        if (pCB->scissors.size() != pPipe->graphicsPipelineCI.pViewportState->scissorCount) {
            result |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                "Dynamic scissorCount from vkCmdSetScissor() is " PRINTF_SIZE_T_SPECIFIER ", but PSO scissorCount is %u. These counts must match.",
                pCB->scissors.size(), pPipe->graphicsPipelineCI.pViewportState->scissorCount);
        }
    }
    return result;
}

// Verify that create state for a pipeline is valid
static VkBool32 verifyPipelineCreateState(layer_data* my_data, const VkDevice device, PIPELINE_NODE* pPipeline)
{
    VkBool32 skipCall = VK_FALSE;
    //if (!validate_pipeline_shaders(my_data, device, &(pPipeline->graphicsPipelineCI))) {
    if (!validate_pipeline_shaders(my_data, device, pPipeline)) {
        skipCall = VK_TRUE;
    }
    // VS is required
    if (!(pPipeline->active_shaders & VK_SHADER_STAGE_VERTEX_BIT)) {
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: Vtx Shader required");
    }
    // Either both or neither TC/TE shaders should be defined
    if (((pPipeline->active_shaders & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) == 0) !=
         ((pPipeline->active_shaders & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) == 0) ) {
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: TE and TC shaders must be included or excluded as a pair");
    }
    // Compute shaders should be specified independent of Gfx shaders
    if ((pPipeline->active_shaders & VK_SHADER_STAGE_COMPUTE_BIT) &&
        (pPipeline->active_shaders & (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT |
                                     VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_GEOMETRY_BIT |
                                     VK_SHADER_STAGE_FRAGMENT_BIT))) {
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: Do not specify Compute Shader for Gfx Pipeline");
    }
    // VK_PRIMITIVE_TOPOLOGY_PATCH_LIST primitive topology is only valid for tessellation pipelines.
    // Mismatching primitive topology and tessellation fails graphics pipeline creation.
    if (pPipeline->active_shaders & (VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) &&
        (pPipeline->iaStateCI.topology != VK_PRIMITIVE_TOPOLOGY_PATCH_LIST)) {
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: VK_PRIMITIVE_TOPOLOGY_PATCH_LIST must be set as IA topology for tessellation pipelines");
    }
    if (pPipeline->iaStateCI.topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST) {
        if (~pPipeline->active_shaders & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) {
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                "Invalid Pipeline CreateInfo State: VK_PRIMITIVE_TOPOLOGY_PATCH_LIST primitive topology is only valid for tessellation pipelines");
        }
        if (!pPipeline->tessStateCI.patchControlPoints || (pPipeline->tessStateCI.patchControlPoints > 32)) {
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                    "Invalid Pipeline CreateInfo State: VK_PRIMITIVE_TOPOLOGY_PATCH_LIST primitive topology used with patchControlPoints value %u."
                    " patchControlPoints should be >0 and <=32.", pPipeline->tessStateCI.patchControlPoints);
        }
    }
    // Viewport state must be included and viewport and scissor counts should always match
    // NOTE : Even if these are flagged as dynamic, counts need to be set correctly for shader compiler
    if (!pPipeline->graphicsPipelineCI.pViewportState) {
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
            "Gfx Pipeline pViewportState is null. Even if viewport and scissors are dynamic PSO must include viewportCount and scissorCount in pViewportState.");
    } else if (pPipeline->graphicsPipelineCI.pViewportState->scissorCount != pPipeline->graphicsPipelineCI.pViewportState->viewportCount) {
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                "Gfx Pipeline viewport count (%u) must match scissor count (%u).", pPipeline->vpStateCI.viewportCount, pPipeline->vpStateCI.scissorCount);
    } else {
        // If viewport or scissor are not dynamic, then verify that data is appropriate for count
        VkBool32 dynViewport = isDynamic(pPipeline, VK_DYNAMIC_STATE_VIEWPORT);
        VkBool32 dynScissor = isDynamic(pPipeline, VK_DYNAMIC_STATE_SCISSOR);
        if (!dynViewport) {
            if (pPipeline->graphicsPipelineCI.pViewportState->viewportCount && !pPipeline->graphicsPipelineCI.pViewportState->pViewports) {
                skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                    "Gfx Pipeline viewportCount is %u, but pViewports is NULL. For non-zero viewportCount, you must either include pViewports data, or "
                    "include viewport in pDynamicState and set it with vkCmdSetViewport().", pPipeline->graphicsPipelineCI.pViewportState->viewportCount);
            }
        }
        if (!dynScissor) {
            if (pPipeline->graphicsPipelineCI.pViewportState->scissorCount && !pPipeline->graphicsPipelineCI.pViewportState->pScissors) {
                skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS",
                    "Gfx Pipeline scissorCount is %u, but pScissors is NULL. For non-zero scissorCount, you must either include pScissors data, or include "
                    "scissor in pDynamicState and set it with vkCmdSetScissor().", pPipeline->graphicsPipelineCI.pViewportState->scissorCount);
            }
        }
    }
    return skipCall;
}

// Init the pipeline mapping info based on pipeline create info LL tree
//  Threading note : Calls to this function should wrapped in mutex
// TODO : this should really just be in the constructor for PIPELINE_NODE
static PIPELINE_NODE* initGraphicsPipeline(layer_data* dev_data, const VkGraphicsPipelineCreateInfo* pCreateInfo, PIPELINE_NODE* pBasePipeline)
{
    PIPELINE_NODE* pPipeline = new PIPELINE_NODE;
    if (pBasePipeline) {
        memcpy((void*)pPipeline, (void*)pBasePipeline, sizeof(PIPELINE_NODE));
    } else {
        memset((void*)pPipeline, 0, sizeof(PIPELINE_NODE));
    }
    // First init create info
    memcpy(&pPipeline->graphicsPipelineCI, pCreateInfo, sizeof(VkGraphicsPipelineCreateInfo));

    size_t bufferSize = 0;
    const VkPipelineVertexInputStateCreateInfo* pVICI = NULL;
    const VkPipelineColorBlendStateCreateInfo*  pCBCI = NULL;

    for (uint32_t i = 0; i < pCreateInfo->stageCount; i++) {
        const VkPipelineShaderStageCreateInfo *pPSSCI = &pCreateInfo->pStages[i];

        switch (pPSSCI->stage) {
            case VK_SHADER_STAGE_VERTEX_BIT:
                memcpy(&pPipeline->vsCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
                memcpy(&pPipeline->tcsCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                break;
            case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
                memcpy(&pPipeline->tesCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                break;
            case VK_SHADER_STAGE_GEOMETRY_BIT:
                memcpy(&pPipeline->gsCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                memcpy(&pPipeline->fsCI, pPSSCI, sizeof(VkPipelineShaderStageCreateInfo));
                pPipeline->active_shaders |= VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case VK_SHADER_STAGE_COMPUTE_BIT:
                // TODO : Flag error, CS is specified through VkComputePipelineCreateInfo
                pPipeline->active_shaders |= VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            default:
                // TODO : Flag error
                break;
        }
    }
    // Copy over GraphicsPipelineCreateInfo structure embedded pointers
    if (pCreateInfo->stageCount != 0) {
        pPipeline->graphicsPipelineCI.pStages = new VkPipelineShaderStageCreateInfo[pCreateInfo->stageCount];
        bufferSize =  pCreateInfo->stageCount * sizeof(VkPipelineShaderStageCreateInfo);
        memcpy((void*)pPipeline->graphicsPipelineCI.pStages, pCreateInfo->pStages, bufferSize);
    }
    if (pCreateInfo->pVertexInputState != NULL) {
        memcpy((void*)&pPipeline->vertexInputCI, pCreateInfo->pVertexInputState , sizeof(VkPipelineVertexInputStateCreateInfo));
        // Copy embedded ptrs
        pVICI = pCreateInfo->pVertexInputState;
        pPipeline->vtxBindingCount = pVICI->vertexBindingDescriptionCount;
        if (pPipeline->vtxBindingCount) {
            pPipeline->pVertexBindingDescriptions = new VkVertexInputBindingDescription[pPipeline->vtxBindingCount];
            bufferSize = pPipeline->vtxBindingCount * sizeof(VkVertexInputBindingDescription);
            memcpy((void*)pPipeline->pVertexBindingDescriptions, pVICI->pVertexBindingDescriptions, bufferSize);
        }
        pPipeline->vtxAttributeCount = pVICI->vertexAttributeDescriptionCount;
        if (pPipeline->vtxAttributeCount) {
            pPipeline->pVertexAttributeDescriptions = new VkVertexInputAttributeDescription[pPipeline->vtxAttributeCount];
            bufferSize = pPipeline->vtxAttributeCount * sizeof(VkVertexInputAttributeDescription);
            memcpy((void*)pPipeline->pVertexAttributeDescriptions, pVICI->pVertexAttributeDescriptions, bufferSize);
        }
        pPipeline->graphicsPipelineCI.pVertexInputState = &pPipeline->vertexInputCI;
    }
    if (pCreateInfo->pInputAssemblyState != NULL) {
        memcpy((void*)&pPipeline->iaStateCI, pCreateInfo->pInputAssemblyState, sizeof(VkPipelineInputAssemblyStateCreateInfo));
        pPipeline->graphicsPipelineCI.pInputAssemblyState = &pPipeline->iaStateCI;
    }
    if (pCreateInfo->pTessellationState != NULL) {
        memcpy((void*)&pPipeline->tessStateCI, pCreateInfo->pTessellationState, sizeof(VkPipelineTessellationStateCreateInfo));
        pPipeline->graphicsPipelineCI.pTessellationState = &pPipeline->tessStateCI;
    }
    if (pCreateInfo->pViewportState != NULL) {
        memcpy((void*)&pPipeline->vpStateCI, pCreateInfo->pViewportState, sizeof(VkPipelineViewportStateCreateInfo));
        pPipeline->graphicsPipelineCI.pViewportState = &pPipeline->vpStateCI;
    }
    if (pCreateInfo->pRasterizationState != NULL) {
        memcpy((void*)&pPipeline->rsStateCI, pCreateInfo->pRasterizationState, sizeof(VkPipelineRasterizationStateCreateInfo));
        pPipeline->graphicsPipelineCI.pRasterizationState = &pPipeline->rsStateCI;
    }
    if (pCreateInfo->pMultisampleState != NULL) {
        memcpy((void*)&pPipeline->msStateCI, pCreateInfo->pMultisampleState, sizeof(VkPipelineMultisampleStateCreateInfo));
        pPipeline->graphicsPipelineCI.pMultisampleState = &pPipeline->msStateCI;
    }
    if (pCreateInfo->pDepthStencilState != NULL) {
        memcpy((void*)&pPipeline->dsStateCI, pCreateInfo->pDepthStencilState, sizeof(VkPipelineDepthStencilStateCreateInfo));
        pPipeline->graphicsPipelineCI.pDepthStencilState = &pPipeline->dsStateCI;
    }
    if (pCreateInfo->pColorBlendState != NULL) {
        memcpy((void*)&pPipeline->cbStateCI, pCreateInfo->pColorBlendState, sizeof(VkPipelineColorBlendStateCreateInfo));
        // Copy embedded ptrs
        pCBCI = pCreateInfo->pColorBlendState;
        pPipeline->attachmentCount = pCBCI->attachmentCount;
        if (pPipeline->attachmentCount) {
            pPipeline->pAttachments = new VkPipelineColorBlendAttachmentState[pPipeline->attachmentCount];
            bufferSize = pPipeline->attachmentCount * sizeof(VkPipelineColorBlendAttachmentState);
            memcpy((void*)pPipeline->pAttachments, pCBCI->pAttachments, bufferSize);
        }
        pPipeline->graphicsPipelineCI.pColorBlendState = &pPipeline->cbStateCI;
    }
    if (pCreateInfo->pDynamicState != NULL) {
        memcpy((void*)&pPipeline->dynStateCI, pCreateInfo->pDynamicState, sizeof(VkPipelineDynamicStateCreateInfo));
        if (pPipeline->dynStateCI.dynamicStateCount) {
            pPipeline->dynStateCI.pDynamicStates = new VkDynamicState[pPipeline->dynStateCI.dynamicStateCount];
            bufferSize = pPipeline->dynStateCI.dynamicStateCount * sizeof(VkDynamicState);
            memcpy((void*)pPipeline->dynStateCI.pDynamicStates, pCreateInfo->pDynamicState->pDynamicStates, bufferSize);
        }
        pPipeline->graphicsPipelineCI.pDynamicState = &pPipeline->dynStateCI;
    }
    pPipeline->active_sets.clear();
    return pPipeline;
}

// Free the Pipeline nodes
static void deletePipelines(layer_data* my_data)
{
    if (my_data->pipelineMap.size() <= 0)
        return;
    for (auto ii=my_data->pipelineMap.begin(); ii!=my_data->pipelineMap.end(); ++ii) {
        if ((*ii).second->graphicsPipelineCI.stageCount != 0) {
            delete[] (*ii).second->graphicsPipelineCI.pStages;
        }
        if ((*ii).second->pVertexBindingDescriptions) {
            delete[] (*ii).second->pVertexBindingDescriptions;
        }
        if ((*ii).second->pVertexAttributeDescriptions) {
            delete[] (*ii).second->pVertexAttributeDescriptions;
        }
        if ((*ii).second->pAttachments) {
            delete[] (*ii).second->pAttachments;
        }
        if ((*ii).second->dynStateCI.dynamicStateCount != 0) {
            delete[] (*ii).second->dynStateCI.pDynamicStates;
        }
        delete (*ii).second;
    }
    my_data->pipelineMap.clear();
}

// For given pipeline, return number of MSAA samples, or one if MSAA disabled
static VkSampleCountFlagBits getNumSamples(layer_data* my_data, const VkPipeline pipeline)
{
    PIPELINE_NODE* pPipe = my_data->pipelineMap[pipeline];
    if (VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO == pPipe->msStateCI.sType) {
        return pPipe->msStateCI.rasterizationSamples;
    }
    return VK_SAMPLE_COUNT_1_BIT;
}

// Validate state related to the PSO
static VkBool32 validatePipelineState(layer_data* my_data, const GLOBAL_CB_NODE* pCB, const VkPipelineBindPoint pipelineBindPoint, const VkPipeline pipeline)
{
    if (VK_PIPELINE_BIND_POINT_GRAPHICS == pipelineBindPoint) {
        // Verify that any MSAA request in PSO matches sample# in bound FB
        VkSampleCountFlagBits psoNumSamples = getNumSamples(my_data, pipeline);
        if (pCB->activeRenderPass) {
            const VkRenderPassCreateInfo* pRPCI = my_data->renderPassMap[pCB->activeRenderPass]->pCreateInfo;
            const VkSubpassDescription* pSD = &pRPCI->pSubpasses[pCB->activeSubpass];
            VkSampleCountFlagBits subpassNumSamples = (VkSampleCountFlagBits) 0;
            uint32_t i;

            for (i = 0; i < pSD->colorAttachmentCount; i++) {
                VkSampleCountFlagBits samples;

                if (pSD->pColorAttachments[i].attachment == VK_ATTACHMENT_UNUSED)
                    continue;

                samples = pRPCI->pAttachments[pSD->pColorAttachments[i].attachment].samples;
                if (subpassNumSamples == (VkSampleCountFlagBits) 0) {
                    subpassNumSamples = samples;
                } else if (subpassNumSamples != samples) {
                    subpassNumSamples = (VkSampleCountFlagBits) -1;
                    break;
                }
            }
            if (pSD->pDepthStencilAttachment && pSD->pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
                const VkSampleCountFlagBits samples = pRPCI->pAttachments[pSD->pDepthStencilAttachment->attachment].samples;
                if (subpassNumSamples == (VkSampleCountFlagBits) 0)
                    subpassNumSamples = samples;
                else if (subpassNumSamples != samples)
                    subpassNumSamples = (VkSampleCountFlagBits) -1;
            }

            if (psoNumSamples != subpassNumSamples) {
                return log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PIPELINE, (uint64_t) pipeline, 0, DRAWSTATE_NUM_SAMPLES_MISMATCH, "DS",
                        "Num samples mismatch! Binding PSO (%#" PRIxLEAST64 ") with %u samples while current RenderPass (%#" PRIxLEAST64 ") w/ %u samples!",
                        (uint64_t) pipeline, psoNumSamples, (uint64_t) pCB->activeRenderPass, subpassNumSamples);
            }
        } else {
            // TODO : I believe it's an error if we reach this point and don't have an activeRenderPass
            //   Verify and flag error as appropriate
        }
        // TODO : Add more checks here
    } else {
        // TODO : Validate non-gfx pipeline updates
    }
    return VK_FALSE;
}

// Block of code at start here specifically for managing/tracking DSs

// Return Pool node ptr for specified pool or else NULL
static DESCRIPTOR_POOL_NODE* getPoolNode(layer_data* my_data, const VkDescriptorPool pool)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (my_data->descriptorPoolMap.find(pool) == my_data->descriptorPoolMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return my_data->descriptorPoolMap[pool];
}

// Return Set node ptr for specified set or else NULL
static SET_NODE* getSetNode(layer_data* my_data, const VkDescriptorSet set)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (my_data->setMap.find(set) == my_data->setMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return my_data->setMap[set];
}

static LAYOUT_NODE* getLayoutNode(layer_data* my_data, const VkDescriptorSetLayout layout) {
    loader_platform_thread_lock_mutex(&globalLock);
    if (my_data->descriptorSetLayoutMap.find(layout) == my_data->descriptorSetLayoutMap.end()) {
        loader_platform_thread_unlock_mutex(&globalLock);
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return my_data->descriptorSetLayoutMap[layout];
}

// Return VK_FALSE if update struct is of valid type, otherwise flag error and return code from callback
static VkBool32 validUpdateStruct(layer_data* my_data, const VkDevice device, const GENERIC_HEADER* pUpdateStruct)
{
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            return VK_FALSE;
        default:
            return log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS",
                    "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdateStruct->sType), pUpdateStruct->sType);
    }
}

// Set count for given update struct in the last parameter
// Return value of skipCall, which is only VK_TRUE is error occurs and callback signals execution to cease
static uint32_t getUpdateCount(layer_data* my_data, const VkDevice device, const GENERIC_HEADER* pUpdateStruct)
{
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
            return ((VkWriteDescriptorSet*)pUpdateStruct)->descriptorCount;
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            // TODO : Need to understand this case better and make sure code is correct
            return ((VkCopyDescriptorSet*)pUpdateStruct)->descriptorCount;
    }
}

// For given Layout Node and binding, return index where that binding begins
static uint32_t getBindingStartIndex(const LAYOUT_NODE* pLayout, const uint32_t binding)
{
    uint32_t offsetIndex = 0;
    for (uint32_t i = 0; i < pLayout->createInfo.bindingCount; i++) {
        if (pLayout->createInfo.pBinding[i].binding == binding)
            break;
        offsetIndex += pLayout->createInfo.pBinding[i].descriptorCount;
    }
    return offsetIndex;
}

// For given layout node and binding, return last index that is updated
static uint32_t getBindingEndIndex(const LAYOUT_NODE* pLayout, const uint32_t binding)
{
    uint32_t offsetIndex = 0;
    for (uint32_t i = 0; i <  pLayout->createInfo.bindingCount; i++) {
        offsetIndex += pLayout->createInfo.pBinding[i].descriptorCount;
        if (pLayout->createInfo.pBinding[i].binding == binding)
            break;
    }
    return offsetIndex-1;
}

// For given layout and update, return the first overall index of the layout that is updated
static uint32_t getUpdateStartIndex(layer_data* my_data, const VkDevice device, const LAYOUT_NODE* pLayout, const uint32_t binding, const uint32_t arrayIndex, const GENERIC_HEADER* pUpdateStruct)
{
    return getBindingStartIndex(pLayout, binding)+arrayIndex;
}

// For given layout and update, return the last overall index of the layout that is updated
static uint32_t getUpdateEndIndex(layer_data* my_data, const VkDevice device, const LAYOUT_NODE* pLayout, const uint32_t binding, const uint32_t arrayIndex, const GENERIC_HEADER* pUpdateStruct)
{
    uint32_t count = getUpdateCount(my_data, device, pUpdateStruct);
    return getBindingStartIndex(pLayout, binding)+arrayIndex+count-1;
}

// Verify that the descriptor type in the update struct matches what's expected by the layout
static VkBool32 validateUpdateConsistency(layer_data* my_data, const VkDevice device, const LAYOUT_NODE* pLayout, const GENERIC_HEADER* pUpdateStruct, uint32_t startIndex, uint32_t endIndex)
{
    // First get actual type of update
    VkBool32 skipCall = VK_FALSE;
    VkDescriptorType actualType;
    uint32_t i = 0;
    switch (pUpdateStruct->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
            actualType = ((VkWriteDescriptorSet*)pUpdateStruct)->descriptorType;
            break;
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            /* no need to validate */
            return VK_FALSE;
            break;
        default:
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS",
                    "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdateStruct->sType), pUpdateStruct->sType);
    }
    if (VK_FALSE == skipCall) {
        // Set first stageFlags as reference and verify that all other updates match it
        VkShaderStageFlags refStageFlags = pLayout->stageFlags[startIndex];
        for (i = startIndex; i <= endIndex; i++) {
            if (pLayout->descriptorTypes[i] != actualType) {
                skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_DESCRIPTOR_TYPE_MISMATCH, "DS",
                    "Write descriptor update has descriptor type %s that does not match overlapping binding descriptor type of %s!",
                    string_VkDescriptorType(actualType), string_VkDescriptorType(pLayout->descriptorTypes[i]));
            }
            if (pLayout->stageFlags[i] != refStageFlags) {
                skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_DESCRIPTOR_STAGEFLAGS_MISMATCH, "DS",
                    "Write descriptor update has stageFlags %x that do not match overlapping binding descriptor stageFlags of %x!",
                    refStageFlags, pLayout->stageFlags[i]);
            }
        }
    }
    return skipCall;
}

// Determine the update type, allocate a new struct of that type, shadow the given pUpdate
//   struct into the pNewNode param. Return VK_TRUE if error condition encountered and callback signals early exit.
// NOTE : Calls to this function should be wrapped in mutex
static VkBool32 shadowUpdateNode(layer_data* my_data, const VkDevice device, GENERIC_HEADER* pUpdate, GENERIC_HEADER** pNewNode)
{
    VkBool32 skipCall = VK_FALSE;
    VkWriteDescriptorSet* pWDS = NULL;
    VkCopyDescriptorSet* pCDS = NULL;
    size_t array_size = 0;
    size_t base_array_size = 0;
    size_t total_array_size = 0;
    size_t baseBuffAddr = 0;
    switch (pUpdate->sType)
    {
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
            pWDS = new VkWriteDescriptorSet;
            *pNewNode = (GENERIC_HEADER*)pWDS;
            memcpy(pWDS, pUpdate, sizeof(VkWriteDescriptorSet));

            switch (pWDS->descriptorType) {
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                {
                    VkDescriptorImageInfo *info = new VkDescriptorImageInfo[pWDS->descriptorCount];
                    memcpy(info, pWDS->pImageInfo, pWDS->descriptorCount * sizeof(VkDescriptorImageInfo));
                    pWDS->pImageInfo = info;
                }
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                {
                    VkBufferView *info = new VkBufferView[pWDS->descriptorCount];
                    memcpy(info, pWDS->pTexelBufferView, pWDS->descriptorCount * sizeof(VkBufferView));
                    pWDS->pTexelBufferView = info;
                }
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                {
                    VkDescriptorBufferInfo *info = new VkDescriptorBufferInfo[pWDS->descriptorCount];
                    memcpy(info, pWDS->pBufferInfo, pWDS->descriptorCount * sizeof(VkDescriptorBufferInfo));
                    pWDS->pBufferInfo = info;
                }
                break;
            default:
                return VK_ERROR_VALIDATION_FAILED;
                break;
            }
            break;
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            pCDS = new VkCopyDescriptorSet;
            *pNewNode = (GENERIC_HEADER*)pCDS;
            memcpy(pCDS, pUpdate, sizeof(VkCopyDescriptorSet));
            break;
        default:
            if (log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_UPDATE_STRUCT, "DS",
                    "Unexpected UPDATE struct of type %s (value %u) in vkUpdateDescriptors() struct tree", string_VkStructureType(pUpdate->sType), pUpdate->sType))
                return VK_TRUE;
    }
    // Make sure that pNext for the end of shadow copy is NULL
    (*pNewNode)->pNext = NULL;
    return skipCall;
}

// Verify that given sampler is valid
static VkBool32 validateSampler(const layer_data* my_data, const VkSampler* pSampler, const VkBool32 immutable)
{
    VkBool32 skipCall = VK_FALSE;
    auto sampIt = my_data->sampleMap.find(*pSampler);
    if (sampIt == my_data->sampleMap.end()) {
        if (!immutable) {
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_SAMPLER, (uint64_t) *pSampler, 0, DRAWSTATE_SAMPLER_DESCRIPTOR_ERROR, "DS",
                "vkUpdateDescriptorSets: Attempt to update descriptor with invalid sampler %#" PRIxLEAST64, (uint64_t) *pSampler);
        } else { // immutable
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_SAMPLER, (uint64_t) *pSampler, 0, DRAWSTATE_SAMPLER_DESCRIPTOR_ERROR, "DS",
                "vkUpdateDescriptorSets: Attempt to update descriptor whose binding has an invalid immutable sampler %#" PRIxLEAST64, (uint64_t) *pSampler);
        }
    } else {
        // TODO : Any further checks we want to do on the sampler?
    }
    return skipCall;
}

// Verify that given imageView is valid
static VkBool32 validateImageView(const layer_data* my_data, const VkImageView* pImageView, const VkImageLayout imageLayout)
{
    VkBool32 skipCall = VK_FALSE;
    auto ivIt = my_data->imageViewMap.find(*pImageView);
    if (ivIt == my_data->imageViewMap.end()) {
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t) *pImageView, 0, DRAWSTATE_IMAGEVIEW_DESCRIPTOR_ERROR, "DS",
                "vkUpdateDescriptorSets: Attempt to update descriptor with invalid imageView %#" PRIxLEAST64, (uint64_t) *pImageView);
    } else {
        // Validate that imageLayout is compatible with aspectMask and image format
        VkImageAspectFlags aspectMask = ivIt->second->subresourceRange.aspectMask;
        VkImage image = ivIt->second->image;
        // TODO : Check here in case we have a bad image
        auto imgIt = my_data->imageMap.find(image);
        if (imgIt == my_data->imageMap.end()) {
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE, (uint64_t) image, 0, DRAWSTATE_IMAGEVIEW_DESCRIPTOR_ERROR, "DS",
                "vkUpdateDescriptorSets: Attempt to update descriptor with invalid image %#" PRIxLEAST64 " in imageView %#" PRIxLEAST64, (uint64_t) image, (uint64_t) *pImageView);
        } else {
            VkFormat format = (*imgIt).second->format;
            VkBool32 ds = vk_format_is_depth_or_stencil(format);
            switch (imageLayout) {
                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    // Only Color bit must be set
                    if ((aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) != VK_IMAGE_ASPECT_COLOR_BIT) {
                        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t) *pImageView, 0,
                            DRAWSTATE_INVALID_IMAGE_ASPECT, "DS", "vkUpdateDescriptorSets: Updating descriptor with layout VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL and imageView %#" PRIxLEAST64 ""
                                    " that does not have VK_IMAGE_ASPECT_COLOR_BIT set.", (uint64_t) *pImageView);
                    }
                    // format must NOT be DS
                    if (ds) {
                        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t) *pImageView, 0,
                            DRAWSTATE_IMAGEVIEW_DESCRIPTOR_ERROR, "DS", "vkUpdateDescriptorSets: Updating descriptor with layout VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL and imageView %#" PRIxLEAST64 ""
                                    " but the image format is %s which is not a color format.", (uint64_t) *pImageView, string_VkFormat(format));
                    }
                    break;
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                    // Depth or stencil bit must be set, but both must NOT be set
                    if (aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) {
                        if (aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) {
                            // both  must NOT be set
                            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t) *pImageView, 0,
                            DRAWSTATE_INVALID_IMAGE_ASPECT, "DS", "vkUpdateDescriptorSets: Updating descriptor with imageView %#" PRIxLEAST64 ""
                                    " that has both STENCIL and DEPTH aspects set", (uint64_t) *pImageView);
                        }
                    } else if (!(aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT)) {
                        // Neither were set
                        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t) *pImageView, 0,
                            DRAWSTATE_INVALID_IMAGE_ASPECT, "DS", "vkUpdateDescriptorSets: Updating descriptor with layout %s and imageView %#" PRIxLEAST64 ""
                                    " that does not have STENCIL or DEPTH aspect set.", string_VkImageLayout(imageLayout), (uint64_t) *pImageView);
                    }
                    // format must be DS
                    if (!ds) {
                        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t) *pImageView, 0,
                            DRAWSTATE_IMAGEVIEW_DESCRIPTOR_ERROR, "DS", "vkUpdateDescriptorSets: Updating descriptor with layout %s and imageView %#" PRIxLEAST64 ""
                                    " but the image format is %s which is not a depth/stencil format.", string_VkImageLayout(imageLayout), (uint64_t) *pImageView, string_VkFormat(format));
                    }
                    break;
                default:
                    // anything to check for other layouts?
                    break;
            }
        }
    }
    return skipCall;
}

// Verify that given bufferView is valid
static VkBool32 validateBufferView(const layer_data* my_data, const VkBufferView* pBufferView)
{
    VkBool32 skipCall = VK_FALSE;
    auto sampIt = my_data->bufferViewMap.find(*pBufferView);
    if (sampIt == my_data->bufferViewMap.end()) {
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_BUFFER_VIEW, (uint64_t) *pBufferView, 0, DRAWSTATE_BUFFERVIEW_DESCRIPTOR_ERROR, "DS",
                "vkUpdateDescriptorSets: Attempt to update descriptor with invalid bufferView %#" PRIxLEAST64, (uint64_t) *pBufferView);
    } else {
        // TODO : Any further checks we want to do on the bufferView?
    }
    return skipCall;
}

// Verify that given bufferInfo is valid
static VkBool32 validateBufferInfo(const layer_data* my_data, const VkDescriptorBufferInfo* pBufferInfo)
{
    VkBool32 skipCall = VK_FALSE;
    auto sampIt = my_data->bufferMap.find(pBufferInfo->buffer);
    if (sampIt == my_data->bufferMap.end()) {
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_BUFFER, (uint64_t) pBufferInfo->buffer, 0, DRAWSTATE_BUFFERINFO_DESCRIPTOR_ERROR, "DS",
                "vkUpdateDescriptorSets: Attempt to update descriptor where bufferInfo has invalid buffer %#" PRIxLEAST64, (uint64_t) pBufferInfo->buffer);
    } else {
        // TODO : Any further checks we want to do on the bufferView?
    }
    return skipCall;
}

static VkBool32 validateUpdateContents(const layer_data* my_data, const VkWriteDescriptorSet *pWDS, const VkDescriptorSetLayoutBinding* pLayoutBinding)
{
    VkBool32 skipCall = VK_FALSE;
    // First verify that for the given Descriptor type, the correct DescriptorInfo data is supplied
    VkBufferView* pBufferView = NULL;
    const VkSampler* pSampler = NULL;
    VkImageView* pImageView = NULL;
    VkImageLayout* pImageLayout = NULL;
    VkDescriptorBufferInfo* pBufferInfo = NULL;
    VkBool32 immutable = VK_FALSE;
    uint32_t i = 0;
    // For given update type, verify that update contents are correct
    switch (pWDS->descriptorType) {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
            for (i=0; i<pWDS->descriptorCount; ++i) {
                skipCall |= validateSampler(my_data, &(pWDS->pImageInfo[i].sampler), immutable);
            }
            break;
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            for (i=0; i<pWDS->descriptorCount; ++i) {
                if (NULL == pLayoutBinding->pImmutableSamplers) {
                    pSampler = &(pWDS->pImageInfo[i].sampler);
                    if (immutable) {
                        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_SAMPLER, (uint64_t) *pSampler, 0, DRAWSTATE_INCONSISTENT_IMMUTABLE_SAMPLER_UPDATE, "DS",
                            "vkUpdateDescriptorSets: Update #%u is not an immutable sampler %#" PRIxLEAST64 ", but previous update(s) from this "
                            "VkWriteDescriptorSet struct used an immutable sampler. All updates from a single struct must either "
                            "use immutable or non-immutable samplers.", i, (uint64_t) *pSampler);
                    }
                } else {
                    if (i>0 && !immutable) {
                        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_SAMPLER, (uint64_t) *pSampler, 0, DRAWSTATE_INCONSISTENT_IMMUTABLE_SAMPLER_UPDATE, "DS",
                            "vkUpdateDescriptorSets: Update #%u is an immutable sampler, but previous update(s) from this "
                            "VkWriteDescriptorSet struct used a non-immutable sampler. All updates from a single struct must either "
                            "use immutable or non-immutable samplers.", i);
                    }
                    immutable = VK_TRUE;
                    pSampler = &(pLayoutBinding->pImmutableSamplers[i]);
                }
                skipCall |= validateSampler(my_data, pSampler, immutable);
            }
            // Intentionally fall through here to also validate image stuff
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            for (i=0; i<pWDS->descriptorCount; ++i) {
                skipCall |= validateImageView(my_data, &(pWDS->pImageInfo[i].imageView), pWDS->pImageInfo[i].imageLayout);
            }
            break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            for (i=0; i<pWDS->descriptorCount; ++i) {
                skipCall |= validateBufferView(my_data, &(pWDS->pTexelBufferView[i]));
            }
            break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            for (i=0; i<pWDS->descriptorCount; ++i) {
                skipCall |= validateBufferInfo(my_data, &(pWDS->pBufferInfo[i]));
            }
            break;
    }
    return skipCall;
}

// update DS mappings based on write and copy update arrays
static VkBool32 dsUpdate(layer_data* my_data, VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pWDS, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pCDS)
{
    VkBool32 skipCall = VK_FALSE;

    loader_platform_thread_lock_mutex(&globalLock);
    LAYOUT_NODE* pLayout = NULL;
    VkDescriptorSetLayoutCreateInfo* pLayoutCI = NULL;
    // Validate Write updates
    uint32_t i = 0;
    for (i=0; i < descriptorWriteCount; i++) {
        VkDescriptorSet ds = pWDS[i].dstSet;
        SET_NODE* pSet = my_data->setMap[ds];
        GENERIC_HEADER* pUpdate = (GENERIC_HEADER*) &pWDS[i];
        pLayout = pSet->pLayout;
        // First verify valid update struct
        if ((skipCall = validUpdateStruct(my_data, device, pUpdate)) == VK_TRUE) {
            break;
        }
        uint32_t binding = 0, endIndex = 0;
        binding = pWDS[i].dstBinding;
        // Make sure that layout being updated has the binding being updated
        if (pLayout->bindings.find(binding) == pLayout->bindings.end()) {
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) ds, 0, DRAWSTATE_INVALID_UPDATE_INDEX, "DS",
                    "Descriptor Set %" PRIu64 " does not have binding to match update binding %u for update type %s!", reinterpret_cast<uint64_t>(ds), binding, string_VkStructureType(pUpdate->sType));
        } else {
            // Next verify that update falls within size of given binding
            endIndex = getUpdateEndIndex(my_data, device, pLayout, binding, pWDS[i].dstArrayElement, pUpdate);
            if (getBindingEndIndex(pLayout, binding) < endIndex) {
                pLayoutCI = &pLayout->createInfo;
                string DSstr = vk_print_vkdescriptorsetlayoutcreateinfo(pLayoutCI, "{DS}    ");
                skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) ds, 0, DRAWSTATE_DESCRIPTOR_UPDATE_OUT_OF_BOUNDS, "DS",
                        "Descriptor update type of %s is out of bounds for matching binding %u in Layout w/ CI:\n%s!", string_VkStructureType(pUpdate->sType), binding, DSstr.c_str());
            } else { // TODO : should we skip update on a type mismatch or force it?
                uint32_t startIndex;
                startIndex = getUpdateStartIndex(my_data, device, pLayout, binding, pWDS[i].dstArrayElement, pUpdate);
                // Layout bindings match w/ update, now verify that update type & stageFlags are the same for entire update
                if ((skipCall = validateUpdateConsistency(my_data, device, pLayout, pUpdate, startIndex, endIndex)) == VK_FALSE) {
                    // The update is within bounds and consistent, but need to make sure contents make sense as well
                    if ((skipCall = validateUpdateContents(my_data, &pWDS[i], &pLayout->createInfo.pBinding[binding])) == VK_FALSE) {
                        // Update is good. Save the update info
                        // Create new update struct for this set's shadow copy
                        GENERIC_HEADER* pNewNode = NULL;
                        skipCall |= shadowUpdateNode(my_data, device, pUpdate, &pNewNode);
                        if (NULL == pNewNode) {
                            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) ds, 0, DRAWSTATE_OUT_OF_MEMORY, "DS",
                                    "Out of memory while attempting to allocate UPDATE struct in vkUpdateDescriptors()");
                        } else {
                            // Insert shadow node into LL of updates for this set
                            pNewNode->pNext = pSet->pUpdateStructs;
                            pSet->pUpdateStructs = pNewNode;
                            // Now update appropriate descriptor(s) to point to new Update node
                            for (uint32_t j = startIndex; j <= endIndex; j++) {
                                assert(j<pSet->descriptorCount);
                                pSet->ppDescriptors[j] = pNewNode;
                            }
                        }
                    }
                }
            }
        }
    }
    // Now validate copy updates
    for (i=0; i < descriptorCopyCount; ++i) {
        SET_NODE *pSrcSet = NULL, *pDstSet = NULL;
        LAYOUT_NODE *pSrcLayout = NULL, *pDstLayout = NULL;
        uint32_t srcStartIndex = 0, srcEndIndex = 0, dstStartIndex = 0, dstEndIndex = 0;
        // For each copy make sure that update falls within given layout and that types match
        pSrcSet = my_data->setMap[pCDS[i].srcSet];
        pDstSet = my_data->setMap[pCDS[i].dstSet];
        pSrcLayout = pSrcSet->pLayout;
        pDstLayout = pDstSet->pLayout;
        // Validate that src binding is valid for src set layout
        if (pSrcLayout->bindings.find(pCDS[i].srcBinding) == pSrcLayout->bindings.end()) {
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pSrcSet->set, 0, DRAWSTATE_INVALID_UPDATE_INDEX, "DS",
                "Copy descriptor update %u has srcBinding %u which is out of bounds for underlying SetLayout %#" PRIxLEAST64 " which only has bindings 0-%u.",
                i, pCDS[i].srcBinding, (uint64_t) pSrcLayout->layout, pSrcLayout->createInfo.bindingCount-1);
        } else if (pDstLayout->bindings.find(pCDS[i].dstBinding) == pDstLayout->bindings.end()) {
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pDstSet->set, 0, DRAWSTATE_INVALID_UPDATE_INDEX, "DS",
                "Copy descriptor update %u has dstBinding %u which is out of bounds for underlying SetLayout %#" PRIxLEAST64 " which only has bindings 0-%u.",
                i, pCDS[i].dstBinding, (uint64_t) pDstLayout->layout, pDstLayout->createInfo.bindingCount-1);
        } else {
            // Proceed with validation. Bindings are ok, but make sure update is within bounds of given layout
            srcEndIndex = getUpdateEndIndex(my_data, device, pSrcLayout, pCDS[i].srcBinding, pCDS[i].srcArrayElement, (const GENERIC_HEADER*)&(pCDS[i]));
            dstEndIndex = getUpdateEndIndex(my_data, device, pDstLayout, pCDS[i].dstBinding, pCDS[i].dstArrayElement, (const GENERIC_HEADER*)&(pCDS[i]));
            if (getBindingEndIndex(pSrcLayout, pCDS[i].srcBinding) < srcEndIndex) {
                pLayoutCI = &pSrcLayout->createInfo;
                string DSstr = vk_print_vkdescriptorsetlayoutcreateinfo(pLayoutCI, "{DS}    ");
                skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pSrcSet->set, 0, DRAWSTATE_DESCRIPTOR_UPDATE_OUT_OF_BOUNDS, "DS",
                    "Copy descriptor src update is out of bounds for matching binding %u in Layout w/ CI:\n%s!", pCDS[i].srcBinding, DSstr.c_str());
            } else if (getBindingEndIndex(pDstLayout, pCDS[i].dstBinding) < dstEndIndex) {
                pLayoutCI = &pDstLayout->createInfo;
                string DSstr = vk_print_vkdescriptorsetlayoutcreateinfo(pLayoutCI, "{DS}    ");
                skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pDstSet->set, 0, DRAWSTATE_DESCRIPTOR_UPDATE_OUT_OF_BOUNDS, "DS",
                    "Copy descriptor dest update is out of bounds for matching binding %u in Layout w/ CI:\n%s!", pCDS[i].dstBinding, DSstr.c_str());
            } else {
                srcStartIndex = getUpdateStartIndex(my_data, device, pSrcLayout, pCDS[i].srcBinding, pCDS[i].srcArrayElement, (const GENERIC_HEADER*)&(pCDS[i]));
                dstStartIndex = getUpdateStartIndex(my_data, device, pDstLayout, pCDS[i].dstBinding, pCDS[i].dstArrayElement, (const GENERIC_HEADER*)&(pCDS[i]));
                for (uint32_t j=0; j<pCDS[i].descriptorCount; ++j) {
                    // For copy just make sure that the types match and then perform the update
                    if (pSrcLayout->descriptorTypes[srcStartIndex+j] != pDstLayout->descriptorTypes[dstStartIndex+j]) {
                        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_DESCRIPTOR_TYPE_MISMATCH, "DS",
                            "Copy descriptor update index %u, update count #%u, has src update descriptor type %s that does not match overlapping dest descriptor type of %s!",
                            i, j+1, string_VkDescriptorType(pSrcLayout->descriptorTypes[srcStartIndex+j]), string_VkDescriptorType(pDstLayout->descriptorTypes[dstStartIndex+j]));
                    } else {
                        // point dst descriptor at corresponding src descriptor
                        pDstSet->ppDescriptors[j+dstStartIndex] = pSrcSet->ppDescriptors[j+srcStartIndex];
                    }
                }
            }
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return skipCall;
}

// Verify that given pool has descriptors that are being requested for allocation
static VkBool32 validate_descriptor_availability_in_pool(layer_data* dev_data, DESCRIPTOR_POOL_NODE* pPoolNode, uint32_t count, const VkDescriptorSetLayout* pSetLayouts)
{
    VkBool32 skipCall = VK_FALSE;
    uint32_t i = 0, j = 0;
    for (i=0; i<count; ++i) {
        LAYOUT_NODE* pLayout = getLayoutNode(dev_data, pSetLayouts[i]);
        if (NULL == pLayout) {
            skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t) pSetLayouts[i], 0, DRAWSTATE_INVALID_LAYOUT, "DS",
                    "Unable to find set layout node for layout %#" PRIxLEAST64 " specified in vkAllocateDescriptorSets() call", (uint64_t) pSetLayouts[i]);
        } else {
            uint32_t typeIndex = 0, poolSizeCount = 0;
            for (j=0; j<pLayout->createInfo.bindingCount; ++j) {
                typeIndex = static_cast<uint32_t>(pLayout->createInfo.pBinding[j].descriptorType);
                poolSizeCount = pLayout->createInfo.pBinding[j].descriptorCount;
                if (poolSizeCount > pPoolNode->availableDescriptorTypeCount[typeIndex]) {
                    skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t) pLayout->layout, 0, DRAWSTATE_DESCRIPTOR_POOL_EMPTY, "DS",
                        "Unable to allocate %u descriptors of type %s from pool %#" PRIxLEAST64 ". This pool only has %u descriptors of this type remaining.",
                            poolSizeCount, string_VkDescriptorType(pLayout->createInfo.pBinding[j].descriptorType), (uint64_t) pPoolNode->pool, pPoolNode->availableDescriptorTypeCount[typeIndex]);
                } else { // Decrement available descriptors of this type
                    pPoolNode->availableDescriptorTypeCount[typeIndex] -= poolSizeCount;
                }
            }
        }
    }
    return skipCall;
}

// Free the shadowed update node for this Set
// NOTE : Calls to this function should be wrapped in mutex
static void freeShadowUpdateTree(SET_NODE* pSet)
{
    GENERIC_HEADER* pShadowUpdate = pSet->pUpdateStructs;
    pSet->pUpdateStructs = NULL;
    GENERIC_HEADER* pFreeUpdate = pShadowUpdate;
    // Clear the descriptor mappings as they will now be invalid
    memset(pSet->ppDescriptors, 0, pSet->descriptorCount*sizeof(GENERIC_HEADER*));
    while(pShadowUpdate) {
        pFreeUpdate = pShadowUpdate;
        pShadowUpdate = (GENERIC_HEADER*)pShadowUpdate->pNext;
        uint32_t index = 0;
        VkWriteDescriptorSet * pWDS = NULL;
        VkCopyDescriptorSet * pCDS = NULL;
        void** ppToFree = NULL;
        switch (pFreeUpdate->sType)
        {
            case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
                pWDS = (VkWriteDescriptorSet*)pFreeUpdate;
                switch (pWDS->descriptorType) {
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    {
                        delete[] pWDS->pImageInfo;
                    }
                    break;
                case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                    {
                        delete[] pWDS->pTexelBufferView;
                    }
                    break;
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                    {
                        delete[] pWDS->pBufferInfo;
                    }
                    break;
                default:
                    break;
                }
                break;
            case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
                break;
            default:
                assert(0);
                break;
        }
        delete pFreeUpdate;
    }
}

// Free all DS Pools including their Sets & related sub-structs
// NOTE : Calls to this function should be wrapped in mutex
static void deletePools(layer_data* my_data)
{
    if (my_data->descriptorPoolMap.size() <= 0)
        return;
    for (auto ii=my_data->descriptorPoolMap.begin(); ii!=my_data->descriptorPoolMap.end(); ++ii) {
        SET_NODE* pSet = (*ii).second->pSets;
        SET_NODE* pFreeSet = pSet;
        while (pSet) {
            pFreeSet = pSet;
            pSet = pSet->pNext;
            // Freeing layouts handled in deleteLayouts() function
            // Free Update shadow struct tree
            freeShadowUpdateTree(pFreeSet);
            if (pFreeSet->ppDescriptors) {
                delete[] pFreeSet->ppDescriptors;
            }
            delete pFreeSet;
        }
        delete (*ii).second;
    }
    my_data->descriptorPoolMap.clear();
}

// WARN : Once deleteLayouts() called, any layout ptrs in Pool/Set data structure will be invalid
// NOTE : Calls to this function should be wrapped in mutex
static void deleteLayouts(layer_data* my_data)
{
    if (my_data->descriptorSetLayoutMap.size() <= 0)
        return;
    for (auto ii=my_data->descriptorSetLayoutMap.begin(); ii!=my_data->descriptorSetLayoutMap.end(); ++ii) {
        LAYOUT_NODE* pLayout = (*ii).second;
        if (pLayout->createInfo.pBinding) {
            for (uint32_t i=0; i<pLayout->createInfo.bindingCount; i++) {
                if (pLayout->createInfo.pBinding[i].pImmutableSamplers)
                    delete[] pLayout->createInfo.pBinding[i].pImmutableSamplers;
            }
            delete[] pLayout->createInfo.pBinding;
        }
        delete pLayout;
    }
    my_data->descriptorSetLayoutMap.clear();
}

// Currently clearing a set is removing all previous updates to that set
//  TODO : Validate if this is correct clearing behavior
static void clearDescriptorSet(layer_data* my_data, VkDescriptorSet set)
{
    SET_NODE* pSet = getSetNode(my_data, set);
    if (!pSet) {
        // TODO : Return error
    } else {
        loader_platform_thread_lock_mutex(&globalLock);
        freeShadowUpdateTree(pSet);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
}

static void clearDescriptorPool(layer_data* my_data, const VkDevice device, const VkDescriptorPool pool, VkDescriptorPoolResetFlags flags)
{
    DESCRIPTOR_POOL_NODE* pPool = getPoolNode(my_data, pool);
    if (!pPool) {
        log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_POOL, (uint64_t) pool, 0, DRAWSTATE_INVALID_POOL, "DS",
                "Unable to find pool node for pool %#" PRIxLEAST64 " specified in vkResetDescriptorPool() call", (uint64_t) pool);
    } else {
        // TODO: validate flags
        // For every set off of this pool, clear it
        SET_NODE* pSet = pPool->pSets;
        while (pSet) {
            clearDescriptorSet(my_data, pSet->set);
        }
        // Reset available count to max count for this pool
        for (uint32_t i=0; i<pPool->availableDescriptorTypeCount.size(); ++i) {
            pPool->availableDescriptorTypeCount[i] = pPool->maxDescriptorTypeCount[i];
        }
    }
}

// For given CB object, fetch associated CB Node from map
static GLOBAL_CB_NODE* getCBNode(layer_data* my_data, const VkCommandBuffer cb)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (my_data->commandBufferMap.count(cb) == 0) {
        loader_platform_thread_unlock_mutex(&globalLock);
        // TODO : How to pass cb as srcObj here?
        log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                "Attempt to use CommandBuffer %#" PRIxLEAST64 " that doesn't exist!", reinterpret_cast<uint64_t>(cb));
        return NULL;
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return my_data->commandBufferMap[cb];
}

// Free all CB Nodes
// NOTE : Calls to this function should be wrapped in mutex
static void deleteCommandBuffers(layer_data* my_data)
{
    if (my_data->commandBufferMap.size() <= 0) {
        return;
    }
    for (auto ii=my_data->commandBufferMap.begin(); ii!=my_data->commandBufferMap.end(); ++ii) {
        vector<CMD_NODE*> cmd_node_list = (*ii).second->pCmds;
        while (!cmd_node_list.empty()) {
            CMD_NODE* cmd_node = cmd_node_list.back();
            delete cmd_node;
            cmd_node_list.pop_back();
        }
        delete (*ii).second;
    }
    my_data->commandBufferMap.clear();
}

static VkBool32 report_error_no_cb_begin(const layer_data* dev_data, const VkCommandBuffer cb, const char* caller_name)
{
    // TODO : How to pass cb as srcObj here?
    return log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NO_BEGIN_COMMAND_BUFFER, "DS",
            "You must call vkBeginCommandBuffer() before this call to %s", caller_name);
}

bool validateCmdsInCmdBuffer(const layer_data* dev_data, const GLOBAL_CB_NODE* pCB, const CMD_TYPE cmd_type) {
    bool skip_call = false;
    for (auto cmd : pCB->pCmds) {
        if (cmd_type == CMD_EXECUTECOMMANDS && cmd->type != CMD_EXECUTECOMMANDS) {
            skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                                 "vkCmdExecuteCommands() cannot be called on a cmd buffer with exsiting commands.");
        }
        if (cmd_type != CMD_EXECUTECOMMANDS && cmd->type == CMD_EXECUTECOMMANDS) {
            skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                                 "Commands cannot be added to a cmd buffer with exsiting secondary commands.");
        }
    }
    return skip_call;
}

static VkBool32 addCmd(const layer_data* my_data, GLOBAL_CB_NODE* pCB, const CMD_TYPE cmd)
{
    VkBool32 skipCall = validateCmdsInCmdBuffer(my_data, pCB, cmd);
    CMD_NODE* pCmd = new CMD_NODE;
    if (pCmd) {
        // init cmd node and append to end of cmd LL
        memset(pCmd, 0, sizeof(CMD_NODE));
        pCmd->cmdNumber = ++pCB->numCmds;
        pCmd->type = cmd;
        pCB->pCmds.push_back(pCmd);
    } else {
        // TODO : How to pass cb as srcObj here?
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_OUT_OF_MEMORY, "DS",
                "Out of memory while attempting to allocate new CMD_NODE for commandBuffer %#" PRIxLEAST64, reinterpret_cast<uint64_t>(pCB->commandBuffer));
    }
    return skipCall;
}

static void resetCB(layer_data* my_data, const VkCommandBuffer cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(my_data, cb);
    if (pCB) {
        vector<CMD_NODE*> cmd_list = pCB->pCmds;
        while (!cmd_list.empty()) {
            delete cmd_list.back();
            cmd_list.pop_back();
        }
        pCB->pCmds.clear();
        // Reset CB state (need to save createInfo)
        VkCommandBufferAllocateInfo saveCBCI = pCB->createInfo;
        pCB->commandBuffer = cb;
        pCB->createInfo = saveCBCI;
        memset(&pCB->beginInfo, 0, sizeof(VkCommandBufferBeginInfo));
        pCB->fence = 0;
        pCB->numCmds = 0;
        memset(pCB->drawCount, 0, NUM_DRAW_TYPES * sizeof(uint64_t));
        pCB->state = CB_NEW;
        pCB->submitCount = 0;
        pCB->status = 0;
        pCB->pCmds.clear();
        pCB->lastBoundPipeline = 0;
        pCB->viewports.clear();
        pCB->scissors.clear();
        pCB->lineWidth = 0;
        pCB->depthBiasConstantFactor = 0;
        pCB->depthBiasClamp = 0;
        pCB->depthBiasSlopeFactor = 0;
        memset(pCB->blendConstants, 0, 4 * sizeof(float));
        pCB->minDepthBounds = 0;
        pCB->maxDepthBounds = 0;
        memset(&pCB->front, 0, sizeof(stencil_data));
        memset(&pCB->back, 0, sizeof(stencil_data));
        pCB->lastBoundDescriptorSet = 0;
        pCB->lastBoundPipelineLayout = 0;
        pCB->activeRenderPass = 0;
        pCB->activeSubpass = 0;
        pCB->framebuffer = 0;
        pCB->boundDescriptorSets.clear();
        pCB->imageLayoutMap.clear();
        pCB->lastVtxBinding = MAX_BINDING;
    }
}

// Set PSO-related status bits for CB, including dynamic state set via PSO
static void set_cb_pso_status(GLOBAL_CB_NODE* pCB, const PIPELINE_NODE* pPipe)
{
    for (uint32_t i = 0; i < pPipe->cbStateCI.attachmentCount; i++) {
        if (0 != pPipe->pAttachments[i].colorWriteMask) {
            pCB->status |= CBSTATUS_COLOR_BLEND_WRITE_ENABLE;
        }
    }
    if (pPipe->dsStateCI.depthWriteEnable) {
        pCB->status |= CBSTATUS_DEPTH_WRITE_ENABLE;
    }
    if (pPipe->dsStateCI.stencilTestEnable) {
        pCB->status |= CBSTATUS_STENCIL_TEST_ENABLE;
    }
    // Account for any dynamic state not set via this PSO
    if (!pPipe->dynStateCI.dynamicStateCount) { // All state is static
        pCB->status = CBSTATUS_ALL;
    } else {
        // First consider all state on
        // Then unset any state that's noted as dynamic in PSO
        // Finally OR that into CB statemask
        CBStatusFlags psoDynStateMask = CBSTATUS_ALL;
        for (uint32_t i=0; i < pPipe->dynStateCI.dynamicStateCount; i++) {
            switch (pPipe->dynStateCI.pDynamicStates[i]) {
                case VK_DYNAMIC_STATE_VIEWPORT:
                    psoDynStateMask &= ~CBSTATUS_VIEWPORT_SET;
                    break;
                case VK_DYNAMIC_STATE_SCISSOR:
                    psoDynStateMask &= ~CBSTATUS_SCISSOR_SET;
                    break;
                case VK_DYNAMIC_STATE_LINE_WIDTH:
                    psoDynStateMask &= ~CBSTATUS_LINE_WIDTH_SET;
                    break;
                case VK_DYNAMIC_STATE_DEPTH_BIAS:
                    psoDynStateMask &= ~CBSTATUS_DEPTH_BIAS_SET;
                    break;
                case VK_DYNAMIC_STATE_BLEND_CONSTANTS:
                    psoDynStateMask &= ~CBSTATUS_BLEND_SET;
                    break;
                case VK_DYNAMIC_STATE_DEPTH_BOUNDS:
                    psoDynStateMask &= ~CBSTATUS_DEPTH_BOUNDS_SET;
                    break;
                case VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK:
                    psoDynStateMask &= ~CBSTATUS_STENCIL_READ_MASK_SET;
                    break;
                case VK_DYNAMIC_STATE_STENCIL_WRITE_MASK:
                    psoDynStateMask &= ~CBSTATUS_STENCIL_WRITE_MASK_SET;
                    break;
                case VK_DYNAMIC_STATE_STENCIL_REFERENCE:
                    psoDynStateMask &= ~CBSTATUS_STENCIL_REFERENCE_SET;
                    break;
                default:
                    // TODO : Flag error here
                    break;
            }
        }
        pCB->status |= psoDynStateMask;
    }
}

// Print the last bound Gfx Pipeline
static VkBool32 printPipeline(layer_data* my_data, const VkCommandBuffer cb)
{
    VkBool32 skipCall = VK_FALSE;
    GLOBAL_CB_NODE* pCB = getCBNode(my_data, cb);
    if (pCB) {
        PIPELINE_NODE *pPipeTrav = getPipeline(my_data, pCB->lastBoundPipeline);
        if (!pPipeTrav) {
            // nothing to print
        } else {
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                    "%s", vk_print_vkgraphicspipelinecreateinfo(&pPipeTrav->graphicsPipelineCI, "{DS}").c_str());
        }
    }
    return skipCall;
}

// Print details of DS config to stdout
static VkBool32 printDSConfig(layer_data* my_data, const VkCommandBuffer cb)
{
    VkBool32 skipCall = VK_FALSE;
    char ds_config_str[1024*256] = {0}; // TODO : Currently making this buffer HUGE w/o overrun protection.  Need to be smarter, start smaller, and grow as needed.
    GLOBAL_CB_NODE* pCB = getCBNode(my_data, cb);
    if (pCB && pCB->lastBoundDescriptorSet) {
        SET_NODE* pSet = getSetNode(my_data, pCB->lastBoundDescriptorSet);
        DESCRIPTOR_POOL_NODE* pPool = getPoolNode(my_data, pSet->pool);
        // Print out pool details
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                "Details for pool %#" PRIxLEAST64 ".", (uint64_t) pPool->pool);
        string poolStr = vk_print_vkdescriptorpoolcreateinfo(&pPool->createInfo, " ");
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                "%s", poolStr.c_str());
        // Print out set details
        char prefix[10];
        uint32_t index = 0;
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                "Details for descriptor set %#" PRIxLEAST64 ".", (uint64_t) pSet->set);
        LAYOUT_NODE* pLayout = pSet->pLayout;
        // Print layout details
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                "Layout #%u, (object %#" PRIxLEAST64 ") for DS %#" PRIxLEAST64 ".", index+1, reinterpret_cast<uint64_t>(pLayout->layout), reinterpret_cast<uint64_t>(pSet->set));
        sprintf(prefix, "  [L%u] ", index);
        string DSLstr = vk_print_vkdescriptorsetlayoutcreateinfo(&pLayout->createInfo, prefix).c_str();
        skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                "%s", DSLstr.c_str());
        index++;
        GENERIC_HEADER* pUpdate = pSet->pUpdateStructs;
        if (pUpdate) {
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                    "Update Chain [UC] for descriptor set %#" PRIxLEAST64 ":", (uint64_t) pSet->set);
            sprintf(prefix, "  [UC] ");
            skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                    "%s", dynamic_display(pUpdate, prefix).c_str());
            // TODO : If there is a "view" associated with this update, print CI for that view
        } else {
            if (0 != pSet->descriptorCount) {
                skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                        "No Update Chain for descriptor set %#" PRIxLEAST64 " which has %u descriptors (vkUpdateDescriptors has not been called)", (uint64_t) pSet->set, pSet->descriptorCount);
            } else {
                skipCall |= log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                        "FYI: No descriptors in descriptor set %#" PRIxLEAST64 ".", (uint64_t) pSet->set);
            }
        }
    }
    return skipCall;
}

static void printCB(layer_data* my_data, const VkCommandBuffer cb)
{
    GLOBAL_CB_NODE* pCB = getCBNode(my_data, cb);
    if (pCB && pCB->pCmds.size() > 0) {
        log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NONE, "DS",
                "Cmds in CB %p", (void*)cb);
        vector<CMD_NODE*> pCmds = pCB->pCmds;
        for (auto ii=pCmds.begin(); ii!=pCmds.end(); ++ii) {
            // TODO : Need to pass cb as srcObj here
            log_msg(my_data->report_data, VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NONE, "DS",
                "  CMD#%" PRIu64 ": %s", (*ii)->cmdNumber, cmdTypeToString((*ii)->type).c_str());
        }
    } else {
        // Nothing to print
    }
}

static VkBool32 synchAndPrintDSConfig(layer_data* my_data, const VkCommandBuffer cb)
{
    VkBool32 skipCall = VK_FALSE;
    if (!(my_data->report_data->active_flags & VK_DBG_REPORT_INFO_BIT)) {
        return skipCall;
    }
    skipCall |= printDSConfig(my_data, cb);
    skipCall |= printPipeline(my_data, cb);
    return skipCall;
}

// Flags validation error if the associated call is made inside a render pass. The apiName
// routine should ONLY be called outside a render pass.
static VkBool32 insideRenderPass(const layer_data* my_data, GLOBAL_CB_NODE *pCB, const char *apiName)
{
    VkBool32 inside = VK_FALSE;
    if (pCB->activeRenderPass) {
        inside = log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER,
                         (uint64_t)pCB->commandBuffer, 0, DRAWSTATE_INVALID_RENDERPASS_CMD, "DS",
                         "%s: It is invalid to issue this call inside an active render pass (%#" PRIxLEAST64 ")",
                         apiName, (uint64_t) pCB->activeRenderPass);
    }
    return inside;
}

// Flags validation error if the associated call is made outside a render pass. The apiName
// routine should ONLY be called inside a render pass.
static VkBool32 outsideRenderPass(const layer_data* my_data, GLOBAL_CB_NODE *pCB, const char *apiName)
{
    VkBool32 outside = VK_FALSE;
    if (!pCB->activeRenderPass) {
        outside = log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER,
                          (uint64_t)pCB->commandBuffer, 0, DRAWSTATE_NO_ACTIVE_RENDERPASS, "DS",
                          "%s: This call must be issued inside an active render pass.", apiName);
    }
    return outside;
}

static void init_draw_state(layer_data *my_data)
{
    uint32_t report_flags = 0;
    uint32_t debug_action = 0;
    FILE *log_output = NULL;
    const char *option_str;
    VkDbgMsgCallback callback;
    // initialize DrawState options
    report_flags = getLayerOptionFlags("DrawStateReportFlags", 0);
    getLayerOptionEnum("DrawStateDebugAction", (uint32_t *) &debug_action);

    if (debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        option_str = getLayerOption("DrawStateLogFilename");
        log_output = getLayerLogOutput(option_str, "DrawState");
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

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
    // TODOSC : Shouldn't need any customization here
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

        init_draw_state(my_data);
    }
    return result;
}

/* hook DestroyInstance to remove tableInstanceMap entry */
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
    // TODOSC : Shouldn't need any customization here
    dispatch_key key = get_dispatch_key(instance);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    pTable->DestroyInstance(instance, pAllocator);

    // Clean up logging callback, if any
    while (my_data->logging_callback.size() > 0) {
        VkDbgMsgCallback callback = my_data->logging_callback.back();
        layer_destroy_msg_callback(my_data->report_data, callback);
        my_data->logging_callback.pop_back();
    }

    layer_debug_report_destroy_instance(my_data->report_data);
    delete my_data->instance_dispatch_table;
    layer_data_map.erase(key);
    // TODO : Potential race here with separate threads creating/destroying instance
    if (layer_data_map.empty()) {
        // Release mutex when destroying last instance.
        loader_platform_thread_delete_mutex(&globalLock);
        globalLockInitialized = 0;
    }
}

static void createDeviceRegisterExtensions(const VkDeviceCreateInfo* pCreateInfo, VkDevice device)
{
    uint32_t i;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    dev_data->device_extensions.debug_marker_enabled = false;
    dev_data->device_extensions.wsi_enabled = false;


    VkLayerDispatchTable    *pDisp = dev_data->device_dispatch_table;
    PFN_vkGetDeviceProcAddr  gpa   = pDisp->GetDeviceProcAddr;

    pDisp->CreateSwapchainKHR        = (PFN_vkCreateSwapchainKHR) gpa(device, "vkCreateSwapchainKHR");
    pDisp->DestroySwapchainKHR       = (PFN_vkDestroySwapchainKHR) gpa(device, "vkDestroySwapchainKHR");
    pDisp->GetSwapchainImagesKHR     = (PFN_vkGetSwapchainImagesKHR) gpa(device, "vkGetSwapchainImagesKHR");
    pDisp->AcquireNextImageKHR       = (PFN_vkAcquireNextImageKHR) gpa(device, "vkAcquireNextImageKHR");
    pDisp->QueuePresentKHR           = (PFN_vkQueuePresentKHR) gpa(device, "vkQueuePresentKHR");    

    for (i = 0; i < pCreateInfo->enabledExtensionNameCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            dev_data->device_extensions.wsi_enabled = true;
        }
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], DEBUG_MARKER_EXTENSION_NAME) == 0) {
            /* Found a matching extension name, mark it enabled and init dispatch table*/
            initDebugMarkerTable(device);
            dev_data->device_extensions.debug_marker_enabled = true;
        }
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateDevice(gpu, pCreateInfo, pAllocator, pDevice);
    // TODOSC : shouldn't need any customization here
    if (result == VK_SUCCESS) {
        layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(gpu), layer_data_map);
        dev_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
        createDeviceRegisterExtensions(pCreateInfo, *pDevice);
    }
    return result;
}

// prototype
static void deleteRenderPasses(layer_data*);
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
{
    // TODOSC : Shouldn't need any customization here
    dispatch_key key = get_dispatch_key(device);
    layer_data* dev_data = get_my_data_ptr(key, layer_data_map);
    // Free all the memory
    loader_platform_thread_lock_mutex(&globalLock);
    deletePipelines(dev_data);
    deleteRenderPasses(dev_data);
    deleteCommandBuffers(dev_data);
    deletePools(dev_data);
    deleteLayouts(dev_data);
    dev_data->imageViewMap.clear();
    dev_data->imageMap.clear();
    dev_data->bufferViewMap.clear();
    dev_data->bufferMap.clear();
    loader_platform_thread_unlock_mutex(&globalLock);

    dev_data->device_dispatch_table->DestroyDevice(device, pAllocator);
    tableDebugMarkerMap.erase(key);
    delete dev_data->device_dispatch_table;
    layer_data_map.erase(key);
}

static const VkLayerProperties ds_global_layers[] = {
    {
        "DrawState",
        VK_API_VERSION,
        VK_MAKE_VERSION(0, 1, 0),
        "Validation layer: DrawState",
    }
};

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
        const char *pLayerName,
        uint32_t *pCount,
        VkExtensionProperties* pProperties)
{
    /* DrawState does not have any global extensions */
    return util_GetExtensionProperties(0, NULL, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(
        uint32_t *pCount,
        VkLayerProperties*    pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(ds_global_layers),
                                   ds_global_layers,
                                   pCount, pProperties);
}

static const VkExtensionProperties ds_device_extensions[] = {
    {
        DEBUG_MARKER_EXTENSION_NAME,
        VK_MAKE_VERSION(0, 1, 0),
    }
};

static const VkLayerProperties ds_device_layers[] = {
    {
        "DrawState",
        VK_API_VERSION,
        VK_MAKE_VERSION(0, 1, 0),
        "Validation layer: DrawState",
    }
};

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(
        VkPhysicalDevice                            physicalDevice,
        const char*                                 pLayerName,
        uint32_t*                                   pCount,
        VkExtensionProperties*                      pProperties)
{
    // DrawState does not have any physical device extensions
    if (pLayerName == NULL) {
        dispatch_key key = get_dispatch_key(physicalDevice);
        layer_data *my_data = get_my_data_ptr(key, layer_data_map);
        return my_data->instance_dispatch_table->EnumerateDeviceExtensionProperties(
                                                    physicalDevice,
                                                    NULL,
                                                    pCount,
                                                    pProperties);
    } else {
        return util_GetExtensionProperties(ARRAY_SIZE(ds_device_extensions),
                                           ds_device_extensions,
                                           pCount, pProperties);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(
        VkPhysicalDevice                            physicalDevice,
        uint32_t*                                   pCount,
        VkLayerProperties*                          pProperties)
{
    /* DrawState physical device layers are the same as global */
    return util_GetLayerProperties(ARRAY_SIZE(ds_device_layers), ds_device_layers,
                                   pCount, pProperties);
}

bool ValidateCmdBufImageLayouts(VkCommandBuffer cmdBuffer) {
    bool skip_call = false;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    for (auto cb_image_data : pCB->imageLayoutMap) {
        auto image_data = dev_data->imageLayoutMap.find(cb_image_data.first);
        if (image_data == dev_data->imageLayoutMap.end()) {
            skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                 "Cannot submit cmd buffer using deleted image %" PRIu64 ".", reinterpret_cast<uint64_t>(cb_image_data.first));
        } else {
            if (dev_data->imageLayoutMap[cb_image_data.first]->layout != cb_image_data.second.initialLayout) {
                skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                     "Cannot submit cmd buffer using image with layout %d when first use is %d.", dev_data->imageLayoutMap[cb_image_data.first]->layout, cb_image_data.second.initialLayout);
            }
            dev_data->imageLayoutMap[cb_image_data.first]->layout = cb_image_data.second.layout;
        }
    }
    return skip_call;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence)
{
    VkBool32 skipCall = VK_FALSE;
    GLOBAL_CB_NODE* pCB = NULL;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    for (uint32_t submit_idx = 0; submit_idx < submitCount; submit_idx++) {
        const VkSubmitInfo *submit = &pSubmits[submit_idx];
        for (uint32_t i=0; i < submit->commandBufferCount; i++) {

#ifndef DISABLE_IMAGE_LAYOUT_VALIDATION
             skipCall |= ValidateCmdBufImageLayouts(submit->pCommandBuffers[i]);
#endif // DISABLE_IMAGE_LAYOUT_VALIDATION

            // Validate that cmd buffers have been updated
            pCB = getCBNode(dev_data, submit->pCommandBuffers[i]);
            loader_platform_thread_lock_mutex(&globalLock);
            pCB->submitCount++; // increment submit count
            if ((pCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) && (pCB->submitCount > 1)) {
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_COMMAND_BUFFER_SINGLE_SUBMIT_VIOLATION, "DS",
                        "CB %#" PRIxLEAST64 " was begun w/ VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT set, but has been submitted %#" PRIxLEAST64 " times.",
                        reinterpret_cast<uint64_t>(pCB->commandBuffer), pCB->submitCount);
            }
            if (CB_UPDATE_COMPLETE != pCB->state) {
                // Flag error for using CB w/o vkEndCommandBuffer() called
                // TODO : How to pass cb as srcObj?
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NO_END_COMMAND_BUFFER, "DS",
                        "You must call vkEndCommandBuffer() on CB %#" PRIxLEAST64 " before this call to vkQueueSubmit()!", reinterpret_cast<uint64_t>(pCB->commandBuffer));
                loader_platform_thread_unlock_mutex(&globalLock);
                return VK_ERROR_VALIDATION_FAILED;
            }
            loader_platform_thread_unlock_mutex(&globalLock);
        }
    }
    if (VK_FALSE == skipCall)
        return dev_data->device_dispatch_table->QueueSubmit(queue, submitCount, pSubmits, fence);
    return VK_ERROR_VALIDATION_FAILED;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyFence(device, fence, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroySemaphore(device, semaphore, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyEvent(device, event, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyQueryPool(device, queryPool, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    dev_data->device_dispatch_table->DestroyBuffer(device, buffer, pAllocator);
    dev_data->bufferMap.erase(buffer);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    dev_data->device_dispatch_table->DestroyBufferView(device, bufferView, pAllocator);
    dev_data->bufferViewMap.erase(bufferView);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    dev_data->device_dispatch_table->DestroyImage(device, image, pAllocator);
    dev_data->imageMap.erase(image);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyImageView(device, imageView, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyShaderModule(device, shaderModule, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyPipeline(device, pipeline, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyPipelineLayout(device, pipelineLayout, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroySampler(device, sampler, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyDescriptorPool(device, descriptorPool, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t count, const VkCommandBuffer *pCommandBuffers)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    for (auto i = 0; i < count; i++) {
        // Delete CB information structure, and remove from commandBufferMap
        auto cb = dev_data->commandBufferMap.find(pCommandBuffers[i]);
        loader_platform_thread_lock_mutex(&globalLock);
        if (cb != dev_data->commandBufferMap.end()) {
            delete (*cb).second;
            dev_data->commandBufferMap.erase(cb);
        }

        // Remove commandBuffer reference from commandPoolMap
        dev_data->commandPoolMap[commandPool].remove(pCommandBuffers[i]);
        loader_platform_thread_unlock_mutex(&globalLock);
    }

    dev_data->device_dispatch_table->FreeCommandBuffers(device, commandPool, count, pCommandBuffers);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    VkResult result = dev_data->device_dispatch_table->CreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);

    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->commandPoolMap[*pCommandPool];
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    loader_platform_thread_lock_mutex(&globalLock);

    // Must remove cmdpool from cmdpoolmap, after removing all cmdbuffers in its list from the commandPoolMap
    if (dev_data->commandPoolMap.find(commandPool) != dev_data->commandPoolMap.end()) {
        for (auto poolCb = dev_data->commandPoolMap[commandPool].begin(); poolCb != dev_data->commandPoolMap[commandPool].end();) {
            auto del_cb = dev_data->commandBufferMap.find(*poolCb);
            delete (*del_cb).second;                                        // delete CB info structure
            dev_data->commandBufferMap.erase(del_cb);                       // Remove this command buffer from cbMap
            poolCb = dev_data->commandPoolMap[commandPool].erase(poolCb);   // Remove CB reference from commandPoolMap's list
        }
    }
    dev_data->commandPoolMap.erase(commandPool);

    loader_platform_thread_unlock_mutex(&globalLock);
    dev_data->device_dispatch_table->DestroyCommandPool(device, commandPool, pAllocator);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyFramebuffer(device, framebuffer, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyRenderPass(device, renderPass, pAllocator);
    // TODO : Clean up any internal data structures using this obj.
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        // TODO : This doesn't create deep copy of pQueueFamilyIndices so need to fix that if/when we want that data to be valid
        dev_data->bufferMap[*pBuffer] = unique_ptr<VkBufferCreateInfo>(new VkBufferCreateInfo(*pCreateInfo));
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateBufferView(device, pCreateInfo, pAllocator, pView);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->bufferViewMap[*pView] = unique_ptr<VkBufferViewCreateInfo>(new VkBufferViewCreateInfo(*pCreateInfo));
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateImage(device, pCreateInfo, pAllocator, pImage);
    if (VK_SUCCESS == result) {
        IMAGE_NODE* image_node = new IMAGE_NODE;
        image_node->layout = pCreateInfo->initialLayout;
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->imageMap[*pImage] = unique_ptr<VkImageCreateInfo>(new VkImageCreateInfo(*pCreateInfo));
        dev_data->imageLayoutMap[*pImage] = image_node;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateImageView(device, pCreateInfo, pAllocator, pView);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->imageViewMap[*pView] = unique_ptr<VkImageViewCreateInfo>(new VkImageViewCreateInfo(*pCreateInfo));
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

//TODO handle pipeline caches
VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineCache(
    VkDevice                                    device,
    const VkPipelineCacheCreateInfo*            pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkPipelineCache*                            pPipelineCache)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreatePipelineCache(device, pCreateInfo, pAllocator, pPipelineCache);
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineCache(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    const VkAllocationCallbacks* pAllocator)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    dev_data->device_dispatch_table->DestroyPipelineCache(device, pipelineCache, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelineCacheData(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->GetPipelineCacheData(device, pipelineCache, pDataSize, pData);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkMergePipelineCaches(
    VkDevice                                    device,
    VkPipelineCache                             dstCache,
    uint32_t                                    srcCacheCount,
    const VkPipelineCache*                      pSrcCaches)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->MergePipelineCaches(device, dstCache, srcCacheCount, pSrcCaches);
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateGraphicsPipelines(
    VkDevice                            device,
    VkPipelineCache                     pipelineCache,
    uint32_t                            count,
    const VkGraphicsPipelineCreateInfo *pCreateInfos,
    const VkAllocationCallbacks        *pAllocator,
    VkPipeline                         *pPipelines)
{
    VkResult result = VK_SUCCESS;
    //TODO What to do with pipelineCache?
    // The order of operations here is a little convoluted but gets the job done
    //  1. Pipeline create state is first shadowed into PIPELINE_NODE struct
    //  2. Create state is then validated (which uses flags setup during shadowing)
    //  3. If everything looks good, we'll then create the pipeline and add NODE to pipelineMap
    VkBool32 skipCall = VK_FALSE;
    // TODO : Improve this data struct w/ unique_ptrs so cleanup below is automatic
    vector<PIPELINE_NODE*> pPipeNode(count);
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    uint32_t i=0;
    loader_platform_thread_lock_mutex(&globalLock);

    for (i=0; i<count; i++) {
        pPipeNode[i] = initGraphicsPipeline(dev_data, &pCreateInfos[i], NULL);
        skipCall |= verifyPipelineCreateState(dev_data, device, pPipeNode[i]);
    }

    loader_platform_thread_unlock_mutex(&globalLock);

    if (VK_FALSE == skipCall) {
        result = dev_data->device_dispatch_table->CreateGraphicsPipelines(device,
            pipelineCache, count, pCreateInfos, pAllocator, pPipelines);
        loader_platform_thread_lock_mutex(&globalLock);
        for (i=0; i<count; i++) {
            pPipeNode[i]->pipeline = pPipelines[i];
            dev_data->pipelineMap[pPipeNode[i]->pipeline] = pPipeNode[i];
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    } else {
        for (i=0; i<count; i++) {
            if (pPipeNode[i]) {
                // If we allocated a pipeNode, need to clean it up here
                delete[] pPipeNode[i]->pVertexBindingDescriptions;
                delete[] pPipeNode[i]->pVertexAttributeDescriptions;
                delete[] pPipeNode[i]->pAttachments;
                delete pPipeNode[i];
            }
        }
        return VK_ERROR_VALIDATION_FAILED;
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateComputePipelines(
    VkDevice                            device,
    VkPipelineCache                     pipelineCache,
    uint32_t                            count,
    const VkComputePipelineCreateInfo  *pCreateInfos,
    const VkAllocationCallbacks        *pAllocator,
    VkPipeline                         *pPipelines)
{
    VkResult result   = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;

    // TODO : Improve this data struct w/ unique_ptrs so cleanup below is automatic
    vector<PIPELINE_NODE*> pPipeNode(count);
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    uint32_t i=0;
    loader_platform_thread_lock_mutex(&globalLock);
    for (i=0; i<count; i++) {
        // TODO: Verify compute stage bits

        // Create and initialize internal tracking data structure
        pPipeNode[i] = new PIPELINE_NODE;
        memset((void*)pPipeNode[i], 0, sizeof(PIPELINE_NODE));
        memcpy(&pPipeNode[i]->computePipelineCI, (const void*)&pCreateInfos[i], sizeof(VkComputePipelineCreateInfo));

        // TODO: Add Compute Pipeline Verification
        // skipCall |= verifyPipelineCreateState(dev_data, device, pPipeNode[i]);
    }
    loader_platform_thread_unlock_mutex(&globalLock);

    if (VK_FALSE == skipCall) {
        result = dev_data->device_dispatch_table->CreateComputePipelines(device, pipelineCache, count, pCreateInfos, pAllocator, pPipelines);
        loader_platform_thread_lock_mutex(&globalLock);
        for (i=0; i<count; i++) {
            pPipeNode[i]->pipeline = pPipelines[i];
            dev_data->pipelineMap[pPipeNode[i]->pipeline] = pPipeNode[i];
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    } else {
        for (i=0; i<count; i++) {
            if (pPipeNode[i]) {
                // Clean up any locally allocated data structures
                delete pPipeNode[i];
            }
        }
        return VK_ERROR_VALIDATION_FAILED;
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateSampler(device, pCreateInfo, pAllocator, pSampler);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->sampleMap[*pSampler] = unique_ptr<SAMPLER_NODE>(new SAMPLER_NODE(pSampler, pCreateInfo));
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
    if (VK_SUCCESS == result) {
        // TODOSC : Capture layout bindings set
        LAYOUT_NODE* pNewNode = new LAYOUT_NODE;
        if (NULL == pNewNode) {
            if (log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t) *pSetLayout, 0, DRAWSTATE_OUT_OF_MEMORY, "DS",
                    "Out of memory while attempting to allocate LAYOUT_NODE in vkCreateDescriptorSetLayout()"))
                return VK_ERROR_VALIDATION_FAILED;
        }
        memcpy((void*)&pNewNode->createInfo, pCreateInfo, sizeof(VkDescriptorSetLayoutCreateInfo));
        pNewNode->createInfo.pBinding = new VkDescriptorSetLayoutBinding[pCreateInfo->bindingCount];
        memcpy((void*)pNewNode->createInfo.pBinding, pCreateInfo->pBinding, sizeof(VkDescriptorSetLayoutBinding)*pCreateInfo->bindingCount);
        // g++ does not like reserve with size 0
        if (pCreateInfo->bindingCount)
            pNewNode->bindings.reserve(pCreateInfo->bindingCount);
        uint32_t totalCount = 0;
        for (uint32_t i=0; i<pCreateInfo->bindingCount; i++) {
            if (!pNewNode->bindings.insert(pCreateInfo->pBinding[i].binding).second) {
                if (log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t) *pSetLayout, 0, DRAWSTATE_INVALID_LAYOUT, "DS",
                            "duplicated binding number in VkDescriptorSetLayoutBinding"))
                    return VK_ERROR_VALIDATION_FAILED;
            }

            totalCount += pCreateInfo->pBinding[i].descriptorCount;
            if (pCreateInfo->pBinding[i].pImmutableSamplers) {
                VkSampler** ppIS = (VkSampler**)&pNewNode->createInfo.pBinding[i].pImmutableSamplers;
                *ppIS = new VkSampler[pCreateInfo->pBinding[i].descriptorCount];
                memcpy(*ppIS, pCreateInfo->pBinding[i].pImmutableSamplers, pCreateInfo->pBinding[i].descriptorCount*sizeof(VkSampler));
            }
        }
        if (totalCount > 0) {
            pNewNode->descriptorTypes.resize(totalCount);
            pNewNode->stageFlags.resize(totalCount);
            uint32_t offset = 0;
            uint32_t j = 0;
            VkDescriptorType dType;
            for (uint32_t i=0; i<pCreateInfo->bindingCount; i++) {
                dType = pCreateInfo->pBinding[i].descriptorType;
                for (j = 0; j < pCreateInfo->pBinding[i].descriptorCount; j++) {
                    pNewNode->descriptorTypes[offset + j] = dType;
                    pNewNode->stageFlags[offset + j] = pCreateInfo->pBinding[i].stageFlags;
                    if ((dType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ||
                        (dType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) {
                        pNewNode->dynamicDescriptorCount++;
                    }
                }
                offset += j;
            }
        }
        pNewNode->layout = *pSetLayout;
        pNewNode->startIndex = 0;
        pNewNode->endIndex = pNewNode->startIndex + totalCount - 1;
        assert(pNewNode->endIndex >= pNewNode->startIndex);
        // Put new node at Head of global Layer list
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->descriptorSetLayoutMap[*pSetLayout] = pNewNode;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);
    if (VK_SUCCESS == result) {
        // TODOSC : Merge capture of the setLayouts per pipeline
        PIPELINE_LAYOUT_NODE& plNode = dev_data->pipelineLayoutMap[*pPipelineLayout];
        plNode.descriptorSetLayouts.resize(pCreateInfo->setLayoutCount);
        uint32_t i = 0;
        for (i=0; i<pCreateInfo->setLayoutCount; ++i) {
            plNode.descriptorSetLayouts[i] = pCreateInfo->pSetLayouts[i];
        }
        plNode.pushConstantRanges.resize(pCreateInfo->pushConstantRangeCount);
        for (i=0; i<pCreateInfo->pushConstantRangeCount; ++i) {
            plNode.pushConstantRanges[i] = pCreateInfo->pPushConstantRanges[i];
        }
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);
    if (VK_SUCCESS == result) {
        // Insert this pool into Global Pool LL at head
        if (log_msg(dev_data->report_data, VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DESCRIPTOR_POOL, (uint64_t) *pDescriptorPool, 0, DRAWSTATE_OUT_OF_MEMORY, "DS",
                "Created Descriptor Pool %#" PRIxLEAST64, (uint64_t) *pDescriptorPool))
            return VK_ERROR_VALIDATION_FAILED;
        loader_platform_thread_lock_mutex(&globalLock);
        DESCRIPTOR_POOL_NODE* pNewNode = new DESCRIPTOR_POOL_NODE(*pDescriptorPool, pCreateInfo);
        if (NULL == pNewNode) {
            if (log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_POOL, (uint64_t) *pDescriptorPool, 0, DRAWSTATE_OUT_OF_MEMORY, "DS",
                    "Out of memory while attempting to allocate DESCRIPTOR_POOL_NODE in vkCreateDescriptorPool()"))
                return VK_ERROR_VALIDATION_FAILED;
        } else {
            dev_data->descriptorPoolMap[*pDescriptorPool] = pNewNode;
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    } else {
        // Need to do anything if pool create fails?
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->ResetDescriptorPool(device, descriptorPool, flags);
    if (VK_SUCCESS == result) {
        clearDescriptorPool(dev_data, device, descriptorPool, flags);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    // Verify that requested descriptorSets are available in pool
    DESCRIPTOR_POOL_NODE *pPoolNode = getPoolNode(dev_data, pAllocateInfo->descriptorPool);
    if (!pPoolNode) {
        skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_POOL, (uint64_t) pAllocateInfo->descriptorPool, 0, DRAWSTATE_INVALID_POOL, "DS",
                "Unable to find pool node for pool %#" PRIxLEAST64 " specified in vkAllocateDescriptorSets() call", (uint64_t) pAllocateInfo->descriptorPool);
    } else { // Make sure pool has all the available descriptors before calling down chain
        skipCall |= validate_descriptor_availability_in_pool(dev_data, pPoolNode, pAllocateInfo->setLayoutCount, pAllocateInfo->pSetLayouts);
    }
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED;
    VkResult result = dev_data->device_dispatch_table->AllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);
    if (VK_SUCCESS == result) {
        DESCRIPTOR_POOL_NODE *pPoolNode = getPoolNode(dev_data, pAllocateInfo->descriptorPool);
        if (pPoolNode) {
            if (pAllocateInfo->setLayoutCount == 0) {
                log_msg(dev_data->report_data, VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, pAllocateInfo->setLayoutCount, 0, DRAWSTATE_NONE, "DS",
                        "AllocateDescriptorSets called with 0 count");
            }
            for (uint32_t i = 0; i < pAllocateInfo->setLayoutCount; i++) {
                log_msg(dev_data->report_data, VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pDescriptorSets[i], 0, DRAWSTATE_NONE, "DS",
                        "Created Descriptor Set %#" PRIxLEAST64, (uint64_t) pDescriptorSets[i]);
                // Create new set node and add to head of pool nodes
                SET_NODE* pNewNode = new SET_NODE;
                if (NULL == pNewNode) {
                    if (log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pDescriptorSets[i], 0, DRAWSTATE_OUT_OF_MEMORY, "DS",
                            "Out of memory while attempting to allocate SET_NODE in vkAllocateDescriptorSets()"))
                        return VK_ERROR_VALIDATION_FAILED;
                } else {
                    memset(pNewNode, 0, sizeof(SET_NODE));
                    // TODO : Pool should store a total count of each type of Descriptor available
                    //  When descriptors are allocated, decrement the count and validate here
                    //  that the count doesn't go below 0. One reset/free need to bump count back up.
                    // Insert set at head of Set LL for this pool
                    pNewNode->pNext = pPoolNode->pSets;
                    pPoolNode->pSets = pNewNode;
                    LAYOUT_NODE* pLayout = getLayoutNode(dev_data, pAllocateInfo->pSetLayouts[i]);
                    if (NULL == pLayout) {
                        if (log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t) pAllocateInfo->pSetLayouts[i], 0, DRAWSTATE_INVALID_LAYOUT, "DS",
                                "Unable to find set layout node for layout %#" PRIxLEAST64 " specified in vkAllocateDescriptorSets() call", (uint64_t) pAllocateInfo->pSetLayouts[i]))
                            return VK_ERROR_VALIDATION_FAILED;
                    }
                    pNewNode->pLayout = pLayout;
                    pNewNode->pool = pAllocateInfo->descriptorPool;
                    pNewNode->set = pDescriptorSets[i];
                    pNewNode->descriptorCount = pLayout->endIndex + 1;
                    if (pNewNode->descriptorCount) {
                        size_t descriptorArraySize = sizeof(GENERIC_HEADER*)*pNewNode->descriptorCount;
                        pNewNode->ppDescriptors = new GENERIC_HEADER*[descriptorArraySize];
                        memset(pNewNode->ppDescriptors, 0, descriptorArraySize);
                    }
                    dev_data->setMap[pDescriptorSets[i]] = pNewNode;
                }
            }
        }
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t count, const VkDescriptorSet* pDescriptorSets)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    DESCRIPTOR_POOL_NODE *pPoolNode = getPoolNode(dev_data, descriptorPool);
    if (pPoolNode && !(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT & pPoolNode->createInfo.flags)) {
        // Can't Free from a NON_FREE pool
        skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE, (uint64_t)device, 0, DRAWSTATE_CANT_FREE_FROM_NON_FREE_POOL, "DS",
                    "It is invalid to call vkFreeDescriptorSets() with a pool created without setting VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT.");
    }
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED;
    VkResult result = dev_data->device_dispatch_table->FreeDescriptorSets(device, descriptorPool, count, pDescriptorSets);
    if (VK_SUCCESS == result) {
        // For each freed descriptor add it back into the pool as available
        for (uint32_t i=0; i<count; ++i) {
            SET_NODE* pSet = dev_data->setMap[pDescriptorSets[i]]; // getSetNode() without locking
            LAYOUT_NODE* pLayout = pSet->pLayout;
            uint32_t typeIndex = 0, poolSizeCount = 0;
            for (uint32_t j=0; j<pLayout->createInfo.bindingCount; ++j) {
                typeIndex = static_cast<uint32_t>(pLayout->createInfo.pBinding[j].descriptorType);
                poolSizeCount = pLayout->createInfo.pBinding[j].descriptorCount;
                pPoolNode->availableDescriptorTypeCount[typeIndex] += poolSizeCount;
            }
        }
    }
    // TODO : Any other clean-up or book-keeping to do here?
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies)
{
    // dsUpdate will return VK_TRUE only if a bailout error occurs, so we want to call down tree when update returns VK_FALSE
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    if (!dsUpdate(dev_data, device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies)) {
        dev_data->device_dispatch_table->UpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pCreateInfo, VkCommandBuffer* pCommandBuffer)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->AllocateCommandBuffers(device, pCreateInfo, pCommandBuffer);
    if (VK_SUCCESS == result) {
        for (auto i = 0; i < pCreateInfo->bufferCount; i++) {
            // Validate command pool
            if (dev_data->commandPoolMap.find(pCreateInfo->commandPool) != dev_data->commandPoolMap.end()) {
                loader_platform_thread_lock_mutex(&globalLock);
                // Add command buffer to its commandPool map
                dev_data->commandPoolMap[pCreateInfo->commandPool].push_back(pCommandBuffer[i]);
                GLOBAL_CB_NODE* pCB = new GLOBAL_CB_NODE;
                // Add command buffer to map
                dev_data->commandBufferMap[pCommandBuffer[i]] = pCB;
                loader_platform_thread_unlock_mutex(&globalLock);
                resetCB(dev_data, pCommandBuffer[i]);
                pCB->commandBuffer = pCommandBuffer[i];
                pCB->createInfo    = *pCreateInfo;
            }
        }
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo)
{
    VkBool32 skipCall = false;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    // Validate command buffer level
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->createInfo.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
            if (pBeginInfo->renderPass || pBeginInfo->framebuffer) {
                // These should be NULL for a Primary CB
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS",
                    "vkBeginCommandBuffer():  Primary Command Buffer (%p) may not specify framebuffer or renderpass parameters", (void*)commandBuffer);
            }
        } else {
            if (!pBeginInfo->renderPass || !pBeginInfo->framebuffer) {
                // These should NOT be null for an Secondary CB
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_BEGIN_CB_INVALID_STATE, "DS",
                    "vkBeginCommandBuffer():  Secondary Command Buffers (%p) must specify framebuffer and renderpass parameters", (void*)commandBuffer);
            }
        }
        pCB->beginInfo = *pBeginInfo;
    } else {
        // TODO : Need to pass commandBuffer as objType here
        skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                "In vkBeginCommandBuffer() and unable to find CommandBuffer Node for CB %p!", (void*)commandBuffer);
    }
    if (skipCall) {
        return VK_ERROR_VALIDATION_FAILED;
    }
    VkResult result = dev_data->device_dispatch_table->BeginCommandBuffer(commandBuffer, pBeginInfo);
    if ((VK_SUCCESS == result) && (pCB != NULL)) {
        if (CB_NEW != pCB->state) {
            resetCB(dev_data, commandBuffer);
        }
        pCB->state = CB_UPDATE_ACTIVE;
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEndCommandBuffer(VkCommandBuffer commandBuffer)
{
    VkBool32 skipCall = VK_FALSE;
    VkResult result = VK_SUCCESS;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    /* TODO: preference is to always call API function after reporting any validation errors */
    if (pCB) {
        if (pCB->state != CB_UPDATE_ACTIVE) {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkEndCommandBuffer()");
        }
    }
    if (VK_FALSE == skipCall) {
        result = dev_data->device_dispatch_table->EndCommandBuffer(commandBuffer);
        if (VK_SUCCESS == result) {
            pCB->state = CB_UPDATE_COMPLETE;
            // Reset CB status flags
            pCB->status = 0;
            printCB(dev_data, commandBuffer);
        }
    } else {
        result = VK_ERROR_VALIDATION_FAILED;
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->ResetCommandBuffer(commandBuffer, flags);
    if (VK_SUCCESS == result) {
        resetCB(dev_data, commandBuffer);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_BINDPIPELINE);
            if ((VK_PIPELINE_BIND_POINT_COMPUTE == pipelineBindPoint) && (pCB->activeRenderPass)) {
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PIPELINE, (uint64_t) pipeline,
                                    0, DRAWSTATE_INVALID_RENDERPASS_CMD, "DS",
                                    "Incorrectly binding compute pipeline (%#" PRIxLEAST64 ") during active RenderPass (%#" PRIxLEAST64 ")",
                                    (uint64_t) pipeline, (uint64_t) pCB->activeRenderPass);
            } else if ((VK_PIPELINE_BIND_POINT_GRAPHICS == pipelineBindPoint) && (!pCB->activeRenderPass)) {
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PIPELINE, (uint64_t) pipeline,
                                    0, DRAWSTATE_NO_ACTIVE_RENDERPASS, "DS", "Incorrectly binding graphics pipeline "
                                    " (%#" PRIxLEAST64 ") without an active RenderPass", (uint64_t) pipeline);
            } else {
                PIPELINE_NODE* pPN = getPipeline(dev_data, pipeline);
                if (pPN) {
                    pCB->lastBoundPipeline = pipeline;
                    loader_platform_thread_lock_mutex(&globalLock);
                    set_cb_pso_status(pCB, pPN);
                    g_lastBoundPipeline = pPN;
                    loader_platform_thread_unlock_mutex(&globalLock);
                    skipCall |= validatePipelineState(dev_data, pCB, pipelineBindPoint, pipeline);
                } else {
                    skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PIPELINE, (uint64_t) pipeline,
                                        0, DRAWSTATE_INVALID_PIPELINE, "DS",
                                        "Attempt to bind Pipeline %#" PRIxLEAST64 " that doesn't exist!", reinterpret_cast<uint64_t>(pipeline));
                }
            }
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdBindPipeline()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetViewport(
    VkCommandBuffer                         commandBuffer,
    uint32_t                            viewportCount,
    const VkViewport*                   pViewports)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_SETVIEWPORTSTATE);
            loader_platform_thread_lock_mutex(&globalLock);
            pCB->status |= CBSTATUS_VIEWPORT_SET;
            pCB->viewports.resize(viewportCount);
            memcpy(pCB->viewports.data(), pViewports, viewportCount * sizeof(VkViewport));
            loader_platform_thread_unlock_mutex(&globalLock);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdSetViewport()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetViewport(commandBuffer, viewportCount, pViewports);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetScissor(
    VkCommandBuffer                         commandBuffer,
    uint32_t                            scissorCount,
    const VkRect2D*                     pScissors)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_SETSCISSORSTATE);
            loader_platform_thread_lock_mutex(&globalLock);
            pCB->status |= CBSTATUS_SCISSOR_SET;
            pCB->scissors.resize(scissorCount);
            memcpy(pCB->scissors.data(), pScissors, scissorCount * sizeof(VkRect2D));
            loader_platform_thread_unlock_mutex(&globalLock);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdSetScissor()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetScissor(commandBuffer, scissorCount, pScissors);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_SETLINEWIDTHSTATE);
            /* TODO: Do we still need this lock? */
            loader_platform_thread_lock_mutex(&globalLock);
            pCB->status |= CBSTATUS_LINE_WIDTH_SET;
            pCB->lineWidth = lineWidth;
            loader_platform_thread_unlock_mutex(&globalLock);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdBindDynamicLineWidthState()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetLineWidth(commandBuffer, lineWidth);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBias(
    VkCommandBuffer                         commandBuffer,
    float                               depthBiasConstantFactor,
    float                               depthBiasClamp,
    float                               depthBiasSlopeFactor)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_SETDEPTHBIASSTATE);
            pCB->status |= CBSTATUS_DEPTH_BIAS_SET;
            pCB->depthBiasConstantFactor = depthBiasConstantFactor;
            pCB->depthBiasClamp = depthBiasClamp;
            pCB->depthBiasSlopeFactor = depthBiasSlopeFactor;
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdSetDepthBias()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetDepthBias(commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4])
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_SETBLENDSTATE);
            pCB->status |= CBSTATUS_BLEND_SET;
            memcpy(pCB->blendConstants, blendConstants, 4 * sizeof(float));
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdSetBlendConstants()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetBlendConstants(commandBuffer, blendConstants);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBounds(
    VkCommandBuffer                         commandBuffer,
    float                               minDepthBounds,
    float                               maxDepthBounds)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_SETDEPTHBOUNDSSTATE);
            pCB->status |= CBSTATUS_DEPTH_BOUNDS_SET;
            pCB->minDepthBounds = minDepthBounds;
            pCB->maxDepthBounds = maxDepthBounds;
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdSetDepthBounds()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetDepthBounds(commandBuffer, minDepthBounds, maxDepthBounds);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilCompareMask(
    VkCommandBuffer                         commandBuffer,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            compareMask)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_SETSTENCILREADMASKSTATE);
            if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
                pCB->front.compareMask = compareMask;
            }
            if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
                pCB->back.compareMask = compareMask;
            }
            /* TODO: Do we need to track front and back separately? */
            /* TODO: We aren't capturing the faceMask, do we need to? */
            pCB->status |= CBSTATUS_STENCIL_READ_MASK_SET;
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdSetStencilCompareMask()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetStencilCompareMask(commandBuffer, faceMask, compareMask);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilWriteMask(
    VkCommandBuffer                         commandBuffer,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            writeMask)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_SETSTENCILWRITEMASKSTATE);
            if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
                pCB->front.writeMask = writeMask;
            }
            if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
                pCB->back.writeMask = writeMask;
            }
            pCB->status |= CBSTATUS_STENCIL_WRITE_MASK_SET;
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdSetStencilWriteMask()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetStencilWriteMask(commandBuffer, faceMask, writeMask);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilReference(
    VkCommandBuffer                         commandBuffer,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            reference)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_SETSTENCILREFERENCESTATE);
            if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
                pCB->front.reference = reference;
            }
            if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
                pCB->back.reference = reference;
            }
            pCB->status |= CBSTATUS_STENCIL_REFERENCE_SET;
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdSetStencilReference()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetStencilReference(commandBuffer, faceMask, reference);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t setCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            if ((VK_PIPELINE_BIND_POINT_COMPUTE == pipelineBindPoint) && (pCB->activeRenderPass)) {
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_RENDERPASS_CMD, "DS",
                        "Incorrectly binding compute DescriptorSets during active RenderPass (%#" PRIxLEAST64 ")", (uint64_t) pCB->activeRenderPass);
            } else if ((VK_PIPELINE_BIND_POINT_GRAPHICS == pipelineBindPoint) && (!pCB->activeRenderPass)) {
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_NO_ACTIVE_RENDERPASS, "DS",
                        "Incorrectly binding graphics DescriptorSets without an active RenderPass");
            } else {
                // Track total count of dynamic descriptor types to make sure we have an offset for each one
                uint32_t totalDynamicDescriptors = 0;
                string errorString = "";
                uint32_t lastSetIndex = firstSet+setCount-1;
                if (lastSetIndex >= pCB->boundDescriptorSets.size())
                    pCB->boundDescriptorSets.resize(lastSetIndex+1);
                VkDescriptorSet oldFinalBoundSet = pCB->boundDescriptorSets[lastSetIndex];
                for (uint32_t i=0; i<setCount; i++) {
                    SET_NODE* pSet = getSetNode(dev_data, pDescriptorSets[i]);
                    if (pSet) {
                        loader_platform_thread_lock_mutex(&globalLock);
                        pCB->lastBoundDescriptorSet = pDescriptorSets[i];
                        pCB->lastBoundPipelineLayout = layout;
                        pCB->boundDescriptorSets[i+firstSet] = pDescriptorSets[i];
                        loader_platform_thread_unlock_mutex(&globalLock);
                        skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pDescriptorSets[i], 0, DRAWSTATE_NONE, "DS",
                                "DS %#" PRIxLEAST64 " bound on pipeline %s", (uint64_t) pDescriptorSets[i], string_VkPipelineBindPoint(pipelineBindPoint));
                        if (!pSet->pUpdateStructs) {
                            skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pDescriptorSets[i], 0, DRAWSTATE_DESCRIPTOR_SET_NOT_UPDATED, "DS",
                                    "DS %#" PRIxLEAST64 " bound but it was never updated. You may want to either update it or not bind it.", (uint64_t) pDescriptorSets[i]);
                        }
                        // Verify that set being bound is compatible with overlapping setLayout of pipelineLayout
                        if (!verify_set_layout_compatibility(dev_data, pSet, layout, i+firstSet, errorString)) {
                            skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pDescriptorSets[i], 0, DRAWSTATE_PIPELINE_LAYOUTS_INCOMPATIBLE, "DS",
                                    "descriptorSet #%u being bound is not compatible with overlapping layout in pipelineLayout due to: %s", i, errorString.c_str());
                        }
                        totalDynamicDescriptors += pSet->pLayout->dynamicDescriptorCount;
                    } else {
                        skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pDescriptorSets[i], 0, DRAWSTATE_INVALID_SET, "DS",
                                "Attempt to bind DS %#" PRIxLEAST64 " that doesn't exist!", (uint64_t) pDescriptorSets[i]);
                    }
                }
                skipCall |= addCmd(dev_data, pCB, CMD_BINDDESCRIPTORSETS);
                // For any previously bound sets, need to set them to "invalid" if they were disturbed by this update
                if (firstSet > 0) { // Check set #s below the first bound set
                    for (uint32_t i=0; i<firstSet; ++i) {
                        if (pCB->boundDescriptorSets[i] && !verify_set_layout_compatibility(dev_data, dev_data->setMap[pCB->boundDescriptorSets[i]], layout, i, errorString)) {
                            skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_PERF_WARN_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) pCB->boundDescriptorSets[i], 0, DRAWSTATE_NONE, "DS",
                                "DescriptorSetDS %#" PRIxLEAST64 " previously bound as set #%u was disturbed by newly bound pipelineLayout (%#" PRIxLEAST64 ")", (uint64_t) pCB->boundDescriptorSets[i], i, (uint64_t) layout);
                            pCB->boundDescriptorSets[i] = VK_NULL_HANDLE;
                        }
                    }
                }
                // Check if newly last bound set invalidates any remaining bound sets
                if ((pCB->boundDescriptorSets.size()-1) > (lastSetIndex)) {
                    if (oldFinalBoundSet && !verify_set_layout_compatibility(dev_data, dev_data->setMap[oldFinalBoundSet], layout, lastSetIndex, errorString)) {
                        skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_PERF_WARN_BIT, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) oldFinalBoundSet, 0, DRAWSTATE_NONE, "DS",
                            "DescriptorSetDS %#" PRIxLEAST64 " previously bound as set #%u is incompatible with set %#" PRIxLEAST64 " newly bound as set #%u so set #%u and any subsequent sets were disturbed by newly bound pipelineLayout (%#" PRIxLEAST64 ")", (uint64_t) oldFinalBoundSet, lastSetIndex, (uint64_t) pCB->boundDescriptorSets[lastSetIndex], lastSetIndex, lastSetIndex+1, (uint64_t) layout);
                        pCB->boundDescriptorSets.resize(lastSetIndex+1);
                    }
                }
                //  dynamicOffsetCount must equal the total number of dynamic descriptors in the sets being bound
                if (totalDynamicDescriptors != dynamicOffsetCount) {
                    skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t) commandBuffer, 0, DRAWSTATE_INVALID_DYNAMIC_OFFSET_COUNT, "DS",
                            "Attempting to bind %u descriptorSets with %u dynamic descriptors, but dynamicOffsetCount is %u. It should exactly match the number of dynamic descriptors.", setCount, totalDynamicDescriptors, dynamicOffsetCount);
                }
            }
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdBindDescriptorSets()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, setCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            VkDeviceSize offset_align = 0;
            switch (indexType) {
                case VK_INDEX_TYPE_UINT16:
                    offset_align = 2;
                    break;
                case VK_INDEX_TYPE_UINT32:
                    offset_align = 4;
                    break;
                default:
                    // ParamChecker should catch bad enum, we'll also throw alignment error below if offset_align stays 0
                    break;
            }
            if (!offset_align || (offset % offset_align)) {
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_VTX_INDEX_ALIGNMENT_ERROR, "DS",
                    "vkCmdBindIndexBuffer() offset (%#" PRIxLEAST64 ") does not fall on alignment (%s) boundary.", offset, string_VkIndexType(indexType));
            }
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdBindIndexBuffer()");
        }
        pCB->status |= CBSTATUS_INDEX_BUFFER_BOUND;
        skipCall |= addCmd(dev_data, pCB, CMD_BINDINDEXBUFFER);
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindVertexBuffers(
    VkCommandBuffer                                 commandBuffer,
    uint32_t                                    startBinding,
    uint32_t                                    bindingCount,
    const VkBuffer*                             pBuffers,
    const VkDeviceSize*                         pOffsets)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            /* TODO: Need to track all the vertex buffers, not just last one */
            pCB->lastVtxBinding = startBinding + bindingCount -1;
            addCmd(dev_data, pCB, CMD_BINDVERTEXBUFFER);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdBindVertexBuffer()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBindVertexBuffers(commandBuffer, startBinding, bindingCount, pBuffers, pOffsets);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            pCB->drawCount[DRAW]++;
            skipCall |= validate_draw_state(dev_data, pCB, VK_FALSE);
            // TODO : Need to pass commandBuffer as srcObj here
            skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NONE, "DS",
                    "vkCmdDraw() call #%" PRIu64 ", reporting DS state:", g_drawCount[DRAW]++);
            skipCall |= synchAndPrintDSConfig(dev_data, commandBuffer);
            if (VK_FALSE == skipCall) {
                skipCall |= addCmd(dev_data, pCB, CMD_DRAW);
            }
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdDraw()");
        }
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdDraw");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    VkBool32 skipCall = VK_FALSE;
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            pCB->drawCount[DRAW_INDEXED]++;
            skipCall |= validate_draw_state(dev_data, pCB, VK_TRUE);
            // TODO : Need to pass commandBuffer as srcObj here
            skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NONE, "DS",
                    "vkCmdDrawIndexed() call #%" PRIu64 ", reporting DS state:", g_drawCount[DRAW_INDEXED]++);
            skipCall |= synchAndPrintDSConfig(dev_data, commandBuffer);
            if (VK_FALSE == skipCall) {
                skipCall |= addCmd(dev_data, pCB, CMD_DRAWINDEXED);
            }
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdDrawIndexed()");
        }
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdDrawIndexed");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    VkBool32 skipCall = VK_FALSE;
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            pCB->drawCount[DRAW_INDIRECT]++;
            skipCall |= validate_draw_state(dev_data, pCB, VK_FALSE);
            // TODO : Need to pass commandBuffer as srcObj here
            skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NONE, "DS",
                    "vkCmdDrawIndirect() call #%" PRIu64 ", reporting DS state:", g_drawCount[DRAW_INDIRECT]++);
            skipCall |= synchAndPrintDSConfig(dev_data, commandBuffer);
            if (VK_FALSE == skipCall) {
                skipCall |= addCmd(dev_data, pCB, CMD_DRAWINDIRECT);
            }
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdDrawIndirect()");
        }
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdDrawIndirect");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDrawIndirect(commandBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            pCB->drawCount[DRAW_INDEXED_INDIRECT]++;
            skipCall |= validate_draw_state(dev_data, pCB, VK_TRUE);
            // TODO : Need to pass commandBuffer as srcObj here
            skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_NONE, "DS",
                    "vkCmdDrawIndexedIndirect() call #%" PRIu64 ", reporting DS state:", g_drawCount[DRAW_INDEXED_INDIRECT]++);
            skipCall |= synchAndPrintDSConfig(dev_data, commandBuffer);
            if (VK_FALSE == skipCall) {
                skipCall |= addCmd(dev_data, pCB, CMD_DRAWINDEXEDINDIRECT);
            }
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdDrawIndexedIndirect()");
        }
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdDrawIndexedIndirect");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDrawIndexedIndirect(commandBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDispatch(VkCommandBuffer commandBuffer, uint32_t x, uint32_t y, uint32_t z)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_DISPATCH);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdDispatch()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdDispatch");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDispatch(commandBuffer, x, y, z);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_DISPATCHINDIRECT);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdDispatchIndirect()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdDispatchIndirect");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdDispatchIndirect(commandBuffer, buffer, offset);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_COPYBUFFER);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdCopyBuffer()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyBuffer");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

bool VerifySourceImageLayout(VkCommandBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout) {
    bool skip_call = false;

#ifdef DISABLE_IMAGE_LAYOUT_VALIDATION
    // TODO: Fix -- initialLayout may have been set in a previous command buffer
    return skip_call;
#endif // DISABLE_IMAGE_LAYOUT_VALIDATION

    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    auto src_image_element = pCB->imageLayoutMap.find(srcImage);
    if (src_image_element == pCB->imageLayoutMap.end()) {
        pCB->imageLayoutMap[srcImage].initialLayout = srcImageLayout;
        pCB->imageLayoutMap[srcImage].layout = srcImageLayout;
        return false;
    }
    if (src_image_element->second.layout != srcImageLayout) {
        skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                             "Cannot copy from an image whose source layout is %d and doesn't match the current layout %d.", srcImageLayout, src_image_element->second.layout);
    }
    if (srcImageLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        if (srcImageLayout == VK_IMAGE_LAYOUT_GENERAL) {
            skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                 "Layout for input image should be TRANSFER_SRC_OPTIMAL instead of GENERAL.");
        } else {
            skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                 "Layout for input image is %d but can only be TRANSFER_SRC_OPTIMAL or GENERAL.", srcImageLayout);
        }
    }
    return skip_call;
}

bool VerifyDestImageLayout(VkCommandBuffer cmdBuffer, VkImage destImage, VkImageLayout destImageLayout) {
    bool skip_call = false;

#ifdef DISABLE_IMAGE_LAYOUT_VALIDATION
    // TODO: Fix -- initialLayout may have been set in a previous command buffer
    return skip_call;
#endif // DISABLE_IMAGE_LAYOUT_VALIDATION

    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    auto dest_image_element = pCB->imageLayoutMap.find(destImage);
    if (dest_image_element == pCB->imageLayoutMap.end()) {
        pCB->imageLayoutMap[destImage].initialLayout = destImageLayout;
        pCB->imageLayoutMap[destImage].layout = destImageLayout;
        return false;
    }
    if (dest_image_element->second.layout != destImageLayout) {
        skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                             "Cannot copy from an image whose dest layout is %d and doesn't match the current layout %d.", destImageLayout, dest_image_element->second.layout);
    }
    if (destImageLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        if (destImageLayout == VK_IMAGE_LAYOUT_GENERAL) {
            skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                 "Layout for output image should be TRANSFER_DST_OPTIMAL instead of GENERAL.");
        } else {
            skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                 "Layout for output image is %d but can only be TRANSFER_DST_OPTIMAL or GENERAL.", destImageLayout);
        }
    }
    return skip_call;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyImage(VkCommandBuffer commandBuffer,
                                             VkImage srcImage,
                                             VkImageLayout srcImageLayout,
                                             VkImage dstImage,
                                             VkImageLayout dstImageLayout,
                                             uint32_t regionCount, const VkImageCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_COPYIMAGE);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdCopyImage()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyImage");
        skipCall |= VerifySourceImageLayout(commandBuffer, srcImage, srcImageLayout);
        skipCall |= VerifyDestImageLayout(commandBuffer, dstImage, dstImageLayout);
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBlitImage(VkCommandBuffer commandBuffer,
                                             VkImage srcImage, VkImageLayout srcImageLayout,
                                             VkImage dstImage, VkImageLayout dstImageLayout,
                                             uint32_t regionCount, const VkImageBlit* pRegions,
                                             VkFilter filter)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_BLITIMAGE);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdBlitImage()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdBlitImage");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer,
                                                     VkBuffer srcBuffer,
                                                     VkImage dstImage, VkImageLayout dstImageLayout,
                                                     uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_COPYBUFFERTOIMAGE);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdCopyBufferToImage()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyBufferToImage");
        skipCall |= VerifyDestImageLayout(commandBuffer, dstImage, dstImageLayout);
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer,
                                                     VkImage srcImage, VkImageLayout srcImageLayout,
                                                     VkBuffer dstBuffer,
                                                     uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_COPYIMAGETOBUFFER);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdCopyImageToBuffer()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyImageToBuffer");
        skipCall |= VerifySourceImageLayout(commandBuffer, srcImage, srcImageLayout);
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const uint32_t* pData)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_UPDATEBUFFER);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdUpdateBuffer()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyUpdateBuffer");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_FILLBUFFER);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdFillBuffer()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyFillBuffer");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearAttachments(
    VkCommandBuffer                                 commandBuffer,
    uint32_t                                    attachmentCount,
    const VkClearAttachment*                    pAttachments,
    uint32_t                                    rectCount,
    const VkClearRect*                          pRects)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            // Warn if this is issued prior to Draw Cmd
            if (!hasDrawCmd(pCB)) {
                // TODO : commandBuffer should be srcObj
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, DRAWSTATE_CLEAR_CMD_BEFORE_DRAW, "DS",
                        "vkCmdClearAttachments() issued on CB object 0x%" PRIxLEAST64 " prior to any Draw Cmds."
                        " It is recommended you use RenderPass LOAD_OP_CLEAR on Attachments prior to any Draw.", reinterpret_cast<uint64_t>(commandBuffer));
            }
            skipCall |= addCmd(dev_data, pCB, CMD_CLEARATTACHMENTS);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdClearAttachments()");
        }
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdClearAttachments");
    }

    // Validate that attachment is in reference list of active subpass
    if (pCB->activeRenderPass) {
        const VkRenderPassCreateInfo *pRPCI = dev_data->renderPassMap[pCB->activeRenderPass]->pCreateInfo;
        const VkSubpassDescription   *pSD   = &pRPCI->pSubpasses[pCB->activeSubpass];

        for (uint32_t attachment_idx = 0; attachment_idx < attachmentCount; attachment_idx++) {
            const VkClearAttachment *attachment = &pAttachments[attachment_idx];
            if (attachment->aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) {
                VkBool32 found = VK_FALSE;
                for (uint32_t i = 0; i < pSD->colorAttachmentCount; i++) {
                    if (attachment->colorAttachment == pSD->pColorAttachments[i].attachment) {
                        found = VK_TRUE;
                        break;
                    }
                }
                if (VK_FALSE == found) {
                    skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER,
                            (uint64_t)commandBuffer, 0, DRAWSTATE_MISSING_ATTACHMENT_REFERENCE, "DS",
                            "vkCmdClearAttachments() attachment index %d not found in attachment reference array of active subpass %d",
                            attachment->colorAttachment, pCB->activeSubpass);
                }
            } else if (attachment->aspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) {
                if (!pSD->pDepthStencilAttachment                                      ||  // Says no DS will be used in active subpass
                    (pSD->pDepthStencilAttachment->attachment == VK_ATTACHMENT_UNUSED)) {  // Says no DS will be used in active subpass

                    skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER,
                        (uint64_t)commandBuffer, 0, DRAWSTATE_MISSING_ATTACHMENT_REFERENCE, "DS",
                        "vkCmdClearAttachments() attachment index %d does not match depthStencilAttachment.attachment (%d) found in active subpass %d",
                        attachment->colorAttachment,
                        (pSD->pDepthStencilAttachment) ? pSD->pDepthStencilAttachment->attachment : VK_ATTACHMENT_UNUSED,
                        pCB->activeSubpass);
                }
            }
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearColorImage(
        VkCommandBuffer commandBuffer,
        VkImage image, VkImageLayout imageLayout,
        const VkClearColorValue *pColor,
        uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_CLEARCOLORIMAGE);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdClearColorImage()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdClearColorImage");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearDepthStencilImage(
        VkCommandBuffer commandBuffer,
        VkImage image, VkImageLayout imageLayout,
        const VkClearDepthStencilValue *pDepthStencil,
        uint32_t rangeCount,
        const VkImageSubresourceRange* pRanges)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_CLEARDEPTHSTENCILIMAGE);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdClearDepthStencilImage()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdClearDepthStencilImage");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResolveImage(VkCommandBuffer commandBuffer,
                                                VkImage srcImage, VkImageLayout srcImageLayout,
                                                VkImage dstImage, VkImageLayout dstImageLayout,
                                                uint32_t regionCount, const VkImageResolve* pRegions)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_RESOLVEIMAGE);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdResolveImage()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdResolveImage");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_SETEVENT);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdSetEvent()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdSetEvent");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdSetEvent(commandBuffer, event, stageMask);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_RESETEVENT);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdResetEvent()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdResetEvent");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdResetEvent(commandBuffer, event, stageMask);
}

bool TransitionImageLayouts(VkCommandBuffer cmdBuffer, uint32_t memBarrierCount, const void* const* ppMemBarriers) {
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    bool skip = false;

#ifdef DISABLE_IMAGE_LAYOUT_VALIDATION
    // TODO: Fix -- pay attention to image subresource ranges -- not all subresources transition at the same time
    return skip;
#endif // DISABLE_IMAGE_LAYOUT_VALIDATION

    for (uint32_t i = 0; i < memBarrierCount; ++i) {
        auto mem_barrier = reinterpret_cast<const VkMemoryBarrier*>(ppMemBarriers[i]);
        if (mem_barrier && mem_barrier->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            auto image_mem_barrier = reinterpret_cast<const VkImageMemoryBarrier*>(mem_barrier);
            auto image_data = pCB->imageLayoutMap.find(image_mem_barrier->image);
            if (image_data == pCB->imageLayoutMap.end()) {
                pCB->imageLayoutMap[image_mem_barrier->image].initialLayout = image_mem_barrier->oldLayout;
                pCB->imageLayoutMap[image_mem_barrier->image].layout = image_mem_barrier->newLayout;
            } else {
                if (image_data->second.layout != image_mem_barrier->oldLayout) {
                    skip |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "You cannot transition the layout from %s when current layout is %s.",
                                    string_VkImageLayout(image_mem_barrier->oldLayout), string_VkImageLayout(image_data->second.layout));
                }
                image_data->second.layout = image_mem_barrier->newLayout;
            }
        }
    }
    return skip;
}

// AccessFlags MUST have 'required_bit' set, and may have one or more of 'optional_bits' set.
// If required_bit is zero, accessMask must have at least one of 'optional_bits' set
bool ValidateMaskBits(const layer_data* my_data, VkCommandBuffer cmdBuffer, const VkAccessFlags& accessMask, const VkImageLayout& layout, VkAccessFlags required_bit, VkAccessFlags optional_bits) {
    bool skip_call = false;
    if ((accessMask & required_bit) || (!required_bit && (accessMask & optional_bits))) {
        if (accessMask & !(required_bit | optional_bits)) {
            skip_call |= log_msg(my_data->report_data, VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_BARRIER, "DS",
                                 "Additional bits in accessMask %d are specified when layout is %s.", accessMask, string_VkImageLayout(layout));
        }
    } else {
        if (!required_bit) {
            skip_call |= log_msg(my_data->report_data, VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_BARRIER, "DS",
                                 "AccessMask %d must contain at least one of access bits %d when layout is %s.",
                                  accessMask, optional_bits, string_VkImageLayout(layout));
        } else {
            skip_call |= log_msg(my_data->report_data, VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_BARRIER, "DS",
                                 "AccessMask %d must have required access bit %d and may have optional bits %d when layout is %s.",
                                  accessMask, required_bit, optional_bits,string_VkImageLayout(layout));
        }
    }
    return skip_call;
}

bool ValidateMaskBitsFromLayouts(const layer_data* my_data, VkCommandBuffer cmdBuffer, const VkAccessFlags& accessMask, const VkImageLayout& layout) {
    bool skip_call = false;
    switch (layout) {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
            skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
            break;
        }
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
            skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT);
            break;
        }
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
            skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, VK_ACCESS_TRANSFER_WRITE_BIT, 0);
            break;
        }
        case VK_IMAGE_LAYOUT_PREINITIALIZED: {
            skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, VK_ACCESS_HOST_WRITE_BIT, 0);
            break;
        }
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL: {
            skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT);
            break;
        }
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
            skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, 0, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT);
            break;
        }
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
            skip_call |= ValidateMaskBits(my_data, cmdBuffer, accessMask, layout, VK_ACCESS_TRANSFER_READ_BIT, 0);
            break;
        }
        case VK_IMAGE_LAYOUT_UNDEFINED: {
            if (accessMask != 0) {
                skip_call |= log_msg(my_data->report_data, VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_BARRIER, "DS",
                                     "Additional bits in accessMask %d are specified when layout is %s.", accessMask, string_VkImageLayout(layout));
            }
            break;
        }
        case VK_IMAGE_LAYOUT_GENERAL:
        default: {
            break;
        }
    }
    return skip_call;
}

bool ValidateBarriers(VkCommandBuffer cmdBuffer, uint32_t memBarrierCount, const void* const* ppMemBarriers) {
    bool skip_call = false;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    if (pCB->activeRenderPass && memBarrierCount) {
        for (uint32_t i = 0; i < memBarrierCount; ++i) {
            auto mem_barrier = reinterpret_cast<const VkMemoryBarrier*>(ppMemBarriers[i]);
            if (mem_barrier && mem_barrier->sType != VK_STRUCTURE_TYPE_MEMORY_BARRIER) {
                skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_BARRIER, "DS",
                                     "Image or Buffers Barriers cannot be used during a render pass.");
            }
        }
        if (!dev_data->renderPassMap[pCB->activeRenderPass]->hasSelfDependency[pCB->activeSubpass]) {
            skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_BARRIER, "DS",
                                 "Barriers cannot be set during subpass %d with no self dependency specified.", pCB->activeSubpass);
        }
    }

    for (uint32_t i = 0; i < memBarrierCount; ++i) {
        auto mem_barrier = reinterpret_cast<const VkMemoryBarrier*>(ppMemBarriers[i]);
        if (mem_barrier && mem_barrier->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            auto image_mem_barrier = reinterpret_cast<const VkImageMemoryBarrier*>(mem_barrier);
            skip_call |= ValidateMaskBitsFromLayouts(dev_data, cmdBuffer, image_mem_barrier->srcAccessMask, image_mem_barrier->oldLayout);
            skip_call |= ValidateMaskBitsFromLayouts(dev_data, cmdBuffer, image_mem_barrier->dstAccessMask, image_mem_barrier->newLayout);
        }
    }

    return skip_call;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags sourceStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const void* const* ppMemoryBarriers)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_WAITEVENTS);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdWaitEvents()");
        }
        skipCall |= TransitionImageLayouts(commandBuffer, memoryBarrierCount, ppMemoryBarriers);
        skipCall |= ValidateBarriers(commandBuffer, memoryBarrierCount, ppMemoryBarriers);
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdWaitEvents(commandBuffer, eventCount, pEvents, sourceStageMask, dstStageMask, memoryBarrierCount, ppMemoryBarriers);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const void* const* ppMemoryBarriers)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_PIPELINEBARRIER);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdPipelineBarrier()");
        }
        skipCall |= TransitionImageLayouts(commandBuffer, memoryBarrierCount, ppMemoryBarriers);
        skipCall |= ValidateBarriers(commandBuffer, memoryBarrierCount, ppMemoryBarriers);
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, ppMemoryBarriers);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t slot, VkFlags flags)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_BEGINQUERY);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdBeginQuery()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdBeginQuery(commandBuffer, queryPool, slot, flags);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t slot)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_ENDQUERY);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdEndQuery()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdEndQuery(commandBuffer, queryPool, slot);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_RESETQUERYPOOL);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdResetQueryPool()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdQueryPool");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdResetQueryPool(commandBuffer, queryPool, startQuery, queryCount);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t startQuery,
                                                     uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset,
                                                     VkDeviceSize stride, VkQueryResultFlags flags)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_COPYQUERYPOOLRESULTS);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdCopyQueryPoolResults()");
        }
        skipCall |= insideRenderPass(dev_data, pCB, "vkCmdCopyQueryPoolResults");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdCopyQueryPoolResults(commandBuffer, queryPool,
                           startQuery, queryCount, dstBuffer, dstOffset, stride, flags);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t slot)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pCB->state == CB_UPDATE_ACTIVE) {
            skipCall |= addCmd(dev_data, pCB, CMD_WRITETIMESTAMP);
        } else {
            skipCall |= report_error_no_cb_begin(dev_data, commandBuffer, "vkCmdWriteTimestamp()");
        }
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdWriteTimestamp(commandBuffer, pipelineStage, queryPool, slot);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);
    if (VK_SUCCESS == result) {
        // Shadow create info and store in map
        VkFramebufferCreateInfo* localFBCI = new VkFramebufferCreateInfo(*pCreateInfo);
        if (pCreateInfo->pAttachments) {
            localFBCI->pAttachments = new VkImageView[localFBCI->attachmentCount];
            memcpy((void*)localFBCI->pAttachments, pCreateInfo->pAttachments, localFBCI->attachmentCount*sizeof(VkImageView));
        }
        dev_data->frameBufferMap[*pFramebuffer] = localFBCI;
    }
    return result;
}

// Store the DAG.
struct DAGNode {
    uint32_t pass;
    std::vector<uint32_t> prev;
    std::vector<uint32_t> next;
};

VkBool32 FindDependency(const int index, const int dependent, const std::vector<DAGNode>& subpass_to_node, std::unordered_set<uint32_t>& processed_nodes) {
    // If we have already checked this node we have not found a dependency path so return false.
    if (processed_nodes.count(index))
        return false;
    processed_nodes.insert(index);
    const DAGNode& node = subpass_to_node[index];
    // Look for a dependency path. If one exists return true else recurse on the previous nodes.
    if (std::find(node.prev.begin(), node.prev.end(), dependent) == node.prev.end()) {
        for (auto elem : node.prev) {
            if (FindDependency(elem, dependent, subpass_to_node, processed_nodes))
                return true;
        }
    } else {
        return true;
    }
    return false;
}

VkBool32 CheckDependencyExists(const layer_data* my_data, VkDevice device, const int subpass, const std::vector<uint32_t>& dependent_subpasses, const std::vector<DAGNode>& subpass_to_node, VkBool32& skip_call) {
    VkBool32 result = true;
    // Loop through all subpasses that share the same attachment and make sure a dependency exists
    for (uint32_t k = 0; k < dependent_subpasses.size(); ++k) {
        if (subpass == dependent_subpasses[k])
            continue;
        const DAGNode& node = subpass_to_node[subpass];
        // Check for a specified dependency between the two nodes. If one exists we are done.
        auto prev_elem = std::find(node.prev.begin(), node.prev.end(), dependent_subpasses[k]);
        auto next_elem = std::find(node.next.begin(), node.next.end(), dependent_subpasses[k]);
        if (prev_elem == node.prev.end() && next_elem == node.next.end()) {
            // If no dependency exits an implicit dependency still might. If so, warn and if not throw an error.
            std::unordered_set<uint32_t> processed_nodes;
            if (FindDependency(subpass, dependent_subpasses[k], subpass_to_node, processed_nodes) ||
                FindDependency(dependent_subpasses[k], subpass, subpass_to_node, processed_nodes)) {
                skip_call |= log_msg(my_data->report_data, VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                     "A dependency between subpasses %d and %d must exist but only an implicit one is specified.",
                                     subpass, dependent_subpasses[k]);
            } else {
                skip_call |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                     "A dependency between subpasses %d and %d must exist but one is not specified.",
                                     subpass, dependent_subpasses[k]);
                result = false;
            }
        }
    }
    return result;
}

VkBool32 CheckPreserved(const layer_data* my_data, VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const int index, const int attachment, const std::vector<DAGNode>& subpass_to_node, int depth, VkBool32& skip_call) {
    const DAGNode& node = subpass_to_node[index];
    // If this node writes to the attachment return true as next nodes need to preserve the attachment.
    const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[index];
    for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
        if (attachment == subpass.pColorAttachments[j].attachment)
            return VK_TRUE;
    }
    if (subpass.pDepthStencilAttachment &&
        subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
        if (attachment == subpass.pDepthStencilAttachment->attachment)
            return VK_TRUE;
    }
    VkBool32 result = VK_FALSE;
    // Loop through previous nodes and see if any of them write to the attachment.
    for (auto elem : node.prev) {
        result |= CheckPreserved(my_data, device, pCreateInfo, elem, attachment, subpass_to_node, depth + 1, skip_call);
    }
    // If the attachment was written to by a previous node than this node needs to preserve it.
    if (result && depth > 0) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[index];
        VkBool32 has_preserved = false;
        for (uint32_t j = 0; j < subpass.preserveAttachmentCount; ++j) {
            if (subpass.pPreserveAttachments[j].attachment == attachment) {
                has_preserved = true;
                break;
            }
        }
        if (!has_preserved) {
            skip_call |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                 "Attachment %d is used by a later subpass and must be preserved in subpass %d.", attachment, index);
        }
    }
    return result;
}

VkBool32 ValidateDependencies(const layer_data* my_data, VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, std::vector<DAGNode>& subpass_to_node) {
   VkBool32 skip_call = false;
    std::vector<std::vector<uint32_t>> output_attachment_to_subpass(pCreateInfo->attachmentCount);
    std::vector<std::vector<uint32_t>> input_attachment_to_subpass(pCreateInfo->attachmentCount);

    // Create DAG
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        DAGNode& subpass_node = subpass_to_node[i];
        subpass_node.pass = i;
    }
    for (uint32_t i = 0; i < pCreateInfo->dependencyCount; ++i) {
        const VkSubpassDependency& dependency = pCreateInfo->pDependencies[i];
        if (dependency.srcSubpass > dependency.dstSubpass) {
            skip_call |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                 "Dependency graph must be specified such that an earlier pass cannot depend on a later pass.");
        }
        subpass_to_node[dependency.dstSubpass].prev.push_back(dependency.srcSubpass);
        subpass_to_node[dependency.srcSubpass].next.push_back(dependency.dstSubpass);
    }

    // Find for each attachment the subpasses that use them.
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[i];
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            input_attachment_to_subpass[subpass.pInputAttachments[j].attachment].push_back(i);
        }
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            output_attachment_to_subpass[subpass.pColorAttachments[j].attachment].push_back(i);
        }
        if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
            output_attachment_to_subpass[subpass.pDepthStencilAttachment->attachment].push_back(i);
        }
    }
    // If there is a dependency needed make sure one exists
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[i];
        // If the attachment is an input then all subpasses that output must have a dependency relationship
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            const uint32_t& attachment = subpass.pInputAttachments[j].attachment;
            CheckDependencyExists(my_data, device, i, output_attachment_to_subpass[attachment], subpass_to_node, skip_call);
        }
        // If the attachment is an output then all subpasses that use the attachment must have a dependency relationship
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            const uint32_t& attachment = subpass.pColorAttachments[j].attachment;
            CheckDependencyExists(my_data, device, i, output_attachment_to_subpass[attachment], subpass_to_node, skip_call);
            CheckDependencyExists(my_data, device, i, input_attachment_to_subpass[attachment], subpass_to_node, skip_call);
        }
        if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
            const uint32_t& attachment = subpass.pDepthStencilAttachment->attachment;
            CheckDependencyExists(my_data, device, i, output_attachment_to_subpass[attachment], subpass_to_node, skip_call);
            CheckDependencyExists(my_data, device, i, input_attachment_to_subpass[attachment], subpass_to_node, skip_call);
        }
    }
    // Loop through implicit dependencies, if this pass reads make sure the attachment is preserved for all passes after it was written.
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[i];
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            CheckPreserved(my_data, device, pCreateInfo, i, subpass.pInputAttachments[j].attachment, subpass_to_node, 0, skip_call);
        }
    }
    return skip_call;
}

bool ValidateLayouts(const layer_data* my_data, VkDevice device, const VkRenderPassCreateInfo* pCreateInfo) {
    bool skip = false;

#ifdef DISABLE_IMAGE_LAYOUT_VALIDATION
    return skip;
#endif // DISABLE_IMAGE_LAYOUT_VALIDATION

    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription& subpass = pCreateInfo->pSubpasses[i];
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            if (subpass.pInputAttachments[j].layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL &&
                subpass.pInputAttachments[j].layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                if (subpass.pInputAttachments[j].layout == VK_IMAGE_LAYOUT_GENERAL) {
                    skip |= log_msg(my_data->report_data, VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for input attachment is GENERAL but should be READ_ONLY_OPTIMAL.");
                } else {
                    skip |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for input attachment is %d but can only be READ_ONLY_OPTIMAL or GENERAL.", subpass.pInputAttachments[j].attachment);
                }
            }
        }
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            if (subpass.pColorAttachments[j].layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
                if (subpass.pColorAttachments[j].layout == VK_IMAGE_LAYOUT_GENERAL) {
                    skip |= log_msg(my_data->report_data, VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for color attachment is GENERAL but should be COLOR_ATTACHMENT_OPTIMAL.");
                } else {
                    skip |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for color attachment is %d but can only be COLOR_ATTACHMENT_OPTIMAL or GENERAL.", subpass.pColorAttachments[j].attachment);
                }
            }
        }
        if ((subpass.pDepthStencilAttachment != NULL) &&
            (subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED)) {
            if (subpass.pDepthStencilAttachment->layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
                if (subpass.pDepthStencilAttachment->layout == VK_IMAGE_LAYOUT_GENERAL) {
                    skip |= log_msg(my_data->report_data, VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for depth attachment is GENERAL but should be DEPTH_STENCIL_ATTACHMENT_OPTIMAL.");
                } else {
                    skip |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for depth attachment is %d but can only be DEPTH_STENCIL_ATTACHMENT_OPTIMAL or GENERAL.", subpass.pDepthStencilAttachment->attachment);
                }
            }
        }
    }
    return skip;
}

bool CreatePassDAG(const layer_data* my_data, VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, std::vector<DAGNode>& subpass_to_node, std::vector<bool>& has_self_dependency) {
    bool skip_call = false;
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        DAGNode& subpass_node = subpass_to_node[i];
        subpass_node.pass = i;
    }
    for (uint32_t i = 0; i < pCreateInfo->dependencyCount; ++i) {
        const VkSubpassDependency& dependency = pCreateInfo->pDependencies[i];
        if (dependency.srcSubpass > dependency.dstSubpass) {
            skip_call |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                 "Depedency graph must be specified such that an earlier pass cannot depend on a later pass.");
        } else if (dependency.srcSubpass == dependency.dstSubpass) {
            has_self_dependency[dependency.srcSubpass] = true;
        }
        subpass_to_node[dependency.dstSubpass].prev.push_back(dependency.srcSubpass);
        subpass_to_node[dependency.srcSubpass].next.push_back(dependency.dstSubpass);
    }
    return skip_call;
}
// TODOSC : Add intercept of vkCreateShaderModule

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
        my_data->shaderModuleMap[*pShaderModule] = new shader_module(pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return res;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass)
{
    bool skip_call = false;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    // Create DAG
    std::vector<bool> has_self_dependency(pCreateInfo->subpassCount);
    std::vector<DAGNode> subpass_to_node(pCreateInfo->subpassCount);
    skip_call |= CreatePassDAG(dev_data, device, pCreateInfo, subpass_to_node, has_self_dependency);
    // Validate using DAG
    skip_call |= ValidateDependencies(dev_data, device, pCreateInfo, subpass_to_node);
    skip_call |= ValidateLayouts(dev_data, device, pCreateInfo);
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED;
    }
    VkResult result = dev_data->device_dispatch_table->CreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
    if (VK_SUCCESS == result) {
        // TODOSC : Merge in tracking of renderpass from ShaderChecker
        // Shadow create info and store in map
        VkRenderPassCreateInfo* localRPCI = new VkRenderPassCreateInfo(*pCreateInfo);
        if (pCreateInfo->pAttachments) {
            localRPCI->pAttachments = new VkAttachmentDescription[localRPCI->attachmentCount];
            memcpy((void*)localRPCI->pAttachments, pCreateInfo->pAttachments, localRPCI->attachmentCount*sizeof(VkAttachmentDescription));
        }
        if (pCreateInfo->pSubpasses) {
            localRPCI->pSubpasses = new VkSubpassDescription[localRPCI->subpassCount];
            memcpy((void*)localRPCI->pSubpasses, pCreateInfo->pSubpasses, localRPCI->subpassCount*sizeof(VkSubpassDescription));

            for (uint32_t i = 0; i < localRPCI->subpassCount; i++) {
                VkSubpassDescription *subpass = (VkSubpassDescription *) &localRPCI->pSubpasses[i];
                const uint32_t attachmentCount = subpass->inputAttachmentCount +
                    subpass->colorAttachmentCount * (1 + (subpass->pResolveAttachments?1:0)) +
                    ((subpass->pDepthStencilAttachment) ? 1 : 0) + subpass->preserveAttachmentCount;
                VkAttachmentReference *attachments = new VkAttachmentReference[attachmentCount];

                memcpy(attachments, subpass->pInputAttachments,
                        sizeof(attachments[0]) * subpass->inputAttachmentCount);
                subpass->pInputAttachments = attachments;
                attachments += subpass->inputAttachmentCount;

                memcpy(attachments, subpass->pColorAttachments,
                        sizeof(attachments[0]) * subpass->colorAttachmentCount);
                subpass->pColorAttachments = attachments;
                attachments += subpass->colorAttachmentCount;

                if (subpass->pResolveAttachments) {
                    memcpy(attachments, subpass->pResolveAttachments,
                            sizeof(attachments[0]) * subpass->colorAttachmentCount);
                    subpass->pResolveAttachments = attachments;
                    attachments += subpass->colorAttachmentCount;
                }

                if (subpass->pDepthStencilAttachment) {
                    memcpy(attachments, subpass->pDepthStencilAttachment,
                            sizeof(attachments[0]) * 1);
                    subpass->pDepthStencilAttachment = attachments;
                    attachments += 1;
                }

                memcpy(attachments, subpass->pPreserveAttachments,
                        sizeof(attachments[0]) * subpass->preserveAttachmentCount);
                subpass->pPreserveAttachments = attachments;
            }
        }
        if (pCreateInfo->pDependencies) {
            localRPCI->pDependencies = new VkSubpassDependency[localRPCI->dependencyCount];
            memcpy((void*)localRPCI->pDependencies, pCreateInfo->pDependencies, localRPCI->dependencyCount*sizeof(VkSubpassDependency));
        }
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->renderPassMap[*pRenderPass] = new RENDER_PASS_NODE(localRPCI);
        dev_data->renderPassMap[*pRenderPass]->hasSelfDependency = has_self_dependency;
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}
// Free the renderpass shadow
static void deleteRenderPasses(layer_data* my_data)
{
    if (my_data->renderPassMap.size() <= 0)
        return;
    for (auto ii=my_data->renderPassMap.begin(); ii!=my_data->renderPassMap.end(); ++ii) {
        const VkRenderPassCreateInfo* pRenderPassInfo = (*ii).second->pCreateInfo;
        if (pRenderPassInfo->pAttachments) {
            delete[] pRenderPassInfo->pAttachments;
        }
        if (pRenderPassInfo->pSubpasses) {
            for (uint32_t i=0; i<pRenderPassInfo->subpassCount; ++i) {
                // Attachements are all allocated in a block, so just need to
                //  find the first non-null one to delete
                if (pRenderPassInfo->pSubpasses[i].pInputAttachments) {
                    delete[] pRenderPassInfo->pSubpasses[i].pInputAttachments;
                } else if (pRenderPassInfo->pSubpasses[i].pColorAttachments) {
                    delete[] pRenderPassInfo->pSubpasses[i].pColorAttachments;
                } else if (pRenderPassInfo->pSubpasses[i].pResolveAttachments) {
                    delete[] pRenderPassInfo->pSubpasses[i].pResolveAttachments;
                } else if (pRenderPassInfo->pSubpasses[i].pPreserveAttachments) {
                    delete[] pRenderPassInfo->pSubpasses[i].pPreserveAttachments;
                }
            }
            delete[] pRenderPassInfo->pSubpasses;
        }
        if (pRenderPassInfo->pDependencies) {
            delete[] pRenderPassInfo->pDependencies;
        }
        delete pRenderPassInfo;
        delete (*ii).second;
    }
    my_data->renderPassMap.clear();
}

bool VerifyFramebufferAndRenderPassLayouts(VkCommandBuffer cmdBuffer, const VkRenderPassBeginInfo* pRenderPassBegin) {
    bool skip_call = false;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    const VkRenderPassCreateInfo* pRenderPassInfo = dev_data->renderPassMap[pRenderPassBegin->renderPass]->pCreateInfo;
    const VkFramebufferCreateInfo* pFramebufferInfo = dev_data->frameBufferMap[pRenderPassBegin->framebuffer];
    if (pRenderPassInfo->attachmentCount != pFramebufferInfo->attachmentCount) {
        skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                             "You cannot start a render pass using a framebuffer with a different number of attachments.");
    }
    for (uint32_t i = 0; i < pRenderPassInfo->attachmentCount; ++i) {
        const VkImageView& image_view = pFramebufferInfo->pAttachments[i];
        const VkImage& image = dev_data->imageViewMap[image_view]->image;
        auto image_data = pCB->imageLayoutMap.find(image);
        if (image_data == pCB->imageLayoutMap.end()) {
            pCB->imageLayoutMap[image].initialLayout = pRenderPassInfo->pAttachments[i].initialLayout;
            pCB->imageLayoutMap[image].layout = pRenderPassInfo->pAttachments[i].initialLayout;
        } else if (pRenderPassInfo->pAttachments[i].initialLayout != image_data->second.layout) {
            skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                 "You cannot start a render pass using attachment %i where the intial layout differs from the starting layout.", i);
        }
    }
    return skip_call;
}

void TransitionSubpassLayouts(VkCommandBuffer cmdBuffer, const VkRenderPassBeginInfo* pRenderPassBegin, const int subpass_index) {
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    auto render_pass_data = dev_data->renderPassMap.find(pRenderPassBegin->renderPass);
    if (render_pass_data == dev_data->renderPassMap.end()) {
        return;
    }
    const VkRenderPassCreateInfo* pRenderPassInfo = render_pass_data->second->pCreateInfo;
    auto framebuffer_data = dev_data->frameBufferMap.find(pRenderPassBegin->framebuffer);
    if (framebuffer_data == dev_data->frameBufferMap.end()) {
        return;
    }
    const VkFramebufferCreateInfo* pFramebufferInfo = framebuffer_data->second;
    const VkSubpassDescription& subpass = pRenderPassInfo->pSubpasses[subpass_index];
    for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
        const VkImageView& image_view = pFramebufferInfo->pAttachments[subpass.pInputAttachments[j].attachment];
        auto image_view_data = dev_data->imageViewMap.find(image_view);
        if (image_view_data !=  dev_data->imageViewMap.end()) {
            auto image_layout = pCB->imageLayoutMap.find(image_view_data->second->image);
            if (image_layout != pCB->imageLayoutMap.end()) {
                image_layout->second.layout = subpass.pInputAttachments[j].layout;
            }
        }
    }
    for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
        const VkImageView& image_view = pFramebufferInfo->pAttachments[subpass.pColorAttachments[j].attachment];
        auto image_view_data = dev_data->imageViewMap.find(image_view);
        if (image_view_data !=  dev_data->imageViewMap.end()) {
            auto image_layout = pCB->imageLayoutMap.find(image_view_data->second->image);
            if (image_layout != pCB->imageLayoutMap.end()) {
                image_layout->second.layout = subpass.pColorAttachments[j].layout;
            }
        }
    }
    if ((subpass.pDepthStencilAttachment != NULL) &&
        (subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED)) {
        const VkImageView& image_view = pFramebufferInfo->pAttachments[subpass.pDepthStencilAttachment->attachment];
        auto image_view_data = dev_data->imageViewMap.find(image_view);
        if (image_view_data !=  dev_data->imageViewMap.end()) {
            auto image_layout = pCB->imageLayoutMap.find(image_view_data->second->image);
            if (image_layout != pCB->imageLayoutMap.end()) {
                image_layout->second.layout = subpass.pDepthStencilAttachment->layout;
            }
        }
    }
}

bool validatePrimaryCommandBuffer(const layer_data* my_data, const GLOBAL_CB_NODE* pCB, const std::string& cmd_name) {
    bool skip_call = false;
    if (pCB->createInfo.level != VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
        skip_call |= log_msg(my_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                             "Cannot execute command %s on a secondary command buffer.", cmd_name.c_str());
    }
    return skip_call;
}

void TransitionFinalSubpassLayouts(VkCommandBuffer cmdBuffer, const VkRenderPassBeginInfo* pRenderPassBegin) {
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, cmdBuffer);
    auto render_pass_data = dev_data->renderPassMap.find(pRenderPassBegin->renderPass);
    if (render_pass_data == dev_data->renderPassMap.end()) {
        return;
    }
    const VkRenderPassCreateInfo* pRenderPassInfo = render_pass_data->second->pCreateInfo;
    auto framebuffer_data = dev_data->frameBufferMap.find(pRenderPassBegin->framebuffer);
    if (framebuffer_data == dev_data->frameBufferMap.end()) {
        return;
    }
    const VkFramebufferCreateInfo* pFramebufferInfo = framebuffer_data->second;
    for (uint32_t i = 0; i < pRenderPassInfo->attachmentCount; ++i) {
        const VkImageView& image_view = pFramebufferInfo->pAttachments[i];
        auto image_view_data = dev_data->imageViewMap.find(image_view);
        if (image_view_data !=  dev_data->imageViewMap.end()) {
            auto image_layout = pCB->imageLayoutMap.find(image_view_data->second->image);
            if (image_layout != pCB->imageLayoutMap.end()) {
                image_layout->second.layout = pRenderPassInfo->pAttachments[i].finalLayout;
            }
        }
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, VkSubpassContents contents)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        if (pRenderPassBegin && pRenderPassBegin->renderPass) {
            skipCall |= VerifyFramebufferAndRenderPassLayouts(commandBuffer, pRenderPassBegin);
            skipCall |= insideRenderPass(dev_data, pCB, "vkCmdBeginRenderPass");
            skipCall |= validatePrimaryCommandBuffer(dev_data, pCB, "vkCmdBeginRenderPass");
            skipCall |= addCmd(dev_data, pCB, CMD_BEGINRENDERPASS);
            pCB->activeRenderPass = pRenderPassBegin->renderPass;
            // This is a shallow copy as that is all that is needed for now
            pCB->activeRenderPassBeginInfo = *pRenderPassBegin;
            pCB->activeSubpass = 0;
            pCB->framebuffer = pRenderPassBegin->framebuffer;
            if (pCB->lastBoundPipeline) {
                skipCall |= validatePipelineState(dev_data, pCB, VK_PIPELINE_BIND_POINT_GRAPHICS, pCB->lastBoundPipeline);
            }
        } else {
            skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_RENDERPASS, "DS",
                    "You cannot use a NULL RenderPass object in vkCmdBeginRenderPass()");
        }
    }
    if (VK_FALSE == skipCall) {
        dev_data->device_dispatch_table->CmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
        // This is a shallow copy as that is all that is needed for now
        dev_data->renderPassBeginInfo = *pRenderPassBegin;
        dev_data->currentSubpass = 0;
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    TransitionSubpassLayouts(commandBuffer, &dev_data->renderPassBeginInfo, ++dev_data->currentSubpass);
    if (pCB) {
        skipCall |= validatePrimaryCommandBuffer(dev_data, pCB, "vkCmdNextSubpass");
        skipCall |= addCmd(dev_data, pCB, CMD_NEXTSUBPASS);
        pCB->activeSubpass++;
        TransitionSubpassLayouts(commandBuffer, &pCB->activeRenderPassBeginInfo, ++pCB->activeSubpass);
        if (pCB->lastBoundPipeline) {
            skipCall |= validatePipelineState(dev_data, pCB, VK_PIPELINE_BIND_POINT_GRAPHICS, pCB->lastBoundPipeline);
        }
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdNextSubpass");
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdNextSubpass(commandBuffer, contents);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderPass(VkCommandBuffer commandBuffer)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    TransitionFinalSubpassLayouts(commandBuffer, &dev_data->renderPassBeginInfo);
    if (pCB) {
        skipCall |= outsideRenderPass(dev_data, pCB, "vkCmdEndRenderpass");
        skipCall |= validatePrimaryCommandBuffer(dev_data, pCB, "vkCmdEndRenderPass");
        skipCall |= addCmd(dev_data, pCB, CMD_ENDRENDERPASS);
        TransitionFinalSubpassLayouts(commandBuffer, &pCB->activeRenderPassBeginInfo);
        pCB->activeRenderPass = 0;
        pCB->activeSubpass = 0;
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdEndRenderPass(commandBuffer);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBuffersCount, const VkCommandBuffer* pCommandBuffers)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (pCB) {
        GLOBAL_CB_NODE* pSubCB = NULL;
        for (uint32_t i=0; i<commandBuffersCount; i++) {
            pSubCB = getCBNode(dev_data, pCommandBuffers[i]);
            if (!pSubCB) {
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
                    "vkCmdExecuteCommands() called w/ invalid Cmd Buffer %p in element %u of pCommandBuffers array.", (void*)pCommandBuffers[i], i);
            } else if (VK_COMMAND_BUFFER_LEVEL_PRIMARY == pSubCB->createInfo.level) {
                skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
                    "vkCmdExecuteCommands() called w/ Primary Cmd Buffer %p in element %u of pCommandBuffers array. All cmd buffers in pCommandBuffers array must be secondary.", (void*)pCommandBuffers[i], i);
            }
        }
        skipCall |= validatePrimaryCommandBuffer(dev_data, pCB, "vkCmdExecuteComands");
        skipCall |= addCmd(dev_data, pCB, CMD_EXECUTECOMMANDS);
    }
    if (VK_FALSE == skipCall)
        dev_data->device_dispatch_table->CmdExecuteCommands(commandBuffer, commandBuffersCount, pCommandBuffers);
}

bool ValidateMapImageLayouts(VkDevice device, VkDeviceMemory mem) {
    bool skip_call = false;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    auto mem_data = dev_data->memImageMap.find(mem);
    if (mem_data != dev_data->memImageMap.end()) {
        auto image_data = dev_data->imageLayoutMap.find(mem_data->second);
        if (image_data != dev_data->imageLayoutMap.end()) {
            if (image_data->second->layout != VK_IMAGE_LAYOUT_PREINITIALIZED && image_data->second->layout != VK_IMAGE_LAYOUT_GENERAL) {
                skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, 0, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                    "Cannot map an image with layout %d. Only GENERAL or PREINITIALIZED are supported.", image_data->second->layout);
            }
        }
    }
    return skip_call;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory(
    VkDevice         device,
    VkDeviceMemory   mem,
    VkDeviceSize     offset,
    VkDeviceSize     size,
    VkFlags          flags,
    void           **ppData)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    bool skip_call = VK_FALSE;
#ifndef DISABLE_IMAGE_LAYOUT_VALIDATION
    skip_call = ValidateMapImageLayouts(device, mem);
#endif // DISABLE_IMAGE_LAYOUT_VALIDATION

    if (VK_FALSE == skip_call) {
        return dev_data->device_dispatch_table->MapMemory(device, mem, offset, size, flags, ppData);
    }
    return VK_ERROR_VALIDATION_FAILED;
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory(
    VkDevice                                    device,
    VkImage                                     image,
    VkDeviceMemory                              mem,
    VkDeviceSize                                memOffset)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->BindImageMemory(device, image, mem, memOffset);
    loader_platform_thread_lock_mutex(&globalLock);
    dev_data->memImageMap[mem] = image;
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateSwapchainKHR(
    VkDevice                        device,
    const VkSwapchainCreateInfoKHR *pCreateInfo,
    const VkAllocationCallbacks    *pAllocator,
    VkSwapchainKHR                 *pSwapchain)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);

    if (VK_SUCCESS == result) {
        SWAPCHAIN_NODE *swapchain_data = new SWAPCHAIN_NODE;
        loader_platform_thread_lock_mutex(&globalLock);
        dev_data->device_extensions.swapchainMap[*pSwapchain] = swapchain_data;
        loader_platform_thread_unlock_mutex(&globalLock);
    }

    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySwapchainKHR(
    VkDevice                        device,
    VkSwapchainKHR                  swapchain,
    const VkAllocationCallbacks     *pAllocator)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    loader_platform_thread_lock_mutex(&globalLock);
    auto swapchain_data = dev_data->device_extensions.swapchainMap.find(swapchain);
    if (swapchain_data != dev_data->device_extensions.swapchainMap.end()) {
        if (swapchain_data->second->images.size() > 0) {
            for (auto swapchain_image : swapchain_data->second->images) {
                auto image_item = dev_data->imageLayoutMap.find(swapchain_image);
                if (image_item != dev_data->imageLayoutMap.end())
                    dev_data->imageLayoutMap.erase(image_item);
            }
        }
        delete swapchain_data->second;
        dev_data->device_extensions.swapchainMap.erase(swapchain);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return dev_data->device_dispatch_table->DestroySwapchainKHR(device, swapchain, pAllocator);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainImagesKHR(
    VkDevice                device,
    VkSwapchainKHR          swapchain,
    uint32_t*               pCount,
    VkImage*                pSwapchainImages)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->device_dispatch_table->GetSwapchainImagesKHR(device, swapchain, pCount, pSwapchainImages);

    if (result == VK_SUCCESS && pSwapchainImages != NULL) {
        // This should never happen and is checked by param checker.
        if (!pCount) return result;
        for (uint32_t i = 0; i < *pCount; ++i) {
            IMAGE_NODE* image_node = new IMAGE_NODE;
            image_node->layout = VK_IMAGE_LAYOUT_UNDEFINED;
            loader_platform_thread_lock_mutex(&globalLock);
            dev_data->device_extensions.swapchainMap[swapchain]->images.push_back(pSwapchainImages[i]);
            dev_data->imageLayoutMap[pSwapchainImages[i]] = image_node;
            loader_platform_thread_unlock_mutex(&globalLock);
        }
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo)
{
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);
    bool skip_call = false;

#ifndef DISABLE_IMAGE_LAYOUT_VALIDATION
    if (pPresentInfo) {
        for (uint32_t i = 0; i < pPresentInfo->swapchainCount; ++i) {
            auto swapchain_data = dev_data->device_extensions.swapchainMap.find(pPresentInfo->pSwapchains[i]);
            if (swapchain_data != dev_data->device_extensions.swapchainMap.end() && pPresentInfo->pImageIndices[i] < swapchain_data->second->images.size()) {
                VkImage image = swapchain_data->second->images[pPresentInfo->pImageIndices[i]];
                auto image_data = dev_data->imageLayoutMap.find(image);
                if (image_data != dev_data->imageLayoutMap.end()) {
                    if (image_data->second->layout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
                        skip_call |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_QUEUE, (uint64_t)queue, 0, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                             "Images passed to present must be in layout PRESENT_SOURCE_KHR but is in %d", image_data->second->layout);
                    }
                }
            }
        }
    }
#endif // DISABLE_IMAGE_LAYOUT_VALIDATION

    if (VK_FALSE == skip_call)
        return dev_data->device_dispatch_table->QueuePresentKHR(queue, pPresentInfo);
    return VK_ERROR_VALIDATION_FAILED;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkDbgCreateMsgCallback(
    VkInstance                          instance,
    VkFlags                             msgFlags,
    const PFN_vkDbgMsgCallback          pfnMsgCallback,
    void*                               pUserData,
    VkDbgMsgCallback*                   pMsgCallback)
{
    layer_data* my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
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
    layer_data* my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkResult res = my_data->instance_dispatch_table->DbgDestroyMsgCallback(instance, msgCallback);
    layer_destroy_msg_callback(my_data->report_data, msgCallback);
    return res;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDbgMarkerBegin(VkCommandBuffer commandBuffer, const char* pMarker)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (!dev_data->device_extensions.debug_marker_enabled) {
        skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)commandBuffer, 0, DRAWSTATE_INVALID_EXTENSION, "DS",
                "Attempt to use CmdDbgMarkerBegin but extension disabled!");
        return;
    } else if (pCB) {
        
        skipCall |= addCmd(dev_data, pCB, CMD_DBGMARKERBEGIN);
    }
    if (VK_FALSE == skipCall)
        debug_marker_dispatch_table(commandBuffer)->CmdDbgMarkerBegin(commandBuffer, pMarker);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDbgMarkerEnd(VkCommandBuffer commandBuffer)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data* dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE* pCB = getCBNode(dev_data, commandBuffer);
    if (!dev_data->device_extensions.debug_marker_enabled) {
        skipCall |= log_msg(dev_data->report_data, VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)commandBuffer, 0, DRAWSTATE_INVALID_EXTENSION, "DS",
                "Attempt to use CmdDbgMarkerEnd but extension disabled!");
        return;
    } else if (pCB) {
        
        skipCall |= addCmd(dev_data, pCB, CMD_DBGMARKEREND);
    }
    if (VK_FALSE == skipCall)
        debug_marker_dispatch_table(commandBuffer)->CmdDbgMarkerEnd(commandBuffer);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev, const char* funcName)
{
    if (dev == NULL)
        return NULL;

    layer_data *dev_data;
    /* loader uses this to force layer initialization; device object is wrapped */
    if (!strcmp(funcName, "vkGetDeviceProcAddr")) {
        VkBaseLayerObject* wrapped_dev = (VkBaseLayerObject*) dev;
        dev_data = get_my_data_ptr(get_dispatch_key(wrapped_dev->baseObject), layer_data_map);
        dev_data->device_dispatch_table = new VkLayerDispatchTable;
        layer_initialize_dispatch_table(dev_data->device_dispatch_table, wrapped_dev);
        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;
    }
    dev_data = get_my_data_ptr(get_dispatch_key(dev), layer_data_map);
    if (!strcmp(funcName, "vkCreateDevice"))
        return (PFN_vkVoidFunction) vkCreateDevice;
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (PFN_vkVoidFunction) vkDestroyDevice;
    if (!strcmp(funcName, "vkQueueSubmit"))
        return (PFN_vkVoidFunction) vkQueueSubmit;
    if (!strcmp(funcName, "vkDestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (PFN_vkVoidFunction) vkDestroyDevice;
    if (!strcmp(funcName, "vkDestroyFence"))
        return (PFN_vkVoidFunction) vkDestroyFence;
    if (!strcmp(funcName, "vkDestroySemaphore"))
        return (PFN_vkVoidFunction) vkDestroySemaphore;
    if (!strcmp(funcName, "vkDestroyEvent"))
        return (PFN_vkVoidFunction) vkDestroyEvent;
    if (!strcmp(funcName, "vkDestroyQueryPool"))
        return (PFN_vkVoidFunction) vkDestroyQueryPool;
    if (!strcmp(funcName, "vkDestroyBuffer"))
        return (PFN_vkVoidFunction) vkDestroyBuffer;
    if (!strcmp(funcName, "vkDestroyBufferView"))
        return (PFN_vkVoidFunction) vkDestroyBufferView;
    if (!strcmp(funcName, "vkDestroyImage"))
        return (PFN_vkVoidFunction) vkDestroyImage;
    if (!strcmp(funcName, "vkDestroyImageView"))
        return (PFN_vkVoidFunction) vkDestroyImageView;
    if (!strcmp(funcName, "vkDestroyShaderModule"))
        return (PFN_vkVoidFunction) vkDestroyShaderModule;
    if (!strcmp(funcName, "vkDestroyPipeline"))
        return (PFN_vkVoidFunction) vkDestroyPipeline;
    if (!strcmp(funcName, "vkDestroyPipelineLayout"))
        return (PFN_vkVoidFunction) vkDestroyPipelineLayout;
    if (!strcmp(funcName, "vkDestroySampler"))
        return (PFN_vkVoidFunction) vkDestroySampler;
    if (!strcmp(funcName, "vkDestroyDescriptorSetLayout"))
        return (PFN_vkVoidFunction) vkDestroyDescriptorSetLayout;
    if (!strcmp(funcName, "vkDestroyDescriptorPool"))
        return (PFN_vkVoidFunction) vkDestroyDescriptorPool;
    if (!strcmp(funcName, "vkDestroyFramebuffer"))
        return (PFN_vkVoidFunction) vkDestroyFramebuffer;
    if (!strcmp(funcName, "vkDestroyRenderPass"))
        return (PFN_vkVoidFunction) vkDestroyRenderPass;
    if (!strcmp(funcName, "vkCreateBuffer"))
        return (PFN_vkVoidFunction) vkCreateBuffer;
    if (!strcmp(funcName, "vkCreateBufferView"))
        return (PFN_vkVoidFunction) vkCreateBufferView;
    if (!strcmp(funcName, "vkCreateImage"))
        return (PFN_vkVoidFunction) vkCreateImage;
    if (!strcmp(funcName, "vkCreateImageView"))
        return (PFN_vkVoidFunction) vkCreateImageView;
    if (!strcmp(funcName, "CreatePipelineCache"))
        return (PFN_vkVoidFunction) vkCreatePipelineCache;
    if (!strcmp(funcName, "DestroyPipelineCache"))
        return (PFN_vkVoidFunction) vkDestroyPipelineCache;
    if (!strcmp(funcName, "GetPipelineCacheData"))
        return (PFN_vkVoidFunction) vkGetPipelineCacheData;
    if (!strcmp(funcName, "MergePipelineCaches"))
        return (PFN_vkVoidFunction) vkMergePipelineCaches;
    if (!strcmp(funcName, "vkCreateGraphicsPipelines"))
        return (PFN_vkVoidFunction) vkCreateGraphicsPipelines;
    if (!strcmp(funcName, "vkCreateComputePipelines"))
        return (PFN_vkVoidFunction) vkCreateComputePipelines;
    if (!strcmp(funcName, "vkCreateSampler"))
        return (PFN_vkVoidFunction) vkCreateSampler;
    if (!strcmp(funcName, "vkCreateDescriptorSetLayout"))
        return (PFN_vkVoidFunction) vkCreateDescriptorSetLayout;
    if (!strcmp(funcName, "vkCreatePipelineLayout"))
        return (PFN_vkVoidFunction) vkCreatePipelineLayout;
    if (!strcmp(funcName, "vkCreateDescriptorPool"))
        return (PFN_vkVoidFunction) vkCreateDescriptorPool;
    if (!strcmp(funcName, "vkResetDescriptorPool"))
        return (PFN_vkVoidFunction) vkResetDescriptorPool;
    if (!strcmp(funcName, "vkAllocateDescriptorSets"))
        return (PFN_vkVoidFunction) vkAllocateDescriptorSets;
    if (!strcmp(funcName, "vkFreeDescriptorSets"))
        return (PFN_vkVoidFunction) vkFreeDescriptorSets;
    if (!strcmp(funcName, "vkUpdateDescriptorSets"))
        return (PFN_vkVoidFunction) vkUpdateDescriptorSets;
    if (!strcmp(funcName, "vkCreateCommandPool"))
        return (PFN_vkVoidFunction) vkCreateCommandPool;
    if (!strcmp(funcName, "vkDestroyCommandPool"))
        return (PFN_vkVoidFunction) vkDestroyCommandPool;
    if (!strcmp(funcName, "vkAllocateCommandBuffers"))
        return (PFN_vkVoidFunction) vkAllocateCommandBuffers;
    if (!strcmp(funcName, "vkFreeCommandBuffers"))
        return (PFN_vkVoidFunction) vkFreeCommandBuffers;
    if (!strcmp(funcName, "vkBeginCommandBuffer"))
        return (PFN_vkVoidFunction) vkBeginCommandBuffer;
    if (!strcmp(funcName, "vkEndCommandBuffer"))
        return (PFN_vkVoidFunction) vkEndCommandBuffer;
    if (!strcmp(funcName, "vkResetCommandBuffer"))
        return (PFN_vkVoidFunction) vkResetCommandBuffer;
    if (!strcmp(funcName, "vkCmdBindPipeline"))
        return (PFN_vkVoidFunction) vkCmdBindPipeline;
    if (!strcmp(funcName, "vkCmdSetViewport"))
        return (PFN_vkVoidFunction) vkCmdSetViewport;
    if (!strcmp(funcName, "vkCmdSetScissor"))
        return (PFN_vkVoidFunction) vkCmdSetScissor;
    if (!strcmp(funcName, "vkCmdSetLineWidth"))
        return (PFN_vkVoidFunction) vkCmdSetLineWidth;
    if (!strcmp(funcName, "vkCmdSetDepthBias"))
        return (PFN_vkVoidFunction) vkCmdSetDepthBias;
    if (!strcmp(funcName, "vkCmdSetBlendConstants"))
        return (PFN_vkVoidFunction) vkCmdSetBlendConstants;
    if (!strcmp(funcName, "vkCmdSetDepthBounds"))
        return (PFN_vkVoidFunction) vkCmdSetDepthBounds;
    if (!strcmp(funcName, "vkCmdSetStencilCompareMask"))
        return (PFN_vkVoidFunction) vkCmdSetStencilCompareMask;
    if (!strcmp(funcName, "vkCmdSetStencilWriteMask"))
        return (PFN_vkVoidFunction) vkCmdSetStencilWriteMask;
    if (!strcmp(funcName, "vkCmdSetStencilReference"))
        return (PFN_vkVoidFunction) vkCmdSetStencilReference;
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
    if (!strcmp(funcName, "vkCmdClearDepthStencilImage"))
        return (PFN_vkVoidFunction) vkCmdClearDepthStencilImage;
    if (!strcmp(funcName, "vkCmdClearAttachments"))
        return (PFN_vkVoidFunction) vkCmdClearAttachments;
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
    if (!strcmp(funcName, "vkCreateFramebuffer"))
        return (PFN_vkVoidFunction) vkCreateFramebuffer;
    if (!strcmp(funcName, "vkCreateShaderModule"))
        return (PFN_vkVoidFunction) vkCreateShaderModule;
    if (!strcmp(funcName, "vkCreateRenderPass"))
        return (PFN_vkVoidFunction) vkCreateRenderPass;
    if (!strcmp(funcName, "vkCmdBeginRenderPass"))
        return (PFN_vkVoidFunction) vkCmdBeginRenderPass;
    if (!strcmp(funcName, "vkCmdNextSubpass"))
        return (PFN_vkVoidFunction) vkCmdNextSubpass;
    if (!strcmp(funcName, "vkCmdEndRenderPass"))
        return (PFN_vkVoidFunction) vkCmdEndRenderPass;
    if (!strcmp(funcName, "vkCmdExecuteCommands"))
        return (PFN_vkVoidFunction) vkCmdExecuteCommands;
    if (!strcmp(funcName, "vkMapMemory"))
        return (PFN_vkVoidFunction) vkMapMemory;

    if (dev_data->device_extensions.wsi_enabled)
    {
        if (!strcmp(funcName, "vkCreateSwapchainKHR"))
            return (PFN_vkVoidFunction) vkCreateSwapchainKHR;
        if (!strcmp(funcName, "vkDestroySwapchainKHR"))
            return (PFN_vkVoidFunction) vkDestroySwapchainKHR;
        if (!strcmp(funcName, "vkGetSwapchainImagesKHR"))
            return (PFN_vkVoidFunction) vkGetSwapchainImagesKHR;
        if (!strcmp(funcName, "vkQueuePresentKHR"))
            return (PFN_vkVoidFunction) vkQueuePresentKHR;
    }

    VkLayerDispatchTable* pTable = dev_data->device_dispatch_table;
    if (dev_data->device_extensions.debug_marker_enabled)
    {
        if (!strcmp(funcName, "vkCmdDbgMarkerBegin"))
            return (PFN_vkVoidFunction) vkCmdDbgMarkerBegin;
        if (!strcmp(funcName, "vkCmdDbgMarkerEnd"))
            return (PFN_vkVoidFunction) vkCmdDbgMarkerEnd;
    }
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

    layer_data* my_data;
    /* loader uses this to force layer initialization; instance object is wrapped */
    if (!strcmp(funcName, "vkGetInstanceProcAddr")) {
        VkBaseLayerObject* wrapped_inst = (VkBaseLayerObject*) instance;
        my_data = get_my_data_ptr(get_dispatch_key(wrapped_inst->baseObject), layer_data_map);
        my_data->instance_dispatch_table = new VkLayerInstanceDispatchTable;
        layer_init_instance_dispatch_table(my_data->instance_dispatch_table, wrapped_inst);
        return (PFN_vkVoidFunction) vkGetInstanceProcAddr;
    }
    if (!strcmp(funcName, "vkCreateInstance"))
        return (PFN_vkVoidFunction) vkCreateInstance;
    if (!strcmp(funcName, "vkDestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(funcName, "vkEnumerateInstanceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateInstanceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceExtensionProperties;
    if (!strcmp(funcName, "vkEnumerateDeviceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateDeviceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceExtensionProperties;

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
