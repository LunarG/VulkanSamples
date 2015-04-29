#!/usr/bin/env python3
#
# VK
#
# Copyright (C) 2014 LunarG, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#
# Authors:
#   Chia-I Wu <olv@lunarg.com>

import sys
import os

import vulkan
import vk_helper

def generate_get_proc_addr_check(name):
    return "    if (!%s || %s[0] != 'v' || %s[1] != 'k')\n" \
           "        return NULL;" % ((name,) * 3)

class Subcommand(object):
    def __init__(self, argv):
        self.argv = argv
        self.headers = vulkan.headers
        self.protos = vulkan.protos
        self.no_addr = False
        self.layer_name = ""

    def run(self):
        print(self.generate())

    def generate(self):
        copyright = self.generate_copyright()
        header = self.generate_header()
        body = self.generate_body()
        footer = self.generate_footer()

        contents = []
        if copyright:
            contents.append(copyright)
        if header:
            contents.append(header)
        if body:
            contents.append(body)
        if footer:
            contents.append(footer)

        return "\n\n".join(contents)

    def generate_copyright(self):
        return """/* THIS FILE IS GENERATED.  DO NOT EDIT. */

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
 */"""

    def generate_header(self):
        return "\n".join(["#include <" + h + ">" for h in self.headers])

    def generate_body(self):
        pass

    def generate_footer(self):
        pass

    # Return set of printf '%' qualifier and input to that qualifier
    def _get_printf_params(self, vk_type, name, output_param, cpp=False):
        # TODO : Need ENUM and STRUCT checks here
        if vk_helper.is_type(vk_type, 'enum'):#"_TYPE" in vk_type: # TODO : This should be generic ENUM check
            return ("%s", "string_%s(%s)" % (vk_type.replace('const ', '').strip('*'), name))
        if "char*" == vk_type:
            return ("%s", name)
        if "uint64" in vk_type:
            if '*' in vk_type:
                return ("%lu", "*%s" % name)
            return ("%lu", name)
        if "size" in vk_type:
            if '*' in vk_type:
                return ("%zu", "*%s" % name)
            return ("%zu", name)
        if "float" in vk_type:
            if '[' in vk_type: # handle array, current hard-coded to 4 (TODO: Make this dynamic)
                if cpp:
                    return ("[%i, %i, %i, %i]", '"[" << %s[0] << "," << %s[1] << "," << %s[2] << "," << %s[3] << "]"' % (name, name, name, name))
                return ("[%f, %f, %f, %f]", "%s[0], %s[1], %s[2], %s[3]" % (name, name, name, name))
            return ("%f", name)
        if "bool" in vk_type or 'xcb_randr_crtc_t' in vk_type:
            return ("%u", name)
        if True in [t in vk_type.lower() for t in ["int", "flags", "mask", "xcb_window_t"]]:
            if '[' in vk_type: # handle array, current hard-coded to 4 (TODO: Make this dynamic)
                if cpp:
                    return ("[%i, %i, %i, %i]", "%s[0] << %s[1] << %s[2] << %s[3]" % (name, name, name, name))
                return ("[%i, %i, %i, %i]", "%s[0], %s[1], %s[2], %s[3]" % (name, name, name, name))
            if '*' in vk_type:
                if 'pUserData' == name:
                    return ("%i", "((pUserData == 0) ? 0 : *(pUserData))")
                if 'const' in vk_type.lower():
                    return ("%p", "(void*)(%s)" % name)
                return ("%i", "*(%s)" % name)
            return ("%i", name)
        # TODO : This is special-cased as there's only one "format" param currently and it's nice to expand it
        if "VkFormat" == vk_type:
            if cpp:
                return ("%p", "&%s" % name)
            return ("{%s.channelFormat = %%s, %s.numericFormat = %%s}" % (name, name), "string_VK_CHANNEL_FORMAT(%s.channelFormat), string_VK_NUM_FORMAT(%s.numericFormat)" % (name, name))
        if output_param:
            return ("%p", "(void*)*%s" % name)
        if vk_helper.is_type(vk_type, 'struct') and '*' not in vk_type:
            return ("%p", "(void*)(&%s)" % name)
        return ("%p", "(void*)(%s)" % name)

    def _gen_layer_dbg_callback_register(self):
        r_body = []
        r_body.append('VK_LAYER_EXPORT VkResult VKAPI vkDbgRegisterMsgCallback(VkInstance instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, void* pUserData)')
        r_body.append('{')
        r_body.append('    // This layer intercepts callbacks')
        r_body.append('    VK_LAYER_DBG_FUNCTION_NODE *pNewDbgFuncNode = (VK_LAYER_DBG_FUNCTION_NODE*)malloc(sizeof(VK_LAYER_DBG_FUNCTION_NODE));')
        r_body.append('    if (!pNewDbgFuncNode)')
        r_body.append('        return VK_ERROR_OUT_OF_HOST_MEMORY;')
        r_body.append('    pNewDbgFuncNode->pfnMsgCallback = pfnMsgCallback;')
        r_body.append('    pNewDbgFuncNode->pUserData = pUserData;')
        r_body.append('    pNewDbgFuncNode->pNext = g_pDbgFunctionHead;')
        r_body.append('    g_pDbgFunctionHead = pNewDbgFuncNode;')
        r_body.append('    // force callbacks if DebugAction hasn\'t been set already other than initial value')
        r_body.append('    if (g_actionIsDefault) {')
        r_body.append('        g_debugAction = VK_DBG_LAYER_ACTION_CALLBACK;')
        r_body.append('    }')
        r_body.append('    VkResult result = nextTable.DbgRegisterMsgCallback(instance, pfnMsgCallback, pUserData);')
        r_body.append('    return result;')
        r_body.append('}')
        return "\n".join(r_body)

    def _gen_layer_dbg_callback_unregister(self):
        ur_body = []
        ur_body.append('VK_LAYER_EXPORT VkResult VKAPI vkDbgUnregisterMsgCallback(VkInstance instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback)')
        ur_body.append('{')
        ur_body.append('    VK_LAYER_DBG_FUNCTION_NODE *pTrav = g_pDbgFunctionHead;')
        ur_body.append('    VK_LAYER_DBG_FUNCTION_NODE *pPrev = pTrav;')
        ur_body.append('    while (pTrav) {')
        ur_body.append('        if (pTrav->pfnMsgCallback == pfnMsgCallback) {')
        ur_body.append('            pPrev->pNext = pTrav->pNext;')
        ur_body.append('            if (g_pDbgFunctionHead == pTrav)')
        ur_body.append('                g_pDbgFunctionHead = pTrav->pNext;')
        ur_body.append('            free(pTrav);')
        ur_body.append('            break;')
        ur_body.append('        }')
        ur_body.append('        pPrev = pTrav;')
        ur_body.append('        pTrav = pTrav->pNext;')
        ur_body.append('    }')
        ur_body.append('    if (g_pDbgFunctionHead == NULL)')
        ur_body.append('    {')
        ur_body.append('        if (g_actionIsDefault)')
        ur_body.append('            g_debugAction = VK_DBG_LAYER_ACTION_LOG_MSG;')
        ur_body.append('        else')
        ur_body.append('            g_debugAction = (VK_LAYER_DBG_ACTION)(g_debugAction & ~((uint32_t)VK_DBG_LAYER_ACTION_CALLBACK));')
        ur_body.append('    }')
        ur_body.append('    VkResult result = nextTable.DbgUnregisterMsgCallback(instance, pfnMsgCallback);')
        ur_body.append('    return result;')
        ur_body.append('}')
        return "\n".join(ur_body)

    def _gen_layer_get_global_extension_info(self, layer="Generic"):
        ggei_body = []
        ggei_body.append('struct extProps {')
        ggei_body.append('    uint32_t version;')
        ggei_body.append('    const char * const name;')
        ggei_body.append('};')
        if layer == 'ObjectTracker':
            ggei_body.append('#define LAYER_EXT_ARRAY_SIZE 4')
            ggei_body.append('static const struct extProps layerExts[LAYER_EXT_ARRAY_SIZE] = {')
            ggei_body.append('    // TODO what is the version?')
            ggei_body.append('    {0x10, "%s"},' % layer)
            ggei_body.append('    {0x10, "Validation"},')
            ggei_body.append('    {0x10, "objTrackGetObjectCount"},')
            ggei_body.append('    {0x10, "objTrackGetObjects"}')
            ggei_body.append('};')
        elif layer == 'Threading':
            ggei_body.append('#define LAYER_EXT_ARRAY_SIZE 2')
            ggei_body.append('static const struct extProps layerExts[LAYER_EXT_ARRAY_SIZE] = {')
            ggei_body.append('    // TODO what is the version?')
            ggei_body.append('    {0x10, "%s"},' % layer)
            ggei_body.append('    {0x10, "Validation"},')
            ggei_body.append('};')
        else:
            ggei_body.append('#define LAYER_EXT_ARRAY_SIZE 1')
            ggei_body.append('static const struct extProps layerExts[LAYER_EXT_ARRAY_SIZE] = {')
            ggei_body.append('    // TODO what is the version?')
            ggei_body.append('    {0x10, "%s"}' % layer)
            ggei_body.append('};')
        ggei_body.append('')
        ggei_body.append('VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionInfo(VkExtensionInfoType infoType, uint32_t extensionIndex, size_t* pDataSize, void* pData)')
        ggei_body.append('{')
        ggei_body.append('    VkExtensionProperties *ext_props;')
        ggei_body.append('    uint32_t *count;')
        ggei_body.append('')
        ggei_body.append('    if (pDataSize == NULL)')
        ggei_body.append('        return VK_ERROR_INVALID_POINTER;')
        ggei_body.append('')
        ggei_body.append('    switch (infoType) {')
        ggei_body.append('        case VK_EXTENSION_INFO_TYPE_COUNT:')
        ggei_body.append('            *pDataSize = sizeof(uint32_t);')
        ggei_body.append('            if (pData == NULL)')
        ggei_body.append('                return VK_SUCCESS;')
        ggei_body.append('            count = (uint32_t *) pData;')
        ggei_body.append('            *count = LAYER_EXT_ARRAY_SIZE;')
        ggei_body.append('            break;')
        ggei_body.append('        case VK_EXTENSION_INFO_TYPE_PROPERTIES:')
        ggei_body.append('            *pDataSize = sizeof(VkExtensionProperties);')
        ggei_body.append('            if (pData == NULL)')
        ggei_body.append('                return VK_SUCCESS;')
        ggei_body.append('            if (extensionIndex >= LAYER_EXT_ARRAY_SIZE)')
        ggei_body.append('                return VK_ERROR_INVALID_VALUE;')
        ggei_body.append('            ext_props = (VkExtensionProperties *) pData;')
        ggei_body.append('            ext_props->version = layerExts[extensionIndex].version;')
        ggei_body.append('            strncpy(ext_props->extName, layerExts[extensionIndex].name,')
        ggei_body.append('                                        VK_MAX_EXTENSION_NAME);')
        ggei_body.append("            ext_props->extName[VK_MAX_EXTENSION_NAME - 1] = '\\0';")
        ggei_body.append('            break;')
        ggei_body.append('        default:')
        ggei_body.append('            return VK_ERROR_INVALID_VALUE;')
        ggei_body.append('    };')
        ggei_body.append('    return VK_SUCCESS;')
        ggei_body.append('}')
        return "\n".join(ggei_body)

    def _gen_layer_get_extension_support(self, layer="Generic"):
        ges_body = []
        ges_body.append('VK_LAYER_EXPORT VkResult VKAPI xglGetExtensionSupport(VkPhysicalDevice gpu, const char* pExtName)')
        ges_body.append('{')
        ges_body.append('    VkResult result;')
        ges_body.append('    VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;')
        ges_body.append('')
        ges_body.append('    /* This entrypoint is NOT going to init its own dispatch table since loader calls here early */')
        ges_body.append('    if (!strncmp(pExtName, "%s", strlen("%s")))' % (layer, layer))
        ges_body.append('    {')
        ges_body.append('        result = VK_SUCCESS;')
        ges_body.append('    } else if (nextTable.GetExtensionSupport != NULL)')
        ges_body.append('    {')
        ges_body.append('        result = nextTable.GetExtensionSupport((VkPhysicalDevice)gpuw->nextObject, pExtName);')
        ges_body.append('    } else')
        ges_body.append('    {')
        ges_body.append('        result = VK_ERROR_INVALID_EXTENSION;')
        ges_body.append('    }')
        ges_body.append('    return result;')
        ges_body.append('}')
        return "\n".join(ges_body)

    def _generate_dispatch_entrypoints(self, qual=""):
        if qual:
            qual += " "

        funcs = []
        intercepted = []
        for proto in self.protos:
            if proto.name == "GetProcAddr":
                intercepted.append(proto)
            else:
                intercept = self.generate_intercept(proto, qual)
                if intercept is None:
                    # fill in default intercept for certain entrypoints
                    if 'DbgRegisterMsgCallback' == proto.name:
                        intercept = self._gen_layer_dbg_callback_register()
                    elif 'DbgUnregisterMsgCallback' == proto.name:
                        intercept = self._gen_layer_dbg_callback_unregister()
                    elif 'GetGlobalExtensionInfo' == proto.name:
                        funcs.append(self._gen_layer_get_global_extension_info(self.layer_name))
                if intercept is not None:
                    funcs.append(intercept)
                    intercepted.append(proto)

        prefix="vk"
        lookups = []
        for proto in intercepted:
            lookups.append("if (!strcmp(name, \"%s\"))" % proto.name)
            lookups.append("    return (void*) %s%s;" %
                    (prefix, proto.name))

        prefix="vk"
        lookups = []
        for proto in intercepted:
            lookups.append("if (!strcmp(name, \"%s\"))" % proto.name)
            lookups.append("    return (void*) %s%s;" %
                    (prefix, proto.name))

        # add customized layer_intercept_proc
        body = []
        body.append("static inline void* layer_intercept_proc(const char *name)")
        body.append("{")
        body.append(generate_get_proc_addr_check("name"))
        body.append("")
        body.append("    name += 2;")
        body.append("    %s" % "\n    ".join(lookups))
        body.append("")
        body.append("    return NULL;")
        body.append("}")
        funcs.append("\n".join(body))
        return "\n\n".join(funcs)


    def _generate_extensions(self):
        exts = []
        exts.append('uint64_t objTrackGetObjectCount(VK_OBJECT_TYPE type)')
        exts.append('{')
        exts.append('    return (type == VkObjectTypeAny) ? numTotalObjs : numObjs[type];')
        exts.append('}')
        exts.append('')
        exts.append('VkResult objTrackGetObjects(VK_OBJECT_TYPE type, uint64_t objCount, OBJTRACK_NODE* pObjNodeArray)')
        exts.append('{')
        exts.append("    // This bool flags if we're pulling all objs or just a single class of objs")
        exts.append('    bool32_t bAllObjs = (type == VkObjectTypeAny);')
        exts.append('    // Check the count first thing')
        exts.append('    uint64_t maxObjCount = (bAllObjs) ? numTotalObjs : numObjs[type];')
        exts.append('    if (objCount > maxObjCount) {')
        exts.append('        char str[1024];')
        exts.append('        sprintf(str, "OBJ ERROR : Received objTrackGetObjects() request for %lu objs, but there are only %lu objs of type %s", objCount, maxObjCount, string_VK_OBJECT_TYPE(type));')
        exts.append('        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, 0, 0, OBJTRACK_OBJCOUNT_MAX_EXCEEDED, "OBJTRACK", str);')
        exts.append('        return VK_ERROR_INVALID_VALUE;')
        exts.append('    }')
        exts.append('    objNode* pTrav = (bAllObjs) ? pGlobalHead : pObjectHead[type];')
        exts.append('    for (uint64_t i = 0; i < objCount; i++) {')
        exts.append('        if (!pTrav) {')
        exts.append('            char str[1024];')
        exts.append('            sprintf(str, "OBJ INTERNAL ERROR : Ran out of %s objs! Should have %lu, but only copied %lu and not the requested %lu.", string_VK_OBJECT_TYPE(type), maxObjCount, i, objCount);')
        exts.append('            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, 0, 0, OBJTRACK_INTERNAL_ERROR, "OBJTRACK", str);')
        exts.append('            return VK_ERROR_UNKNOWN;')
        exts.append('        }')
        exts.append('        memcpy(&pObjNodeArray[i], pTrav, sizeof(OBJTRACK_NODE));')
        exts.append('        pTrav = (bAllObjs) ? pTrav->pNextGlobal : pTrav->pNextObj;')
        exts.append('    }')
        exts.append('    return VK_SUCCESS;')
        exts.append('}')
        return "\n".join(exts)

    def _generate_layer_gpa_function(self, extensions=[]):
        func_body = []
        func_body.append("VK_LAYER_EXPORT void* VKAPI vkGetProcAddr(VkPhysicalDevice gpu, const char* funcName)\n"
                         "{\n"
                         "    VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;\n"
                         "    void* addr;\n"
                         "    if (gpu == VK_NULL_HANDLE)\n"
                         "        return NULL;\n"
                         "    pCurObj = gpuw;\n"
                         "    loader_platform_thread_once(&tabOnce, init%s);\n\n"
                         "    addr = layer_intercept_proc(funcName);\n"
                         "    if (addr)\n"
                         "        return addr;" % self.layer_name)

        if 0 != len(extensions):
            for ext_name in extensions:
                func_body.append('    else if (!strncmp("%s", funcName, sizeof("%s")))\n'
                                 '        return %s;' % (ext_name, ext_name, ext_name))
        func_body.append("    else {\n"
                         "        if (gpuw->pGPA == NULL)\n"
                         "            return NULL;\n"
                         "        return gpuw->pGPA((VkPhysicalDevice)gpuw->nextObject, funcName);\n"
                         "    }\n"
                         "}\n")
        return "\n".join(func_body)

    def _generate_layer_initialization(self, init_opts=False, prefix='vk', lockname=None):
        func_body = ["#include \"vk_dispatch_table_helper.h\""]
        func_body.append('static void init%s(void)\n'
                         '{\n' % self.layer_name)
        if init_opts:
            func_body.append('    const char *strOpt;')
            func_body.append('    // initialize %s options' % self.layer_name)
            func_body.append('    getLayerOptionEnum("%sReportLevel", (uint32_t *) &g_reportingLevel);' % self.layer_name)
            func_body.append('    g_actionIsDefault = getLayerOptionEnum("%sDebugAction", (uint32_t *) &g_debugAction);' % self.layer_name)
            func_body.append('')
            func_body.append('    if (g_debugAction & VK_DBG_LAYER_ACTION_LOG_MSG)')
            func_body.append('    {')
            func_body.append('        strOpt = getLayerOption("%sLogFilename");' % self.layer_name)
            func_body.append('        if (strOpt)')
            func_body.append('        {')
            func_body.append('            g_logFile = fopen(strOpt, "w");')
            func_body.append('        }')
            func_body.append('        if (g_logFile == NULL)')
            func_body.append('            g_logFile = stdout;')
            func_body.append('    }')
            func_body.append('')
        func_body.append('    PFN_vkGetProcAddr fpNextGPA;\n'
                         '    fpNextGPA = pCurObj->pGPA;\n'
                         '    assert(fpNextGPA);\n')

        func_body.append("    layer_initialize_dispatch_table(&nextTable, fpNextGPA, (VkPhysicalDevice) pCurObj->nextObject);")
        if lockname is not None:
            func_body.append("    if (!%sLockInitialized)" % lockname)
            func_body.append("    {")
            func_body.append("        // TODO/TBD: Need to delete this mutex sometime.  How???")
            func_body.append("        loader_platform_thread_create_mutex(&%sLock);" % lockname)
            func_body.append("        %sLockInitialized = 1;" % lockname)
            func_body.append("    }")
        func_body.append("}\n")
        return "\n".join(func_body)

    def _generate_layer_initialization_with_lock(self, prefix='vk'):
        func_body = ["#include \"vk_dispatch_table_helper.h\""]
        func_body.append('static void init%s(void)\n'
                         '{\n'
                         '    PFN_vkGetProcAddr fpNextGPA;\n'
                         '    fpNextGPA = pCurObj->pGPA;\n'
                         '    assert(fpNextGPA);\n' % self.layer_name)

        func_body.append("    layer_initialize_dispatch_table(&nextTable, fpNextGPA, (VkPhysicalDevice) pCurObj->nextObject);\n")
        func_body.append("    if (!printLockInitialized)")
        func_body.append("    {")
        func_body.append("        // TODO/TBD: Need to delete this mutex sometime.  How???")
        func_body.append("        loader_platform_thread_create_mutex(&printLock);")
        func_body.append("        printLockInitialized = 1;")
        func_body.append("    }")
        func_body.append("}\n")
        return "\n".join(func_body)

