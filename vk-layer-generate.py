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

def proto_is_global(proto):
    if proto.params[0].ty == "VkInstance" or proto.params[0].ty == "VkPhysicalDevice" or proto.name == "CreateInstance" or proto.name == "GetGlobalExtensionCount" or proto.name == "GetGlobalExtensionProperties" or proto.name == "GetPhysicalDeviceExtensionCount" or proto.name == "GetPhysicalDeviceExtensionProperties":
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

    def _gen_create_msg_callback(self):
        r_body = []
        r_body.append('%s' % self.lineinfo.get())
        r_body.append('VK_LAYER_EXPORT VkResult VKAPI vkDbgCreateMsgCallback(VkInstance instance, VkFlags msgFlags, const PFN_vkDbgMsgCallback pfnMsgCallback, void* pUserData, VkDbgMsgCallback* pMsgCallback)')
        r_body.append('{')
        # Switch to this code section for the new per-instance storage and debug callbacks
        if self.layer_name == 'ObjectTracker':
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
        if self.layer_name == 'ObjectTracker':
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
        if layer == 'APIDump' or layer == 'Generic':
            ggep_body.append('%s' % self.lineinfo.get())
            ggep_body.append('#define LAYER_EXT_ARRAY_SIZE 1')
            ggep_body.append('static const VkExtensionProperties layerExts[LAYER_EXT_ARRAY_SIZE] = {')
            ggep_body.append('    {')
            ggep_body.append('        VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES,')
            ggep_body.append('        "%s",' % layer)
            ggep_body.append('        0x10,')
            ggep_body.append('        "layer: %s",' % layer)
            ggep_body.append('    }')
            ggep_body.append('};')
        else:
            ggep_body.append('%s' % self.lineinfo.get())
            ggep_body.append('#define LAYER_EXT_ARRAY_SIZE 2')
            ggep_body.append('static const VkExtensionProperties layerExts[LAYER_EXT_ARRAY_SIZE] = {')
            ggep_body.append('    {')
            ggep_body.append('        VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES,')
            ggep_body.append('        "%s",' % layer)
            ggep_body.append('        0x10,')
            ggep_body.append('        "layer: %s",' % layer)
            ggep_body.append('    },')
            ggep_body.append('    {')
            ggep_body.append('        VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES,')
            ggep_body.append('        "Validation",')
            ggep_body.append('        0x10,')
            ggep_body.append('        "layer: %s",' % layer)
            ggep_body.append('    }')
            ggep_body.append('};')
        ggep_body.append('')
        ggep_body.append('%s' % self.lineinfo.get())

        ggep_body.append('')
        ggep_body.append('VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionProperties(uint32_t extensionIndex,  VkExtensionProperties* pProperties)')
        ggep_body.append('{')
        ggep_body.append('    if (extensionIndex >= LAYER_EXT_ARRAY_SIZE)')
        ggep_body.append('        return VK_ERROR_INVALID_VALUE;')
        ggep_body.append('    memcpy(pProperties, &layerExts[extensionIndex], sizeof(VkExtensionProperties));')
        ggep_body.append('    return VK_SUCCESS;')
        ggep_body.append('}')
        return "\n".join(ggep_body)

    def _gen_layer_get_global_extension_count(self, layer="Generic"):
        ggec_body = []
        ggec_body.append('VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionCount(uint32_t* pCount)')
        ggec_body.append('{')
        ggec_body.append('    *pCount = LAYER_EXT_ARRAY_SIZE;')
        ggec_body.append('     return VK_SUCCESS;')
        ggec_body.append('}')
        return "\n".join(ggec_body)

    def _gen_layer_get_physical_device_extension_props(self, layer="Generic"):
        gpdep_body = []
        if layer == 'APIDump' or layer == 'Generic':
            gpdep_body.append('#define LAYER_DEV_EXT_ARRAY_SIZE 1')
            gpdep_body.append('static const VkExtensionProperties layerDevExts[LAYER_DEV_EXT_ARRAY_SIZE] = {')
            gpdep_body.append('    {')
            gpdep_body.append('        VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES,')
            gpdep_body.append('        "%s",' % layer)
            gpdep_body.append('        0x10,')
            gpdep_body.append('        "layer: %s",' % layer)
            gpdep_body.append('    }')
            gpdep_body.append('};')
        elif layer == 'ObjectTracker':
            gpdep_body.append('#define LAYER_DEV_EXT_ARRAY_SIZE 2')
            gpdep_body.append('static const VkExtensionProperties layerDevExts[LAYER_DEV_EXT_ARRAY_SIZE] = {')
            gpdep_body.append('    {')
            gpdep_body.append('        VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES,')
            gpdep_body.append('        "%s",' % layer)
            gpdep_body.append('        0x10,')
            gpdep_body.append('        "layer: %s",' % layer)
            gpdep_body.append('    },')
            gpdep_body.append('    {')
            gpdep_body.append('        VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES,')
            gpdep_body.append('        "Validation",')
            gpdep_body.append('        0x10,')
            gpdep_body.append('        "layer: %s",' % layer)
            gpdep_body.append('    }')
            gpdep_body.append('};')
        else:
            gpdep_body.append('#define LAYER_DEV_EXT_ARRAY_SIZE 2')
            gpdep_body.append('static const VkExtensionProperties layerDevExts[LAYER_DEV_EXT_ARRAY_SIZE] = {')
            gpdep_body.append('    {')
            gpdep_body.append('        VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES,')
            gpdep_body.append('        "%s",' % layer)
            gpdep_body.append('        0x10,')
            gpdep_body.append('        "layer: %s",' % layer)
            gpdep_body.append('    },')
            gpdep_body.append('    {')
            gpdep_body.append('        VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES,')
            gpdep_body.append('        "Validation",')
            gpdep_body.append('        0x10,')
            gpdep_body.append('        "layer: %s",' % layer)
            gpdep_body.append('    }')
            gpdep_body.append('};')
        gpdep_body.append('VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceExtensionProperties(VkPhysicalDevice physicalDevice, uint32_t extensionIndex,  VkExtensionProperties* pProperties)')
        gpdep_body.append('{')
        gpdep_body.append('    if (extensionIndex >= LAYER_DEV_EXT_ARRAY_SIZE)')
        gpdep_body.append('        return VK_ERROR_INVALID_VALUE;')
        gpdep_body.append('    memcpy(pProperties, &layerDevExts[extensionIndex], sizeof(VkExtensionProperties));')
        gpdep_body.append('    return VK_SUCCESS;')
        gpdep_body.append('}')
        gpdep_body.append('')
        return "\n".join(gpdep_body)

    def _gen_layer_get_physical_device_extension_count(self, layer="Generic"):
        gpdec_body = []
        gpdec_body.append('VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceExtensionCount(VkPhysicalDevice physicalDevice, uint32_t* pCount)')
        gpdec_body.append('{')
        gpdec_body.append('    *pCount = LAYER_DEV_EXT_ARRAY_SIZE;')
        gpdec_body.append('     return VK_SUCCESS;')
        gpdec_body.append('}')
        gpdec_body.append('')
        return "\n".join(gpdec_body)


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
                    elif 'GetGlobalExtensionCount' == proto.name:
                        intercept = self._gen_layer_get_global_extension_count(self.layer_name)
                    elif 'GetPhysicalDeviceExtensionProperties' == proto.name:
                        intercept = self._gen_layer_get_physical_device_extension_props(self.layer_name)
                    elif 'GetPhysicalDeviceExtensionCount' == proto.name:
                        intercept = self._gen_layer_get_physical_device_extension_count(self.layer_name)


                if intercept is not None:
                    funcs.append(intercept)
                    if not "WSI" in proto.name:
                        intercepted.append(proto)

        prefix="vk"
        lookups = []
        for proto in intercepted:
            lookups.append("if (!strcmp(name, \"%s\"))" % proto.name)
            lookups.append("    return (void*) %s%s;" %
                    (prefix, proto.name))

        # add customized layer_intercept_proc
        body = []
        body.append('%s' % self.lineinfo.get())
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
            if proto.name == "CreateDevice":
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
        exts.append('%s' % self.lineinfo.get())
        exts.append(self._gen_create_msg_callback())
        exts.append(self._gen_destroy_msg_callback())
        return "\n".join(exts)

    def _generate_layer_gpa_function(self, extensions=[], instance_extensions=[]):
        func_body = []
