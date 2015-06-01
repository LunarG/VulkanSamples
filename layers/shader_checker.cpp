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
 */
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <map>
#include <unordered_map>
#include <map>
#include <vector>
#include "loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vkLayer.h"
#include "layers_config.h"
#include "layers_msg.h"
#include "vk_enum_string_helper.h"
#include "shader_checker.h"
// The following is #included again to catch certain OS-specific functions
// being used:
#include "loader_platform.h"

#include "spirv/spirv.h"


static std::unordered_map<void *, VkLayerDispatchTable *> tableMap;
static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(g_initOnce);


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
        case spv::OpTypeSampler:
        case spv::OpTypeFilter:
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

struct shader_source {
    /* the spirv image itself */
    std::vector<uint32_t> words;
    /* a mapping of <id> to the first word of its def. this is useful because walking type
     * trees requires jumping all over the instruction stream.
     */
    std::unordered_map<unsigned, unsigned> type_def_index;

    shader_source(VkShaderCreateInfo const *pCreateInfo) :
        words((uint32_t *)pCreateInfo->pCode, (uint32_t *)pCreateInfo->pCode + pCreateInfo->codeSize / sizeof(uint32_t)) {

        build_type_def_index(words, type_def_index);
    }
};


static std::unordered_map<void *, shader_source *> shader_map;


static void
initLayer()
{
    const char *strOpt;
    // initialize ShaderChecker options
    getLayerOptionEnum("ShaderCheckerReportLevel", (uint32_t *) &g_reportingLevel);
    g_actionIsDefault = getLayerOptionEnum("ShaderCheckerDebugAction", (uint32_t *) &g_debugAction);

    if (g_debugAction & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        strOpt = getLayerOption("ShaderCheckerLogFilename");
        if (strOpt)
        {
            g_logFile = fopen(strOpt, "w");
        }
        if (g_logFile == NULL)
            g_logFile = stdout;
    }
}


static VkLayerDispatchTable * initLayerTable(const VkBaseLayerObject *gpuw)
{
    VkLayerDispatchTable *pTable;

    assert(gpuw);
    std::unordered_map<void *, VkLayerDispatchTable *>::const_iterator it = tableMap.find((void *) gpuw->baseObject);
    if (it == tableMap.end())
    {
        pTable =  new VkLayerDispatchTable;
        tableMap[(void *) gpuw->baseObject] = pTable;
    } else
    {
        return it->second;
    }

    layer_initialize_dispatch_table(pTable, gpuw->pGPA, (VkPhysicalDevice) gpuw->nextObject);

    return pTable;
}


VK_LAYER_EXPORT VkResult VKAPI vkCreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo, VkDevice* pDevice)
{
    VkLayerDispatchTable* pTable = tableMap[gpu];
    VkResult result = pTable->CreateDevice(gpu, pCreateInfo, pDevice);

    loader_platform_thread_once(&g_initOnce, initLayer);
    // create a mapping for the device object into the dispatch table
    tableMap.emplace(*pDevice, pTable);
    return result;
}


VK_LAYER_EXPORT VkResult VKAPI vkEnumerateLayers(VkPhysicalDevice physicalDevice, size_t maxStringSize, size_t* pLayerCount, char* const* pOutLayers, void* pReserved)
{
    if (pLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL || pOutLayers[1] == NULL || pReserved == NULL)
        return VK_ERROR_INVALID_POINTER;

    if (*pLayerCount < 1)
        return VK_ERROR_INITIALIZATION_FAILED;
    *pLayerCount = 1;
    strncpy((char *) pOutLayers[0], "ShaderChecker", maxStringSize);
    return VK_SUCCESS;
}


