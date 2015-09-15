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
from source_line_info import sourcelineinfo
from collections import defaultdict

def proto_is_global(proto):
    if proto.params[0].ty == "VkInstance" or proto.params[0].ty == "VkPhysicalDevice" or proto.name == "CreateInstance" or proto.name == "GetGlobalLayerProperties" or proto.name == "GetGlobalExtensionProperties" or proto.name == "GetPhysicalDeviceLayerProperties" or proto.name == "GetPhysicalDeviceExtensionProperties":
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
        self.lineinfo = sourcelineinfo()

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
        if vk_type.strip('*') in vulkan.object_non_dispatch_list:
            if '*' in vk_type:
                return ("%lu", "%s->handle" % name)
            return ("%lu", "%s.handle" % name)
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
        if "bool" in vk_type.lower() or 'xcb_randr_crtc_t' in vk_type:
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
            return ("{%s.channelFormat = %%s, %s.numericFormat = %%s}" % (name, name), "string_VK_CHANNEL_FORMAT(%s.channelFormat), string_VK_FORMAT_NUM(%s.numericFormat)" % (name, name))
        if output_param:
            return ("%p", "(void*)*%s" % name)
        if vk_helper.is_type(vk_type, 'struct') and '*' not in vk_type:
            return ("%p", "(void*)(&%s)" % name)
        return ("%p", "(void*)(%s)" % name)

    def _gen_create_msg_callback(self):
        r_body = []
        r_body.append('%s' % self.lineinfo.get())
        r_body.append('VK_LAYER_EXPORT VkResult VKAPI vkDbgCreateMsgCallback(VkInstance instance, VkFlags msgFlags, const PFN_vkDbgMsgCallback pfnMsgCallback, void* pUserData, VkDbgMsgCallback* pMsgCallback)')
        r_body.append('{')
        # Switch to this code section for the new per-instance storage and debug callbacks
        if self.layer_name == 'ObjectTracker' or self.layer_name == 'Threading':
            r_body.append('    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(%s_instance_table_map, instance);' % self.layer_name )
            r_body.append('    VkResult result = pInstanceTable->DbgCreateMsgCallback(instance, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);')
            r_body.append('    if (VK_SUCCESS == result) {')
            r_body.append('        layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);')
            r_body.append('        result = layer_create_msg_callback(my_data->report_data, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);')
            r_body.append('    }')
            r_body.append('    return result;')
        else:
            # Old version of callbacks for compatibility
            r_body.append('    return layer_create_msg_callback(instance, instance_dispatch_table(instance), msgFlags, pfnMsgCallback, pUserData, pMsgCallback);')
        r_body.append('}')
        return "\n".join(r_body)

    def _gen_destroy_msg_callback(self):
        r_body = []
        r_body.append('%s' % self.lineinfo.get())
        r_body.append('VK_LAYER_EXPORT VkResult VKAPI vkDbgDestroyMsgCallback(VkInstance instance, VkDbgMsgCallback msgCallback)')
        r_body.append('{')
        # Switch to this code section for the new per-instance storage and debug callbacks
        if self.layer_name == 'ObjectTracker' or self.layer_name == 'Threading':
            r_body.append('    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(%s_instance_table_map, instance);' % self.layer_name )
            r_body.append('    VkResult result = pInstanceTable->DbgDestroyMsgCallback(instance, msgCallback);')
            r_body.append('    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);')
            r_body.append('    layer_destroy_msg_callback(my_data->report_data, msgCallback);')
            r_body.append('    return result;')
        else:
            # Old version of callbacks for compatibility
            r_body.append('    return layer_destroy_msg_callback(instance, instance_dispatch_table(instance), msgCallback);')
        r_body.append('}')
        return "\n".join(r_body)

    def _gen_layer_get_global_extension_props(self, layer="Generic"):
        ggep_body = []
        # generated layers do not provide any global extensions
        ggep_body.append('%s' % self.lineinfo.get())

        ggep_body.append('')
        ggep_body.append('VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionProperties(const char *pLayerName, uint32_t *pCount,  VkExtensionProperties* pProperties)')
        ggep_body.append('{')
        ggep_body.append('    return util_GetExtensionProperties(0, NULL, pCount, pProperties);')
        ggep_body.append('}')
        return "\n".join(ggep_body)

    def _gen_layer_get_global_layer_props(self, layer="Generic"):
        ggep_body = []
        if layer == 'Generic':
            # Do nothing, extension definition part of generic.h
            ggep_body.append('%s' % self.lineinfo.get())
        else:
            ggep_body.append('%s' % self.lineinfo.get())
            ggep_body.append('static const VkLayerProperties globalLayerProps[] = {')
            ggep_body.append('    {')
            ggep_body.append('        "%s",' % layer)
            ggep_body.append('        VK_API_VERSION, // specVersion')
            ggep_body.append('        VK_MAKE_VERSION(0, 1, 0), // implVersion')
            ggep_body.append('        "layer: %s",' % layer)
            ggep_body.append('    }')
            ggep_body.append('};')
        ggep_body.append('')
        ggep_body.append('%s' % self.lineinfo.get())
        ggep_body.append('')
        ggep_body.append('VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalLayerProperties(uint32_t *pCount,  VkLayerProperties* pProperties)')
        ggep_body.append('{')
        ggep_body.append('    return util_GetLayerProperties(ARRAY_SIZE(globalLayerProps), globalLayerProps, pCount, pProperties);')
        ggep_body.append('}')
        return "\n".join(ggep_body)

    def _gen_layer_get_physical_device_layer_props(self, layer="Generic"):
        gpdlp_body = []
        if layer == 'Generic':
            # Do nothing, extension definition part of generic.h
            gpdlp_body.append('%s' % self.lineinfo.get())
        else:
            gpdlp_body.append('%s' % self.lineinfo.get())
            gpdlp_body.append('static const VkLayerProperties deviceLayerProps[] = {')
            gpdlp_body.append('    {')
            gpdlp_body.append('        "%s",' % layer)
            gpdlp_body.append('        VK_API_VERSION,')
            gpdlp_body.append('        VK_MAKE_VERSION(0, 1, 0),')
            gpdlp_body.append('        "layer: %s",' % layer)
            gpdlp_body.append('    }')
            gpdlp_body.append('};')
        gpdlp_body.append('VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount, VkLayerProperties* pProperties)')
        gpdlp_body.append('{')
        gpdlp_body.append('    return util_GetLayerProperties(ARRAY_SIZE(deviceLayerProps), deviceLayerProps, pCount, pProperties);')
        gpdlp_body.append('}')
        gpdlp_body.append('')
        return "\n".join(gpdlp_body)

    def _generate_dispatch_entrypoints(self, qual=""):
        if qual:
            qual += " "

        funcs = []
        intercepted = []
        for proto in self.protos:
            if proto.name == "GetDeviceProcAddr" or proto.name == "GetInstanceProcAddr":
                continue
            else:
                intercept = self.generate_intercept(proto, qual)
                if intercept is None:
                    # fill in default intercept for certain entrypoints
                    if 'DbgCreateMsgCallback' == proto.name:
                        intercept = self._gen_layer_dbg_create_msg_callback()
                    elif 'DbgDestroyMsgCallback' == proto.name:
                        intercept = self._gen_layer_dbg_destroy_msg_callback()
                    elif 'CreateDevice' == proto.name:
                        funcs.append('/* CreateDevice HERE */')
                    elif 'GetGlobalExtensionProperties' == proto.name:
                        intercept = self._gen_layer_get_global_extension_props(self.layer_name)
                    elif 'GetGlobalLayerProperties' == proto.name:
                        intercept = self._gen_layer_get_global_layer_props(self.layer_name)
                    elif 'GetPhysicalDeviceLayerProperties' == proto.name:
                        intercept = self._gen_layer_get_physical_device_layer_props(self.layer_name)

                if intercept is not None:
                    funcs.append(intercept)
                    if not "KHR" in proto.name:
                        intercepted.append(proto)

        prefix="vk"
        lookups = []
        for proto in intercepted:
            lookups.append("if (!strcmp(name, \"%s\"))" % proto.name)
            lookups.append("    return (PFN_vkVoidFunction) %s%s;" %
                    (prefix, proto.name))

        # add customized layer_intercept_proc
        body = []
        body.append('%s' % self.lineinfo.get())
        body.append("static inline PFN_vkVoidFunction layer_intercept_proc(const char *name)")
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
            if proto.name == "CreateDevice":
                continue
            lookups.append("if (!strcmp(name, \"%s\"))" % proto.name)
            lookups.append("    return (PFN_vkVoidFunction) %s%s;" % (prefix, proto.name))

        body.append("static inline PFN_vkVoidFunction layer_intercept_instance_proc(const char *name)")
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
        exts.append('%s' % self.lineinfo.get())
        exts.append(self._gen_create_msg_callback())
        exts.append(self._gen_destroy_msg_callback())
        return "\n".join(exts)

    def _generate_layer_gpa_function(self, extensions=[], instance_extensions=[]):
        func_body = []