#
# New style fo GPA Functions for the new layer_data/layer_logging changes
#
        if self.layer_name == 'ObjectTracker':
            func_body.append("VK_LAYER_EXPORT void* VKAPI vkGetDeviceProcAddr(VkDevice device, const char* funcName)\n"
                             "{\n"
                             "    void* addr;\n"
                             "    if (device == VK_NULL_HANDLE) {\n"
                             "        return NULL;\n"
                             "    }\n"
                             "    /* loader uses this to force layer initialization; device object is wrapped */\n"
                             "    if (!strcmp(\"vkGetDeviceProcAddr\", funcName)) {\n"
                             "        initDeviceTable(%s_device_table_map, (const VkBaseLayerObject *) device);\n"
                             "        return (void *) vkGetDeviceProcAddr;\n"
                             "    }\n\n"
                             "    addr = layer_intercept_proc(funcName);\n"
                             "    if (addr)\n"
                             "        return addr;" % self.layer_name)
            if 0 != len(extensions):
                for (ext_enable, ext_list) in extensions:
                    extra_space = ""
                    if 0 != len(ext_enable):
                        func_body.append('    if (deviceExtMap.size() == 0 || deviceExtMap[get_dispatch_table(ObjectTracker_device_table_map, device)].%s)' % ext_enable)
                        func_body.append('    {')
                        extra_space = "    "
                    for ext_name in ext_list:
                        func_body.append('    %sif (!strcmp("%s", funcName))\n'
                                         '        %sreturn reinterpret_cast<void*>(%s);' % (extra_space, ext_name, extra_space, ext_name))
                    if 0 != len(ext_enable):
                        func_body.append('    }\n')
            func_body.append("\n    if (get_dispatch_table(%s_device_table_map, device)->GetDeviceProcAddr == NULL)\n"
                             "        return NULL;\n"
                             "    return get_dispatch_table(%s_device_table_map, device)->GetDeviceProcAddr(device, funcName);\n"
                             "}\n" % (self.layer_name, self.layer_name))
            func_body.append("VK_LAYER_EXPORT void* VKAPI vkGetInstanceProcAddr(VkInstance instance, const char* funcName)\n"
                             "{\n"
                             "    void* addr;\n"
                             "    if (instance == VK_NULL_HANDLE) {\n"
                             "        return NULL;\n"
                             "    }\n"
                             "    /* loader uses this to force layer initialization; instance object is wrapped */\n"
                             "    if (!strcmp(\"vkGetInstanceProcAddr\", funcName)) {\n"
                             "        initInstanceTable(%s_instance_table_map, (const VkBaseLayerObject *) instance);\n"
                             "        return (void *) vkGetInstanceProcAddr;\n"
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
            func_body.append("VK_LAYER_EXPORT void* VKAPI vkGetDeviceProcAddr(VkDevice device, const char* funcName)\n"
                             "{\n"
                             "    void* addr;\n"
                             "    if (device == VK_NULL_HANDLE) {\n"
                             "        return NULL;\n"
                             "    }\n"
                             "    loader_platform_thread_once(&initOnce, init%s);\n\n"
                             "    /* loader uses this to force layer initialization; device object is wrapped */\n"
                             "    if (!strcmp(\"vkGetDeviceProcAddr\", funcName)) {\n"
                             "        initDeviceTable((const VkBaseLayerObject *) device);\n"
                             "        return (void *) vkGetDeviceProcAddr;\n"
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
                                         '            return reinterpret_cast<void*>(%s);' % (extra_space, ext_name, ext_name))
                    if 0 != len(ext_enable):
                        func_body.append('    }')
            func_body.append('%s' % self.lineinfo.get())
            func_body.append("    {\n"
                             "        if (pDisp->GetDeviceProcAddr == NULL)\n"
                             "            return NULL;\n"
                             "        return pDisp->GetDeviceProcAddr(device, funcName);\n"
                             "    }\n"
                             "}\n")
            func_body.append("VK_LAYER_EXPORT void* VKAPI vkGetInstanceProcAddr(VkInstance instance, const char* funcName)\n"
                             "{\n"
                             "    void* addr;\n"
                             "    if (instance == VK_NULL_HANDLE) {\n"
                             "        return NULL;\n"
                             "    }\n"
                             "    loader_platform_thread_once(&initOnce, init%s);\n\n"
                             "    /* loader uses this to force layer initialization; instance object is wrapped */\n"
                             "    if (!strcmp(\"vkGetInstanceProcAddr\", funcName)) {\n"
                             "        initInstanceTable((const VkBaseLayerObject *) instance);\n"
                             "        return (void *) vkGetInstanceProcAddr;\n"
                             "    }\n\n"
                             "    addr = layer_intercept_instance_proc(funcName);\n"
                             "    if (addr)\n"
                             "        return addr;" % self.layer_name)
            if 0 != len(instance_extensions):
                for ext_name in instance_extensions:
                    func_body.append("    {\n"
                                     "        void *fptr;\n"
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

class LayerFuncsSubcommand(Subcommand):
    def generate_header(self):
        return '#include <vk_layer.h>\n#include "loader.h"'

    def generate_body(self):
        return self._generate_dispatch_entrypoints("static")

class LayerDispatchSubcommand(Subcommand):
    def generate_header(self):
        return '#include "layer_wrappers.h"'

    def generate_body(self):
        return self._generate_layer_initialization()

class GenericLayerSubcommand(Subcommand):
    def generate_header(self):
        gen_header = []
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
        gen_header.append('')
        gen_header.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(initOnce);')
        gen_header.append('struct devExts {')
        gen_header.append('    bool wsi_lunarg_enabled;')
        gen_header.append('};')
        gen_header.append('static std::unordered_map<void *, struct devExts>     deviceExtMap;')
        gen_header.append('')
        gen_header.append('static void createDeviceRegisterExtensions(const VkDeviceCreateInfo* pCreateInfo, VkDevice device)')
        gen_header.append('{')
        gen_header.append('    uint32_t i, ext_idx;')
        gen_header.append('    VkLayerDispatchTable *pDisp  = device_dispatch_table(device);')
        gen_header.append('    deviceExtMap[pDisp].wsi_lunarg_enabled = false;')
        gen_header.append('    for (i = 0; i < pCreateInfo->extensionCount; i++) {')
        gen_header.append('        if (strcmp(pCreateInfo->pEnabledExtensions[i].name, VK_WSI_LUNARG_EXTENSION_NAME) == 0)')
        gen_header.append('            deviceExtMap[pDisp].wsi_lunarg_enabled = true;')
        gen_header.append('')
        gen_header.append('    }')
        gen_header.append('}')
        gen_header.append('')
        return "\n".join(gen_header)
    def generate_intercept(self, proto, qual):
        if proto.name in [ 'GetGlobalExtensionCount', 'GetGlobalExtensionProperties', 'GetPhysicalDeviceExtensionCount', 'GetPhysicalDeviceExtensionProperties' ]:
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
                     '    layerCbMsg(VK_DBG_REPORT_INFO_BIT,VK_OBJECT_TYPE_PHYSICAL_DEVICE, gpu, 0, 0, (char *) "GENERIC", (char *) str);\n'
                     '    %sdevice_dispatch_table(*pDevice)->%s;\n'
                     '    if (result == VK_SUCCESS) {\n'
                     '        enable_debug_report(pCreateInfo->extensionCount, pCreateInfo->pEnabledExtensions);\n'
                     '        createDeviceRegisterExtensions(pCreateInfo, *pDevice);\n'
                     '    }\n'
                     '    sprintf(str, "Completed layered %s\\n");\n'
                     '    layerCbMsg(VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_PHYSICAL_DEVICE, gpu, 0, 0, (char *) "GENERIC", (char *) str);\n'
                     '    fflush(stdout);\n'
                     '    %s'
                     '}' % (qual, decl, proto.name, ret_val, proto.c_call(), proto.name, stmt))
        elif proto.name == "DestroyDevice":
            funcs.append('%s' % self.lineinfo.get())
            funcs.append('%s%s\n'
                         '{\n'
                         '    dispatch_key key = get_dispatch_key(device);\n'
                         '    VkLayerDispatchTable *pDisp  =  device_dispatch_table(device);\n'
                         '    VkResult res = pDisp->DestroyDevice(device);\n'
                         '    deviceExtMap.erase(pDisp);\n'
                         '    destroy_device_dispatch_table(key);\n'
                         '    return res;\n'
                         '}\n' % (qual, decl))
        elif proto.name == "DestroyInstance":
            funcs.append('%s' % self.lineinfo.get())
            funcs.append('%s%s\n'
                         '{\n'
                         '    dispatch_key key = get_dispatch_key(instance);\n'
                         '    VkResult res = instance_dispatch_table(instance)->DestroyInstance(instance);\n'
                         '    destroy_instance_dispatch_table(key);\n'
                         '    return res;\n'
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
        extensions=[('wsi_lunarg_enabled', 
                     ['vkCreateSwapChainWSI', 'vkDestroySwapChainWSI',
                      'vkGetSwapChainInfoWSI', 'vkQueuePresentWSI'])]
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
        header_txt.append('%s' % self.lineinfo.get())
        header_txt.append('#include "vk_loader_platform.h"')
        header_txt.append('#include "vk_layer.h"')
        header_txt.append('#include "vk_struct_string_helper_cpp.h"')
        header_txt.append('#include "vk_layer_table.h"')
        header_txt.append('#include <unordered_map>')
        header_txt.append('')
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
        header_txt.append('    bool wsi_lunarg_enabled;')
        header_txt.append('};')
        header_txt.append('')
        header_txt.append('static std::unordered_map<void *, struct devExts>     deviceExtMap;')
        header_txt.append('')
        header_txt.append('static void createDeviceRegisterExtensions(const VkDeviceCreateInfo* pCreateInfo, VkDevice device)')
        header_txt.append('{')
        header_txt.append('    uint32_t i, ext_idx;')
        header_txt.append('    VkLayerDispatchTable *pDisp  = device_dispatch_table(device);')
        header_txt.append('    deviceExtMap[pDisp].wsi_lunarg_enabled = false;')
        header_txt.append('    for (i = 0; i < pCreateInfo->extensionCount; i++) {')
        header_txt.append('        if (strcmp(pCreateInfo->pEnabledExtensions[i].name, VK_WSI_LUNARG_EXTENSION_NAME) == 0)')
        header_txt.append('            deviceExtMap[pDisp].wsi_lunarg_enabled = true;')
        header_txt.append('')
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
        if proto.name in [ 'GetGlobalExtensionCount','GetGlobalExtensionProperties','GetPhysicalDeviceExtensionCount','GetPhysicalDeviceExtensionProperties']:
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
        extensions=[('wsi_lunarg_enabled',
                    ['vkCreateSwapChainWSI', 'vkDestroySwapChainWSI',
                    'vkGetSwapChainInfoWSI', 'vkQueuePresentWSI'])]
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

    def generate_intercept(self, proto, qual):
        if proto.name in [ 'DbgCreateMsgCallback', 'GetGlobalExtensionCount', 'GetGlobalExtensionProperties','GetPhysicalDeviceExtensionCount', 'GetPhysicalDeviceExtensionProperties' ]:
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
            "DestroyInstance",
            "GetPhysicalDeviceQueueProperties",
            "CreateDevice",
            "DestroyDevice",
            "GetDeviceQueue",
            "QueueSubmit",
            "DestroyObject",
            "GetObjectMemoryRequirements",
            "QueueBindSparseImageMemory",
            "QueueBindSparseBufferMemory",
            "GetFenceStatus",
            "WaitForFences",
            "AllocDescriptorSets",
            "MapMemory",
            "UnmapMemory",
            "FreeMemory",
            "DestroySwapChainWSI"
        ]
        decl = proto.c_func(prefix="vk", attr="VKAPI")
        param0_name = proto.params[0].name
        using_line = ''
        create_line = ''
        object_params = {} # dict of parameters that are VkObject types mapping to the size of array types or '0' if not array
        # TODO : Should also check through struct params & add any objects embedded in struct chains
        # TODO : A few of the skipped types are just "hard" cases that need some more work to support
        #   Need to handle NULL fences on queue submit, binding null memory, and WSI Image objects
        for p in proto.params:
            if p.ty in vulkan.core.objects and p.ty not in ['VkPhysicalDevice', 'VkQueue', 'VkFence', 'VkImage', 'VkDeviceMemory']:
                object_params[p.name] = 0
            elif vk_helper.is_type(p.ty.replace('const ', '').strip('*'), 'struct'):
                struct_type = p.ty.replace('const ', '').strip('*')
                if vk_helper.typedef_rev_dict[struct_type] in vk_helper.struct_dict:
                    struct_type = vk_helper.typedef_rev_dict[struct_type]
                for m in sorted(vk_helper.struct_dict[struct_type]):
                    if vk_helper.struct_dict[struct_type][m]['type'] in vulkan.core.objects and vk_helper.struct_dict[struct_type][m]['type'] not in ['VkPhysicalDevice', 'VkQueue', 'VkFence', 'VkImage', 'VkDeviceMemory']:
                        param_name = '%s->%s' % (p.name, vk_helper.struct_dict[struct_type][m]['name'])
                        object_params[param_name] = {}
                        if vk_helper.struct_dict[struct_type][m]['dyn_array']:
                            object_params[param_name] = '%s->%s' % (p.name, vk_helper.struct_dict[struct_type][m]['array_size'])
                        else:
                            object_params[param_name] = 0
        funcs = []
        mutex_unlock = False
        if proto.name in explicit_object_tracker_functions:
            funcs.append('%s%s\n'
                     '{\n'
                     '    return explicit_%s;\n'
                     '}' % (qual, decl, proto.c_call()))
            return "".join(funcs)
        else:
            if 'Create' in proto.name or 'Alloc' in proto.name:
                create_line =  '    loader_platform_thread_lock_mutex(&objLock);\n'
                create_line += '    if (result == VK_SUCCESS) {\n'
                create_line += '        create_obj(%s, *%s, %s);\n' % (param0_name, proto.params[-1].name, obj_type_mapping[proto.params[-1].ty.strip('*').replace('const ', '')])
                create_line += '    }\n'
                create_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
            if len(object_params) > 0:
                if not mutex_unlock:
                    using_line += '    loader_platform_thread_lock_mutex(&objLock);\n'
                    mutex_unlock = True
                for opn in object_params:
                    if 0 != object_params[opn]:
                        using_line += '    for (uint32_t i=0; i<%s; i++)\n' % object_params[opn]
                        using_line += '        validate_object(%s, %s[i]);\n' % (param0_name, opn)
                    else:
                        if '->' in opn:
                            using_line += '    if (%s)\n' % (opn.split('-')[0])
                            using_line += '        validate_object(%s, %s);\n' % (param0_name, opn)
                        else:
                            using_line += '    validate_object(%s, %s);\n' % (param0_name, opn)
            if mutex_unlock:
                using_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
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
                     '%s'
                     '%s'
                     '}' % (qual, decl, using_line, ret_val, table_type, dispatch_param, proto.c_call(), create_line, stmt))
        return "\n\n".join(funcs)

    def generate_body(self):
        self.layer_name = "ObjectTracker"
        extensions=[('wsi_lunarg_enabled',
                    ['vkCreateSwapChainWSI', 'vkDestroySwapChainWSI',
                     'vkGetSwapChainInfoWSI', 'vkQueuePresentWSI']),
                    ('',
                    ['objTrackGetObjectsCount', 'objTrackGetObjects',
                     'objTrackGetObjectsOfTypeCount', 'objTrackGetObjectsOfType'])]
        body = [self._generate_dispatch_entrypoints("VK_LAYER_EXPORT"),
                self._generate_extensions(),
                self._generate_layer_gpa_function(extensions,
                                                  instance_extensions=['msg_callback_get_proc_addr'])]
        return "\n\n".join(body)