struct extProps {
    uint32_t version;
    const char * const name;
};
#define SHADER_CHECKER_LAYER_EXT_ARRAY_SIZE 2
static const struct extProps shaderCheckerExts[SHADER_CHECKER_LAYER_EXT_ARRAY_SIZE] = {
    // TODO what is the version?
    0x10, "ShaderChecker",
    0x10, "Validation",
};

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionInfo(
                                               VkExtensionInfoType infoType,
                                               uint32_t extensionIndex,
                                               size_t*  pDataSize,
                                               void*    pData)
{
    /* This entrypoint is NOT going to init it's own dispatch table since loader calls here early */
    VkExtensionProperties *ext_props;
    uint32_t *count;

    if (pDataSize == NULL)
        return VK_ERROR_INVALID_POINTER;

    switch (infoType) {
        case VK_EXTENSION_INFO_TYPE_COUNT:
            *pDataSize = sizeof(uint32_t);
            if (pData == NULL)
                return VK_SUCCESS;
            count = (uint32_t *) pData;
            *count = SHADER_CHECKER_LAYER_EXT_ARRAY_SIZE;
            break;
        case VK_EXTENSION_INFO_TYPE_PROPERTIES:
            *pDataSize = sizeof(VkExtensionProperties);
            if (pData == NULL)
                return VK_SUCCESS;
            if (extensionIndex >= SHADER_CHECKER_LAYER_EXT_ARRAY_SIZE)
                return VK_ERROR_INVALID_VALUE;
            ext_props = (VkExtensionProperties *) pData;
            ext_props->version = shaderCheckerExts[extensionIndex].version;
            strncpy(ext_props->extName, shaderCheckerExts[extensionIndex].name,
                                        VK_MAX_EXTENSION_NAME);
            ext_props->extName[VK_MAX_EXTENSION_NAME - 1] = '\0';
            break;
        default:
            return VK_ERROR_INVALID_VALUE;
    };

    return VK_SUCCESS;
}


static char const *
storage_class_name(unsigned sc)
{
    switch (sc) {
    case spv::StorageClassInput: return "input";
    case spv::StorageClassOutput: return "output";
    case spv::StorageClassUniformConstant: return "const uniform";
    case spv::StorageClassUniform: return "uniform";
    case spv::StorageClassWorkgroupLocal: return "workgroup local";
    case spv::StorageClassWorkgroupGlobal: return "workgroup global";
    case spv::StorageClassPrivateGlobal: return "private global";
    case spv::StorageClassFunction: return "function";
    case spv::StorageClassGeneric: return "generic";
    case spv::StorageClassPrivate: return "private";
    case spv::StorageClassAtomicCounter: return "atomic counter";
    default: return "unknown";
    }
}


/* returns ptr to null terminator */
static char *
describe_type(char *dst, shader_source const *src, unsigned type)
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
        default:
            return dst + sprintf(dst, "oddtype");
    }
}