class LayerFuncsSubcommand(Subcommand):
    def generate_header(self):
        return '#include <vkLayer.h>\n#include "loader.h"'

    def generate_body(self):
        return self._generate_dispatch_entrypoints("static")

class LayerDispatchSubcommand(Subcommand):
    def generate_header(self):
        return '#include "layer_wrappers.h"'

    def generate_body(self):
        return self._generate_layer_initialization()

class GenericLayerSubcommand(Subcommand):
    def generate_header(self):
        return '#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include "loader_platform.h"\n#include "vkLayer.h"\n//The following is #included again to catch certain OS-specific functions being used:\n#include "loader_platform.h"\n\n#include "layers_config.h"\n#include "layers_msg.h"\n\nstatic VkLayerDispatchTable nextTable;\nstatic VkBaseLayerObject *pCurObj;\n\nstatic LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabOnce);'

    def generate_intercept(self, proto, qual):
        if proto.name in [ 'DbgRegisterMsgCallback', 'DbgUnregisterMsgCallback' , 'GetGlobalExtensionInfo']:
            # use default version
            return None
        decl = proto.c_func(prefix="vk", attr="VKAPI")
        ret_val = ''
        stmt = ''
        funcs = []
        if proto.ret != "void":
            ret_val = "VkResult result = "
            stmt = "    return result;\n"
        if proto.name == "EnumerateLayers":
            funcs.append('%s%s\n'
                     '{\n'
                     '    char str[1024];\n'
                     '    if (gpu != VK_NULL_HANDLE) {\n'
                     '        sprintf(str, "At start of layered %s\\n");\n'
                     '        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, gpu, 0, 0, (char *) "GENERIC", (char *) str);\n'
                     '        pCurObj = (VkBaseLayerObject *) gpu;\n'
                     '        loader_platform_thread_once(&tabOnce, init%s);\n'
                     '        %snextTable.%s;\n'
                     '        sprintf(str, "Completed layered %s\\n");\n'
                     '        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, gpu, 0, 0, (char *) "GENERIC", (char *) str);\n'
                     '        fflush(stdout);\n'
                     '    %s'
                     '    } else {\n'
                     '        if (pLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL)\n'
                     '            return VK_ERROR_INVALID_POINTER;\n'
                     '        // This layer compatible with all GPUs\n'
                     '        *pLayerCount = 1;\n'
                     '        strncpy((char *) pOutLayers[0], "%s", maxStringSize);\n'
                     '        return VK_SUCCESS;\n'
                     '    }\n'
                     '}' % (qual, decl, proto.name, self.layer_name, ret_val, proto.c_call(), proto.name, stmt, self.layer_name))
        else:
            funcs.append('%s%s\n'
                     '{\n'
                     '    %snextTable.%s;\n'
                     '%s'
                     '}' % (qual, decl, ret_val, proto.c_call(), stmt))
        return "\n\n".join(funcs)

    def generate_body(self):
        self.layer_name = "Generic"
        body = [self._generate_layer_initialization(True),
                self._generate_dispatch_entrypoints("VK_LAYER_EXPORT"),
                self._generate_layer_gpa_function()]

        return "\n\n".join(body)