class ThreadingSubcommand(Subcommand):
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
        header_txt.append('#include "vk_enum_validate_helper.h"')
        header_txt.append('#include "vk_struct_validate_helper.h"')
        header_txt.append('//The following is #included again to catch certain OS-specific functions being used:')
        header_txt.append('#include "vk_loader_platform.h"\n')
        header_txt.append('#include "vk_layer_msg.h"\n')
        header_txt.append('#include "vk_layer_table.h"\n')
        header_txt.append('')
        header_txt.append('')
        header_txt.append('static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(initOnce);')
        header_txt.append('')
        header_txt.append('using namespace std;')
        header_txt.append('static unordered_map<int, void*> proxy_objectsInUse;\n')
        header_txt.append('static unordered_map<VkObject, loader_platform_thread_id> objectsInUse;\n')
        header_txt.append('static int threadingLockInitialized = 0;')
        header_txt.append('static loader_platform_thread_mutex threadingLock;')
        header_txt.append('static loader_platform_thread_cond threadingCond;')
        header_txt.append('static int printLockInitialized = 0;')
        header_txt.append('static loader_platform_thread_mutex printLock;\n')
        header_txt.append('%s' % self.lineinfo.get())
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
        header_txt.append('            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, 0, 0, THREADING_CHECKER_MULTIPLE_THREADS, "THREADING", str);')
        header_txt.append('            // Wait for thread-safe access to object')
        header_txt.append('            while (objectsInUse.find(object) != objectsInUse.end()) {')
        header_txt.append('                loader_platform_thread_cond_wait(&threadingCond, &threadingLock);')
        header_txt.append('            }')
        header_txt.append('            objectsInUse[object] = tid;')
        header_txt.append('        } else {')
        header_txt.append('            char str[1024];')
        header_txt.append('            sprintf(str, "THREADING ERROR : object of type %s is recursively used in thread %ld", type, tid);')
        header_txt.append('            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, 0, 0, THREADING_CHECKER_SINGLE_THREAD_REUSE, "THREADING", str);')
        header_txt.append('        }')
        header_txt.append('    }')
        header_txt.append('    loader_platform_thread_unlock_mutex(&threadingLock);')
        header_txt.append('}')
        header_txt.append('%s' % self.lineinfo.get())
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
        if proto.name in [ 'DbgCreateMsgCallback' ]:
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
        table = 'device'
        if proto.ret != "void":
            ret_val = "%s result = " % proto.ret
            stmt = "    return result;\n"
        if proto_is_global(proto):
           table = 'instance'

        # Memory range calls are special in needed thread checking within structs
        if proto.name in ["FlushMappedMemoryRanges","InvalidateMappedMemoryRanges"]:
            funcs.append('%s%s\n'
                     '{\n'
                     '    for (int i=0; i<memRangeCount; i++) {\n'
                     '        useObject((VkObject) pMemRanges[i].mem, "VkDeviceMemory");\n'
                     '    }\n'
                     '    %s%s_dispatch_table(%s)->%s;\n'
                     '    for (int i=0; i<memRangeCount; i++) {\n'
                     '        finishUsingObject((VkObject) pMemRanges[i].mem);\n'
                     '    }\n'
                     '%s'
                     '}' % (qual, decl, ret_val, table, proto.params[0].name, proto.c_call(), stmt))
            return "\n".join(funcs)
        # All functions that do a Get are thread safe
        if 'Get' in proto.name:
            return None
        # All WSI functions are thread safe
        if 'WSI' in proto.name:
            return None
        # Initialize in early calls
        if proto.name == "CreateDevice":
            funcs.append('%s' % self.lineinfo.get())
            funcs.append('%s%s\n'
                     '{\n'
                     '    %sdevice_dispatch_table(*pDevice)->%s;\n'
                     '%s'
                     '}' % (qual, decl, ret_val, proto.c_call(), stmt))
            return "\n".join(funcs)
        elif proto.params[0].ty == "VkPhysicalDevice":
            funcs.append('%s' % self.lineinfo.get())
            funcs.append('%s%s\n'
                     '{\n'
                     '    %s%s_dispatch_table(%s)->%s;\n'
                     '%s'
                     '}' % (qual, decl, ret_val, table, proto.params[0].name, proto.c_call(), stmt))
            return "\n".join(funcs)
        # Functions changing command buffers need thread safe use of first parameter
        if proto.params[0].ty == "VkCmdBuffer":
            funcs.append('%s' % self.lineinfo.get())
            funcs.append('%s%s\n'
                     '{\n'
                     '    useObject((VkObject) %s, "%s");\n'
                     '    %s%s_dispatch_table(%s)->%s;\n'
                     '    finishUsingObject((VkObject) %s);\n'
                     '%s'
                     '}' % (qual, decl, proto.params[0].name, proto.params[0].ty, ret_val, table, proto.params[0].name, proto.c_call(), proto.params[0].name, stmt))
            return "\n".join(funcs)
        # Non-Cmd functions that do a Wait are thread safe
        if 'Wait' in proto.name:
            return None
        # Watch use of certain types of objects passed as any parameter
        checked_params = []
        for param in proto.params:
            if param.ty in thread_check_objects:
                checked_params.append(param)
        if proto.name == "DestroyDevice":
            funcs.append('%s%s\n'
                         '{\n'
                         '    dispatch_key key = get_dispatch_key(device);\n'
                         '    %s%s_dispatch_table(%s)->%s;\n'
                         '    destroy_device_dispatch_table(key);\n'
                         '    return result;\n'
                         '}\n' % (qual, decl, ret_val, table, proto.params[0].name, proto.c_call()))
            return "\n".join(funcs);
        elif proto.name == "DestroyInstance":
            funcs.append('%s%s\n'
                         '{\n'
                         '    dispatch_key key = get_dispatch_key(instance);\n'
                         '    %s%s_dispatch_table(%s)->%s;\n'
                         '    destroy_instance_dispatch_table(key);\n'
                         '    return result;\n'
                         '}\n' % (qual, decl, ret_val, table, proto.params[0].name, proto.c_call()))
            return "\n".join(funcs);
        elif proto.name == "CreateInstance":
            funcs.append('%s%s\n'
                         '{\n'
                         '    loader_platform_thread_once(&initOnce, initThreading);\n'
                         '\n'
                         '    %s %s_dispatch_table(*pInstance)->CreateInstance(pCreateInfo, pInstance);\n'
                         '\n'
                         '    if (result == VK_SUCCESS) {\n'
                         '        enable_debug_report(pCreateInfo->extensionCount, pCreateInfo->pEnabledExtensions);\n'
                         '        VkLayerInstanceDispatchTable *pTable = instance_dispatch_table(*pInstance);\n'
                         '        debug_report_init_instance_extension_dispatch_table(\n'
                         '                    pTable,\n'
                         '                    pTable->GetInstanceProcAddr,\n'
                         '                    *pInstance);\n'
                         '    }\n'
                         '    return result;\n'
                         '}\n' % (qual, decl, ret_val, table))
            return "\n".join(funcs);
        if len(checked_params) == 0:
            return None
        # Surround call with useObject and finishUsingObject for each checked_param
        funcs.append('%s' % self.lineinfo.get())
        funcs.append('%s%s' % (qual, decl))
        funcs.append('{')
        for param in checked_params:
            funcs.append('    useObject((VkObject) %s, "%s");' % (param.name, param.ty))
        funcs.append('    %s%s_dispatch_table(%s)->%s;' % (ret_val, table, proto.params[0].name, proto.c_call()))
        for param in checked_params:
            funcs.append('    finishUsingObject((VkObject) %s);' % param.name)
        funcs.append('%s'
                 '}' % stmt)
        return "\n".join(funcs)

    def generate_body(self):
        self.layer_name = "Threading"
        body = [self._generate_layer_initialization(True, lockname='threading', condname='threading'),
                self._generate_dispatch_entrypoints("VK_LAYER_EXPORT"),
                self._generate_layer_gpa_function(extensions=[],
                                                  instance_extensions=['msg_callback_get_proc_addr']),
                self._gen_create_msg_callback(),
                self._gen_destroy_msg_callback()]
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