static bool
types_match(shader_source const *a, shader_source const *b, unsigned a_type, unsigned b_type)
{
    auto a_type_def_it = a->type_def_index.find(a_type);
    auto b_type_def_it = b->type_def_index.find(b_type);

    if (a_type_def_it == a->type_def_index.end()) {
        printf("ERR: can't find def for type %d in producing shader %p; SPIRV probably invalid.\n",
                a_type, a);
        return false;
    }

    if (b_type_def_it == b->type_def_index.end()) {
        printf("ERR: can't find def for type %d in consuming shader %p; SPIRV probably invalid.\n",
                b_type, b);
        return false;
    }

    /* walk two type trees together, and complain about differences */
    unsigned int const *a_code = (unsigned int const *)&a->words[a_type_def_it->second];
    unsigned int const *b_code = (unsigned int const *)&b->words[b_type_def_it->second];

    unsigned a_opcode = a_code[0] & 0x0ffffu;
    unsigned b_opcode = b_code[0] & 0x0ffffu;

    if (a_opcode != b_opcode) {
        printf("  - FAIL: type def opcodes differ: %d vs %d\n", a_opcode, b_opcode);
        return false;
    }

    switch (a_opcode) {
        case spv::OpTypeBool:
            return true;
        case spv::OpTypeInt:
            /* match on width, signedness */
            return a_code[2] == b_code[2] && a_code[3] == b_code[3];
        case spv::OpTypeFloat:
            /* match on width */
            return a_code[2] == b_code[2];
        case spv::OpTypeVector:
        case spv::OpTypeMatrix:
        case spv::OpTypeArray:
            /* match on element type, count. these all have the same layout */
            return types_match(a, b, a_code[2], b_code[2]) && a_code[3] == b_code[3];
        case spv::OpTypeStruct:
            /* match on all element types */
            {
                unsigned a_len = a_code[0] >> 16;
                unsigned b_len = b_code[0] >> 16;

                if (a_len != b_len) {
                    return false;   /* structs cannot match if member counts differ */
                }

                for (unsigned i = 2; i < a_len; i++) {
                    if (!types_match(a, b, a_code[i], b_code[i])) {
                        return false;
                    }
                }

                return true;
            }
        case spv::OpTypePointer:
            /* match on pointee type. storage class is expected to differ */
            return types_match(a, b, a_code[3], b_code[3]);

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
collect_interface_by_location(shader_source const *src, spv::StorageClass sinterface,
                              std::map<uint32_t, interface_var> &out,
                              std::map<uint32_t, interface_var> &builtins_out)
{
    unsigned int const *code = (unsigned int const *)&src->words[0];
    size_t size = src->words.size();

    if (code[0] != spv::MagicNumber) {
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, SHADER_CHECKER_NON_SPIRV_SHADER, "SC",
                   "Shader is not SPIR-V, unable to extract interface");
        return;
    }

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
                char str[1024];
                sprintf(str, "var %d (type %d) in %s interface has no Location or Builtin decoration\n",
                       code[word+2], code[word+1], storage_class_name(sinterface));
                layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, SHADER_CHECKER_INCONSISTENT_SPIRV, "SC", str);
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


VK_LAYER_EXPORT VkResult VKAPI vkCreateShader(VkDevice device, const VkShaderCreateInfo *pCreateInfo,
                                                   VkShader *pShader)
{
    VkLayerDispatchTable* pTable = tableMap[(VkBaseLayerObject *)device];
    VkResult res = pTable->CreateShader(device, pCreateInfo, pShader);

    shader_map[(VkBaseLayerObject *) *pShader] = new shader_source(pCreateInfo);
    return res;
}


static void
validate_interface_between_stages(shader_source const *producer, char const *producer_name,
                                  shader_source const *consumer, char const *consumer_name)
{
    std::map<uint32_t, interface_var> outputs;
    std::map<uint32_t, interface_var> inputs;

    std::map<uint32_t, interface_var> builtin_outputs;
    std::map<uint32_t, interface_var> builtin_inputs;

    char str[1024];

    collect_interface_by_location(producer, spv::StorageClassOutput, outputs, builtin_outputs);
    collect_interface_by_location(consumer, spv::StorageClassInput, inputs, builtin_inputs);

    auto a_it = outputs.begin();
    auto b_it = inputs.begin();

    /* maps sorted by key (location); walk them together to find mismatches */
    while ((outputs.size() > 0 && a_it != outputs.end()) || ( inputs.size() && b_it != inputs.end())) {
        bool a_at_end = outputs.size() == 0 || a_it == outputs.end();
        bool b_at_end = inputs.size() == 0  || b_it == inputs.end();
        auto a_first = (outputs.size() > 0 ? a_it->first : 0);
        auto b_first = (inputs.size()  > 0 ? b_it->first : 0);

        if (b_at_end || a_first < b_first) {
            sprintf(str, "%s writes to output location %d which is not consumed by %s\n",
                   producer_name, a_first, consumer_name);
            layerCbMsg(VK_DBG_MSG_WARNING, VK_VALIDATION_LEVEL_0, NULL, 0, SHADER_CHECKER_OUTPUT_NOT_CONSUMED, "SC", str);
            a_it++;
        }
        else if (a_at_end || a_first > b_first) {
            sprintf(str, "%s consumes input location %d which is not written by %s\n",
                   consumer_name, b_first, producer_name);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, SHADER_CHECKER_OUTPUT_NOT_CONSUMED, "SC", str);
            b_it++;
        }
        else {
            if (types_match(producer, consumer, a_it->second.type_id, b_it->second.type_id)) {
                /* OK! */
            }
            else {
                char producer_type[1024];
                char consumer_type[1024];
                describe_type(producer_type, producer, a_it->second.type_id);
                describe_type(consumer_type, consumer, b_it->second.type_id);

                sprintf(str, "Type mismatch on location %d: '%s' vs '%s'\n", a_it->first,
                       producer_type, consumer_type);
                layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC", str);
            }
            a_it++;
            b_it++;
        }
    }
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
    case VK_FORMAT_R10G10B10A2_SINT:
    case VK_FORMAT_B10G10R10A2_SINT:
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
    case VK_FORMAT_R10G10B10A2_UINT:
    case VK_FORMAT_B10G10R10A2_UINT:
        return FORMAT_TYPE_UINT;
    default:
        return FORMAT_TYPE_FLOAT;
    }
}


/* characterizes a SPIR-V type appearing in an interface to a FF stage,
 * for comparison to a VkFormat's characterization above. */
static unsigned
get_fundamental_type(shader_source const *src, unsigned type)
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