#
# New style of GPA Functions for the new layer_data/layer_logging changes
#
        if self.layer_name == 'ObjectTracker' or self.layer_name == 'Threading':
            func_body.append("VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI vkGetDeviceProcAddr(VkDevice device, const char* funcName)\n"
                             "{\n"
                             "    PFN_vkVoidFunction addr;\n"
                             "    if (device == VK_NULL_HANDLE) {\n"
                             "        return NULL;\n"
                             "    }\n"
                             "    /* loader uses this to force layer initialization; device object is wrapped */\n"
                             "    if (!strcmp(\"vkGetDeviceProcAddr\", funcName)) {\n"
                             "        initDeviceTable(%s_device_table_map, (const VkBaseLayerObject *) device);\n"
                             "        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;\n"
                             "    }\n\n"
                             "    addr = layer_intercept_proc(funcName);\n"
                             "    if (addr)\n"
                             "        return addr;" % self.layer_name)
            if 0 != len(extensions):
                func_body.append('%s' % self.lineinfo.get())
                func_body.append('    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);')
                for (ext_enable, ext_list) in extensions:
                    extra_space = ""
                    if 0 != len(ext_enable):
                        func_body.append('    if (my_device_data->%s) {' % ext_enable)
                        extra_space = "    "
                    for ext_name in ext_list:
                        func_body.append('    %sif (!strcmp("%s", funcName))\n'
                                         '        %sreturn reinterpret_cast<PFN_vkVoidFunction>(%s);' % (extra_space, ext_name, extra_space, ext_name))
                    if 0 != len(ext_enable):
                        func_body.append('    }\n')
            func_body.append("\n    if (get_dispatch_table(%s_device_table_map, device)->GetDeviceProcAddr == NULL)\n"
                             "        return NULL;\n"
                             "    return get_dispatch_table(%s_device_table_map, device)->GetDeviceProcAddr(device, funcName);\n"
                             "}\n" % (self.layer_name, self.layer_name))
            func_body.append("VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI vkGetInstanceProcAddr(VkInstance instance, const char* funcName)\n"
                             "{\n"
                             "    PFN_vkVoidFunction addr;\n"
                             "    if (instance == VK_NULL_HANDLE) {\n"
                             "        return NULL;\n"
                             "    }\n"
                             "    /* loader uses this to force layer initialization; instance object is wrapped */\n"
                             "    if (!strcmp(\"vkGetInstanceProcAddr\", funcName)) {\n"
                             "        initInstanceTable(%s_instance_table_map, (const VkBaseLayerObject *) instance);\n"
                             "        return (PFN_vkVoidFunction) vkGetInstanceProcAddr;\n"
                             "    }\n\n"
                             "    addr = layer_intercept_instance_proc(funcName);\n"
                             "    if (addr) {\n"
                             "        return addr;"
                             "    }\n" % self.layer_name)

            if 0 != len(instance_extensions):
                for ext_name in instance_extensions:
                    func_body.append("    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);\n"
                                     "    addr = debug_report_get_instance_proc_addr(my_data->report_data, funcName);\n"
                                     "    if (addr) {\n"
                                     "        return addr;\n"
                                     "    }\n"
                                     "    if (get_dispatch_table(%s_instance_table_map, instance)->GetInstanceProcAddr == NULL) {\n"
                                     "        return NULL;\n"
                                     "    }\n"
                                     "    return get_dispatch_table(%s_instance_table_map, instance)->GetInstanceProcAddr(instance, funcName);\n"
                                     "}\n" % (self.layer_name, self.layer_name))
            return "\n".join(func_body)
        else:
#
# TODO::  Old-style GPA Functions -- no local storage, no new logging mechanism.  Left for compatibility.
#
            func_body.append('%s' % self.lineinfo.get())
            func_body.append("VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI vkGetDeviceProcAddr(VkDevice device, const char* funcName)\n"
                             "{\n"
                             "    PFN_vkVoidFunction addr;\n"
                             "    if (device == VK_NULL_HANDLE) {\n"
                             "        return NULL;\n"
                             "    }\n"
                             "    loader_platform_thread_once(&initOnce, init%s);\n\n"
                             "    /* loader uses this to force layer initialization; device object is wrapped */\n"
                             "    if (!strcmp(\"vkGetDeviceProcAddr\", funcName)) {\n"
                             "        initDeviceTable((const VkBaseLayerObject *) device);\n"
                             "        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;\n"
                             "    }\n\n"
                             "    addr = layer_intercept_proc(funcName);\n"
                             "    if (addr)\n"
                             "        return addr;" % self.layer_name)
            func_body.append('')
            func_body.append('    VkLayerDispatchTable *pDisp =  device_dispatch_table(device);')
            if 0 != len(extensions):
                extra_space = ""
                for (ext_enable, ext_list) in extensions:
                    if 0 != len(ext_enable):
                        func_body.append('    if (deviceExtMap.size() == 0 || deviceExtMap[pDisp].%s)' % ext_enable)
                        func_body.append('    {')
                        extra_space = "    "
                    for ext_name in ext_list:
                        func_body.append('    %sif (!strcmp("%s", funcName))\n'
                                         '            return reinterpret_cast<PFN_vkVoidFunction>(%s);' % (extra_space, ext_name, ext_name))
                    if 0 != len(ext_enable):
                        func_body.append('    }')
            func_body.append('%s' % self.lineinfo.get())
            func_body.append("    {\n"
                             "        if (pDisp->GetDeviceProcAddr == NULL)\n"
                             "            return NULL;\n"
                             "        return pDisp->GetDeviceProcAddr(device, funcName);\n"
                             "    }\n"
                             "}\n")
            func_body.append("VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI vkGetInstanceProcAddr(VkInstance instance, const char* funcName)\n"
                             "{\n"
                             "    PFN_vkVoidFunction addr;\n"
                             "    if (instance == VK_NULL_HANDLE) {\n"
                             "        return NULL;\n"
                             "    }\n"
                             "    loader_platform_thread_once(&initOnce, init%s);\n\n"
                             "    /* loader uses this to force layer initialization; instance object is wrapped */\n"
                             "    if (!strcmp(\"vkGetInstanceProcAddr\", funcName)) {\n"
                             "        initInstanceTable((const VkBaseLayerObject *) instance);\n"
                             "        return (PFN_vkVoidFunction) vkGetInstanceProcAddr;\n"
                             "    }\n\n"
                             "    addr = layer_intercept_instance_proc(funcName);\n"
                             "    if (addr)\n"
                             "        return addr;" % self.layer_name)
            if 0 != len(instance_extensions):
                for ext_name in instance_extensions:
                    func_body.append("    {\n"
                                     "        PFN_vkVoidFunction fptr;\n"
                                     "        fptr = %s(funcName);\n"
                                     "        if (fptr) return fptr;\n"
                                     "    }\n" % ext_name)
            func_body.append("    VkLayerInstanceDispatchTable* pTable = instance_dispatch_table(instance);\n"
                             "    if (pTable->GetInstanceProcAddr == NULL)\n"
                             "        return NULL;\n"
                             "    return pTable->GetInstanceProcAddr(instance, funcName);\n"
                             "}\n")
            return "\n".join(func_body)


    def _generate_layer_initialization(self, init_opts=False, prefix='vk', lockname=None, condname=None):
        func_body = ["#include \"vk_dispatch_table_helper.h\""]
        func_body.append('%s' % self.lineinfo.get())
        func_body.append('static void init%s(void)\n'
                         '{\n' % self.layer_name)
        if init_opts:
            func_body.append('%s' % self.lineinfo.get())
            func_body.append('    const char *strOpt;')
            func_body.append('    // initialize %s options' % self.layer_name)
            func_body.append('    getLayerOptionEnum("%sReportLevel", (uint32_t *) &g_reportFlags);' % self.layer_name)
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
            func_body.append('%s' % self.lineinfo.get())
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
        return "\n".join(func_body)

    def _generate_new_layer_initialization(self, init_opts=False, prefix='vk', lockname=None, condname=None):
        func_body = ["#include \"vk_dispatch_table_helper.h\""]
        func_body.append('%s' % self.lineinfo.get())
        func_body.append('static void init%s(layer_data *my_data)\n'
                         '{\n' % self.layer_name)
        if init_opts:
            func_body.append('%s' % self.lineinfo.get())
            func_body.append('    uint32_t report_flags = 0;')
            func_body.append('    uint32_t debug_action = 0;')
            func_body.append('    FILE *log_output = NULL;')
            func_body.append('    const char *strOpt;')
            func_body.append('    // initialize %s options' % self.layer_name)
            func_body.append('    report_flags = getLayerOptionFlags("%sReportFlags", 0);' % self.layer_name)
            func_body.append('    g_actionIsDefault = getLayerOptionEnum("%sDebugAction", (uint32_t *) &debug_action);' % self.layer_name)
            func_body.append('')
            func_body.append('    if (debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)')
            func_body.append('    {')
            func_body.append('        strOpt = getLayerOption("%sLogFilename");' % self.layer_name)
            func_body.append('        log_output = getLayerLogOutput(strOpt, "%s");' % self.layer_name)
            func_body.append('        layer_create_msg_callback(my_data->report_data, report_flags, log_callback, (void *) log_output, &my_data->logging_callback);')
            func_body.append('    }')
            func_body.append('')
        if lockname is not None:
            func_body.append('%s' % self.lineinfo.get())
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
        return "\n".join(func_body)

class LayerFuncsSubcommand(Subcommand):
    def generate_header(self):
        return '#include <vk_layer.h>\n#include "loader.h"'

    def generate_body(self):
        return self._generate_dispatch_entrypoints("static")

