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
import re

import vulkan
import vk_helper

def proto_is_global(proto):
    if proto.params[0].ty == "VkInstance" or proto.params[0].ty == "VkPhysicalDevice" or proto.name == "CreateInstance" or proto.name == "GetGlobalExtensionInfo" or proto.name == "GetDisplayInfoWSI":
       return True
    else:
       return False

def generate_get_proc_addr_check(name):
    return "    if (!%s || %s[0] != 'v' || %s[1] != 'k')\n" \
           "        return NULL;" % ((name,) * 3)

def ucc_to_U_C_C(CamelCase):
    temp = re.sub('(.)([A-Z][a-z]+)',  r'\1_\2', CamelCase)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', temp).upper()

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
        if self.layer_name == "ObjectTracker" or self.layer_name == "Threading":
            r_body.append('    VK_LAYER_DBG_FUNCTION_NODE *pNewDbgFuncNode = new VK_LAYER_DBG_FUNCTION_NODE;')
        else:
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
        r_body.append('    VkResult result = nextInstanceTable.DbgRegisterMsgCallback(instance, pfnMsgCallback, pUserData);')
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
        if self.layer_name == "ObjectTracker" or self.layer_name == "Threading":
            ur_body.append('            delete pTrav;')
        else:
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
        ur_body.append('    VkResult result = nextInstanceTable.DbgUnregisterMsgCallback(instance, pfnMsgCallback);')
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
            ggei_body.append('#define LAYER_EXT_ARRAY_SIZE 6')
            ggei_body.append('static const struct extProps layerExts[LAYER_EXT_ARRAY_SIZE] = {')
            ggei_body.append('    // TODO what is the version?')
            ggei_body.append('    {0x10, "%s"},' % layer)
            ggei_body.append('    {0x10, "Validation"},')
            ggei_body.append('    {0x10, "objTrackGetObjectsCount"},')
            ggei_body.append('    {0x10, "objTrackGetObjects"},')
            ggei_body.append('    {0x10, "objTrackGetObjectsOfTypeCount"},')
            ggei_body.append('    {0x10, "objTrackGetObjectsOfType"}')
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

    def _generate_dispatch_entrypoints(self, qual=""):
        if qual:
            qual += " "

        funcs = []
        intercepted = []
        for proto in self.protos:
            if proto.name == "GetDeviceProcAddr" or proto.name == "GetInstanceProcAddr":
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
                        intercept = self._gen_layer_get_global_extension_info(self.layer_name)
                if intercept is not None:
                    funcs.append(intercept)
                    intercepted.append(proto)

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
        # add layer_intercept_instance_proc
        lookups = []
        for proto in self.protos:
            if not proto_is_global(proto):
                continue

            if not proto in intercepted:
                continue
            lookups.append("if (!strcmp(name, \"%s\"))" % proto.name)
            lookups.append("    return (void*) %s%s;" % (prefix, proto.name))

        body.append("static inline void* layer_intercept_instance_proc(const char *name)")
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
        exts.append('uint64_t objTrackGetObjectsCount(void)')
        exts.append('{')
        exts.append('    return numTotalObjs;')
        exts.append('}')
        exts.append('')
        exts.append('VkResult objTrackGetObjects(uint64_t objCount, OBJTRACK_NODE* pObjNodeArray)')
        exts.append('{')
        exts.append("    // This bool flags if we're pulling all objs or just a single class of objs")
        exts.append('    // Check the count first thing')
        exts.append('    uint64_t maxObjCount = numTotalObjs;')
        exts.append('    if (objCount > maxObjCount) {')
        exts.append('        char str[1024];')
        exts.append('        sprintf(str, "OBJ ERROR : Received objTrackGetObjects() request for %lu objs, but there are only %lu total objs", objCount, maxObjCount);')
        exts.append('        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, 0, 0, OBJTRACK_OBJCOUNT_MAX_EXCEEDED, "OBJTRACK", str);')
        exts.append('        return VK_ERROR_INVALID_VALUE;')
        exts.append('    }')
        exts.append('    auto it = objMap.begin();')
        exts.append('    for (uint64_t i = 0; i < objCount; i++) {')
        exts.append('        if (objMap.end() == it) {')
        exts.append('            char str[1024];')
        exts.append('            sprintf(str, "OBJ INTERNAL ERROR : Ran out of objs! Should have %lu, but only copied %lu and not the requested %lu.", maxObjCount, i, objCount);')
        exts.append('            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, 0, 0, OBJTRACK_INTERNAL_ERROR, "OBJTRACK", str);')
        exts.append('            return VK_ERROR_UNKNOWN;')
        exts.append('        }')
        exts.append('        memcpy(&pObjNodeArray[i], it->second, sizeof(OBJTRACK_NODE));')
        exts.append('        ++it;')
        exts.append('    }')
        exts.append('    return VK_SUCCESS;')
        exts.append('}')
        exts.append('uint64_t objTrackGetObjectsOfTypeCount(VkObjectType type)')
        exts.append('{')
        exts.append('    return numObjs[type];')
        exts.append('}')
        exts.append('')
        exts.append('VkResult objTrackGetObjectsOfType(VkObjectType type, uint64_t objCount, OBJTRACK_NODE* pObjNodeArray)')
        exts.append('{')
        exts.append('    // Check the count first thing')
        exts.append('    uint64_t maxObjCount = numObjs[type];')
        exts.append('    if (objCount > maxObjCount) {')
        exts.append('        char str[1024];')
        exts.append('        sprintf(str, "OBJ ERROR : Received objTrackGetObjects() request for %lu objs, but there are only %lu objs of type %s", objCount, maxObjCount, string_from_vulkan_object_type(type));')
        exts.append('        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, 0, 0, OBJTRACK_OBJCOUNT_MAX_EXCEEDED, "OBJTRACK", str);')
        exts.append('        return VK_ERROR_INVALID_VALUE;')
        exts.append('    }')
        exts.append('    auto it = objMap.begin();')
        exts.append('    for (uint64_t i = 0; i < objCount; i++) {')
        exts.append('        // Get next object of correct type')
        exts.append('        while ((objMap.end() != it) && (it->second->objType != type))')
        exts.append('            ++it;')
        exts.append('        if (objMap.end() == it) {')
        exts.append('            char str[1024];')
        exts.append('            sprintf(str, "OBJ INTERNAL ERROR : Ran out of %s objs! Should have %lu, but only copied %lu and not the requested %lu.", string_from_vulkan_object_type(type), maxObjCount, i, objCount);')
        exts.append('            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, 0, 0, OBJTRACK_INTERNAL_ERROR, "OBJTRACK", str);')
        exts.append('            return VK_ERROR_UNKNOWN;')
        exts.append('        }')
        exts.append('        memcpy(&pObjNodeArray[i], it->second, sizeof(OBJTRACK_NODE));')
        exts.append('    }')
        exts.append('    return VK_SUCCESS;')
        exts.append('}')
        return "\n".join(exts)

    def _generate_layer_gpa_function(self, extensions=[], instance_extensions=[]):
        func_body = []
        func_body.append("VK_LAYER_EXPORT void* VKAPI vkGetDeviceProcAddr(VkDevice device, const char* funcName)\n"
                         "{\n"
                         "    VkBaseLayerObject* devw = (VkBaseLayerObject *) device;\n"
                         "    void* addr;\n"
                         "    if (device == VK_NULL_HANDLE)\n"
                         "        return NULL;\n"
                         "    pCurObj = devw;\n"
                         "    loader_platform_thread_once(&initOnce, init%s);\n\n"
                         "    loader_platform_thread_once(&tabDeviceOnce, initDeviceTable);\n\n"
                         "    addr = layer_intercept_proc(funcName);\n"
                         "    if (addr)\n"
                         "        return addr;" % self.layer_name)

        if 0 != len(extensions):
            cpp_prefix = ''
            cpp_postfix = ''
            if self.layer_name == 'ObjectTracker':
                cpp_prefix = "reinterpret_cast<void*>("
                cpp_postfix = ")"
            for ext_name in extensions:
                func_body.append('    else if (!strncmp("%s", funcName, sizeof("%s")))\n'
                                 '        return %s%s%s;' % (ext_name, ext_name, cpp_prefix, ext_name, cpp_postfix))
        func_body.append("    else {\n"
                         "        if (devw->pGPA == NULL)\n"
                         "            return NULL;\n"
                         "        return devw->pGPA((VkObject)devw->nextObject, funcName);\n"
                         "    }\n"
                         "}\n")
        func_body.append("VK_LAYER_EXPORT void* VKAPI vkGetInstanceProcAddr(VkInstance inst, const char* funcName)\n"
                         "{\n"
                         "    VkBaseLayerObject* instw = (VkBaseLayerObject *) inst;\n"
                         "    void* addr;\n"
                         "    if (inst == VK_NULL_HANDLE)\n"
                         "        return NULL;\n"
                         "    pCurObj = instw;\n"
                         "    loader_platform_thread_once(&initOnce, init%s);\n\n"
                         "    loader_platform_thread_once(&tabInstanceOnce, initInstanceTable);\n\n"
                         "    addr = layer_intercept_instance_proc(funcName);\n"
                         "    if (addr)\n"
                         "        return addr;" % self.layer_name)

        if 0 != len(instance_extensions):
            for ext_name in instance_extensions:
                func_body.append('    else if (!strcmp("%s", funcName))\n'
                                 '        return %s;' % (ext_name, ext_name))
        func_body.append("    else {\n"
                         "        if (instw->pGPA == NULL)\n"
                         "            return NULL;\n"
                         "        return instw->pGPA((VkObject)instw->nextObject, funcName);\n"
                         "    }\n"
                         "}\n")
        return "\n".join(func_body)

    def _generate_layer_initialization(self, init_opts=False, prefix='vk', lockname=None, condname=None):
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

        if lockname is not None:
            func_body.append("    if (!%sLockInitialized)" % lockname)
            func_body.append("    {")
            func_body.append("        // TODO/TBD: Need to delete this mutex sometime.  How???")
            func_body.append("        loader_platform_thread_create_mutex(&%sLock);" % lockname)
            if condname is not None:
                func_body.append("        loader_platform_thread_init_cond(&%sCond);" % condname)
            func_body.append("        %sLockInitialized = 1;" % lockname)
            func_body.append("    }")
        func_body.append("}\n")
        func_body.append('')
        func_body.append('static void initDeviceTable(void)')
        func_body.append('{')
        func_body.append('    PFN_vkGetDeviceProcAddr fpNextGPA;')
        func_body.append('    fpNextGPA = (PFN_vkGetDeviceProcAddr) pCurObj->pGPA;')
        func_body.append('    assert(fpNextGPA);')
        func_body.append('    layer_initialize_dispatch_table(&nextTable, fpNextGPA, (VkDevice) pCurObj->nextObject);')
        func_body.append('}')
        func_body.append('')
        func_body.append('static void initInstanceTable(void)')
        func_body.append('{')
        func_body.append('    PFN_vkGetInstanceProcAddr fpNextGPA;')
        func_body.append('    fpNextGPA = (PFN_vkGetInstanceProcAddr) pCurObj->pGPA;')
        func_body.append('    assert(fpNextGPA);')
        func_body.append('    layer_init_instance_dispatch_table(&nextInstanceTable, fpNextGPA, (VkInstance) pCurObj->nextObject);')
        func_body.append('}')
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
        return '#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include "loader_platform.h"\n#include "vkLayer.h"\n//The following is #included again to catch certain OS-specific functions being used:\n#include "loader_platform.h"\n\n#include "layers_config.h"\n#include "layers_msg.h"\n\nstatic VkLayerDispatchTable nextTable;\nstatic VkLayerInstanceDispatchTable nextInstanceTable;\nstatic VkBaseLayerObject *pCurObj;\n\nstatic LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabDeviceOnce);\nstatic LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabInstanceOnce);\nstatic LOADER_PLATFORM_THREAD_ONCE_DECLARATION(initOnce);'

    def generate_intercept(self, proto, qual):
        if proto.name in [ 'DbgRegisterMsgCallback', 'DbgUnregisterMsgCallback' , 'GetGlobalExtensionInfo']:
            # use default version
            return None
        decl = proto.c_func(prefix="vk", attr="VKAPI")
        ret_val = ''
        stmt = ''
        funcs = []
        table = ''
        if proto_is_global(proto):
           table = 'Instance'

        if proto.ret != "void":
            ret_val = "%s result = " % proto.ret
            stmt = "    return result;\n"
        if proto.name == "EnumerateLayers":
            funcs.append('%s%s\n'
                     '{\n'
                     '    char str[1024];\n'
                     '    if (gpu != VK_NULL_HANDLE) {\n'
                     '        sprintf(str, "At start of layered %s\\n");\n'
                     '        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, gpu, 0, 0, (char *) "GENERIC", (char *) str);\n'
                     '        pCurObj = (VkBaseLayerObject *) gpu;\n'
                     '        loader_platform_thread_once(&initOnce, init%s);\n'
                     '        loader_platform_thread_once(&tabDeviceOnce, initDeviceTable);\n'
                     '        %snext%sTable.%s;\n'
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
                     '}' % (qual, decl, proto.name, self.layer_name, ret_val, table, proto.c_call(), proto.name, stmt, self.layer_name))
        else:
            funcs.append('%s%s\n'
                     '{\n'
                     '    %snext%sTable.%s;\n'
                     '%s'
                     '}' % (qual, decl, ret_val, table, proto.c_call(), stmt))
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
        header_txt.append('static VkLayerInstanceDispatchTable nextInstanceTable;')
        header_txt.append('static VkBaseLayerObject *pCurObj;')
        header_txt.append('static bool g_APIDumpDetailed = true;')
        header_txt.append('')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabInstanceOnce);')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabDeviceOnce);')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(initOnce);')
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
        func_body.append('    if (!printLockInitialized)')
        func_body.append('    {')
        func_body.append('        // TODO/TBD: Need to delete this mutex sometime.  How???')
        func_body.append('        loader_platform_thread_create_mutex(&printLock);')
        func_body.append('        printLockInitialized = 1;')
        func_body.append('    }')
        func_body.append('}')
        func_body.append('')
        func_body.append('static void initDeviceTable(void)')
        func_body.append('{')
        func_body.append('    PFN_vkGetDeviceProcAddr fpNextGPA;')
        func_body.append('    fpNextGPA = (PFN_vkGetDeviceProcAddr) pCurObj->pGPA;')
        func_body.append('    assert(fpNextGPA);')
        func_body.append('    layer_initialize_dispatch_table(&nextTable, fpNextGPA, (VkDevice) pCurObj->nextObject);')
        func_body.append('}')
        func_body.append('')
        func_body.append('static void initInstanceTable(void)')
        func_body.append('{')
        func_body.append('    PFN_vkGetInstanceProcAddr fpNextGPA;')
        func_body.append('    fpNextGPA = (PFN_vkGetInstanceProcAddr) pCurObj->pGPA;')
        func_body.append('    assert(fpNextGPA);')
        func_body.append('    layer_init_instance_dispatch_table(&nextInstanceTable, fpNextGPA, (VkInstance) pCurObj->nextObject);')
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
            ret_val = "%s result = " % proto.ret
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
        if proto.ret == "VkResult":
            log_func += ') = " << string_VkResult((VkResult)result) << endl'
            log_func_no_addr += ') = " << string_VkResult((VkResult)result) << endl'
        elif proto.ret == "void*":
            log_func += ') = " << result << endl'
            log_func_no_addr += ') = " << result << endl'
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
        table = ''
        if proto_is_global(proto):
           table = 'Instance'

        if proto.name == "EnumerateLayers":
            funcs.append('%s%s\n'
                     '{\n'
                     '    using namespace StreamControl;\n'
                     '    if (gpu != NULL) {\n'
                     '        pCurObj = (VkBaseLayerObject *) gpu;\n'
                     '        loader_platform_thread_once(&initOnce, init%s);\n'
                     '        loader_platform_thread_once(&tabDeviceOnce, initDeviceTable);\n'
                     '        %snext%sTable.%s;\n'
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
                         '}' % (qual, decl, self.layer_name, ret_val, table, proto.c_call(),f_open, log_func, f_close, stmt, self.layer_name))
        else:
            funcs.append('%s%s\n'
                     '{\n'
                     '    using namespace StreamControl;\n'
                     '    %snext%sTable.%s;\n'
                     '    %s%s%s\n'
                     '%s'
                     '}' % (qual, decl, ret_val, table, proto.c_call(), f_open, log_func, f_close, stmt))
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
        header_txt.append('#include "object_track.h"\n\n')
        header_txt.append('#include <unordered_map>')
        header_txt.append('using namespace std;')
        header_txt.append('// The following is #included again to catch certain OS-specific functions being used:')
        header_txt.append('#include "loader_platform.h"')
        header_txt.append('#include "layers_config.h"')
        header_txt.append('#include "layers_msg.h"')
        header_txt.append('static VkLayerDispatchTable nextTable;\nstatic VkLayerInstanceDispatchTable nextInstanceTable;\n')
        header_txt.append('static VkBaseLayerObject *pCurObj;')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(initOnce);')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabDeviceOnce);')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabInstanceOnce);')
        header_txt.append('static long long unsigned int object_track_index = 0;')
        header_txt.append('static int objLockInitialized = 0;')
        header_txt.append('static loader_platform_thread_mutex objLock;')
        header_txt.append('// Objects stored in a global map w/ struct containing basic info')
        header_txt.append('unordered_map<VkObject, OBJTRACK_NODE*> objMap;')
        header_txt.append('')
        header_txt.append('#define NUM_OBJECT_TYPES (VK_NUM_OBJECT_TYPE + (VK_OBJECT_TYPE_SWAP_CHAIN_WSI - VK_OBJECT_TYPE_DISPLAY_WSI))')
        header_txt.append('')
        header_txt.append('static uint64_t                         numObjs[NUM_OBJECT_TYPES]     = {0};')
        header_txt.append('static uint64_t                         numTotalObjs                  = 0;')
        header_txt.append('static VkPhysicalDeviceQueueProperties *queueInfo                     = NULL;')
        header_txt.append('static uint32_t                         queueCount                    = 0;')
        header_txt.append('')
        header_txt.append('// For each Queue\'s doubly linked-list of mem refs')
        header_txt.append('typedef struct _OT_MEM_INFO {')
        header_txt.append('    VkDeviceMemory       mem;')
        header_txt.append('    struct _OT_MEM_INFO *pNextMI;')
        header_txt.append('    struct _OT_MEM_INFO *pPrevMI;')
        header_txt.append('')
        header_txt.append('} OT_MEM_INFO;')
        header_txt.append('')
        header_txt.append('// Track Queue information')
        header_txt.append('typedef struct _OT_QUEUE_INFO {')
        header_txt.append('    OT_MEM_INFO                     *pMemRefList;')
        header_txt.append('    struct _OT_QUEUE_INFO           *pNextQI;')
        header_txt.append('    uint32_t                         queueNodeIndex;')
        header_txt.append('    VkQueue                          queue;')
        header_txt.append('    uint32_t                         refCount;')
        header_txt.append('} OT_QUEUE_INFO;')
        header_txt.append('')
        header_txt.append('// Global list of QueueInfo structures, one per queue')
        header_txt.append('static OT_QUEUE_INFO *g_pQueueInfo = NULL;')
        header_txt.append('')
        header_txt.append('// Convert an object type enum to an object type array index')
        header_txt.append('static uint32_t objTypeToIndex(uint32_t objType)')
        header_txt.append('{')
        header_txt.append('    uint32_t index = objType;')
        header_txt.append('    if (objType > VK_OBJECT_TYPE_END_RANGE) {')
        header_txt.append('        // These come from vk_wsi_lunarg.h, rebase')
        header_txt.append('        index = (index -(VK_WSI_LUNARG_EXTENSION_NUMBER * -1000)) + VK_OBJECT_TYPE_END_RANGE;')
        header_txt.append('    }')
        header_txt.append('    return index;')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('// Validate that object is in the object map')
        header_txt.append('static void validate_object(const VkObject object)')
        header_txt.append('{')
        header_txt.append('    if (objMap.find(object) == objMap.end()) {')
        header_txt.append('        char str[1024];')
        header_txt.append('        sprintf(str, "Invalid Object %p", (void*)object);')
        header_txt.append('        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, object, 0, OBJTRACK_INVALID_OBJECT, "OBJTRACK", str);')
        header_txt.append('    }')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('// Validate that object parameter matches designated object type')
        header_txt.append('static void validateObjectType(')
        header_txt.append('    const char  *apiName,')
        header_txt.append('    VkObjectType objType,')
        header_txt.append('    VkObject     object)')
        header_txt.append('{')
        header_txt.append('    if (objMap.find(object) != objMap.end()) {')
        header_txt.append('        OBJTRACK_NODE* pNode = objMap[object];')
        header_txt.append('        // Found our object, check type')
        header_txt.append('        if (strcmp(string_VkObjectType(pNode->objType), string_VkObjectType(objType)) != 0) {')
        header_txt.append('            char str[1024];')
        header_txt.append('            sprintf(str, "ERROR: Object Parameter Type %s does not match designated type %s",')
        header_txt.append('                string_VkObjectType(pNode->objType), string_VkObjectType(objType));')
        header_txt.append('            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, object, 0, OBJTRACK_OBJECT_TYPE_MISMATCH, "OBJTRACK", str);')
        header_txt.append('        }')
        header_txt.append('    }')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('// Add new queue to head of global queue list')
        header_txt.append('static void addQueueInfo(uint32_t queueNodeIndex, VkQueue queue)')
        header_txt.append('{')
        header_txt.append('    OT_QUEUE_INFO *pQueueInfo = new OT_QUEUE_INFO;')
        header_txt.append('')
        header_txt.append('    if (pQueueInfo != NULL) {')
        header_txt.append('        memset(pQueueInfo, 0, sizeof(OT_QUEUE_INFO));')
        header_txt.append('        pQueueInfo->queue       = queue;')
        header_txt.append('        pQueueInfo->queueNodeIndex = queueNodeIndex;')
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
        header_txt.append('            delete pDelMemInfo;')
        header_txt.append('        }')
        header_txt.append('        pDelQueueInfo = pQueueInfo;')
        header_txt.append('        pQueueInfo    = pQueueInfo->pNextQI;')
        header_txt.append('        delete pDelQueueInfo;')
        header_txt.append('    }')
        header_txt.append('    g_pQueueInfo = pQueueInfo;')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('static void create_obj(VkObject vkObj, VkObjectType objType) {')
        header_txt.append('    char str[1024];')
        header_txt.append('    sprintf(str, "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_from_vulkan_object_type(objType), reinterpret_cast<VkUintPtrLeast64>(vkObj));')
        header_txt.append('    layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, vkObj, 0, OBJTRACK_NONE, "OBJTRACK", str);')
        header_txt.append('    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;')
        header_txt.append('    pNewObjNode->vkObj = vkObj;')
        header_txt.append('    pNewObjNode->objType = objType;')
        header_txt.append('    pNewObjNode->status  = OBJSTATUS_NONE;')
        header_txt.append('    objMap[vkObj] = pNewObjNode;')
        header_txt.append('    uint32_t objIndex = objTypeToIndex(objType);')
        header_txt.append('    numObjs[objIndex]++;')
        header_txt.append('    numTotalObjs++;')
        header_txt.append('}')
        header_txt.append('// Parse global list to find obj type, then remove obj from obj type list, finally')
        header_txt.append('//   remove obj from global list')
        header_txt.append('static void destroy_obj(VkObject vkObj) {')
        header_txt.append('    if (objMap.find(vkObj) != objMap.end()) {')
        header_txt.append('        OBJTRACK_NODE* pNode = objMap[vkObj];')
        header_txt.append('        uint32_t objIndex = objTypeToIndex(pNode->objType);')
        header_txt.append('        assert(numTotalObjs > 0);')
        header_txt.append('        numTotalObjs--;')
        header_txt.append('        assert(numObjs[objIndex] > 0);')
        header_txt.append('        numObjs[objIndex]--;')
        header_txt.append('        char str[1024];')
        header_txt.append('        sprintf(str, "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%lu total objs remain & %lu %s objs).", string_from_vulkan_object_type(pNode->objType), reinterpret_cast<VkUintPtrLeast64>(pNode->vkObj), numTotalObjs, numObjs[objIndex], string_from_vulkan_object_type(pNode->objType));')
        header_txt.append('        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, vkObj, 0, OBJTRACK_NONE, "OBJTRACK", str);')
        header_txt.append('        delete pNode;')
        header_txt.append('        objMap.erase(vkObj);')
        header_txt.append('    }')
        header_txt.append('    else {')
        header_txt.append('        char str[1024];')
        header_txt.append('        sprintf(str, "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?", reinterpret_cast<VkUintPtrLeast64>(vkObj));')
        header_txt.append('        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, vkObj, 0, OBJTRACK_DESTROY_OBJECT_FAILED, "OBJTRACK", str);')
        header_txt.append('    }')
        header_txt.append('}')
        header_txt.append('// Set selected flag state for an object node')
        header_txt.append('static void set_status(VkObject vkObj, VkObjectType objType, ObjectStatusFlags status_flag) {')
        header_txt.append('    if (vkObj != VK_NULL_HANDLE) {')
        header_txt.append('        if (objMap.find(vkObj) != objMap.end()) {')
        header_txt.append('            OBJTRACK_NODE* pNode = objMap[vkObj];')
        header_txt.append('            pNode->status |= status_flag;')
        header_txt.append('            return;')
        header_txt.append('        }')
        header_txt.append('        else {')
        header_txt.append('            // If we do not find it print an error')
        header_txt.append('            char str[1024];')
        header_txt.append('            sprintf(str, "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type", reinterpret_cast<VkUintPtrLeast64>(vkObj), string_from_vulkan_object_type(objType));')
        header_txt.append('            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, vkObj, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK", str);')
        header_txt.append('        }')
        header_txt.append('    }')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('// Reset selected flag state for an object node')
        header_txt.append('static void reset_status(VkObject vkObj, VkObjectType objType, ObjectStatusFlags status_flag) {')
        header_txt.append('    if (objMap.find(vkObj) != objMap.end()) {')
        header_txt.append('        OBJTRACK_NODE* pNode = objMap[vkObj];')
        header_txt.append('        pNode->status &= ~status_flag;')
        header_txt.append('        return;')
        header_txt.append('    }')
        header_txt.append('    else {')
        header_txt.append('        // If we do not find it print an error')
        header_txt.append('        char str[1024];')
        header_txt.append('        sprintf(str, "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type", reinterpret_cast<VkUintPtrLeast64>(vkObj), string_from_vulkan_object_type(objType));')
        header_txt.append('        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, vkObj, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK", str);')
        header_txt.append('    }')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('static void setGpuQueueInfoState(size_t* pDataSize, void *pData) {')
        header_txt.append('    queueCount = ((uint32_t)*pDataSize / sizeof(VkPhysicalDeviceQueueProperties));')
        header_txt.append('    queueInfo  = (VkPhysicalDeviceQueueProperties*)realloc((void*)queueInfo, *pDataSize);')
        header_txt.append('    if (queueInfo != NULL) {')
        header_txt.append('        memcpy(queueInfo, pData, *pDataSize);')
        header_txt.append('    }')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('// Check Queue type flags for selected queue operations')
        header_txt.append('static void validateQueueFlags(VkQueue queue, const char *function) {')
        header_txt.append('    OT_QUEUE_INFO *pQueueInfo = g_pQueueInfo;')
        header_txt.append('    while ((pQueueInfo != NULL) && (pQueueInfo->queue != queue)) {')
        header_txt.append('        pQueueInfo = pQueueInfo->pNextQI;')
        header_txt.append('    }')
        header_txt.append('    if (pQueueInfo != NULL) {')
        header_txt.append('        char str[1024];\n')
        header_txt.append('        if ((queueInfo != NULL) && (queueInfo[pQueueInfo->queueNodeIndex].queueFlags & VK_QUEUE_SPARSE_MEMMGR_BIT) == 0) {')
        header_txt.append('            sprintf(str, "Attempting %s on a non-memory-management capable queue -- VK_QUEUE_SPARSE_MEMMGR_BIT not set", function);')
        header_txt.append('            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, queue, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK", str);')
        header_txt.append('        } else {')
        header_txt.append('            sprintf(str, "Attempting %s on a possibly non-memory-management capable queue -- VK_QUEUE_SPARSE_MEMMGR_BIT not known", function);')
        header_txt.append('            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, queue, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK", str);')
        header_txt.append('        }')
        header_txt.append('    }')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('// Check object status for selected flag state')
        header_txt.append('static bool32_t validate_status(VkObject vkObj, VkObjectType objType, ObjectStatusFlags status_mask, ObjectStatusFlags status_flag, VK_DBG_MSG_TYPE error_level, OBJECT_TRACK_ERROR error_code, const char* fail_msg) {')
        header_txt.append('    if (objMap.find(vkObj) != objMap.end()) {')
        header_txt.append('        OBJTRACK_NODE* pNode = objMap[vkObj];')
        header_txt.append('        if ((pNode->status & status_mask) != status_flag) {')
        header_txt.append('            char str[1024];')
        header_txt.append('            sprintf(str, "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_from_vulkan_object_type(objType), reinterpret_cast<VkUintPtrLeast64>(vkObj), fail_msg);')
        header_txt.append('            layerCbMsg(error_level, VK_VALIDATION_LEVEL_0, vkObj, 0, error_code, "OBJTRACK", str);')
        header_txt.append('            return VK_FALSE;')
        header_txt.append('        }')
        header_txt.append('        return VK_TRUE;')
        header_txt.append('    }')
        header_txt.append('    else {')
        header_txt.append('        // If we do not find it print an error')
        header_txt.append('        char str[1024];')
        header_txt.append('        sprintf(str, "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type", reinterpret_cast<VkUintPtrLeast64>(vkObj), string_from_vulkan_object_type(objType));')
        header_txt.append('        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, vkObj, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK", str);')
        header_txt.append('        return VK_FALSE;')
        header_txt.append('    }')
        header_txt.append('}')
        header_txt.append('')
        return "\n".join(header_txt)

    def generate_intercept(self, proto, qual):
        if proto.name in [ 'DbgRegisterMsgCallback', 'DbgUnregisterMsgCallback', 'GetGlobalExtensionInfo' ]:
            # use default version
            return None

        # Create map of object names to object type enums of the form VkName : VkObjectTypeName
        obj_type_mapping = {base_t : base_t.replace("Vk", "VkObjectType") for base_t in vulkan.object_type_list}
        # Convert object type enum names from UpperCamelCase to UPPER_CASE_WITH_UNDERSCORES
        for objectName, objectTypeEnum in obj_type_mapping.items():
            obj_type_mapping[objectName] = ucc_to_U_C_C(objectTypeEnum);
        # Command Buffer Object doesn't follow the rule.
        obj_type_mapping['VkCmdBuffer'] = "VK_OBJECT_TYPE_COMMAND_BUFFER"

        decl = proto.c_func(prefix="vk", attr="VKAPI")
        param0_name = proto.params[0].name
        using_line = ''
        create_line = ''
        destroy_line = ''
        object_params = []
        # TODO : Should also check through struct params & add any objects embedded in struct chains
        # TODO : A few of the skipped types are just "hard" cases that need some more work to support
        #   Need to handle NULL fences on queue submit, binding null memory, and WSI Image objects
        for p in proto.params:
            if p.ty in vulkan.core.objects and p.ty not in ['VkPhysicalDevice', 'VkQueue', 'VkFence', 'VkImage', 'VkDeviceMemory']:
                object_params.append(p.name)
        funcs = []
        mutex_unlock = False
        if 'QueueSubmit' in proto.name:
            using_line  = '    loader_platform_thread_lock_mutex(&objLock);\n'
            using_line += '    set_status(fence, VK_OBJECT_TYPE_FENCE, OBJSTATUS_FENCE_IS_SUBMITTED);\n'
            using_line += '    // TODO: Fix for updated memory reference mechanism\n'
            using_line += '    // validate_memory_mapping_status(pMemRefs, memRefCount);\n'
            using_line += '    // validate_mem_ref_count(memRefCount);\n'
            mutex_unlock = True
        elif 'QueueBindSparse' in proto.name:
            using_line  = '    loader_platform_thread_lock_mutex(&objLock);\n'
            using_line += '    validateQueueFlags(queue, "%s");\n' % (proto.name)
            mutex_unlock = True
        elif 'QueueBindObject' in proto.name:
            using_line  = '    loader_platform_thread_lock_mutex(&objLock);\n'
            using_line += '    validateObjectType("vk%s", objType, object);\n' % (proto.name)
            mutex_unlock = True
        elif 'GetObjectInfo' in proto.name:
            using_line  = '    loader_platform_thread_lock_mutex(&objLock);\n'
            using_line += '    validateObjectType("vk%s", objType, object);\n' % (proto.name)
            mutex_unlock = True
        elif 'GetFenceStatus' in proto.name:
            using_line  = '    loader_platform_thread_lock_mutex(&objLock);\n'
            using_line += '    // Warn if submitted_flag is not set\n'
            using_line += '    validate_status(fence, VK_OBJECT_TYPE_FENCE, OBJSTATUS_FENCE_IS_SUBMITTED, OBJSTATUS_FENCE_IS_SUBMITTED, VK_DBG_MSG_ERROR, OBJTRACK_INVALID_FENCE, "Status Requested for Unsubmitted Fence");\n'
            mutex_unlock = True
        elif 'WaitForFences' in proto.name:
            using_line  = '    loader_platform_thread_lock_mutex(&objLock);\n'
            using_line += '    // Warn if waiting on unsubmitted fence\n'
            using_line += '    for (uint32_t i = 0; i < fenceCount; i++) {\n'
            using_line += '        validate_status(pFences[i], VK_OBJECT_TYPE_FENCE, OBJSTATUS_FENCE_IS_SUBMITTED, OBJSTATUS_FENCE_IS_SUBMITTED, VK_DBG_MSG_ERROR, OBJTRACK_INVALID_FENCE, "Waiting for Unsubmitted Fence");\n'
            using_line += '    }\n'
            mutex_unlock = True
        elif 'MapMemory' in proto.name:
            using_line  = '    loader_platform_thread_lock_mutex(&objLock);\n'
            using_line += '    set_status(mem, VK_OBJECT_TYPE_DEVICE_MEMORY, OBJSTATUS_GPU_MEM_MAPPED);\n'
            mutex_unlock = True
        elif 'UnmapMemory' in proto.name:
            using_line =  '    loader_platform_thread_lock_mutex(&objLock);\n'
            using_line += '    reset_status(mem, VK_OBJECT_TYPE_DEVICE_MEMORY, OBJSTATUS_GPU_MEM_MAPPED);\n'
            mutex_unlock = True
        elif 'AllocDescriptor' in proto.name: # Allocates array of DSs
            create_line  = '    loader_platform_thread_lock_mutex(&objLock);\n'
            create_line += '    for (uint32_t i = 0; i < *pCount; i++) {\n'
            create_line += '        create_obj(pDescriptorSets[i], VK_OBJECT_TYPE_DESCRIPTOR_SET);\n'
            create_line += '    }\n'
            create_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
        elif 'Create' in proto.name or 'Alloc' in proto.name:
            create_line =  '    loader_platform_thread_lock_mutex(&objLock);\n'
            create_line += '    create_obj(*%s, %s);\n' % (proto.params[-1].name, obj_type_mapping[proto.params[-1].ty.strip('*').replace('const ', '')])
            create_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
        if 'GetDeviceQueue' in proto.name:
            destroy_line  = '    loader_platform_thread_lock_mutex(&objLock);\n'
            destroy_line += '    addQueueInfo(queueNodeIndex, *pQueue);\n'
            destroy_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
        elif 'DestroyObject' in proto.name:
            destroy_line =  '    loader_platform_thread_lock_mutex(&objLock);\n'
            destroy_line += '    validateObjectType("vk%s", objType, object);\n' % (proto.name)
            destroy_line += '    destroy_obj(%s);\n' % (proto.params[2].name)
            destroy_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
        elif 'DestroyDevice' in proto.name:
            destroy_line =  '    loader_platform_thread_lock_mutex(&objLock);\n'
            destroy_line += '    destroy_obj(device);\n'
            destroy_line += '    // Report any remaining objects\n'
            destroy_line += '    for (auto it = objMap.begin(); it != objMap.end(); ++it) {\n'
            destroy_line += '        OBJTRACK_NODE* pNode = it->second;'
            destroy_line += '        if ((pNode->objType == VK_OBJECT_TYPE_PHYSICAL_DEVICE) || (pNode->objType == VK_OBJECT_TYPE_QUEUE)) {\n'
            destroy_line += '            // Cannot destroy physical device so ignore\n'
            destroy_line += '        } else {\n'
            destroy_line += '            char str[1024];\n'
            destroy_line += '            sprintf(str, "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_from_vulkan_object_type(pNode->objType), reinterpret_cast<VkUintPtrLeast64>(pNode->vkObj));\n'
            destroy_line += '            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, device, 0, OBJTRACK_OBJECT_LEAK, "OBJTRACK", str);\n'
            destroy_line += '        }\n'
            destroy_line += '    }\n'
            destroy_line += '    // Clean up Queue\'s MemRef Linked Lists\n'
            destroy_line += '    destroyQueueMemRefLists();\n'
            destroy_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
        elif 'Free' in proto.name:
            destroy_line  = '    loader_platform_thread_lock_mutex(&objLock);\n'
            destroy_line += '    destroy_obj(%s);\n' % (proto.params[1].name)
            destroy_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
        elif 'Destroy' in proto.name:
            destroy_line  = '    loader_platform_thread_lock_mutex(&objLock);\n'
            destroy_line += '    destroy_obj(%s);\n' % (param0_name)
            destroy_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
        if len(object_params) > 0:
            if not mutex_unlock:
                using_line += '    loader_platform_thread_lock_mutex(&objLock);\n'
                mutex_unlock = True
            for opn in object_params:
                using_line += '    validate_object(%s);\n' % (opn)
        if mutex_unlock:
            using_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
        ret_val = ''
        stmt = ''
        table = ''
        if proto.ret != "void":
            ret_val = "%s result = " % proto.ret
            stmt = "    return result;\n"
        if proto_is_global(proto):
           table = 'Instance'

        if proto.name == "EnumerateLayers":
            funcs.append('%s%s\n'
                     '{\n'
                     '    if (gpu != VK_NULL_HANDLE) {\n'
                     '        pCurObj = (VkBaseLayerObject *) gpu;\n'
                     '        loader_platform_thread_once(&initOnce, init%s);\n'
                     '        loader_platform_thread_once(&tabDeviceOnce, initDeviceTable);\n'
                     '        %snext%sTable.%s;\n'
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
                     '}' % (qual, decl, self.layer_name, ret_val, table, proto.c_call(), create_line, destroy_line, stmt, self.layer_name))
        elif 'GetPhysicalDeviceInfo' in proto.name:
            gpu_state  = '    if (infoType == VK_PHYSICAL_DEVICE_INFO_TYPE_QUEUE_PROPERTIES) {\n'
            gpu_state += '        if (pData != NULL) {\n'
            gpu_state += '            loader_platform_thread_lock_mutex(&objLock);\n'
            gpu_state += '            setGpuQueueInfoState(pDataSize, pData);\n'
            gpu_state += '            loader_platform_thread_unlock_mutex(&objLock);\n'
            gpu_state += '        }\n'
            gpu_state += '    }\n'
            funcs.append('%s%s\n'
                     '{\n'
                     '    %snext%sTable.%s;\n'
                     '%s%s'
                     '%s'
                     '%s'
                     '}' % (qual, decl, ret_val, table, proto.c_call(), create_line, destroy_line, gpu_state, stmt))
        else:
            funcs.append('%s%s\n'
                     '{\n'
                     '%s'
                     '    %snext%sTable.%s;\n'
                     '%s%s'
                     '%s'
                     '}' % (qual, decl, using_line, ret_val, table, proto.c_call(), create_line, destroy_line, stmt))
        return "\n\n".join(funcs)

    def generate_body(self):
        self.layer_name = "ObjectTracker"
        body = [self._generate_layer_initialization(True, lockname='obj'),
                self._generate_dispatch_entrypoints("VK_LAYER_EXPORT"),
                self._generate_extensions(),
                self._generate_layer_gpa_function(extensions=['objTrackGetObjectsCount', 'objTrackGetObjects', 'objTrackGetObjectsOfTypeCount', 'objTrackGetObjectsOfType'])]
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
        header_txt.append('static VkLayerInstanceDispatchTable nextInstanceTable;')
        header_txt.append('static VkBaseLayerObject *pCurObj;')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabDeviceOnce);')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabInstanceOnce);')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(initOnce);\n')
        header_txt.append('using namespace std;')
        header_txt.append('static unordered_map<int, void*> proxy_objectsInUse;\n')
        header_txt.append('static unordered_map<VkObject, loader_platform_thread_id> objectsInUse;\n')
        header_txt.append('static int threadingLockInitialized = 0;')
        header_txt.append('static loader_platform_thread_mutex threadingLock;')
        header_txt.append('static loader_platform_thread_cond threadingCond;')
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
        header_txt.append('        if (objectsInUse[object] != tid) {')
        header_txt.append('            char str[1024];')
        header_txt.append('            sprintf(str, "THREADING ERROR : object of type %s is simultaneously used in thread %ld and thread %ld", type, objectsInUse[object], tid);')
        header_txt.append('            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, 0, 0, THREADING_CHECKER_MULTIPLE_THREADS, "THREADING", str);')
        header_txt.append('            // Wait for thread-safe access to object')
        header_txt.append('            while (objectsInUse.find(object) != objectsInUse.end()) {')
        header_txt.append('                loader_platform_thread_cond_wait(&threadingCond, &threadingLock);')
        header_txt.append('            }')
        header_txt.append('            objectsInUse[object] = tid;')
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
        header_txt.append('    loader_platform_thread_cond_broadcast(&threadingCond);')
        header_txt.append('    loader_platform_thread_unlock_mutex(&threadingLock);')
        header_txt.append('}')
        return "\n".join(header_txt)

    def generate_intercept(self, proto, qual):
        if proto.name in [ 'DbgRegisterMsgCallback', 'DbgUnregisterMsgCallback' ]:
            # use default version
            return None
        decl = proto.c_func(prefix="vk", attr="VKAPI")
        thread_check_objects = [
            "VkQueue",
            "VkDeviceMemory",
            "VkObject",
            "VkBuffer",
            "VkImage",
            "VkDescriptorSet",
            "VkDescriptorPool",
            "VkCmdBuffer",
            "VkSemaphore"
        ]
        ret_val = ''
        stmt = ''
        funcs = []
        table = ''
        if proto.ret != "void":
            ret_val = "%s result = " % proto.ret
            stmt = "    return result;\n"
        if proto_is_global(proto):
           table = 'Instance'

        if proto.name == "EnumerateLayers":
            funcs.append('%s%s\n'
                     '{\n'
                     '    if (gpu != NULL) {\n'
                     '        pCurObj = (VkBaseLayerObject *) %s;\n'
                     '        loader_platform_thread_once(&initOnce, init%s);\n'
                     '        loader_platform_thread_once(&tabDeviceOnce, initDeviceTable);\n'
                     '        %snext%sTable.%s;\n'
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
                     '}' % (qual, decl, proto.params[0].name, self.layer_name, ret_val, table, proto.c_call(), stmt, self.layer_name))
            return "\n".join(funcs)
        # Memory range calls are special in needed thread checking within structs
        if proto.name in ["FlushMappedMemoryRanges","InvalidateMappedMemoryRanges"]:
            funcs.append('%s%s\n'
                     '{\n'
                     '    for (int i=0; i<memRangeCount; i++) {\n'
                     '        useObject((VkObject) pMemRanges[i].mem, "VkDeviceMemory");\n'
                     '    }\n'
                     '    %snext%sTable.%s;\n'
                     '    for (int i=0; i<memRangeCount; i++) {\n'
                     '        finishUsingObject((VkObject) pMemRanges[i].mem);\n'
                     '    }\n'
                     '%s'
                     '}' % (qual, decl, ret_val, table, proto.c_call(), stmt))
            return "\n".join(funcs)
        # All functions that do a Get are thread safe
        if 'Get' in proto.name:
            return None
        # All WSI functions are thread safe
        if 'WSI' in proto.name:
            return None
        # Initialize in early calls
        if proto.params[0].ty == "VkPhysicalDevice":
            funcs.append('%s%s\n'
                     '{\n'
                     '    %snext%sTable.%s;\n'
                     '%s'
                     '}' % (qual, decl, ret_val, table, proto.c_call(), stmt))
            return "\n".join(funcs)
        # Functions changing command buffers need thread safe use of first parameter
        if proto.params[0].ty == "VkCmdBuffer":
            funcs.append('%s%s\n'
                     '{\n'
                     '    useObject((VkObject) %s, "%s");\n'
                     '    %snext%sTable.%s;\n'
                     '    finishUsingObject((VkObject) %s);\n'
                     '%s'
                     '}' % (qual, decl, proto.params[0].name, proto.params[0].ty, ret_val, table, proto.c_call(), proto.params[0].name, stmt))
            return "\n".join(funcs)
        # Non-Cmd functions that do a Wait are thread safe
        if 'Wait' in proto.name:
            return None
        # Watch use of certain types of objects passed as any parameter
        checked_params = []
        for param in proto.params:
            if param.ty in thread_check_objects:
                checked_params.append(param)
        if len(checked_params) == 0:
            return None
        # Surround call with useObject and finishUsingObject for each checked_param
        funcs.append('%s%s' % (qual, decl))
        funcs.append('{')
        for param in checked_params:
            funcs.append('    useObject((VkObject) %s, "%s");' % (param.name, param.ty))
        funcs.append('    %snext%sTable.%s;' % (ret_val, table, proto.c_call()))
        for param in checked_params:
            funcs.append('    finishUsingObject((VkObject) %s);' % param.name)
        funcs.append('%s'
                 '}' % stmt)
        return "\n".join(funcs)

    def generate_body(self):
        self.layer_name = "Threading"
        body = [self._generate_layer_initialization(True, lockname='threading', condname='threading'),
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