static void
validate_vi_against_vs_inputs(VkPipelineVertexInputCreateInfo const *vi, shader_source const *vs)
{
    std::map<uint32_t, interface_var> inputs;
    /* we collect builtin inputs, but they will never appear in the VI state --
     * the vs builtin inputs are generated in the pipeline, not sourced from buffers (VertexID, etc)
     */
    std::map<uint32_t, interface_var> builtin_inputs;
    char str[1024];

    collect_interface_by_location(vs, spv::StorageClassInput, inputs, builtin_inputs);

    /* Build index by location */
    std::map<uint32_t, VkVertexInputAttributeDescription const *> attribs;
    for (unsigned i = 0; i < vi->attributeCount; i++)
        attribs[vi->pVertexAttributeDescriptions[i].location] = &vi->pVertexAttributeDescriptions[i];

    auto it_a = attribs.begin();
    auto it_b = inputs.begin();

    while ((attribs.size() > 0 && it_a != attribs.end()) || (inputs.size() > 0 && it_b != inputs.end())) {
        bool a_at_end = attribs.size() == 0 || it_a == attribs.end();
        bool b_at_end = inputs.size() == 0  || it_b == inputs.end();
        auto a_first = (attribs.size() > 0 ? it_a->first : 0);
        auto b_first = (inputs.size()  > 0 ? it_b->first : 0);
        if (b_at_end || a_first < b_first) {
            sprintf(str, "Vertex attribute at location %d not consumed by VS", a_first);
            layerCbMsg(VK_DBG_MSG_WARNING, VK_VALIDATION_LEVEL_0, NULL, 0, SHADER_CHECKER_OUTPUT_NOT_CONSUMED, "SC", str);
            it_a++;
        }
        else if (a_at_end || b_first < a_first) {
            sprintf(str, "VS consumes input at location %d but not provided", b_first);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, SHADER_CHECKER_INPUT_NOT_PRODUCED, "SC", str);
            it_b++;
        }
        else {
            unsigned attrib_type = get_format_type(it_a->second->format);
            unsigned input_type = get_fundamental_type(vs, it_b->second.type_id);

            /* type checking */
            if (attrib_type != FORMAT_TYPE_UNDEFINED && input_type != FORMAT_TYPE_UNDEFINED && attrib_type != input_type) {
                char vs_type[1024];
                describe_type(vs_type, vs, it_b->second.type_id);
                sprintf(str, "Attribute type of `%s` at location %d does not match VS input type of `%s`",
                        string_VkFormat(it_a->second->format), a_first, vs_type);
                layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC", str);
            }

            /* OK! */
            it_a++;
            it_b++;
        }
    }
}


static void
validate_fs_outputs_against_cb(shader_source const *fs, VkPipelineCbStateCreateInfo const *cb)
{
    std::map<uint32_t, interface_var> outputs;
    std::map<uint32_t, interface_var> builtin_outputs;
    char str[1024];

    /* TODO: dual source blend index (spv::DecIndex, zero if not provided) */

    collect_interface_by_location(fs, spv::StorageClassOutput, outputs, builtin_outputs);

    /* Check for legacy gl_FragColor broadcast: In this case, we should have no user-defined outputs,
     * and all color attachment should be UNORM/SNORM/FLOAT.
     */
    if (builtin_outputs.find(spv::BuiltInFragColor) != builtin_outputs.end()) {
        bool broadcast_err = false;
        if (outputs.size()) {
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, SHADER_CHECKER_FS_MIXED_BROADCAST, "SC",
                       "Should not have user-defined FS outputs when using broadcast");
            broadcast_err = true;
        }

        for (unsigned i = 0; i < cb->attachmentCount; i++) {
            unsigned attachmentType = get_format_type(cb->pAttachments[i].format);
            if (attachmentType == FORMAT_TYPE_SINT || attachmentType == FORMAT_TYPE_UINT) {
                layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC",
                           "CB format should not be SINT or UINT when using broadcast");
                broadcast_err = true;
            }
        }

        return;
    }

    auto it = outputs.begin();
    uint32_t attachment = 0;

    /* Walk attachment list and outputs together -- this is a little overpowered since attachments
     * are currently dense, but the parallel with matching between shader stages is nice.
     */

    while ((outputs.size() > 0 && it != outputs.end()) || attachment < cb->attachmentCount) {
        if (attachment == cb->attachmentCount || ( it != outputs.end() && it->first < attachment)) {
            sprintf(str, "FS writes to output location %d with no matching attachment", it->first);
            layerCbMsg(VK_DBG_MSG_WARNING, VK_VALIDATION_LEVEL_0, NULL, 0, SHADER_CHECKER_OUTPUT_NOT_CONSUMED, "SC", str);
            it++;
        }
        else if (it == outputs.end() || it->first > attachment) {
            sprintf(str, "Attachment %d not written by FS", attachment);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, SHADER_CHECKER_INPUT_NOT_PRODUCED, "SC", str);
            attachment++;
        }
        else {
            unsigned output_type = get_fundamental_type(fs, it->second.type_id);
            unsigned att_type = get_format_type(cb->pAttachments[attachment].format);

            /* type checking */
            if (att_type != FORMAT_TYPE_UNDEFINED && output_type != FORMAT_TYPE_UNDEFINED && att_type != output_type) {
                char fs_type[1024];
                describe_type(fs_type, fs, it->second.type_id);
                sprintf(str, "Attachment %d of type `%s` does not match FS output type of `%s`",
                        attachment, string_VkFormat(cb->pAttachments[attachment].format), fs_type);
                layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC", str);
            }

            /* OK! */
            it++;
            attachment++;
        }
    }
}