class GenericLayerSubcommand(Subcommand):
    def generate_header(self):
        gen_header = []
        gen_header.append('%s' % self.lineinfo.get())
        gen_header.append('#include <stdio.h>')
        gen_header.append('#include <stdlib.h>')
        gen_header.append('#include <string.h>')
        gen_header.append('#include <unordered_map>')
        gen_header.append('#include "vk_loader_platform.h"')
        gen_header.append('#include "vk_layer.h"')
        gen_header.append('//The following is #included again to catch certain OS-specific functions being used:')
        gen_header.append('')
        gen_header.append('#include "vk_loader_platform.h"')
        gen_header.append('#include "vk_layer_config.h"')
        gen_header.append('#include "vk_layer_msg.h"')
        gen_header.append('#include "vk_layer_table.h"')
        gen_header.append('#include "vk_layer_extension_utils.h"')
        gen_header.append('')
        gen_header.append('#include "generic.h"')
        gen_header.append('')
        gen_header.append('%s' % self.lineinfo.get())
        gen_header.append('#define LAYER_EXT_ARRAY_SIZE 1')
        gen_header.append('#define LAYER_DEV_EXT_ARRAY_SIZE 1')
        gen_header.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(initOnce);')
        gen_header.append('struct devExts {')
        gen_header.append('    bool wsi_enabled;')
        gen_header.append('};')
        gen_header.append('static std::unordered_map<void *, struct devExts>     deviceExtMap;')
        gen_header.append('')
        gen_header.append('static void createDeviceRegisterExtensions(const VkDeviceCreateInfo* pCreateInfo, VkDevice device)')
        gen_header.append('{')
        gen_header.append('    uint32_t i;')
        gen_header.append('    VkLayerDispatchTable *pDisp  = device_dispatch_table(device);')
        gen_header.append('    deviceExtMap[pDisp].wsi_enabled = false;')
        gen_header.append('    for (i = 0; i < pCreateInfo->extensionCount; i++) {')
        gen_header.append('        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME) == 0)')
        gen_header.append('            deviceExtMap[pDisp].wsi_enabled = true;')
        gen_header.append('')
        gen_header.append('    }')
        gen_header.append('}')
        gen_header.append('')
        return "\n".join(gen_header)

    def generate_intercept(self, proto, qual):
        if proto.name in [ 'GetGlobalLayerProperties', 'GetGlobalExtensionProperties', 'GetPhysicalDeviceLayerProperties', 'GetPhysicalDeviceExtensionProperties' ]:
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
            funcs.append('%s' % self.lineinfo.get())
            ret_val = "%s result = " % proto.ret
            stmt = "    return result;\n"
        if proto.name == "CreateDevice":
            funcs.append('%s' % self.lineinfo.get())
            funcs.append('%s%s\n'
                     '{\n'
                     '    char str[1024];\n'
                     '    sprintf(str, "At start of layered %s\\n");\n'
                     '    layerCbMsg(VK_DBG_REPORT_INFO_BIT,VK_OBJECT_TYPE_PHYSICAL_DEVICE, (uint64_t)physicalDevice, 0, 0, (char *) "GENERIC", (char *) str);\n'
                     '    %sdevice_dispatch_table(*pDevice)->%s;\n'
                     '    if (result == VK_SUCCESS) {\n'
                     '        enable_debug_report(pCreateInfo->extensionCount, pCreateInfo->ppEnabledExtensionNames);\n'
                     '        createDeviceRegisterExtensions(pCreateInfo, *pDevice);\n'
                     '    }\n'
                     '    sprintf(str, "Completed layered %s\\n");\n'
                     '    layerCbMsg(VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_PHYSICAL_DEVICE, (uint64_t)physicalDevice, 0, 0, (char *) "GENERIC", (char *) str);\n'
                     '    fflush(stdout);\n'
                     '    %s'
                     '}' % (qual, decl, proto.name, ret_val, proto.c_call(), proto.name, stmt))
        elif proto.name == "DestroyDevice":
            funcs.append('%s' % self.lineinfo.get())
            funcs.append('%s%s\n'
                         '{\n'
                         '    dispatch_key key = get_dispatch_key(device);\n'
                         '    VkLayerDispatchTable *pDisp  =  device_dispatch_table(device);\n'
                         '    pDisp->DestroyDevice(device);\n'
                         '    deviceExtMap.erase(pDisp);\n'
                         '    destroy_device_dispatch_table(key);\n'
                         '}\n' % (qual, decl))
        elif proto.name == "DestroyInstance":
            funcs.append('%s' % self.lineinfo.get())
            funcs.append('%s%s\n'
                         '{\n'
                         '    dispatch_key key = get_dispatch_key(instance);\n'
                         '    instance_dispatch_table(instance)->DestroyInstance(instance);\n'
                         '    destroy_instance_dispatch_table(key);\n'
                         '}\n' % (qual, decl))
        else:
            funcs.append('%s' % self.lineinfo.get())
            # CreateInstance needs to use the second parm instead of the first to set the correct dispatch table
            dispatch_param = proto.params[0].name
            # Must use 'instance' table for these APIs, 'device' table otherwise
            table_type = ""
            if proto_is_global(proto):
                table_type = "instance"
            else:
                table_type = "device"
            if 'CreateInstance' in proto.name:
               dispatch_param = '*' + proto.params[1].name
            funcs.append('%s%s\n'
                     '{\n'
                     '    %s%s_dispatch_table(%s)->%s;\n'
                     '%s'
                     '}' % (qual, decl, ret_val, table_type, dispatch_param, proto.c_call(), stmt))
        return "\n\n".join(funcs)

    def generate_body(self):
        self.layer_name = "Generic"
        extensions=[('wsi_enabled', 
                     ['vkCreateSwapchainKHR', 'vkDestroySwapchainKHR',
                      'vkGetSwapchainImagesKHR', 'vkQueuePresentKHR'])]
        body = [self._generate_layer_initialization(True),
                self._generate_dispatch_entrypoints("VK_LAYER_EXPORT"),
                self._gen_create_msg_callback(),
                self._gen_destroy_msg_callback(),
                self._generate_layer_gpa_function(extensions)]

        return "\n\n".join(body)