class APIDumpSubcommand(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include <fstream>')
        header_txt.append('#include <iostream>')
        header_txt.append('#include <string>')
        header_txt.append('')
        header_txt.append('static std::ofstream fileStream;')
        header_txt.append('static std::string fileName = "vk_apidump.txt";')
        header_txt.append('std::ostream* outputStream = NULL;')
        header_txt.append('void ConfigureOutputStream(bool writeToFile, bool flushAfterWrite)')
        header_txt.append('{')
        header_txt.append('    if(writeToFile)')
        header_txt.append('    {')
        header_txt.append('        fileStream.open(fileName);')
        header_txt.append('        outputStream = &fileStream;')
        header_txt.append('    }')
        header_txt.append('    else')
        header_txt.append('    {')
        header_txt.append('        outputStream = &std::cout;')
        header_txt.append('    }')
        header_txt.append('')
        header_txt.append('    if(flushAfterWrite)')
        header_txt.append('    {')
        header_txt.append('        outputStream->sync_with_stdio(true);')
        header_txt.append('    }')
        header_txt.append('    else')
        header_txt.append('    {')
        header_txt.append('        outputStream->sync_with_stdio(false);')
        header_txt.append('    }')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('#include "loader_platform.h"')
        header_txt.append('#include "vkLayer.h"')
        header_txt.append('#include "vk_struct_string_helper_cpp.h"')
        header_txt.append('')
        header_txt.append('// The following is #included again to catch certain OS-specific functions being used:')
        header_txt.append('#include "loader_platform.h"')
        header_txt.append('')
        header_txt.append('static VkLayerDispatchTable nextTable;')
        header_txt.append('static VkBaseLayerObject *pCurObj;')
        header_txt.append('static bool g_APIDumpDetailed = true;')
        header_txt.append('')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabOnce);')
        header_txt.append('static int printLockInitialized = 0;')
        header_txt.append('static loader_platform_thread_mutex printLock;')
        header_txt.append('')
        header_txt.append('#define MAX_TID 513')
        header_txt.append('static loader_platform_thread_id tidMapping[MAX_TID] = {0};')
        header_txt.append('static uint32_t maxTID = 0;')
        header_txt.append('// Map actual TID to an index value and return that index')
        header_txt.append('//  This keeps TIDs in range from 0-MAX_TID and simplifies compares between runs')
        header_txt.append('static uint32_t getTIDIndex() {')
        header_txt.append('    loader_platform_thread_id tid = loader_platform_get_thread_id();')
        header_txt.append('    for (uint32_t i = 0; i < maxTID; i++) {')
        header_txt.append('        if (tid == tidMapping[i])')
        header_txt.append('            return i;')
        header_txt.append('    }')
        header_txt.append("    // Don't yet have mapping, set it and return newly set index")
        header_txt.append('    uint32_t retVal = (uint32_t) maxTID;')
        header_txt.append('    tidMapping[maxTID++] = tid;')
        header_txt.append('    assert(maxTID < MAX_TID);')
        header_txt.append('    return retVal;')
        header_txt.append('}')
        return "\n".join(header_txt)

    def generate_init(self):
        func_body = []
        func_body.append('#include "vk_dispatch_table_helper.h"')
        func_body.append('#include "layers_config.h"')
        func_body.append('')
        func_body.append('static void init%s(void)' % self.layer_name)
        func_body.append('{')
        func_body.append('    using namespace StreamControl;')
        func_body.append('')
        func_body.append('    char const*const detailedStr = getLayerOption("APIDumpDetailed");')
        func_body.append('    if(detailedStr != NULL)')
        func_body.append('    {')
        func_body.append('        if(strcmp(detailedStr, "TRUE") == 0)')
        func_body.append('        {')
        func_body.append('            g_APIDumpDetailed = true;')
        func_body.append('        }')
        func_body.append('        else if(strcmp(detailedStr, "FALSE") == 0)')
        func_body.append('        {')
        func_body.append('            g_APIDumpDetailed = false;')
        func_body.append('        }')
        func_body.append('    }')
        func_body.append('')
        func_body.append('    char const*const writeToFileStr = getLayerOption("APIDumpFile");')
        func_body.append('    bool writeToFile = false;')
        func_body.append('    if(writeToFileStr != NULL)')
        func_body.append('    {')
        func_body.append('        if(strcmp(writeToFileStr, "TRUE") == 0)')
        func_body.append('        {')
        func_body.append('            writeToFile = true;')
        func_body.append('        }')
        func_body.append('        else if(strcmp(writeToFileStr, "FALSE") == 0)')
        func_body.append('        {')
        func_body.append('            writeToFile = false;')
        func_body.append('        }')
        func_body.append('    }')
        func_body.append('')
        func_body.append('    char const*const noAddrStr = getLayerOption("APIDumpNoAddr");')
        func_body.append('    if(noAddrStr != NULL)')
        func_body.append('    {')
        func_body.append('        if(strcmp(noAddrStr, "FALSE") == 0)')
        func_body.append('        {')
        func_body.append('            StreamControl::writeAddress = true;')
        func_body.append('        }')
        func_body.append('        else if(strcmp(noAddrStr, "TRUE") == 0)')
        func_body.append('        {')
        func_body.append('            StreamControl::writeAddress = false;')
        func_body.append('        }')
        func_body.append('    }')
        func_body.append('')
        func_body.append('    char const*const flushAfterWriteStr = getLayerOption("APIDumpFlush");')
        func_body.append('    bool flushAfterWrite = false;')
        func_body.append('    if(flushAfterWriteStr != NULL)')
        func_body.append('    {')
        func_body.append('        if(strcmp(flushAfterWriteStr, "TRUE") == 0)')
        func_body.append('        {')
        func_body.append('            flushAfterWrite = true;')
        func_body.append('        }')
        func_body.append('        else if(strcmp(flushAfterWriteStr, "FALSE") == 0)')
        func_body.append('        {')
        func_body.append('            flushAfterWrite = false;')
        func_body.append('        }')
        func_body.append('    }')
        func_body.append('')
        func_body.append('    ConfigureOutputStream(writeToFile, flushAfterWrite);')
        func_body.append('')
        func_body.append('    PFN_vkGetProcAddr fpNextGPA;')
        func_body.append('    fpNextGPA = pCurObj->pGPA;')
        func_body.append('    assert(fpNextGPA);')
        func_body.append('    layer_initialize_dispatch_table(&nextTable, fpNextGPA, (VkPhysicalDevice) pCurObj->nextObject);')
        func_body.append('')
        func_body.append('    if (!printLockInitialized)')
        func_body.append('    {')
        func_body.append('        // TODO/TBD: Need to delete this mutex sometime.  How???')
        func_body.append('        loader_platform_thread_create_mutex(&printLock);')
        func_body.append('        printLockInitialized = 1;')
        func_body.append('    }')
        func_body.append('}')
        func_body.append('')
        return "\n".join(func_body)

    def generate_intercept(self, proto, qual):
        if proto.name in [ 'GetGlobalExtensionInfo']:
            return None
        decl = proto.c_func(prefix="vk", attr="VKAPI")
        ret_val = ''
        stmt = ''
        funcs = []
        sp_param_dict = {} # Store 'index' for struct param to print, or an name of binding "Count" param for array to print
        create_params = 0 # Num of params at end of function that are created and returned as output values
        if 'AllocDescriptorSets' in proto.name:
            create_params = -2
        elif 'Create' in proto.name or 'Alloc' in proto.name or 'MapMemory' in proto.name:
            create_params = -1
        if proto.ret != "void":
            ret_val = "VkResult result = "
            stmt = "    return result;\n"
        f_open = 'loader_platform_thread_lock_mutex(&printLock);\n    '
        log_func = '    if (StreamControl::writeAddress == true) {'
        log_func += '\n        (*outputStream) << "t{" << getTIDIndex() << "} vk%s(' % proto.name
        log_func_no_addr = '\n        (*outputStream) << "t{" << getTIDIndex() << "} vk%s(' % proto.name
        f_close = '\n    loader_platform_thread_unlock_mutex(&printLock);'
        pindex = 0
        prev_count_name = ''
        for p in proto.params:
            cp = False
            if 0 != create_params:
                # If this is any of the N last params of the func, treat as output
                for y in range(-1, create_params-1, -1):
                    if p.name == proto.params[y].name:
                        cp = True
            (pft, pfi) = self._get_printf_params(p.ty, p.name, cp, cpp=True)
            log_func += '%s = " << %s << ", ' % (p.name, pfi)
            if "%p" == pft:
                log_func_no_addr += '%s = address, ' % (p.name)
            else:
                log_func_no_addr += '%s = " << %s << ", ' % (p.name, pfi)
            if prev_count_name != '' and (prev_count_name.replace('Count', '')[1:] in p.name):
                sp_param_dict[pindex] = prev_count_name
                prev_count_name = ''
            elif 'pDescriptorSets' == p.name and proto.params[-1].name == 'pCount':
                sp_param_dict[pindex] = '*pCount'
            elif vk_helper.is_type(p.ty.strip('*').replace('const ', ''), 'struct'):
                sp_param_dict[pindex] = 'index'
            if p.name.endswith('Count'):
                if '*' in p.ty:
                    prev_count_name = "*%s" % p.name
                else:
                    prev_count_name = p.name
            pindex += 1
        log_func = log_func.strip(', ')
        log_func_no_addr = log_func_no_addr.strip(', ')
        if proto.ret != "void":
            log_func += ') = " << string_VkResult((VkResult)result) << endl'
            log_func_no_addr += ') = " << string_VkResult((VkResult)result) << endl'
        else:
            log_func += ')\\n"'
            log_func_no_addr += ')\\n"'
        log_func += ';'
        log_func_no_addr += ';'
        log_func += '\n    }\n    else {%s;\n    }' % log_func_no_addr;
        #print("Proto %s has param_dict: %s" % (proto.name, sp_param_dict))
        if len(sp_param_dict) > 0:
            indent = '    '
            log_func += '\n%sif (g_APIDumpDetailed) {' % indent
            indent += '    '
            i_decl = False
            log_func += '\n%sstring tmp_str;' % indent
            for sp_index in sp_param_dict:
                #print("sp_index: %s" % str(sp_index))
                if 'index' == sp_param_dict[sp_index]:
                    cis_print_func = 'vk_print_%s' % (proto.params[sp_index].ty.replace('const ', '').strip('*').lower())
                    local_name = proto.params[sp_index].name
                    if '*' not in proto.params[sp_index].ty:
                        local_name = '&%s' % proto.params[sp_index].name
                    log_func += '\n%sif (%s) {' % (indent, local_name)
                    indent += '    '
                    log_func += '\n%stmp_str = %s(%s, "    ");' % (indent, cis_print_func, local_name)
                    log_func += '\n%s(*outputStream) << "   %s (" << %s << ")" << endl << tmp_str << endl;' % (indent, local_name, local_name)
                    indent = indent[4:]
                    log_func += '\n%s}' % (indent)
                else: # We have a count value stored to iterate over an array
                    print_cast = ''
                    print_func = ''
                    if vk_helper.is_type(proto.params[sp_index].ty.strip('*').replace('const ', ''), 'struct'):
                        print_cast = '&'
                        print_func = 'vk_print_%s' % proto.params[sp_index].ty.replace('const ', '').strip('*').lower()
                    else:
                        print_cast = ''
                        print_func = 'string_convert_helper'
                        #cis_print_func = 'tmp_str = string_convert_helper((void*)%s[i], "    ");' % proto.params[sp_index].name
                    cis_print_func = 'tmp_str = %s(%s%s[i], "    ");' % (print_func, print_cast, proto.params[sp_index].name)
                    if not i_decl:
                        log_func += '\n%suint32_t i;' % (indent)
                        i_decl = True
                    log_func += '\n%sfor (i = 0; i < %s; i++) {' % (indent, sp_param_dict[sp_index])
                    indent += '    '
                    log_func += '\n%s%s' % (indent, cis_print_func)
                    log_func += '\n%sif (StreamControl::writeAddress == true) {' % (indent)
                    indent += '    '
                    log_func += '\n%s(*outputStream) << "   %s[" << i << "] (" << %s%s[i] << ")" << endl << tmp_str << endl;' % (indent, proto.params[sp_index].name, '&', proto.params[sp_index].name)
                    indent = indent[4:]
                    log_func += '\n%s} else {' % (indent)
                    indent += '    '
                    log_func += '\n%s(*outputStream) << "   %s[" << i << "] (address)" << endl << "    address" << endl;' % (indent, proto.params[sp_index].name)
                    indent = indent[4:]
                    log_func += '\n%s}' % (indent)
                    indent = indent[4:]
                    log_func += '\n%s}' % (indent)
            indent = indent[4:]
            log_func += '\n%s}' % (indent)
        if proto.name == "EnumerateLayers":
            funcs.append('%s%s\n'
                     '{\n'
                     '    using namespace StreamControl;\n'
                     '    if (gpu != NULL) {\n'
                     '        pCurObj = (VkBaseLayerObject *) gpu;\n'
                     '        loader_platform_thread_once(&tabOnce, init%s);\n'
                     '        %snextTable.%s;\n'
                     '        %s    %s    %s\n'
                     '    %s'
                     '    } else {\n'
                     '        if (pLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL)\n'
                     '            return VK_ERROR_INVALID_POINTER;\n'
                     '        // This layer compatible with all GPUs\n'
                     '        *pLayerCount = 1;\n'
                     '        strncpy((char *) pOutLayers[0], "%s", maxStringSize);\n'
                     '        return VK_SUCCESS;\n'
                     '    }\n'
                         '}' % (qual, decl, self.layer_name, ret_val, proto.c_call(),f_open, log_func, f_close, stmt, self.layer_name))
        elif 'GetExtensionSupport' == proto.name:
            funcs.append('%s%s\n'
                         '{\n'
                         '    VkResult result;\n'
                         '    /* This entrypoint is NOT going to init its own dispatch table since loader calls here early */\n'
                         '    if (!strncmp(pExtName, "%s", strlen("%s")))\n'
                         '    {\n'
                         '        result = VK_SUCCESS;\n'
                         '    } else if (nextTable.GetExtensionSupport != NULL)\n'
                         '    {\n'
                         '        result = nextTable.%s;\n'
                         '        %s    %s        %s\n'
                         '    } else\n'
                         '    {\n'
                         '        result = VK_ERROR_INVALID_EXTENSION;\n'
                         '    }\n'
                         '%s'
                         '}' % (qual, decl, self.layer_name, self.layer_name, proto.c_call(), f_open, log_func, f_close, stmt))
        else:
            funcs.append('%s%s\n'
                     '{\n'
                     '    using namespace StreamControl;\n'
                     '    %snextTable.%s;\n'
                     '    %s%s%s\n'
                     '%s'
                     '}' % (qual, decl, ret_val, proto.c_call(), f_open, log_func, f_close, stmt))
        return "\n\n".join(funcs)

    def generate_body(self):
        self.layer_name = "APIDump"
        body = [self.generate_init(),
                self._generate_dispatch_entrypoints("VK_LAYER_EXPORT"),
                self._generate_layer_gpa_function()]
        return "\n\n".join(body)