VK_LAYER_EXPORT VkResult VKAPI vkCreateGraphicsPipeline(VkDevice device,
                                                             const VkGraphicsPipelineCreateInfo *pCreateInfo,
                                                             VkPipeline *pPipeline)
{
    /* TODO: run cross-stage validation for GS, TCS, TES stages */

    /* We seem to allow pipeline stages to be specified out of order, so collect and identify them
     * before trying to do anything more: */

    shader_source const *vs_source = 0;
    shader_source const *fs_source = 0;
    VkPipelineCbStateCreateInfo const *cb = 0;
    VkPipelineVertexInputCreateInfo const *vi = 0;
    char str[1024];

    for (auto stage = pCreateInfo; stage; stage = (decltype(stage))stage->pNext) {
        if (stage->sType == VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO) {
            auto shader_stage = (VkPipelineShaderStageCreateInfo const *)stage;

            if (shader_stage->shader.stage == VK_SHADER_STAGE_VERTEX) {
                vs_source = shader_map[(void *)(shader_stage->shader.shader)];
            }
            else if (shader_stage->shader.stage == VK_SHADER_STAGE_FRAGMENT) {
                fs_source = shader_map[(void *)(shader_stage->shader.shader)];
            }
            else {
                sprintf(str, "Unknown shader stage %d\n", shader_stage->shader.stage);
                layerCbMsg(VK_DBG_MSG_WARNING, VK_VALIDATION_LEVEL_0, NULL, 0, SHADER_CHECKER_UNKNOWN_STAGE, "SC", str);
            }
        }
        else if (stage->sType == VK_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO) {
            cb = (VkPipelineCbStateCreateInfo const *)stage;
        }
        else if (stage->sType == VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO) {
            vi = (VkPipelineVertexInputCreateInfo const *)stage;
        }
    }

    sprintf(str, "Pipeline: vi=%p vs=%p fs=%p cb=%p\n", vi, vs_source, fs_source, cb);
    layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, SHADER_CHECKER_NONE, "SC", str);

    if (vi && vs_source) {
        validate_vi_against_vs_inputs(vi, vs_source);
    }

    if (vs_source && fs_source) {
        validate_interface_between_stages(vs_source, "vertex shader",
                                          fs_source, "fragment shader");
    }

    if (fs_source && cb) {
        validate_fs_outputs_against_cb(fs_source, cb);
    }

    VkLayerDispatchTable *pTable = tableMap[(VkBaseLayerObject *)device];
    VkResult res = pTable->CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    return res;
}


VK_LAYER_EXPORT void * VKAPI vkGetProcAddr(VkPhysicalDevice gpu, const char* pName)
{
    if (gpu == NULL)
        return NULL;

    initLayerTable((const VkBaseLayerObject *) gpu);

    loader_platform_thread_once(&g_initOnce, initLayer);

#define ADD_HOOK(fn)    \
    if (!strncmp(#fn, pName, sizeof(#fn))) \
        return (void *) fn

    ADD_HOOK(vkGetProcAddr);
    ADD_HOOK(vkEnumerateLayers);
    ADD_HOOK(vkCreateDevice);
    ADD_HOOK(vkCreateShader);
    ADD_HOOK(vkCreateGraphicsPipeline);

    VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;
    if (gpuw->pGPA == NULL)
        return NULL;
    return gpuw->pGPA((VkPhysicalDevice) gpuw->nextObject, pName);
}