class APIDumpSubcommand(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('%s' % self.lineinfo.get())
        header_txt.append('#include <fstream>')
        header_txt.append('#include <iostream>')
        header_txt.append('#include <string>')
        header_txt.append('#include <string.h>')
        header_txt.append('')
        header_txt.append('#include "vk_loader_platform.h"')
        header_txt.append('#include "vk_layer.h"')
        header_txt.append('#include "vk_struct_string_helper_cpp.h"')
        header_txt.append('#include "vk_layer_table.h"')
        header_txt.append('#include "vk_layer_extension_utils.h"')
        header_txt.append('#include <unordered_map>')
        header_txt.append('')
        header_txt.append('static std::ofstream fileStream;')
        header_txt.append('static std::string fileName = "vk_apidump.txt";')
        header_txt.append('std::ostream* outputStream = NULL;')
        header_txt.append('void ConfigureOutputStream(bool writeToFile, bool flushAfterWrite)')
        header_txt.append('{')
        header_txt.append('    if(writeToFile)')
        header_txt.append('    {')
        header_txt.append('        fileStream.open(fileName);')
        header_txt.append('        if ((fileStream.rdstate() & fileStream.failbit) != 0) {')
        header_txt.append('            outputStream = &std::cout;')
        header_txt.append('            (*outputStream) << endl << "APIDump ERROR: Bad output filename specified: " << fileName << ". Writing to STDOUT instead" << endl << endl;')
        header_txt.append('        }')
        header_txt.append('        else')
        header_txt.append('            outputStream = &fileStream;')
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
        header_txt.append('%s' % self.lineinfo.get())
        header_txt.append('// The following is #included again to catch certain OS-specific functions being used:')
        header_txt.append('#include "vk_loader_platform.h"')
        header_txt.append('')
        header_txt.append('static VkBaseLayerObject *pCurObj;')
        header_txt.append('static bool g_APIDumpDetailed = true;')
        header_txt.append('')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(initOnce);')
        header_txt.append('')
        header_txt.append('static int printLockInitialized = 0;')
        header_txt.append('static loader_platform_thread_mutex printLock;')
        header_txt.append('')
        header_txt.append('%s' % self.lineinfo.get())
        header_txt.append('#define LAYER_EXT_ARRAY_SIZE 1')
        header_txt.append('#define LAYER_DEV_EXT_ARRAY_SIZE 1')
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
        header_txt.append('struct devExts {')
        header_txt.append('    bool wsi_enabled;')
        header_txt.append('};')
        header_txt.append('')
        header_txt.append('static std::unordered_map<void *, struct devExts>     deviceExtMap;')
        header_txt.append('')
        header_txt.append('static void createDeviceRegisterExtensions(const VkDeviceCreateInfo* pCreateInfo, VkDevice device)')
        header_txt.append('{')
        header_txt.append('    uint32_t i;')
        header_txt.append('    VkLayerDispatchTable *pDisp  = device_dispatch_table(device);')
        header_txt.append('    deviceExtMap[pDisp].wsi_enabled = false;')
        header_txt.append('    for (i = 0; i < pCreateInfo->extensionCount; i++) {')
        header_txt.append('        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME) == 0)')
        header_txt.append('            deviceExtMap[pDisp].wsi_enabled = true;')
        header_txt.append('')
        header_txt.append('    }')
        header_txt.append('}')
        header_txt.append('')
        header_txt.append('void interpret_memBarriers(const void* const* ppMemBarriers, uint32_t memBarrierCount)')
        header_txt.append('{')
        header_txt.append('    if (ppMemBarriers) {')
        header_txt.append('        string tmp_str;')
        header_txt.append('        for (uint32_t i = 0; i < memBarrierCount; i++) {')
        header_txt.append('            switch(*(VkStructureType*)ppMemBarriers[i])')
        header_txt.append('            {')
        header_txt.append('                case VK_STRUCTURE_TYPE_MEMORY_BARRIER:')
        header_txt.append('                    tmp_str = vk_print_vkmemorybarrier((VkMemoryBarrier*)ppMemBarriers[i], "    ");')
        header_txt.append('                    break;')
        header_txt.append('                case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:')
        header_txt.append('                    tmp_str = vk_print_vkbuffermemorybarrier((VkBufferMemoryBarrier*)ppMemBarriers[i], "    ");')
        header_txt.append('                    break;')
        header_txt.append('                case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:')
        header_txt.append('                    tmp_str = vk_print_vkimagememorybarrier((VkImageMemoryBarrier*)ppMemBarriers[i], "    ");')
        header_txt.append('                    break;')
        header_txt.append('                default:')
        header_txt.append('                    break;')
        header_txt.append('            }')
        header_txt.append('')
        header_txt.append('            if (StreamControl::writeAddress == true) {')
        header_txt.append('                (*outputStream) << "   ppMemBarriers[" << i << "] (" << &ppMemBarriers[i] << ")" << endl << tmp_str << endl;')
        header_txt.append('            } else {')
        header_txt.append('                (*outputStream) << "   ppMemBarriers[" << i << "] (address)" << endl << "    address" << endl;')
        header_txt.append('            }')
        header_txt.append('        }')
        header_txt.append('    }')
        header_txt.append('}')
        header_txt.append('')
        return "\n".join(header_txt)

    def generate_init(self):
        func_body = []
        func_body.append('%s' % self.lineinfo.get())
        func_body.append('#include "vk_dispatch_table_helper.h"')
        func_body.append('#include "vk_layer_config.h"')
        func_body.append('')
        func_body.append('static void init%s(void)' % self.layer_name)
        func_body.append('{')
        func_body.append('    using namespace StreamControl;')
        func_body.append('')
        func_body.append('    char const*const logName = getLayerOption("APIDumpLogFilename");')
        func_body.append('    if(logName != NULL)')
        func_body.append('    {')
        func_body.append('        fileName = logName;')
        func_body.append('    }')
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
        func_body.append('%s' % self.lineinfo.get())
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
        func_body.append('%s' % self.lineinfo.get())
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
        return "\n".join(func_body)

    def generate_intercept(self, proto, qual):
        if proto.name in [ 'GetGlobalLayerProperties','GetGlobalExtensionProperties','GetPhysicalDeviceLayerProperties','GetPhysicalDeviceExtensionProperties']:
            return None
        decl = proto.c_func(prefix="vk", attr="VKAPI")
        ret_val = ''
        stmt = ''
        funcs = []
        sp_param_dict = {} # Store 'index' for struct param to print, or an name of binding "Count" param for array to print
        create_params = 0 # Num of params at end of function that are created and returned as output values
        if 'AllocDescriptorSets' in proto.name:
            create_params = -1
        elif 'Create' in proto.name or 'Alloc' in proto.name or 'MapMemory' in proto.name:
            create_params = -1
        if proto.ret != "void":
            ret_val = "%s result = " % proto.ret
            stmt = "    return result;\n"
        f_open = 'loader_platform_thread_lock_mutex(&printLock);\n    '
        log_func = '%s\n' % self.lineinfo.get()
        log_func += '    if (StreamControl::writeAddress == true) {'
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
            if p.name == "pSwapchain":
                log_func += '%s = " << %s->handle << ", ' % (p.name, p.name)
            elif p.name == "swapchain":
                log_func += '%s = " << %s.handle << ", ' % (p.name, p.name)
            else:
                log_func += '%s = " << %s << ", ' % (p.name, pfi)
            if "%p" == pft:
                log_func_no_addr += '%s = address, ' % (p.name)
            else:
                log_func_no_addr += '%s = " << %s << ", ' % (p.name, pfi)
            if prev_count_name != '' and (prev_count_name.replace('Count', '')[1:] in p.name):
                sp_param_dict[pindex] = prev_count_name
                prev_count_name = ''
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
        log_func += '\n    }\n    else {%s\n    }' % log_func_no_addr;
        log_func += '\n%s' % self.lineinfo.get()
        #print("Proto %s has param_dict: %s" % (proto.name, sp_param_dict))
        if len(sp_param_dict) > 0:
            indent = '    '
            log_func += '\n%sif (g_APIDumpDetailed) {' % indent
            indent += '    '
            i_decl = False
            log_func += '\n%s' % self.lineinfo.get()
            log_func += '\n%sstring tmp_str;' % indent
            for sp_index in sp_param_dict:
                #print("sp_index: %s" % str(sp_index))
                if 'index' == sp_param_dict[sp_index]:
                    cis_print_func = 'vk_print_%s' % (proto.params[sp_index].ty.replace('const ', '').strip('*').lower())
                    local_name = proto.params[sp_index].name
                    if '*' not in proto.params[sp_index].ty:
                        local_name = '&%s' % proto.params[sp_index].name
                    log_func += '\n%s' % self.lineinfo.get()
                    log_func += '\n%sif (%s) {' % (indent, local_name)
                    indent += '    '
                    log_func += '\n%stmp_str = %s(%s, "    ");' % (indent, cis_print_func, local_name)
                    log_func += '\n%s(*outputStream) << "   %s (" << %s << ")" << endl << tmp_str << endl;' % (indent, local_name, local_name)
                    indent = indent[4:]
                    log_func += '\n%s}' % (indent)
                elif 'memBarrierCount' == sp_param_dict[sp_index]: # call helper function
                    log_func += '\n%sif (ppMemBarriers) {' % (indent)
                    log_func += '\n%s    interpret_memBarriers(ppMemBarriers, memBarrierCount);' % (indent)
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
                    if proto.params[sp_index].ty.strip('*').replace('const ', '') in vulkan.object_non_dispatch_list:
                        cis_print_func = 'tmp_str = %s(%s%s[i].handle, "    ");' % (print_func, print_cast, proto.params[sp_index].name)
                    else:
                        cis_print_func = 'tmp_str = %s(%s%s[i], "    ");' % (print_func, print_cast, proto.params[sp_index].name)
                    if not i_decl:
                        log_func += '\n%suint32_t i;' % (indent)
                        i_decl = True
                    log_func += '\n%sif (%s) {' % (indent, proto.params[sp_index].name)
                    indent += '    '
                    log_func += '\n%s' % self.lineinfo.get()
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
            indent = indent[4:]
            log_func += '\n%s}' % (indent)
        table_type = ''
        if proto_is_global(proto):
           table_type = 'instance'
        else:
           table_type = 'device'

        if proto.name == "CreateInstance":
            dispatch_param = '*' + proto.params[1].name
        else:
            dispatch_param = proto.params[0].name

        if proto.name == "CreateDevice":
            funcs.append('%s\n' % self.lineinfo.get())
            funcs.append('%s%s\n'
                     '{\n'
                     '    using namespace StreamControl;\n'
                     '    %sdevice_dispatch_table(*pDevice)->%s;\n'
                     '    if (result == VK_SUCCESS)\n'
                     '        createDeviceRegisterExtensions(pCreateInfo, *pDevice);\n'
                     '    %s%s%s\n'
                     '%s'
                     '}' % (qual, decl, ret_val, proto.c_call(), f_open, log_func, f_close, stmt))
        elif proto.name == "DestroyDevice":
            funcs.append('%s%s\n'
                 '{\n'
                 '    using namespace StreamControl;\n'
                 '    dispatch_key key = get_dispatch_key(device);\n'
                         '    VkLayerDispatchTable *pDisp  = %s_dispatch_table(%s);\n'
                 '    %spDisp->%s;\n'
                 '    deviceExtMap.erase(pDisp);\n'
                 '    destroy_device_dispatch_table(key);\n'
                 '    %s%s%s\n'
                 '%s'
                 '}' % (qual, decl, table_type, dispatch_param, ret_val, proto.c_call(), f_open, log_func, f_close, stmt))
        elif proto.name == "DestroyInstance":
            funcs.append('%s%s\n'
                 '{\n'
                 '    using namespace StreamControl;\n'
                 '    dispatch_key key = get_dispatch_key(instance);\n'
                 '    %s%s_dispatch_table(%s)->%s;\n'
                 '    destroy_instance_dispatch_table(key);\n'
                 '    %s%s%s\n'
                 '%s'
                 '}' % (qual, decl, ret_val, table_type, dispatch_param, proto.c_call(), f_open, log_func, f_close, stmt))
        else:
            funcs.append('%s%s\n'
                     '{\n'
                     '    using namespace StreamControl;\n'
                     '    %s%s_dispatch_table(%s)->%s;\n'
                     '    %s%s%s\n'
                     '%s'
                     '}' % (qual, decl, ret_val, table_type, dispatch_param, proto.c_call(), f_open, log_func, f_close, stmt))
        return "\n\n".join(funcs)

    def generate_body(self):
        self.layer_name = "APIDump"
        extensions=[('wsi_enabled',
                    ['vkCreateSwapchainKHR', 'vkDestroySwapchainKHR',
                    'vkGetSwapchainImagesKHR', 'vkQueuePresentKHR'])]
        body = [self.generate_init(),
                self._generate_dispatch_entrypoints("VK_LAYER_EXPORT"),
                self._generate_layer_gpa_function(extensions)]
        return "\n\n".join(body)

class ObjectTrackerSubcommand(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('%s' % self.lineinfo.get())
        header_txt.append('#include <stdio.h>')
        header_txt.append('#include <stdlib.h>')
        header_txt.append('#include <string.h>')
        header_txt.append('#include <inttypes.h>')
        header_txt.append('')
        header_txt.append('#include "vulkan.h"')
        header_txt.append('#include "vk_loader_platform.h"')
        header_txt.append('')
        header_txt.append('#include <unordered_map>')
        header_txt.append('using namespace std;')
        header_txt.append('// The following is #included again to catch certain OS-specific functions being used:')
        header_txt.append('#include "vk_loader_platform.h"')
        header_txt.append('#include "vk_layer_config.h"')
        header_txt.append('#include "vk_layer_msg.h"')
        header_txt.append('#include "vk_debug_report_lunarg.h"')
        header_txt.append('#include "vk_layer_table.h"')
        header_txt.append('#include "vk_layer_data.h"')
        header_txt.append('#include "vk_layer_logging.h"')
        header_txt.append('')
#       NOTE:  The non-autoGenerated code is in the object_track.h header file
        header_txt.append('#include "object_track.h"')
        header_txt.append('')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(initOnce);')
        header_txt.append('')
        return "\n".join(header_txt)

    def generate_maps(self):
        maps_txt = []
        for o in vulkan.core.objects:
            maps_txt.append('unordered_map<const void*, OBJTRACK_NODE*> %sMap;' % (o))
        maps_txt.append('unordered_map<const void*, OBJTRACK_NODE*> VkSwapchainKHRMap;')
        return "\n".join(maps_txt)

    def generate_procs(self):
        procs_txt = []
        for o in vulkan.core.objects:
            procs_txt.append('%s' % self.lineinfo.get())
            if o in vulkan.object_dispatch_list:
                procs_txt.append('static void create_obj(%s dispatchable_object, %s vkObj, VkDbgObjectType objType)' % (o, o))
                procs_txt.append('{')
                procs_txt.append('    log_msg(mdd(dispatchable_object), VK_DBG_REPORT_INFO_BIT, objType, reinterpret_cast<uint64_t>(vkObj), 0, OBJTRACK_NONE, "OBJTRACK",')
                procs_txt.append('        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDbgObjectType(objType),')
                procs_txt.append('        reinterpret_cast<uint64_t>(vkObj));')
            else:
                procs_txt.append('static void create_obj(VkDevice dispatchable_object, %s vkObj, VkDbgObjectType objType)' % (o))
                procs_txt.append('{')
                procs_txt.append('    log_msg(mdd(dispatchable_object), VK_DBG_REPORT_INFO_BIT, objType, vkObj.handle, 0, OBJTRACK_NONE, "OBJTRACK",')
                procs_txt.append('        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDbgObjectType(objType),')
                procs_txt.append('        vkObj.handle);')
            procs_txt.append('')
            procs_txt.append('    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;')
            procs_txt.append('    pNewObjNode->objType = objType;')
            procs_txt.append('    pNewObjNode->status  = OBJSTATUS_NONE;')
            if o in vulkan.object_dispatch_list:
                procs_txt.append('    pNewObjNode->vkObj  = reinterpret_cast<uint64_t>(vkObj);')
                procs_txt.append('    %sMap[vkObj] = pNewObjNode;' % (o))
            else:
                procs_txt.append('    pNewObjNode->vkObj  = vkObj.handle;')
                procs_txt.append('    %sMap[(void*)vkObj.handle] = pNewObjNode;' % (o))
            procs_txt.append('    uint32_t objIndex = objTypeToIndex(objType);')
            procs_txt.append('    numObjs[objIndex]++;')
            procs_txt.append('    numTotalObjs++;')
            procs_txt.append('}')
            procs_txt.append('')
            procs_txt.append('%s' % self.lineinfo.get())
            # TODO : This is not complete and currently requires some hand-coded function in the header
            #  Really we want to capture the set of all objects and their associated dispatchable objects
            #  that are bound by the API calls:
            #    foreach API Call
            #        foreach object type seen by call
            #            create validate_object(disp_obj, object)
            if o in vulkan.object_dispatch_list:
                procs_txt.append('static VkBool32 validate_object(%s dispatchable_object, %s object)' % (o, o))
            else:
                procs_txt.append('static VkBool32 validate_object(VkDevice dispatchable_object, %s object)' % (o))
            procs_txt.append('{')
            if o in vulkan.object_dispatch_list:
                procs_txt.append('    if (%sMap.find(object) == %sMap.end()) {' % (o, o))
                procs_txt.append('        return log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, reinterpret_cast<uint64_t>(object), 0, OBJTRACK_INVALID_OBJECT, "OBJTRACK",')
                procs_txt.append('            "Invalid %s Object %%p",reinterpret_cast<uint64_t>(object));' % o)
            else:
                procs_txt.append('    if (%sMap.find((void*)object.handle) == %sMap.end()) {' % (o, o))
                procs_txt.append('        return log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, object.handle, 0, OBJTRACK_INVALID_OBJECT, "OBJTRACK",')
                procs_txt.append('            "Invalid %s Object %%p", object.handle);' % o)
            procs_txt.append('    }')
            procs_txt.append('    return VK_FALSE;')
            procs_txt.append('}')
            procs_txt.append('')
            procs_txt.append('')
            procs_txt.append('%s' % self.lineinfo.get())
            if o in vulkan.object_dispatch_list:
                procs_txt.append('static void destroy_obj(%s dispatchable_object, %s object)' % (o, o))
            else:
                procs_txt.append('static void destroy_obj(VkDevice dispatchable_object, %s object)' % (o))
            procs_txt.append('{')
            if o in vulkan.object_dispatch_list:
                procs_txt.append('    if (%sMap.find(object) != %sMap.end()) {' % (o, o))
                procs_txt.append('        OBJTRACK_NODE* pNode = %sMap[object];' % (o))
            else:
                procs_txt.append('    if (%sMap.find((void*)object.handle) != %sMap.end()) {' % (o, o))
                procs_txt.append('        OBJTRACK_NODE* pNode = %sMap[(void*)object.handle];' % (o))
            procs_txt.append('        uint32_t objIndex = objTypeToIndex(pNode->objType);')
            procs_txt.append('        assert(numTotalObjs > 0);')
            procs_txt.append('        numTotalObjs--;')
            procs_txt.append('        assert(numObjs[objIndex] > 0);')
            procs_txt.append('        numObjs[objIndex]--;')
            if o in vulkan.object_dispatch_list:
                procs_txt.append('        log_msg(mdd(dispatchable_object), VK_DBG_REPORT_INFO_BIT, pNode->objType, reinterpret_cast<uint64_t>(object), 0, OBJTRACK_NONE, "OBJTRACK",')
                procs_txt.append('           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%lu total objs remain & %lu %s objs).",')
                procs_txt.append('            string_VkDbgObjectType(pNode->objType), reinterpret_cast<uint64_t>(object), numTotalObjs, numObjs[objIndex],')
            else:
                procs_txt.append('        log_msg(mdd(dispatchable_object), VK_DBG_REPORT_INFO_BIT, pNode->objType, object.handle, 0, OBJTRACK_NONE, "OBJTRACK",')
                procs_txt.append('           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%lu total objs remain & %lu %s objs).",')
                procs_txt.append('            string_VkDbgObjectType(pNode->objType), object.handle, numTotalObjs, numObjs[objIndex],')
            procs_txt.append('            string_VkDbgObjectType(pNode->objType));')
            procs_txt.append('        delete pNode;')
            if o in vulkan.object_dispatch_list:
                procs_txt.append('        %sMap.erase(object);' % (o))
                procs_txt.append('    } else {')
                procs_txt.append('        log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, reinterpret_cast<uint64_t>(object), 0, OBJTRACK_NONE, "OBJTRACK",')
                procs_txt.append('            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",')
                procs_txt.append('           reinterpret_cast<uint64_t>(object));')
                procs_txt.append('    }')
            else:
                procs_txt.append('        %sMap.erase((void*)object.handle);' % (o))
                procs_txt.append('    } else {')
                procs_txt.append('        log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, object.handle, 0, OBJTRACK_NONE, "OBJTRACK",')
                procs_txt.append('            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",')
                procs_txt.append('            object.handle);')
                procs_txt.append('    }')
            procs_txt.append('}')
            procs_txt.append('')
            procs_txt.append('%s' % self.lineinfo.get())
            if o in vulkan.object_dispatch_list:
                procs_txt.append('static VkBool32 set_status(%s dispatchable_object, %s object, VkDbgObjectType objType, ObjectStatusFlags status_flag)' % (o, o))
                procs_txt.append('{')
                procs_txt.append('    if (object != VK_NULL_HANDLE) {')
                procs_txt.append('        if (%sMap.find(object) != %sMap.end()) {' % (o, o))
                procs_txt.append('            OBJTRACK_NODE* pNode = %sMap[object];' % (o))
            else:
                procs_txt.append('static VkBool32 set_status(VkDevice dispatchable_object, %s object, VkDbgObjectType objType, ObjectStatusFlags status_flag)' % (o))
                procs_txt.append('{')
                procs_txt.append('    if (object != VK_NULL_HANDLE) {')
                procs_txt.append('        if (%sMap.find((void*)object.handle) != %sMap.end()) {' % (o, o))
                procs_txt.append('            OBJTRACK_NODE* pNode = %sMap[(void*)object.handle];' % (o))
            procs_txt.append('            pNode->status |= status_flag;')
            procs_txt.append('        }')
            procs_txt.append('        else {')
            procs_txt.append('            // If we do not find it print an error')
            if o in vulkan.object_dispatch_list:
                procs_txt.append('            return log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, reinterpret_cast<uint64_t>(object), 0, OBJTRACK_NONE, "OBJTRACK",')
                procs_txt.append('                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",')
                procs_txt.append('                reinterpret_cast<uint64_t>(object), string_VkDbgObjectType(objType));')
            else:
                procs_txt.append('            return log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, object.handle, 0, OBJTRACK_NONE, "OBJTRACK",')
                procs_txt.append('                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",')
                procs_txt.append('                object.handle, string_VkDbgObjectType(objType));')
            procs_txt.append('        }')
            procs_txt.append('    }')
            procs_txt.append('    return VK_FALSE;')
            procs_txt.append('}')
            procs_txt.append('')
            procs_txt.append('%s' % self.lineinfo.get())
            procs_txt.append('static VkBool32 validate_status(')
            if o in vulkan.object_dispatch_list:
                procs_txt.append('%s dispatchable_object, %s object,' % (o, o))
            else:
                procs_txt.append('VkDevice dispatchable_object, %s object,' % (o))
            procs_txt.append('    VkDbgObjectType     objType,')
            procs_txt.append('    ObjectStatusFlags   status_mask,')
            procs_txt.append('    ObjectStatusFlags   status_flag,')
            procs_txt.append('    VkFlags             msg_flags,')
            procs_txt.append('    OBJECT_TRACK_ERROR  error_code,')
            procs_txt.append('    const char         *fail_msg)')
            procs_txt.append('{')
            if o in vulkan.object_dispatch_list:
                procs_txt.append('    if (%sMap.find(object) != %sMap.end()) {' % (o, o))
                procs_txt.append('        OBJTRACK_NODE* pNode = %sMap[object];' % (o))
            else:
                procs_txt.append('    if (%sMap.find((void*)object.handle) != %sMap.end()) {' % (o, o))
                procs_txt.append('        OBJTRACK_NODE* pNode = %sMap[(void*)object.handle];' % (o))
            procs_txt.append('        if ((pNode->status & status_mask) != status_flag) {')
            if o in vulkan.object_dispatch_list:
                procs_txt.append('            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, reinterpret_cast<uint64_t>(object), 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",')
                procs_txt.append('                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDbgObjectType(objType),')
                procs_txt.append('                 reinterpret_cast<uint64_t>(object), fail_msg);')
            else:
                procs_txt.append('            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, object.handle, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",')
                procs_txt.append('                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkDbgObjectType(objType),')
                procs_txt.append('                 object.handle, fail_msg);')
            procs_txt.append('            return VK_FALSE;')
            procs_txt.append('        }')
            procs_txt.append('        return VK_TRUE;')
            procs_txt.append('    }')
            procs_txt.append('    else {')
            procs_txt.append('        // If we do not find it print an error')
            if o in vulkan.object_dispatch_list:
                procs_txt.append('        log_msg(mdd(dispatchable_object), msg_flags, (VkDbgObjectType) 0, reinterpret_cast<uint64_t>(object), 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",')
                procs_txt.append('            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",')
                procs_txt.append('            reinterpret_cast<uint64_t>(object), string_VkDbgObjectType(objType));')
            else:
                procs_txt.append('        log_msg(mdd(dispatchable_object), msg_flags, (VkDbgObjectType) 0, object.handle, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",')
                procs_txt.append('            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",')
                procs_txt.append('            object.handle, string_VkDbgObjectType(objType));')
            procs_txt.append('        return VK_FALSE;')
            procs_txt.append('    }')
            procs_txt.append('}')
            procs_txt.append('')
            procs_txt.append('%s' % self.lineinfo.get())
            if o in vulkan.object_dispatch_list:
                procs_txt.append('static VkBool32 reset_status(%s dispatchable_object, %s object, VkDbgObjectType objType, ObjectStatusFlags status_flag)' % (o, o))
                procs_txt.append('{')
                procs_txt.append('    if (%sMap.find(object) != %sMap.end()) {' % (o, o))
                procs_txt.append('        OBJTRACK_NODE* pNode = %sMap[object];' % (o))
            else:
                procs_txt.append('static VkBool32 reset_status(VkDevice dispatchable_object, %s object, VkDbgObjectType objType, ObjectStatusFlags status_flag)' % (o))
                procs_txt.append('{')
                procs_txt.append('    if (%sMap.find((void*)object.handle) != %sMap.end()) {' % (o, o))
                procs_txt.append('        OBJTRACK_NODE* pNode = %sMap[(void*)object.handle];' % (o))
            procs_txt.append('        pNode->status &= ~status_flag;')
            procs_txt.append('    }')
            procs_txt.append('    else {')
            procs_txt.append('        // If we do not find it print an error')
            if o in vulkan.object_dispatch_list:
                procs_txt.append('        return log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, objType, reinterpret_cast<uint64_t>(object), 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",')
                procs_txt.append('            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",')
                procs_txt.append('            reinterpret_cast<uint64_t>(object), string_VkDbgObjectType(objType));')
            else:
                procs_txt.append('        return log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, objType, object.handle, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",')
                procs_txt.append('            "Unable to reset status for non-existent object 0x%" PRIxLEAST64 " of %s type",')
                procs_txt.append('            object.handle, string_VkDbgObjectType(objType));')
            procs_txt.append('    }')
            procs_txt.append('    return VK_FALSE;')
            procs_txt.append('}')
            procs_txt.append('')
        return "\n".join(procs_txt)

    def generate_destroy_instance(self):
        gedi_txt = []
        gedi_txt.append('%s' % self.lineinfo.get())
        gedi_txt.append('void vkDestroyInstance(')
        gedi_txt.append('VkInstance instance)')
        gedi_txt.append('{')
        gedi_txt.append('    loader_platform_thread_lock_mutex(&objLock);')
        gedi_txt.append('    validate_object(instance, instance);')
        gedi_txt.append('')
        gedi_txt.append('    destroy_obj(instance, instance);')
        gedi_txt.append('    // Report any remaining objects in LL')
        for o in vulkan.core.objects:
            if o in ['VkPhysicalDevice', 'VkQueue']:
                continue
            gedi_txt.append('    for (auto it = %sMap.begin(); it != %sMap.end(); ++it) {' % (o, o))
            gedi_txt.append('        OBJTRACK_NODE* pNode = it->second;')
            gedi_txt.append('        log_msg(mid(instance), VK_DBG_REPORT_ERROR_BIT, pNode->objType, pNode->vkObj, 0, OBJTRACK_OBJECT_LEAK, "OBJTRACK",')
            gedi_txt.append('                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDbgObjectType(pNode->objType),')
            gedi_txt.append('                pNode->vkObj);')
            gedi_txt.append('    }')
            gedi_txt.append('')
        gedi_txt.append('    dispatch_key key = get_dispatch_key(instance);')
        gedi_txt.append('    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(ObjectTracker_instance_table_map, instance);')
        gedi_txt.append('    pInstanceTable->DestroyInstance(instance);')
        gedi_txt.append('')
        gedi_txt.append('    // Clean up logging callback, if any')
        gedi_txt.append('    layer_data *my_data = get_my_data_ptr(key, layer_data_map);')
        gedi_txt.append('    if (my_data->logging_callback) {')
        gedi_txt.append('        layer_destroy_msg_callback(my_data->report_data, my_data->logging_callback);')
        gedi_txt.append('    }')
        gedi_txt.append('')
        gedi_txt.append('    layer_debug_report_destroy_instance(mid(instance));')
        gedi_txt.append('    layer_data_map.erase(pInstanceTable);')
        gedi_txt.append('')
        gedi_txt.append('    ObjectTracker_instance_table_map.erase(key);')
        gedi_txt.append('    assert(ObjectTracker_instance_table_map.size() == 0 && "Should not have any instance mappings hanging around");')
        gedi_txt.append('')
        gedi_txt.append('    loader_platform_thread_unlock_mutex(&objLock);')
        gedi_txt.append('}')
        gedi_txt.append('')
        return "\n".join(gedi_txt)

    def generate_destroy_device(self):
        gedd_txt = []
        gedd_txt.append('%s' % self.lineinfo.get())
        gedd_txt.append('void vkDestroyDevice(')
        gedd_txt.append('VkDevice device)')
        gedd_txt.append('{')
        gedd_txt.append('    loader_platform_thread_lock_mutex(&objLock);')
        gedd_txt.append('    validate_object(device, device);')
        gedd_txt.append('')
        gedd_txt.append('    destroy_obj(device, device);')
        gedd_txt.append('    // Report any remaining objects in LL')
        for o in vulkan.core.objects:
            if o in ['VkInstance', 'VkPhysicalDevice', 'VkQueue']:
                continue
            gedd_txt.append('    for (auto it = %sMap.begin(); it != %sMap.end(); ++it) {' % (o, o))
            gedd_txt.append('        OBJTRACK_NODE* pNode = it->second;')
            gedd_txt.append('        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, pNode->objType, pNode->vkObj, 0, OBJTRACK_OBJECT_LEAK, "OBJTRACK",')
            gedd_txt.append('                "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDbgObjectType(pNode->objType),')
            gedd_txt.append('                pNode->vkObj);')
            gedd_txt.append('    }')
            gedd_txt.append('')
        gedd_txt.append("    // Clean up Queue's MemRef Linked Lists")
        gedd_txt.append('    destroyQueueMemRefLists();')
        gedd_txt.append('')
        gedd_txt.append('    loader_platform_thread_unlock_mutex(&objLock);')
        gedd_txt.append('')
        gedd_txt.append('    dispatch_key key = get_dispatch_key(device);')
        gedd_txt.append('    VkLayerDispatchTable *pDisp = get_dispatch_table(ObjectTracker_device_table_map, device);')
        gedd_txt.append('    pDisp->DestroyDevice(device);')
        gedd_txt.append('    ObjectTracker_device_table_map.erase(key);')
        gedd_txt.append('    assert(ObjectTracker_device_table_map.size() == 0 && "Should not have any instance mappings hanging around");')
        gedd_txt.append('')
        gedd_txt.append('}')
        gedd_txt.append('')
        return "\n".join(gedd_txt)

    def generate_command_buffer_validates(self):
        cbv_txt = []
        cbv_txt.append('%s' % self.lineinfo.get())
        for o in ['VkPipeline', 'VkDynamicViewportState', 'VkDynamicLineWidthState', 'VkDynamicDepthBiasState', 'VkDynamicBlendState', 'VkDynamicDepthBoundsState', 'VkDynamicStencilState',
                  'VkPipelineLayout', 'VkBuffer', 'VkEvent', 'VkQueryPool', 'VkRenderPass', 'VkFramebuffer']:
            cbv_txt.append('static VkBool32 validate_object(VkCmdBuffer dispatchable_object, %s object)' % (o))
            cbv_txt.append('{')
            cbv_txt.append('    if (%sMap.find((void*)object.handle) == %sMap.end()) {' % (o, o))
            cbv_txt.append('        return log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, object.handle, 0, OBJTRACK_INVALID_OBJECT, "OBJTRACK",')
            cbv_txt.append('            "Invalid %s Object %%p", object.handle);' % (o))
            cbv_txt.append('    }')
            cbv_txt.append('    return VK_FALSE;')
            cbv_txt.append('}')
            cbv_txt.append('')
        return "\n".join(cbv_txt)

    def generate_intercept(self, proto, qual):
        if proto.name in [ 'DbgCreateMsgCallback', 'GetGlobalLayerProperties', 'GetGlobalExtensionProperties','GetPhysicalDeviceLayerProperties', 'GetPhysicalDeviceExtensionProperties' ]:
            # use default version
            return None

        # Create map of object names to object type enums of the form VkName : VkObjectTypeName
        obj_type_mapping = {base_t : base_t.replace("Vk", "VkObjectType") for base_t in vulkan.object_type_list}
        # Convert object type enum names from UpperCamelCase to UPPER_CASE_WITH_UNDERSCORES
        for objectName, objectTypeEnum in obj_type_mapping.items():
            obj_type_mapping[objectName] = ucc_to_U_C_C(objectTypeEnum);
        # Command Buffer Object doesn't follow the rule.
        obj_type_mapping['VkCmdBuffer'] = "VK_OBJECT_TYPE_COMMAND_BUFFER"
        obj_type_mapping['VkShaderModule'] = "VK_OBJECT_TYPE_SHADER_MODULE"

        explicit_object_tracker_functions = [
            "CreateInstance",
            "EnumeratePhysicalDevices",
            "GetPhysicalDeviceQueueFamilyProperties",
            "CreateDevice",
            "GetDeviceQueue",
            "QueueBindSparseImageMemory",
            "QueueBindSparseBufferMemory",
            "AllocDescriptorSets",
            "FreeDescriptorSets",
            "MapMemory",
            "UnmapMemory",
            "FreeMemory",
            "DestroySwapchainKHR"
        ]
        decl = proto.c_func(prefix="vk", attr="VKAPI")
        param0_name = proto.params[0].name
        using_line = ''
        create_line = ''
        destroy_line = ''
        # Dict below tracks params that are vk objects. Dict is "loop count"->["params w/ that loop count"] where '0' is params that aren't in an array
        loop_params = defaultdict(list) # Dict uses loop count as key to make final code generation cleaner so params shared in single loop where needed
        # TODO : For now skipping objs that can be NULL. Really should check these and have special case that allows them to be NULL
        #  or better yet, these should be encoded into an API json definition and we generate checks from there
        #  Until then, this is a dict where each func name is a list of object params that can be null (so don't need to be validated)
        #   param names may be directly passed to the function, or may be a field in a struct param
        valid_null_object_names = {'CreateGraphicsPipelines' : ['basePipelineHandle'],
                                   'CreateComputePipelines' : ['basePipelineHandle'],
                                   'BeginCommandBuffer' : ['renderPass', 'framebuffer'],
                                   'QueueSubmit' : ['fence'],
                                  }
        # TODO : A few of the skipped types are just "hard" cases that need some more work to support
        #   Need to handle NULL fences on queue submit, binding null memory, and WSI Image objects
        param_count = 'NONE' # keep track of arrays passed directly into API functions
        for p in proto.params:
            base_type = p.ty.replace('const ', '').strip('*')
            if 'count' in p.name.lower():
                param_count = p.name
            if base_type in vulkan.core.objects:
                # This is an object to potentially check for validity. First see if it's an array
                if '*' in p.ty and 'const' in p.ty and param_count != 'NONE':
                    loop_params[param_count].append(p.name)
                # Not an array, check for just a base Object that's not in exceptions
                elif '*' not in p.ty and (proto.name not in valid_null_object_names or p.name not in valid_null_object_names[proto.name]):
                    loop_params[0].append(p.name)
            elif vk_helper.is_type(base_type, 'struct'):
                struct_type = base_type
                if vk_helper.typedef_rev_dict[struct_type] in vk_helper.struct_dict:
                    struct_type = vk_helper.typedef_rev_dict[struct_type]
                for m in sorted(vk_helper.struct_dict[struct_type]):
                    if vk_helper.struct_dict[struct_type][m]['type'] in vulkan.core.objects and vk_helper.struct_dict[struct_type][m]['type'] not in ['VkPhysicalDevice', 'VkQueue', 'VkFence', 'VkImage', 'VkDeviceMemory']:
                        if proto.name not in valid_null_object_names or vk_helper.struct_dict[struct_type][m]['name'] not in valid_null_object_names[proto.name]:
                            # If we have a count and this param is a ptr w/ last letter 's', then this is a dynamically sized array param
                            #  This is not great, but gets the job done for now
                            if param_count != 'NONE' and '*' in p.ty and 's' == p.name[-1]:
                                param_name = '%s[i].%s' % (p.name, vk_helper.struct_dict[struct_type][m]['name'])
                            else:
                                param_name = '%s->%s' % (p.name, vk_helper.struct_dict[struct_type][m]['name'])
                            if vk_helper.struct_dict[struct_type][m]['dyn_array']:
                                loop_count = '%s->%s' % (p.name, vk_helper.struct_dict[struct_type][m]['array_size'])
                                loop_params[loop_count].append(param_name)
                            else:
                                if '[' in param_name: # dynamic array param, set size
                                    loop_params[param_count].append(param_name)
                                else:
                                    loop_params[0].append(param_name)
        funcs = []
        mutex_unlock = False
        funcs.append('%s\n' % self.lineinfo.get())
        if proto.name in explicit_object_tracker_functions:
            funcs.append('%s%s\n'
                     '{\n'
                     '    return explicit_%s;\n'
                     '}' % (qual, decl, proto.c_call()))
            return "".join(funcs)
        elif 'DestroyInstance' in proto.name or 'DestroyDevice' in proto.name:
            return ""
        else:
            if 'Create' in proto.name or 'Alloc' in proto.name:
                create_line =  '    loader_platform_thread_lock_mutex(&objLock);\n'
                create_line += '    if (result == VK_SUCCESS) {\n'
                create_line += '        create_obj(%s, *%s, %s);\n' % (param0_name, proto.params[-1].name, obj_type_mapping[proto.params[-1].ty.strip('*').replace('const ', '')])
                create_line += '    }\n'
                create_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
            if 'Destroy' in proto.name:
                destroy_line =  '    loader_platform_thread_lock_mutex(&objLock);\n'
#                destroy_line += '    if (result == VK_SUCCESS) {\n'
                if 'DestroyCommandBuffer' in proto.name:
                    destroy_line += '    destroy_obj(%s, %s);\n' % (proto.params[-1].name, proto.params[-1].name)
                else:
                    destroy_line += '    destroy_obj(%s, %s);\n' % (param0_name, proto.params[-1].name)
#                destroy_line += '    }\n'
                destroy_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
            if len(loop_params) > 0:
                using_line += '    VkBool32 skipCall = VK_FALSE;\n'
                if not mutex_unlock:
                    using_line += '    loader_platform_thread_lock_mutex(&objLock);\n'
                    mutex_unlock = True
                for lc in loop_params:
                    if 0 == lc: # No looping required for these params
                        for opn in loop_params[lc]:
                            if '->' in opn:
                                using_line += '    if (%s)\n' % (opn.split('-')[0])
                                using_line += '        skipCall |= validate_object(%s, %s);\n' % (param0_name, opn)
                            else:
                                using_line += '    skipCall |= validate_object(%s, %s);\n' % (param0_name, opn)
                    else:
                        base_param = loop_params[lc][0].split('-')[0].split('[')[0]
                        using_line += '    if (%s) {\n' % base_param
                        if 'setCount' == lc: # TODO : Hacky. This is one case where loop doesn't start from 0
                            using_line += '        for (uint32_t i=firstSet; i<%s; i++) {\n' % lc
                        else:
                            using_line += '        for (uint32_t i=0; i<%s; i++) {\n' % lc
                        for opn in loop_params[lc]:
                            if '[' in opn: # API func param is array
                                using_line += '            skipCall |= validate_object(%s, %s);\n' % (param0_name, opn)
                            else: # struct element is array
                                using_line += '            skipCall |= validate_object(%s, %s[i]);\n' % (param0_name, opn)
                        using_line += '        }\n'
                        using_line += '    }\n'
            if mutex_unlock:
                using_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
            if len(loop_params) > 0:
                using_line += '    if (skipCall)\n'
                if proto.ret != "void":
                    using_line += '        return VK_ERROR_VALIDATION_FAILED;\n'
                else:
                    using_line += '        return;\n'
            ret_val = ''
            stmt = ''
            if proto.ret != "void":
                ret_val = "%s result = " % proto.ret
                stmt = "    return result;\n"

            dispatch_param = proto.params[0].name
            if 'CreateInstance' in proto.name:
               dispatch_param = '*' + proto.params[1].name

            # Must use 'instance' table for these APIs, 'device' table otherwise
            table_type = ""
            if proto_is_global(proto):
                table_type = "instance"
            else:
                table_type = "device"
            funcs.append('%s%s\n'
                     '{\n'
                     '%s'
                     '    %sget_dispatch_table(ObjectTracker_%s_table_map, %s)->%s;\n'
                     '%s%s'
                     '%s'
                     '}' % (qual, decl, using_line, ret_val, table_type, dispatch_param, proto.c_call(), create_line, destroy_line, stmt))
        return "\n\n".join(funcs)

    def generate_body(self):
        self.layer_name = "ObjectTracker"
        extensions=[('wsi_enabled',
                    ['vkCreateSwapchainKHR', 'vkDestroySwapchainKHR',
                     'vkGetSwapchainImagesKHR', 'vkQueuePresentKHR'])]
        body = [self.generate_maps(),
                self.generate_procs(),
                self.generate_destroy_instance(),
                self.generate_destroy_device(),
                self.generate_command_buffer_validates(),
                self._generate_dispatch_entrypoints("VK_LAYER_EXPORT"),
                self._generate_extensions(),
                self._generate_layer_gpa_function(extensions,
                                                  instance_extensions=['msg_callback_get_proc_addr'])]
        return "\n\n".join(body)

class ThreadingSubcommand(Subcommand):
    thread_check_dispatchable_objects = [
        "VkQueue",
        "VkCmdBuffer",
    ]
    thread_check_nondispatchable_objects = [
        "VkDeviceMemory",
        "VkBuffer",
        "VkImage",
        "VkDescriptorSet",
        "VkDescriptorPool",
        "VkSemaphore"
    ]
    thread_check_object_types = {
        'VkInstance' : 'VK_OBJECT_TYPE_INSTANCE',
        'VkPhysicalDevice' : 'VK_OBJECT_TYPE_PHYSICAL_DEVICE',
        'VkDevice' : 'VK_OBJECT_TYPE_DEVICE',
        'VkQueue' : 'VK_OBJECT_TYPE_QUEUE',
        'VkCmdBuffer' : 'VK_OBJECT_TYPE_COMMAND_BUFFER',
        'VkFence' : 'VK_OBJECT_TYPE_FENCE',
        'VkDeviceMemory' : 'VK_OBJECT_TYPE_DEVICE_MEMORY',
        'VkBuffer' : 'VK_OBJECT_TYPE_BUFFER',
        'VkImage' : 'VK_OBJECT_TYPE_IMAGE',
        'VkSemaphore' : 'VK_OBJECT_TYPE_SEMAPHORE',
        'VkEvent' : 'VK_OBJECT_TYPE_EVENT',
        'VkQueryPool' : 'VK_OBJECT_TYPE_QUERY_POOL',
        'VkBufferView' : 'VK_OBJECT_TYPE_BUFFER_VIEW',
        'VkImageView' : 'VK_OBJECT_TYPE_IMAGE_VIEW',
        'VkShaderModule' : 'VK_OBJECT_TYPE_SHADER_MODULE',
        'VkShader' : 'VK_OBJECT_TYPE_SHADER',
        'VkPipelineCache' : 'VK_OBJECT_TYPE_PIPELINE_CACHE',
        'VkPipelineLayout' : 'VK_OBJECT_TYPE_PIPELINE_LAYOUT',
        'VkRenderPass' : 'VK_OBJECT_TYPE_RENDER_PASS',
        'VkPipeline' : 'VK_OBJECT_TYPE_PIPELINE',
        'VkDescriptorSetLayout' : 'VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT',
        'VkSampler' : 'VK_OBJECT_TYPE_SAMPLER',
        'VkDescriptorPool' : 'VK_OBJECT_TYPE_DESCRIPTOR_POOL',
        'VkDescriptorSet' : 'VK_OBJECT_TYPE_DESCRIPTOR_SET',
        'VkDynamicViewportState' : 'VK_OBJECT_TYPE_DYNAMIC_VIEWPORT_STATE',
        'VkDynamicLineWidthState' : 'VK_OBJECT_TYPE_DYNAMIC_LINE_WIDTH_STATE',
        'VkDynamicDepthBiasState' : 'VK_OBJECT_TYPE_DYNAMIC_DEPTH_BIAS_STATE',
        'VkDynamicBlendState' : 'VK_OBJECT_TYPE_DYNAMIC_BLEND_STATE',
        'VkDynamicDepthBoundsState' : 'VK_OBJECT_TYPE_DYNAMIC_DEPTH_BOUNDS_STATE',
        'VkDynamicStencilState' : 'VK_OBJECT_TYPE_DYNAMIC_STENCIL_STATE',
        'VkFramebuffer' : 'VK_OBJECT_TYPE_FRAMEBUFFER',
        'VkCmdPool' : 'VK_OBJECT_TYPE_CMD_POOL',
    }
    def generate_useObject(self, ty):
        obj_type = self.thread_check_object_types[ty]
        if ty in self.thread_check_dispatchable_objects:
            key = "object"
            msg_object = "reinterpret_cast<uint64_t>(object)"
        else:
            key = "object.handle"
            msg_object = "object.handle"
        header_txt = []
        header_txt.append('%s' % self.lineinfo.get())
        header_txt.append('static void useObject(const void* dispatchable_object, %s object)' % ty)
        header_txt.append('{')
        header_txt.append('    loader_platform_thread_id tid = loader_platform_get_thread_id();')
        header_txt.append('    loader_platform_thread_lock_mutex(&threadingLock);')
        header_txt.append('    if (%sObjectsInUse.find(%s) == %sObjectsInUse.end()) {' % (ty, key, ty))
        header_txt.append('        %sObjectsInUse[%s] = tid;' % (ty, key))
        header_txt.append('    } else {')
        header_txt.append('        if (%sObjectsInUse[%s] != tid) {' % (ty, key))
        header_txt.append('            log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, %s, %s,' % (obj_type, msg_object))
        header_txt.append('                /*location*/ 0, THREADING_CHECKER_MULTIPLE_THREADS, "THREADING",')
        header_txt.append('                "THREADING ERROR : object of type %s is simultaneously used in thread %%ld and thread %%ld",' % (ty))
        header_txt.append('                %sObjectsInUse[%s], tid);' % (ty, key))
        header_txt.append('            // Wait for thread-safe access to object')
        header_txt.append('            while (%sObjectsInUse.find(%s) != %sObjectsInUse.end()) {' % (ty, key, ty))
        header_txt.append('                loader_platform_thread_cond_wait(&threadingCond, &threadingLock);')
        header_txt.append('            }')
        header_txt.append('            %sObjectsInUse[%s] = tid;' % (ty, key))
        header_txt.append('        } else {')
        header_txt.append('            log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, %s, %s,' % (obj_type, msg_object))
        header_txt.append('                /*location*/ 0, THREADING_CHECKER_MULTIPLE_THREADS, "THREADING",')
        header_txt.append('                "THREADING ERROR : object of type %s is recursively used in thread %%ld",' % (ty))
        header_txt.append('                tid);')
        header_txt.append('        }')
        header_txt.append('    }')
        header_txt.append('    loader_platform_thread_unlock_mutex(&threadingLock);')
        header_txt.append('}')
        return "\n".join(header_txt)
    def generate_finishUsingObject(self, ty):
        if ty in self.thread_check_dispatchable_objects:
            key = "object"
        else:
            key = "object.handle"
        header_txt = []
        header_txt.append('%s' % self.lineinfo.get())
        header_txt.append('static void finishUsingObject(%s object)' % ty)
        header_txt.append('{')
        header_txt.append('    // Object is no longer in use')
        header_txt.append('    loader_platform_thread_lock_mutex(&threadingLock);')
        header_txt.append('    %sObjectsInUse.erase(%s);' % (ty, key))
        header_txt.append('    loader_platform_thread_cond_broadcast(&threadingCond);')
        header_txt.append('    loader_platform_thread_unlock_mutex(&threadingLock);')
        header_txt.append('}')
        return "\n".join(header_txt)
    def generate_header(self):
        header_txt = []
        header_txt.append('%s' % self.lineinfo.get())
        header_txt.append('#include <stdio.h>')
        header_txt.append('#include <stdlib.h>')
        header_txt.append('#include <string.h>')
        header_txt.append('#include <unordered_map>')
        header_txt.append('#include "vk_loader_platform.h"')
        header_txt.append('#include "vk_layer.h"')
        header_txt.append('#include "threading.h"')
        header_txt.append('#include "vk_layer_config.h"')
        header_txt.append('#include "vk_layer_extension_utils.h"')
        header_txt.append('#include "vk_enum_validate_helper.h"')
        header_txt.append('#include "vk_struct_validate_helper.h"')
        header_txt.append('//The following is #included again to catch certain OS-specific functions being used:')
        header_txt.append('#include "vk_loader_platform.h"')
        header_txt.append('#include "vk_layer_msg.h"')
        header_txt.append('#include "vk_layer_table.h"')
        header_txt.append('#include "vk_layer_logging.h"')
        header_txt.append('')
        header_txt.append('')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(initOnce);')
        header_txt.append('')
        header_txt.append('using namespace std;')
        for ty in self.thread_check_dispatchable_objects:
            header_txt.append('static unordered_map<%s, loader_platform_thread_id> %sObjectsInUse;' % (ty, ty))
        for ty in self.thread_check_nondispatchable_objects:
            header_txt.append('static unordered_map<uint64_t, loader_platform_thread_id> %sObjectsInUse;' % ty)
        header_txt.append('static int threadingLockInitialized = 0;')
        header_txt.append('static loader_platform_thread_mutex threadingLock;')
        header_txt.append('static loader_platform_thread_cond threadingCond;')
        header_txt.append('%s' % self.lineinfo.get())
        for ty in self.thread_check_dispatchable_objects + self.thread_check_nondispatchable_objects:
            header_txt.append(self.generate_useObject(ty))
            header_txt.append(self.generate_finishUsingObject(ty))
        header_txt.append('%s' % self.lineinfo.get())
        return "\n".join(header_txt)

    def generate_intercept(self, proto, qual):
        if proto.name in [ 'DbgCreateMsgCallback' ]:
            # use default version
            return None
        decl = proto.c_func(prefix="vk", attr="VKAPI")
        ret_val = ''
        stmt = ''
        funcs = []
        table = 'device'
        if proto.ret != "void":
            ret_val = "%s result = " % proto.ret
            stmt = "    return result;\n"
        if proto_is_global(proto):
           table = 'instance'

        # Memory range calls are special in needed thread checking within structs
        if proto.name in ["FlushMappedMemoryRanges","InvalidateMappedMemoryRanges"]:
            funcs.append('%s' % self.lineinfo.get())
            funcs.append('%s%s\n' % (qual, decl) +
                     '{\n'
                     '    for (uint32_t i=0; i<memRangeCount; i++) {\n'
                     '        useObject((const void *) %s, pMemRanges[i].mem);\n' % proto.params[0].name +
                     '    }\n'
                     '    VkLayerDispatchTable *pDeviceTable = get_dispatch_table(Threading_%s_table_map, %s);\n' % (table, proto.params[0].name) +
                     '    %s pDeviceTable->%s;\n' % (ret_val, proto.c_call()) +
                     '    for (uint32_t i=0; i<memRangeCount; i++) {\n'
                     '        finishUsingObject(pMemRanges[i].mem);\n'
                     '    }\n'
                     '%s' % (stmt) +
                     '}')
            return "\n".join(funcs)
        # All functions that do a Get are thread safe
        if 'Get' in proto.name:
            return None
        # All WSI functions are thread safe
        if 'KHR' in proto.name:
            return None
        # Initialize in early calls
        if proto.name == "CreateDevice":
            funcs.append('%s' % self.lineinfo.get())
            funcs.append('%s%s\n' % (qual, decl) +
                     '{\n'
                     '    VkLayerDispatchTable *pDeviceTable = get_dispatch_table(Threading_device_table_map, (void *) *pDevice);\n'
                     '    VkResult result = pDeviceTable->%s;\n' % (proto.c_call()) +
                     '    if (result == VK_SUCCESS) {\n'
                     '        layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(%s), layer_data_map);\n' % proto.params[0].name +
                     '        layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);\n'
                     '        my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);\n'
                     '    }\n'
                     '\n'
                     '    return result;'
                     '}')
            return "\n".join(funcs)
        elif proto.params[0].ty == "VkPhysicalDevice":
            return None
        # Functions changing command buffers need thread safe use of first parameter
        if proto.params[0].ty == "VkCmdBuffer":
            funcs.append('%s' % self.lineinfo.get())
            funcs.append('%s%s\n' % (qual, decl) +
                     '{\n'
                     '    useObject((const void *) %s, %s);\n' % (proto.params[0].name, proto.params[0].name) +
                     '    VkLayerDispatchTable *pDeviceTable = get_dispatch_table(Threading_%s_table_map, %s);\n' % (table, proto.params[0].name) +
                     '    %spDeviceTable->%s;\n' % (ret_val, proto.c_call()) +
                     '    finishUsingObject(%s);\n' % proto.params[0].name +
                     '%s' % stmt +
                     '}')
            return "\n".join(funcs)
        # Non-Cmd functions that do a Wait are thread safe
        if 'Wait' in proto.name:
            return None
        # Watch use of certain types of objects passed as any parameter
        checked_params = []
        for param in proto.params:
            if param.ty in self.thread_check_dispatchable_objects or param.ty in self.thread_check_nondispatchable_objects:
                checked_params.append(param)
        if proto.name == "DestroyDevice":
            funcs.append('%s%s\n' % (qual, decl) +
                         '{\n'
                         '    dispatch_key key = get_dispatch_key(device);\n'
                         '    VkLayerDispatchTable *pDeviceTable = get_dispatch_table(Threading_%s_table_map, %s);\n' % (table, proto.params[0].name) +
                         '    %spDeviceTable->%s;\n' % (ret_val, proto.c_call()) +
                         '    Threading_device_table_map.erase(key);\n'
                         '}\n')
            return "\n".join(funcs);
        elif proto.name == "DestroyInstance":
            funcs.append('%s%s\n' % (qual, decl) +
                         '{\n'
                         '    dispatch_key key = get_dispatch_key(instance);\n'
                         '    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(Threading_instance_table_map, %s);\n' % proto.params[0].name +
                         '    %spInstanceTable->%s;\n' % (ret_val, proto.c_call()) +
                         '    destroy_instance_dispatch_table(key);\n'
                         '\n'
                         '    // Clean up logging callback, if any\n'
                         '    layer_data *my_data = get_my_data_ptr(key, layer_data_map);\n'
                         '    if (my_data->logging_callback) {\n'
                         '        layer_destroy_msg_callback(my_data->report_data, my_data->logging_callback);\n'
                         '    }\n'
                         '\n'
                         '    layer_debug_report_destroy_instance(my_data->report_data);\n'
                         '    layer_data_map.erase(pInstanceTable);\n'
                         '\n'
                         '    Threading_instance_table_map.erase(key);\n'
                         '}\n')
            return "\n".join(funcs);
        elif proto.name == "CreateInstance":
            funcs.append('%s%s\n'
                         '{\n'
                         '    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(Threading_instance_table_map, *pInstance);\n'
                         '    VkResult result = pInstanceTable->CreateInstance(pCreateInfo, pInstance);\n'
                         '\n'
                         '    if (result == VK_SUCCESS) {\n'
                         '        layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);\n'
                         '        my_data->report_data = debug_report_create_instance(\n'
                         '                                   pInstanceTable,\n'
                         '                                   *pInstance,\n'
                         '                                   pCreateInfo->extensionCount,\n'
                         '                                   pCreateInfo->ppEnabledExtensionNames);\n'
                         '        initThreading(my_data);\n'
                         '    }\n'
                         '    return result;\n'
                         '}\n' % (qual, decl))
            return "\n".join(funcs);
        if len(checked_params) == 0:
            return None
        # Surround call with useObject and finishUsingObject for each checked_param
        funcs.append('%s' % self.lineinfo.get())
        funcs.append('%s%s' % (qual, decl))
        funcs.append('{')
        for param in checked_params:
            funcs.append('    useObject((const void *) %s, %s);' % (proto.params[0].name, param.name))
        funcs.append('    VkLayerDispatchTable *pDeviceTable = get_dispatch_table(Threading_%s_table_map, %s);' % (table, proto.params[0].name));
        funcs.append('    %spDeviceTable->%s;' % (ret_val, proto.c_call()))
        for param in checked_params:
            funcs.append('    finishUsingObject(%s);' % param.name)
        funcs.append('%s'
                 '}' % stmt)
        return "\n".join(funcs)

    def generate_body(self):
        self.layer_name = "Threading"
        body = [self._generate_new_layer_initialization(True, lockname='threading', condname='threading'),
                self._generate_dispatch_entrypoints("VK_LAYER_EXPORT"),
                self._generate_layer_gpa_function(extensions=[],
                                                  instance_extensions=['msg_callback_get_proc_addr']),
                self._gen_create_msg_callback(),
                self._gen_destroy_msg_callback()]
        return "\n\n".join(body)

def main():
    subcommands = {
            "layer-funcs" : LayerFuncsSubcommand,
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