class ObjectTrackerSubcommand(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <inttypes.h>\n#include "loader_platform.h"')
        header_txt.append('#include "object_track.h"\n\nstatic VkLayerDispatchTable nextTable;\nstatic VkBaseLayerObject *pCurObj;')
        header_txt.append('// The following is #included again to catch certain OS-specific functions being used:')
        header_txt.append('#include "loader_platform.h"')
        header_txt.append('#include "layers_config.h"')
        header_txt.append('#include "layers_msg.h"')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabOnce);')
        header_txt.append('static long long unsigned int object_track_index = 0;')
        header_txt.append('static int objLockInitialized = 0;')
        header_txt.append('static loader_platform_thread_mutex objLock;')
        header_txt.append('')
        header_txt.append('// We maintain a "Global" list which links every object and a')
        header_txt.append('//  per-Object list which just links objects of a given type')
        header_txt.append('// The object node has both pointers so the actual nodes are shared between the two lists')
        header_txt.append('typedef struct _objNode {')
        header_txt.append('    OBJTRACK_NODE   obj;')
        header_txt.append('    struct _objNode *pNextObj;')
        header_txt.append('    struct _objNode *pNextGlobal;')
        header_txt.append('} objNode;')
        header_txt.append('')
        header_txt.append('static objNode *pObjectHead[VkNumObjectType] = {0};')
        header_txt.append('static objNode *pGlobalHead = NULL;')
        header_txt.append('static uint64_t numObjs[VkNumObjectType] = {0};')
        header_txt.append('static uint64_t numTotalObjs = 0;')
        header_txt.append('static uint32_t maxMemReferences = 0;')
        header_txt.append('')
        header_txt.append('// For each Queue\'s doubly linked-list of mem refs')
        header_txt.append('typedef struct _OT_MEM_INFO {')
        header_txt.append('    VkDeviceMemory        mem;')
        header_txt.append('    struct _OT_MEM_INFO *pNextMI;')
        header_txt.append('    struct _OT_MEM_INFO *pPrevMI;')
        header_txt.append('')
        header_txt.append('} OT_MEM_INFO;')
        header_txt.append('')
        header_txt.append('// Track Queue information')
        header_txt.append('typedef struct _OT_QUEUE_INFO {')
        header_txt.append('    OT_MEM_INFO           *pMemRefList;')
        header_txt.append('    struct _OT_QUEUE_INFO *pNextQI;')
        header_txt.append('    VkQueue               queue;')
        header_txt.append('    uint32_t               refCount;')
        header_txt.append('} OT_QUEUE_INFO;')
        header_txt.append('')
        header_txt.append('// Global list of QueueInfo structures, one per queue')
        header_txt.append('static OT_QUEUE_INFO *g_pQueueInfo;')
        header_txt.append('')
        header_txt.append('// Add new queue to head of global queue list')
        header_txt.append('static void addQueueInfo(VkQueue queue)')
        header_txt.append('{')
        header_txt.append('    OT_QUEUE_INFO *pQueueInfo = malloc(sizeof(OT_QUEUE_INFO));')
        header_txt.append('    memset(pQueueInfo, 0, sizeof(OT_QUEUE_INFO));')
        header_txt.append('    pQueueInfo->queue = queue;')
        header_txt.append('')
        header_txt.append('    if (pQueueInfo != NULL) {')
        header_txt.append('        pQueueInfo->pNextQI   = g_pQueueInfo;')
        header_txt.append('        g_pQueueInfo          = pQueueInfo;')
        header_txt.append('    }')
        header_txt.append('    else {')
        header_txt.append('        char str[1024];')
        header_txt.append('        sprintf(str, "ERROR:  VK_ERROR_OUT_OF_HOST_MEMORY -- could not allocate memory for Queue Information");')
        header_txt.append('        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, queue, 0, OBJTRACK_INTERNAL_ERROR, "OBJTRACK", str);')
        header_txt.append('    }')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('// Destroy memRef lists and free all memory')
        header_txt.append('static void destroyQueueMemRefLists()')
        header_txt.append('{')
        header_txt.append('    OT_QUEUE_INFO *pQueueInfo    = g_pQueueInfo;')
        header_txt.append('    OT_QUEUE_INFO *pDelQueueInfo = NULL;')
        header_txt.append('    while (pQueueInfo != NULL) {')
        header_txt.append('        OT_MEM_INFO *pMemInfo = pQueueInfo->pMemRefList;')
        header_txt.append('        while (pMemInfo != NULL) {')
        header_txt.append('            OT_MEM_INFO *pDelMemInfo = pMemInfo;')
        header_txt.append('            pMemInfo = pMemInfo->pNextMI;')
        header_txt.append('            free(pDelMemInfo);')
        header_txt.append('        }')
        header_txt.append('        pDelQueueInfo = pQueueInfo;')
        header_txt.append('        pQueueInfo    = pQueueInfo->pNextQI;')
        header_txt.append('        free(pDelQueueInfo);')
        header_txt.append('    }')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('// Debug function to print global list and each individual object list')
        header_txt.append('static void ll_print_lists()')
        header_txt.append('{')
        header_txt.append('    objNode* pTrav = pGlobalHead;')
        header_txt.append('    printf("=====GLOBAL OBJECT LIST (%lu total objs):\\n", numTotalObjs);')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        printf("   ObjNode (%p) w/ %s obj 0x%" PRId64 " has pNextGlobal %p\\n", (void*)pTrav, string_VK_OBJECT_TYPE(pTrav->obj.objType), pTrav->obj.vkObj, (void*)pTrav->pNextGlobal);')
        header_txt.append('        pTrav = pTrav->pNextGlobal;')
        header_txt.append('    }')
        header_txt.append('    for (uint32_t i = 0; i < VkNumObjectType; i++) {')
        header_txt.append('        pTrav = pObjectHead[i];')
        header_txt.append('        if (pTrav) {')
        header_txt.append('            printf("=====%s OBJECT LIST (%lu objs):\\n", string_VK_OBJECT_TYPE(pTrav->obj.objType), numObjs[i]);')
        header_txt.append('            while (pTrav) {')
        header_txt.append('                printf("   ObjNode (%p) w/ %s obj 0x%" PRId64 " has pNextObj %p\\n", (void*)pTrav, string_VK_OBJECT_TYPE(pTrav->obj.objType), pTrav->obj.vkObj, (void*)pTrav->pNextObj);')
        header_txt.append('                pTrav = pTrav->pNextObj;')
        header_txt.append('            }')
        header_txt.append('        }')
        header_txt.append('    }')
        header_txt.append('}')
        header_txt.append('static void ll_insert_obj(VkObject vkObj, VK_OBJECT_TYPE objType) {')
        header_txt.append('    char str[1024];')
        header_txt.append('    sprintf(str, "OBJ[%llu] : CREATE %s object 0x%" PRId64, object_track_index++, string_VK_OBJECT_TYPE(objType), vkObj);')
        header_txt.append('    layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, vkObj, 0, OBJTRACK_NONE, "OBJTRACK", str);')
        header_txt.append('    objNode* pNewObjNode = (objNode*)malloc(sizeof(objNode));')
        header_txt.append('    pNewObjNode->obj.vkObj = vkObj;')
        header_txt.append('    pNewObjNode->obj.objType = objType;')
        header_txt.append('    pNewObjNode->obj.status  = OBJSTATUS_NONE;')
        header_txt.append('    pNewObjNode->obj.numUses = 0;')
        header_txt.append('    // insert at front of global list')
        header_txt.append('    pNewObjNode->pNextGlobal = pGlobalHead;')
        header_txt.append('    pGlobalHead = pNewObjNode;')
        header_txt.append('    // insert at front of object list')
        header_txt.append('    pNewObjNode->pNextObj = pObjectHead[objType];')
        header_txt.append('    pObjectHead[objType] = pNewObjNode;')
        header_txt.append('    // increment obj counts')
        header_txt.append('    numObjs[objType]++;')
        header_txt.append('    numTotalObjs++;')
        header_txt.append('    //sprintf(str, "OBJ_STAT : %lu total objs & %lu %s objs.", numTotalObjs, numObjs[objType], string_VK_OBJECT_TYPE(objType));')
        header_txt.append('    if (0) ll_print_lists();')
        header_txt.append('}')
        header_txt.append('static void ll_increment_use_count(VkObject vkObj, VK_OBJECT_TYPE objType) {')
        header_txt.append('    objNode *pTrav = pObjectHead[objType];')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        if (pTrav->obj.vkObj == vkObj) {')
        header_txt.append('            pTrav->obj.numUses++;')
        header_txt.append('            char str[1024];')
        header_txt.append('            sprintf(str, "OBJ[%llu] : USING %s object 0x%" PRId64 " (%lu total uses)", object_track_index++, string_VK_OBJECT_TYPE(objType), vkObj, pTrav->obj.numUses);')
        header_txt.append('            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, vkObj, 0, OBJTRACK_NONE, "OBJTRACK", str);')
        header_txt.append('            return;')
        header_txt.append('        }')
        header_txt.append('        pTrav = pTrav->pNextObj;')
        header_txt.append('    }')
        header_txt.append('    // If we do not find obj, insert it and then increment count')
        header_txt.append('    char str[1024];')
        header_txt.append('    sprintf(str, "Unable to increment count for obj 0x%" PRId64 ", will add to list as %s type and increment count", vkObj, string_VK_OBJECT_TYPE(objType));')
        header_txt.append('    layerCbMsg(VK_DBG_MSG_WARNING, VK_VALIDATION_LEVEL_0, vkObj, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK", str);')
        header_txt.append('')
        header_txt.append('    ll_insert_obj(vkObj, objType);')
        header_txt.append('    ll_increment_use_count(vkObj, objType);')
        header_txt.append('}')
        header_txt.append('// We usually do not know Obj type when we destroy it so have to fetch')
        header_txt.append('//  Type from global list w/ ll_destroy_obj()')
        header_txt.append('//   and then do the full removal from both lists w/ ll_remove_obj_type()')
        header_txt.append('static void ll_remove_obj_type(VkObject vkObj, VK_OBJECT_TYPE objType) {')
        header_txt.append('    objNode *pTrav = pObjectHead[objType];')
        header_txt.append('    objNode *pPrev = pObjectHead[objType];')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        if (pTrav->obj.vkObj == vkObj) {')
        header_txt.append('            pPrev->pNextObj = pTrav->pNextObj;')
        header_txt.append('            // update HEAD of Obj list as needed')
        header_txt.append('            if (pObjectHead[objType] == pTrav)')
        header_txt.append('                pObjectHead[objType] = pTrav->pNextObj;')
        header_txt.append('            assert(numObjs[objType] > 0);')
        header_txt.append('            numObjs[objType]--;')
        header_txt.append('            char str[1024];')
        header_txt.append('            sprintf(str, "OBJ[%llu] : DESTROY %s object 0x%" PRId64, object_track_index++, string_VK_OBJECT_TYPE(objType), vkObj);')
        header_txt.append('            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, vkObj, 0, OBJTRACK_NONE, "OBJTRACK", str);')
        header_txt.append('            return;')
        header_txt.append('        }')
        header_txt.append('        pPrev = pTrav;')
        header_txt.append('        pTrav = pTrav->pNextObj;')
        header_txt.append('    }')
        header_txt.append('    char str[1024];')
        header_txt.append('    sprintf(str, "OBJ INTERNAL ERROR : Obj 0x%" PRId64 " was in global list but not in %s list", vkObj, string_VK_OBJECT_TYPE(objType));')
        header_txt.append('    layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, vkObj, 0, OBJTRACK_INTERNAL_ERROR, "OBJTRACK", str);')
        header_txt.append('}')
        header_txt.append('// Parse global list to find obj type, then remove obj from obj type list, finally')
        header_txt.append('//   remove obj from global list')
        header_txt.append('static void ll_destroy_obj(VkObject vkObj) {')
        header_txt.append('    objNode *pTrav = pGlobalHead;')
        header_txt.append('    objNode *pPrev = pGlobalHead;')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        if (pTrav->obj.vkObj == vkObj) {')
        header_txt.append('            ll_remove_obj_type(vkObj, pTrav->obj.objType);')
        header_txt.append('            pPrev->pNextGlobal = pTrav->pNextGlobal;')
        header_txt.append('            // update HEAD of global list if needed')
        header_txt.append('            if (pGlobalHead == pTrav)')
        header_txt.append('                pGlobalHead = pTrav->pNextGlobal;')
        header_txt.append('            assert(numTotalObjs > 0);')
        header_txt.append('            numTotalObjs--;')
        header_txt.append('            char str[1024];')
        header_txt.append('            sprintf(str, "OBJ_STAT Removed %s obj 0x%" PRId64 " that was used %lu times (%lu total objs remain & %lu %s objs).", string_VK_OBJECT_TYPE(pTrav->obj.objType), pTrav->obj.vkObj, pTrav->obj.numUses, numTotalObjs, numObjs[pTrav->obj.objType], string_VK_OBJECT_TYPE(pTrav->obj.objType));')
        header_txt.append('            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, vkObj, 0, OBJTRACK_NONE, "OBJTRACK", str);')
        header_txt.append('            free(pTrav);')
        header_txt.append('            return;')
        header_txt.append('        }')
        header_txt.append('        pPrev = pTrav;')
        header_txt.append('        pTrav = pTrav->pNextGlobal;')
        header_txt.append('    }')
        header_txt.append('    char str[1024];')
        header_txt.append('    sprintf(str, "Unable to remove obj 0x%" PRId64 ". Was it created? Has it already been destroyed?", vkObj);')
        header_txt.append('    layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, vkObj, 0, OBJTRACK_DESTROY_OBJECT_FAILED, "OBJTRACK", str);')
        header_txt.append('}')
        header_txt.append('// Set selected flag state for an object node')
        header_txt.append('static void set_status(VkObject vkObj, VK_OBJECT_TYPE objType, OBJECT_STATUS status_flag) {')
        header_txt.append('    if (vkObj != VK_NULL_HANDLE) {')
        header_txt.append('        objNode *pTrav = pObjectHead[objType];')
        header_txt.append('        while (pTrav) {')
        header_txt.append('            if (pTrav->obj.vkObj == vkObj) {')
        header_txt.append('                pTrav->obj.status |= status_flag;')
        header_txt.append('                return;')
        header_txt.append('            }')
        header_txt.append('            pTrav = pTrav->pNextObj;')
        header_txt.append('        }')
        header_txt.append('        // If we do not find it print an error')
        header_txt.append('        char str[1024];')
        header_txt.append('        sprintf(str, "Unable to set status for non-existent object 0x%" PRId64 " of %s type", vkObj, string_VK_OBJECT_TYPE(objType));')
        header_txt.append('        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, vkObj, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK", str);')
        header_txt.append('    }');
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('// Track selected state for an object node')
        header_txt.append('static void track_object_status(VkObject vkObj, VkStateBindPoint stateBindPoint) {')
        header_txt.append('    objNode *pTrav = pObjectHead[VkObjectTypeCmdBuffer];')
        header_txt.append('')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        if (pTrav->obj.vkObj == vkObj) {')
        header_txt.append('            if (stateBindPoint == VK_STATE_BIND_POINT_VIEWPORT) {')
        header_txt.append('                pTrav->obj.status |= OBJSTATUS_VIEWPORT_BOUND;')
        header_txt.append('            } else if (stateBindPoint == VK_STATE_BIND_POINT_RASTER) {')
        header_txt.append('                pTrav->obj.status |= OBJSTATUS_RASTER_BOUND;')
        header_txt.append('            } else if (stateBindPoint == VK_STATE_BIND_POINT_COLOR_BLEND) {')
        header_txt.append('                pTrav->obj.status |= OBJSTATUS_COLOR_BLEND_BOUND;')
        header_txt.append('            } else if (stateBindPoint == VK_STATE_BIND_POINT_DEPTH_STENCIL) {')
        header_txt.append('                pTrav->obj.status |= OBJSTATUS_DEPTH_STENCIL_BOUND;')
        header_txt.append('            }')
        header_txt.append('            return;')
        header_txt.append('        }')
        header_txt.append('        pTrav = pTrav->pNextObj;')
        header_txt.append('    }')
        header_txt.append('    // If we do not find it print an error')
        header_txt.append('    char str[1024];')
        header_txt.append('    sprintf(str, "Unable to track status for non-existent Command Buffer object 0x%" PRId64, vkObj);')
        header_txt.append('    layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, vkObj, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK", str);')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('// Reset selected flag state for an object node')
        header_txt.append('static void reset_status(VkObject vkObj, VK_OBJECT_TYPE objType, OBJECT_STATUS status_flag) {')
        header_txt.append('    objNode *pTrav = pObjectHead[objType];')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        if (pTrav->obj.vkObj == vkObj) {')
        header_txt.append('            pTrav->obj.status &= ~status_flag;')
        header_txt.append('            return;')
        header_txt.append('        }')
        header_txt.append('        pTrav = pTrav->pNextObj;')
        header_txt.append('    }')
        header_txt.append('    // If we do not find it print an error')
        header_txt.append('    char str[1024];')
        header_txt.append('    sprintf(str, "Unable to reset status for non-existent object 0x%" PRId64 " of %s type", vkObj, string_VK_OBJECT_TYPE(objType));')
        header_txt.append('    layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, vkObj, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK", str);')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('// Check object status for selected flag state')
        header_txt.append('static bool32_t validate_status(VkObject vkObj, VK_OBJECT_TYPE objType, OBJECT_STATUS status_mask, OBJECT_STATUS status_flag, VK_DBG_MSG_TYPE error_level, OBJECT_TRACK_ERROR error_code, char* fail_msg) {')
        header_txt.append('    objNode *pTrav = pObjectHead[objType];')
        header_txt.append('    while (pTrav) {')
        header_txt.append('        if (pTrav->obj.vkObj == vkObj) {')
        header_txt.append('            if ((pTrav->obj.status & status_mask) != status_flag) {')
        header_txt.append('                char str[1024];')
        header_txt.append('                sprintf(str, "OBJECT VALIDATION WARNING: %s object 0x%" PRId64 ": %s", string_VK_OBJECT_TYPE(objType), vkObj, fail_msg);')
        header_txt.append('                layerCbMsg(error_level, VK_VALIDATION_LEVEL_0, vkObj, 0, error_code, "OBJTRACK", str);')
        header_txt.append('                return VK_FALSE;')
        header_txt.append('            }')
        header_txt.append('            return VK_TRUE;')
        header_txt.append('        }')
        header_txt.append('        pTrav = pTrav->pNextObj;')
        header_txt.append('    }')
        header_txt.append('    if (objType != VkObjectTypeSwapChainImageWSI &&')
        header_txt.append('        objType != VkObjectTypeSwapChainMemoryWSI) {')
        header_txt.append('        // If we do not find it print an error')
        header_txt.append('        char str[1024];')
        header_txt.append('        sprintf(str, "Unable to obtain status for non-existent object 0x%" PRId64 " of %s type", vkObj, string_VK_OBJECT_TYPE(objType));')
        header_txt.append('        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, vkObj, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK", str);')
        header_txt.append('    }')
        header_txt.append('    return VK_FALSE;')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('static void validate_draw_state_flags(VkObject vkObj) {')
        header_txt.append('    validate_status(vkObj, VkObjectTypeCmdBuffer, OBJSTATUS_VIEWPORT_BOUND,      OBJSTATUS_VIEWPORT_BOUND,      VK_DBG_MSG_ERROR,    OBJTRACK_VIEWPORT_NOT_BOUND,      "Viewport object not bound to this command buffer");')
        header_txt.append('    validate_status(vkObj, VkObjectTypeCmdBuffer, OBJSTATUS_RASTER_BOUND,        OBJSTATUS_RASTER_BOUND,        VK_DBG_MSG_ERROR,    OBJTRACK_RASTER_NOT_BOUND,        "Raster object not bound to this command buffer");')
        header_txt.append('    validate_status(vkObj, VkObjectTypeCmdBuffer, OBJSTATUS_COLOR_BLEND_BOUND,   OBJSTATUS_COLOR_BLEND_BOUND,   VK_DBG_MSG_UNKNOWN,  OBJTRACK_COLOR_BLEND_NOT_BOUND,   "Color-blend object not bound to this command buffer");')
        header_txt.append('    validate_status(vkObj, VkObjectTypeCmdBuffer, OBJSTATUS_DEPTH_STENCIL_BOUND, OBJSTATUS_DEPTH_STENCIL_BOUND, VK_DBG_MSG_UNKNOWN,  OBJTRACK_DEPTH_STENCIL_NOT_BOUND, "Depth-stencil object not bound to this command buffer");')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('static void setGpuQueueInfoState(void *pData) {')
        header_txt.append('    maxMemReferences = ((VkPhysicalDeviceQueueProperties *)pData)->maxMemReferences;')
        header_txt.append('}')
        return "\n".join(header_txt)

    def generate_intercept(self, proto, qual):
        if proto.name in [ 'DbgRegisterMsgCallback', 'DbgUnregisterMsgCallback', 'GetGlobalExtensionInfo' ]:
            # use default version
            return None
        obj_type_mapping = {base_t : base_t.replace("Vk", "VkObjectType") for base_t in vulkan.object_type_list}
        # For the various "super-types" we have to use function to distinguish sub type
        for obj_type in ["VK_BASE_OBJECT", "VK_OBJECT", "VK_DYNAMIC_STATE_OBJECT", "VkObject"]:
            obj_type_mapping[obj_type] = "ll_get_obj_type(object)"

        decl = proto.c_func(prefix="vk", attr="VKAPI")
        param0_name = proto.params[0].name
        p0_type = proto.params[0].ty.strip('*').replace('const ', '')
        create_line = ''
        destroy_line = ''
        funcs = []
        # Special cases for API funcs that don't use an object as first arg
        if True in [no_use_proto in proto.name for no_use_proto in ['GlobalOption', 'GetPhysicalDeviceInfo', 'CreateInstance', 'QueueSubmit', 'QueueAddMemReferences', 'QueueRemoveMemReferences', 'QueueWaitIdle', 'QueueBindObjectMemory', 'QueueBindObjectMemoryRange', 'QueueBindImageMemoryRange', 'QueuePresentWSI', 'GetGlobalExtensionInfo', 'CreateDevice', 'GetGpuInfo', 'QueueSignalSemaphore', 'QueueWaitSemaphore']]:
            using_line = ''
        else:
            using_line = '    loader_platform_thread_lock_mutex(&objLock);\n'
            using_line += '    ll_increment_use_count(%s, %s);\n' % (param0_name, obj_type_mapping[p0_type])
            using_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
        if 'QueueSubmit' in proto.name:
            using_line += '    set_status(fence, VkObjectTypeFence, OBJSTATUS_FENCE_IS_SUBMITTED);\n'
            using_line += '    // TODO: Fix for updated memory reference mechanism\n'
            using_line += '    // validate_memory_mapping_status(pMemRefs, memRefCount);\n'
            using_line += '    // validate_mem_ref_count(memRefCount);\n'
        elif 'GetFenceStatus' in proto.name:
            using_line += '    // Warn if submitted_flag is not set\n'
            using_line += '    validate_status(fence, VkObjectTypeFence, OBJSTATUS_FENCE_IS_SUBMITTED, OBJSTATUS_FENCE_IS_SUBMITTED, VK_DBG_MSG_ERROR, OBJTRACK_INVALID_FENCE, "Status Requested for Unsubmitted Fence");\n'
        elif 'WaitForFences' in proto.name:
            using_line += '    // Warn if waiting on unsubmitted fence\n'
            using_line += '    for (uint32_t i = 0; i < fenceCount; i++) {\n'
            using_line += '        validate_status(pFences[i], VkObjectTypeFence, OBJSTATUS_FENCE_IS_SUBMITTED, OBJSTATUS_FENCE_IS_SUBMITTED, VK_DBG_MSG_ERROR, OBJTRACK_INVALID_FENCE, "Waiting for Unsubmitted Fence");\n'
            using_line += '    }\n'
        elif 'EndCommandBuffer' in proto.name:
            using_line += '    reset_status(cmdBuffer, VkObjectTypeCmdBuffer, (OBJSTATUS_VIEWPORT_BOUND    |\n'
            using_line += '                                                                OBJSTATUS_RASTER_BOUND      |\n'
            using_line += '                                                                OBJSTATUS_COLOR_BLEND_BOUND |\n'
            using_line += '                                                                OBJSTATUS_DEPTH_STENCIL_BOUND));\n'
        elif 'CmdBindDynamicStateObject' in proto.name:
            using_line += '    track_object_status(cmdBuffer, stateBindPoint);\n'
        elif 'CmdDraw' in proto.name:
            using_line += '    validate_draw_state_flags(cmdBuffer);\n'
        elif 'MapMemory' in proto.name:
            using_line += '    set_status(mem, VkObjectTypeDeviceMemory, OBJSTATUS_GPU_MEM_MAPPED);\n'
        elif 'UnmapMemory' in proto.name:
            using_line += '    reset_status(mem, VkObjectTypeDeviceMemory, OBJSTATUS_GPU_MEM_MAPPED);\n'
        if 'AllocDescriptor' in proto.name: # Allocates array of DSs
            create_line =  '    for (uint32_t i = 0; i < *pCount; i++) {\n'
            create_line += '        loader_platform_thread_lock_mutex(&objLock);\n'
            create_line += '        ll_insert_obj(pDescriptorSets[i], VkObjectTypeDescriptorSet);\n'
            create_line += '        loader_platform_thread_unlock_mutex(&objLock);\n'
            create_line += '    }\n'
        elif 'Create' in proto.name or 'Alloc' in proto.name:
            create_line = '    loader_platform_thread_lock_mutex(&objLock);\n'
            create_line += '    ll_insert_obj(*%s, %s);\n' % (proto.params[-1].name, obj_type_mapping[proto.params[-1].ty.strip('*').replace('const ', '')])
            create_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
        if 'DestroyObject' in proto.name:
            destroy_line = '    loader_platform_thread_lock_mutex(&objLock);\n'
            destroy_line += '    ll_destroy_obj(%s);\n' % (proto.params[2].name)
            destroy_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
            using_line = ''
        else:
            if 'Destroy' in proto.name:
                destroy_line = '    loader_platform_thread_lock_mutex(&objLock);\n'
                destroy_line += '    ll_destroy_obj(%s);\n' % (param0_name)
                destroy_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
                using_line = ''
            else:
                if 'Free' in proto.name:
                    destroy_line = '    loader_platform_thread_lock_mutex(&objLock);\n'
                    destroy_line += '    ll_destroy_obj(%s);\n' % (proto.params[1].name)
                    destroy_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
                    using_line = ''
            if 'DestroyDevice' in proto.name:
                destroy_line += '    // Report any remaining objects in LL\n    objNode *pTrav = pGlobalHead;\n    while (pTrav) {\n'
                destroy_line += '        if (pTrav->obj.objType == VkObjectTypeSwapChainImageWSI ||\n'
                destroy_line += '            pTrav->obj.objType == VkObjectTypeSwapChainMemoryWSI) {\n'
                destroy_line += '            objNode *pDel = pTrav;\n'
                destroy_line += '            pTrav = pTrav->pNextGlobal;\n'
                destroy_line += '            ll_destroy_obj((pDel->obj.vkObj));\n'
                destroy_line += '        } else if ((pTrav->obj.objType == VkObjectTypePhysicalDevice) || (pTrav->obj.objType == VkObjectTypeQueue)) {\n'
                destroy_line += '            // Cannot destroy physical device so ignore\n'
                destroy_line += '            pTrav = pTrav->pNextGlobal;\n'
                destroy_line += '        } else {\n'
                destroy_line += '            char str[1024];\n'
                destroy_line += '            sprintf(str, "OBJ ERROR : %s object 0x%" PRId64 " has not been destroyed (was used %lu times).", string_VK_OBJECT_TYPE(pTrav->obj.objType), pTrav->obj.vkObj, pTrav->obj.numUses);\n'
                destroy_line += '            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, device, 0, OBJTRACK_OBJECT_LEAK, "OBJTRACK", str);\n'
                destroy_line += '            pTrav = pTrav->pNextGlobal;\n'
                destroy_line += '        }\n'
                destroy_line += '    }\n'
                destroy_line += '    // Clean up Queue\'s MemRef Linked Lists\n'
                destroy_line += '    destroyQueueMemRefLists();\n'
            if 'GetDeviceQueue' in proto.name:
                destroy_line = '    addQueueInfo(*pQueue);\n'
        ret_val = ''
        stmt = ''
        if proto.ret != "void":
            ret_val = "VkResult result = "
            stmt = "    return result;\n"
        if proto.name == "EnumerateLayers":
            funcs.append('%s%s\n'
                     '{\n'
                     '    if (gpu != VK_NULL_HANDLE) {\n'
                     '    %s'
                     '        pCurObj = (VkBaseLayerObject *) gpu;\n'
                     '        loader_platform_thread_once(&tabOnce, init%s);\n'
                     '        %snextTable.%s;\n'
                     '    %s%s'
                     '    %s'
                     '    } else {\n'
                     '        if (pLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL)\n'
                     '            return VK_ERROR_INVALID_POINTER;\n'
                     '        // This layer compatible with all GPUs\n'
                     '        *pLayerCount = 1;\n'
                     '        strncpy((char *) pOutLayers[0], "%s", maxStringSize);\n'
                     '        return VK_SUCCESS;\n'
                     '    }\n'
                         '}' % (qual, decl, using_line, self.layer_name, ret_val, proto.c_call(), create_line, destroy_line, stmt, self.layer_name))
        elif 'GetExtensionSupport' == proto.name:
            funcs.append('%s%s\n'
                     '{\n'
                     '    VkResult result;\n'
                     '    /* This entrypoint is NOT going to init its own dispatch table since loader calls this early */\n'
                     '    if (!strncmp(pExtName, "%s", strlen("%s")) ||\n'
                     '        !strncmp(pExtName, "objTrackGetObjectCount", strlen("objTrackGetObjectCount")) ||\n'
                     '        !strncmp(pExtName, "objTrackGetObjects", strlen("objTrackGetObjects")))\n'
                     '    {\n'
                     '        result = VK_SUCCESS;\n'
                     '    } else if (nextTable.GetExtensionSupport != NULL)\n'
                     '    {\n'
                     '    %s'
                     '        result = nextTable.%s;\n'
                     '    } else\n'
                     '    {\n'
                     '        result = VK_ERROR_INVALID_EXTENSION;\n'
                     '    }\n'
                     '%s'
                     '}' % (qual, decl, self.layer_name, self.layer_name, using_line, proto.c_call(),  stmt))
        elif 'GetPhysicalDeviceInfo' in proto.name:
            gpu_state =  '    if (infoType == VK_PHYSICAL_DEVICE_INFO_TYPE_QUEUE_PROPERTIES) {\n'
            gpu_state += '        if (pData != NULL) {\n'
            gpu_state += '            setGpuQueueInfoState(pData);\n'
            gpu_state += '        }\n'
            gpu_state += '    }\n'
            funcs.append('%s%s\n'
                     '{\n'
                     '%s'
                     '    pCurObj = (VkBaseLayerObject *) gpu;\n'
                     '    loader_platform_thread_once(&tabOnce, init%s);\n'
                     '    %snextTable.%s;\n'
                     '%s%s'
                     '%s'
                     '%s'
                     '}' % (qual, decl, using_line, self.layer_name, ret_val, proto.c_call(), create_line, destroy_line, gpu_state, stmt))
        else:
            funcs.append('%s%s\n'
                     '{\n'
                     '%s'
                     '    %snextTable.%s;\n'
                     '%s%s'
                     '%s'
                     '}' % (qual, decl, using_line, ret_val, proto.c_call(), create_line, destroy_line, stmt))
        return "\n\n".join(funcs)

    def generate_body(self):
        self.layer_name = "ObjectTracker"
        body = [self._generate_layer_initialization(True, lockname='obj'),
                self._generate_dispatch_entrypoints("VK_LAYER_EXPORT"),
                self._generate_extensions(),
                self._generate_layer_gpa_function(extensions=['objTrackGetObjectCount', 'objTrackGetObjects'])]

        return "\n\n".join(body)

class ThreadingSubcommand(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('#include <stdio.h>')
        header_txt.append('#include <stdlib.h>')
        header_txt.append('#include <string.h>')
        header_txt.append('#include <unordered_map>')
        header_txt.append('#include "loader_platform.h"')
        header_txt.append('#include "vkLayer.h"')
        header_txt.append('#include "threading.h"')
        header_txt.append('#include "layers_config.h"')
        header_txt.append('#include "vk_enum_validate_helper.h"')
        header_txt.append('#include "vk_struct_validate_helper.h"')
        header_txt.append('//The following is #included again to catch certain OS-specific functions being used:')
        header_txt.append('#include "loader_platform.h"\n')
        header_txt.append('#include "layers_msg.h"\n')
        header_txt.append('static VkLayerDispatchTable nextTable;')
        header_txt.append('static VkBaseLayerObject *pCurObj;')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabOnce);\n')
        header_txt.append('using namespace std;')
        header_txt.append('static unordered_map<int, void*> proxy_objectsInUse;\n')
        header_txt.append('static unordered_map<VkObject, loader_platform_thread_id> objectsInUse;\n')
        header_txt.append('static int threadingLockInitialized = 0;')
        header_txt.append('static loader_platform_thread_mutex threadingLock;')
        header_txt.append('static int printLockInitialized = 0;')
        header_txt.append('static loader_platform_thread_mutex printLock;\n')
        header_txt.append('')
        header_txt.append('static void useObject(VkObject object, const char* type)')
        header_txt.append('{')
        header_txt.append('    loader_platform_thread_id tid = loader_platform_get_thread_id();')
        header_txt.append('    loader_platform_thread_lock_mutex(&threadingLock);')
        header_txt.append('    if (objectsInUse.find(object) == objectsInUse.end()) {')
        header_txt.append('        objectsInUse[object] = tid;')
        header_txt.append('    } else {')
        header_txt.append('        if (objectsInUse[object] == tid) {')
        header_txt.append('            char str[1024];')
        header_txt.append('            sprintf(str, "THREADING ERROR : object of type %s is simultaneously used in thread %ld and thread %ld", type, objectsInUse[object], tid);')
        header_txt.append('            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, 0, 0, THREADING_CHECKER_MULTIPLE_THREADS, "THREADING", str);')
        header_txt.append('        } else {')
        header_txt.append('            char str[1024];')
        header_txt.append('            sprintf(str, "THREADING ERROR : object of type %s is recursively used in thread %ld", type, tid);')
        header_txt.append('            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, 0, 0, THREADING_CHECKER_SINGLE_THREAD_REUSE, "THREADING", str);')
        header_txt.append('        }')
        header_txt.append('    }')
        header_txt.append('    loader_platform_thread_unlock_mutex(&threadingLock);')
        header_txt.append('}')
        header_txt.append('static void finishUsingObject(VkObject object)')
        header_txt.append('{')
        header_txt.append('    // Object is no longer in use')
        header_txt.append('    loader_platform_thread_lock_mutex(&threadingLock);')
        header_txt.append('    objectsInUse.erase(object);')
        header_txt.append('    loader_platform_thread_unlock_mutex(&threadingLock);')
        header_txt.append('}')
        return "\n".join(header_txt)

    def generate_intercept(self, proto, qual):
        if proto.name in [ 'DbgRegisterMsgCallback', 'DbgUnregisterMsgCallback' ]:
            # use default version
            return None
        decl = proto.c_func(prefix="vk", attr="VKAPI")
        ret_val = ''
        stmt = ''
        funcs = []
        if proto.ret != "void":
            ret_val = "VkResult result = "
            stmt = "    return result;\n"
        if proto.name == "EnumerateLayers":
            funcs.append('%s%s\n'
                     '{\n'
                     '    if (gpu != NULL) {\n'
                     '        pCurObj = (VkBaseLayerObject *) %s;\n'
                     '        loader_platform_thread_once(&tabOnce, init%s);\n'
                     '        %snextTable.%s;\n'
                     '        fflush(stdout);\n'
                     '    %s'
                     '    } else {\n'
                     '        if (pLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL)\n'
                     '            return VK_ERROR_INVALID_POINTER;\n'
                     '        // This layer compatible with all GPUs\n'
                     '        *pLayerCount = 1;\n'
                     '        strncpy((char *) pOutLayers[0], "%s", maxStringSize);\n'
                     '        return VK_SUCCESS;\n'
                     '    }\n'
                     '}' % (qual, decl, proto.params[0].name, self.layer_name, ret_val, proto.c_call(), stmt, self.layer_name))
        # All functions that do a Get are thread safe
        elif 'Get' in proto.name:
            return None
        # All Wsi functions are thread safe
        elif 'WsiX11' in proto.name:
            return None
        # All functions that start with a device parameter are thread safe
        elif proto.params[0].ty in { "VkDevice" }:
            return None
        # Only watch core objects passed as first parameter
        elif proto.params[0].ty not in vulkan.core.objects:
            return None
        elif proto.params[0].ty != "VkPhysicalDevice":
            funcs.append('%s%s\n'
                     '{\n'
                     '    useObject((VkObject) %s, "%s");\n'
                     '    %snextTable.%s;\n'
                     '    finishUsingObject((VkObject) %s);\n'
                     '%s'
                     '}' % (qual, decl, proto.params[0].name, proto.params[0].ty, ret_val, proto.c_call(), proto.params[0].name, stmt))
        else:
            funcs.append('%s%s\n'
                     '{\n'
                     '    pCurObj =  (VkBaseLayerObject *) %s;\n'
                     '    loader_platform_thread_once(&tabOnce, init%s);\n'
                     '    %snextTable.%s;\n'
                     '%s'
                     '}' % (qual, decl, proto.params[0].name, self.layer_name, ret_val, proto.c_call(), stmt))
        return "\n\n".join(funcs)

    def generate_body(self):
        self.layer_name = "Threading"
        body = [self._generate_layer_initialization(True, lockname='threading'),
                self._generate_dispatch_entrypoints("VK_LAYER_EXPORT"),
                self._generate_layer_gpa_function()]
        return "\n\n".join(body)

def main():
    subcommands = {
            "layer-funcs" : LayerFuncsSubcommand,
            "layer-dispatch" : LayerDispatchSubcommand,
            "Generic" : GenericLayerSubcommand,
            "APIDump" : APIDumpSubcommand,
            "ObjectTracker" : ObjectTrackerSubcommand,
            "Threading" : ThreadingSubcommand,
    }

    if len(sys.argv) < 3 or sys.argv[1] not in subcommands or not os.path.exists(sys.argv[2]):
        print("Usage: %s <subcommand> <input_header> [options]" % sys.argv[0])
        print
        print("Available subcommands are: %s" % " ".join(subcommands))
        exit(1)

    hfp = vk_helper.HeaderFileParser(sys.argv[2])
    hfp.parse()
    vk_helper.enum_val_dict = hfp.get_enum_val_dict()
    vk_helper.enum_type_dict = hfp.get_enum_type_dict()
    vk_helper.struct_dict = hfp.get_struct_dict()
    vk_helper.typedef_fwd_dict = hfp.get_typedef_fwd_dict()
    vk_helper.typedef_rev_dict = hfp.get_typedef_rev_dict()
    vk_helper.types_dict = hfp.get_types_dict()

    subcmd = subcommands[sys.argv[1]](sys.argv[2:])
    subcmd.run()

if __name__ == "__main__":
    main()
