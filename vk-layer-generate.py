#!/usr/bin/env python3
#
# VK
#
# Copyright (c) 2015-2016 The Khronos Group Inc.
# Copyright (c) 2015-2016 Valve Corporation
# Copyright (c) 2015-2016 LunarG, Inc.
# Copyright (c) 2015-2016 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Author: Tobin Ehlis <tobine@google.com>
# Author: Courtney Goeltzenleuchter <courtneygo@google.com>
# Author: Jon Ashburn <jon@lunarg.com>
# Author: Mark Lobodzinski <mark@lunarg.com>
# Author: Mike Stroyan <stroyan@google.com>
# Author: Tony Barbour <tony@LunarG.com>
# Author: Chia-I Wu <olv@google.com>
# Author: Gwan-gyeong Mun <kk.moon@samsung.com>

import sys
import os
import re

import vulkan
import vk_helper
from source_line_info import sourcelineinfo
from collections import defaultdict

def proto_is_global(proto):
    global_function_names = [
        "CreateInstance",
        "EnumerateInstanceLayerProperties",
        "EnumerateInstanceExtensionProperties",
        "EnumerateDeviceLayerProperties",
        "EnumerateDeviceExtensionProperties",
        "CreateXcbSurfaceKHR",
        "GetPhysicalDeviceXcbPresentationSupportKHR",
        "CreateXlibSurfaceKHR",
        "GetPhysicalDeviceXlibPresentationSupportKHR",
        "CreateWaylandSurfaceKHR",
        "GetPhysicalDeviceWaylandPresentationSupportKHR",
        "CreateMirSurfaceKHR",
        "GetPhysicalDeviceMirPresentationSupportKHR",
        "CreateAndroidSurfaceKHR",
        "CreateWin32SurfaceKHR",
        "GetPhysicalDeviceWin32PresentationSupportKHR"
    ]
    if proto.params[0].ty == "VkInstance" or proto.params[0].ty == "VkPhysicalDevice" or proto.name in global_function_names:
       return True
    else:
       return False

def wsi_name(ext_name):
    wsi_prefix = ""
    if 'Xcb' in ext_name:
        wsi_prefix = 'XCB'
    elif 'Xlib' in ext_name:
        wsi_prefix = 'XLIB'
    elif 'Win32' in ext_name:
        wsi_prefix = 'WIN32'
    elif 'Mir' in ext_name:
        wsi_prefix = 'MIR'
    elif 'Wayland' in ext_name:
        wsi_prefix = 'WAYLAND'
    elif 'Android' in ext_name:
        wsi_prefix = 'ANDROID'
    else:
        wsi_prefix = ''
    return wsi_prefix

def wsi_ifdef(ext_name):
    wsi_prefix = wsi_name(ext_name)
    if not wsi_prefix:
        return ''
    else:
        return "#ifdef VK_USE_PLATFORM_%s_KHR" % wsi_prefix

def wsi_endif(ext_name):
    wsi_prefix = wsi_name(ext_name)
    if not wsi_prefix:
        return ''
    else:
        return "#endif  // VK_USE_PLATFORM_%s_KHR" % wsi_prefix

def generate_get_proc_addr_check(name):
    return "    if (!%s || %s[0] != 'v' || %s[1] != 'k')\n" \
           "        return NULL;" % ((name,) * 3)

def ucc_to_U_C_C(CamelCase):
    temp = re.sub('(.)([A-Z][a-z]+)',  r'\1_\2', CamelCase)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', temp).upper()

# Parse complete struct chain and add any new ndo_uses to the dict
def gather_object_uses_in_struct(obj_list, struct_type):
    struct_uses = {}
    if vk_helper.typedef_rev_dict[struct_type] in vk_helper.struct_dict:
        struct_type = vk_helper.typedef_rev_dict[struct_type]
        # Parse elements of this struct param to identify objects and/or arrays of objects
        for m in sorted(vk_helper.struct_dict[struct_type]):
            array_len = "%s" % (str(vk_helper.struct_dict[struct_type][m]['array_size']))
            base_type = vk_helper.struct_dict[struct_type][m]['type']
            mem_name = vk_helper.struct_dict[struct_type][m]['name']
            if array_len != '0':
                mem_name = "%s[%s]" % (mem_name, array_len)
            if base_type in obj_list:
                #if array_len not in ndo_uses:
                #    struct_uses[array_len] = []
                #struct_uses[array_len].append("%s%s,%s" % (name_prefix, struct_name, base_type))
                struct_uses[mem_name] = base_type
            elif vk_helper.is_type(base_type, 'struct'):
                sub_uses = gather_object_uses_in_struct(obj_list, base_type)
                if len(sub_uses) > 0:
                    struct_uses[mem_name] = sub_uses
    return struct_uses

# For the given list of object types, Parse the given list of params
#  and return dict of params that use one of the obj_list types
# Format of the dict is that terminal elements have <name>,<type>
#  non-terminal elements will have <name>[<array_size>]
# TODO : This analysis could be done up-front at vk_helper time
def get_object_uses(obj_list, params):
    obj_uses = {}
    local_decls = {}
    param_count = 'NONE' # track params that give array sizes
    for p in params:
        base_type = p.ty.replace('const ', '').strip('*')
        array_len = ''
        is_ptr = False
        if 'count' in p.name.lower():
            param_count = p.name
        ptr_txt = ''
        if '*' in p.ty:
            is_ptr = True
            ptr_txt = '*'
        if base_type in obj_list:
            if is_ptr and 'const' in p.ty and param_count != 'NONE':
                array_len = "[%s]" % param_count
                # Non-arrays we can overwrite in place, but need local decl for arrays
                local_decls[p.name] = '%s%s' % (base_type, ptr_txt)
            #if array_len not in obj_uses:
            #    obj_uses[array_len] = {}
            # obj_uses[array_len][p.name] = base_type
            obj_uses["%s%s" % (p.name, array_len)] = base_type
        elif vk_helper.is_type(base_type, 'struct'):
            struct_name = p.name
            if 'NONE' != param_count:
                struct_name = "%s[%s]" % (struct_name, param_count)
            struct_uses = gather_object_uses_in_struct(obj_list, base_type)
            if len(struct_uses) > 0:
                obj_uses[struct_name] = struct_uses
                # This is a top-level struct w/ uses below it, so need local decl
                local_decls['%s' % (p.name)] = '%s%s' % (base_type, ptr_txt)
    return (obj_uses, local_decls)

class Subcommand(object):
    def __init__(self, outfile):
        self.outfile = outfile
        self.headers = vulkan.headers
        self.protos = vulkan.protos
        self.no_addr = False
        self.layer_name = ""
        self.lineinfo = sourcelineinfo()
        self.wsi = sys.argv[1]

    def run(self):
        if self.outfile:
            with open(self.outfile, "w") as outfile:
                outfile.write(self.generate())
        else:
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
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (c) 2015-2016 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Tobin Ehlis <tobine@google.com>
 * Author: Courtney Goeltzenleuchter <courtneygo@google.com>
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Mike Stroyan <stroyan@google.com>
 * Author: Tony Barbour <tony@LunarG.com>
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
                return ("%lu", "%s" % name)
            return ("%lu", "%s" % name)
        if "size" in vk_type:
            if '*' in vk_type:
                return ("%lu", "(unsigned long)*%s" % name)
            return ("%lu", "(unsigned long)%s" % name)
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
                    return ("0x%p", "(void*)(%s)" % name)
                return ("%i", "*(%s)" % name)
            return ("%i", name)
        # TODO : This is special-cased as there's only one "format" param currently and it's nice to expand it
        if "VkFormat" == vk_type:
            if cpp:
                return ("0x%p", "&%s" % name)
            return ("{%s.channelFormat = %%s, %s.numericFormat = %%s}" % (name, name), "string_VK_COLOR_COMPONENT_FORMAT(%s.channelFormat), string_VK_FORMAT_RANGE_SIZE(%s.numericFormat)" % (name, name))
        if output_param:
            return ("0x%p", "(void*)*%s" % name)
        if vk_helper.is_type(vk_type, 'struct') and '*' not in vk_type:
            return ("0x%p", "(void*)(&%s)" % name)
        return ("0x%p", "(void*)(%s)" % name)

    def _gen_create_msg_callback(self):
        r_body = []
        r_body.append('%s' % self.lineinfo.get())
        r_body.append('VKAPI_ATTR VkResult VKAPI_CALL CreateDebugReportCallbackEXT(')
        r_body.append('        VkInstance                                   instance,')
        r_body.append('        const VkDebugReportCallbackCreateInfoEXT*    pCreateInfo,')
        r_body.append('        const VkAllocationCallbacks*                 pAllocator,')
        r_body.append('        VkDebugReportCallbackEXT*                    pCallback)')
        r_body.append('{')
        # Switch to this code section for the new per-instance storage and debug callbacks
        if self.layer_name in ['object_tracker', 'unique_objects']:
            r_body.append('    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(%s_instance_table_map, instance);' % self.layer_name )
            r_body.append('    VkResult result = pInstanceTable->CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pCallback);')
            r_body.append('    if (VK_SUCCESS == result) {')
            r_body.append('        layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);')
            r_body.append('        result = layer_create_msg_callback(my_data->report_data,')
            r_body.append('                                           false,')
            r_body.append('                                           pCreateInfo,')
            r_body.append('                                           pAllocator,')
            r_body.append('                                           pCallback);')
            r_body.append('    }')
            r_body.append('    return result;')
        else:
            r_body.append('    VkResult result = instance_dispatch_table(instance)->CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pCallback);')
            r_body.append('    if (VK_SUCCESS == result) {')
            r_body.append('        layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);')
            r_body.append('        result = layer_create_msg_callback(my_data->report_data, false, pCreateInfo, pAllocator, pCallback);')
            r_body.append('    }')
            r_body.append('    return result;')
        r_body.append('}')
        return "\n".join(r_body)

    def _gen_destroy_msg_callback(self):
        r_body = []
        r_body.append('%s' % self.lineinfo.get())
        r_body.append('VKAPI_ATTR void VKAPI_CALL DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT msgCallback, const VkAllocationCallbacks *pAllocator)')
        r_body.append('{')
        # Switch to this code section for the new per-instance storage and debug callbacks
        if self.layer_name in ['object_tracker', 'unique_objects']:
            r_body.append('    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(%s_instance_table_map, instance);' % self.layer_name )
        else:
            r_body.append('    VkLayerInstanceDispatchTable *pInstanceTable = instance_dispatch_table(instance);')
        r_body.append('    pInstanceTable->DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);')
        r_body.append('    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);')
        r_body.append('    layer_destroy_msg_callback(my_data->report_data, msgCallback, pAllocator);')
        r_body.append('}')
        return "\n".join(r_body)

    def _gen_debug_report_msg(self):
        r_body = []
        r_body.append('%s' % self.lineinfo.get())
        r_body.append('VKAPI_ATTR void VKAPI_CALL DebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT    flags, VkDebugReportObjectTypeEXT objType, uint64_t object, size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg)')
        r_body.append('{')
        # Switch to this code section for the new per-instance storage and debug callbacks
        if self.layer_name == 'object_tracker':
            r_body.append('    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(%s_instance_table_map, instance);' % self.layer_name )
        else:
            r_body.append('    VkLayerInstanceDispatchTable *pInstanceTable = instance_dispatch_table(instance);')
        r_body.append('    pInstanceTable->DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);')
        r_body.append('}')
        return "\n".join(r_body)

    def _gen_layer_logging_workaround(self):
        body = []
        body.append('%s' % self.lineinfo.get())
        body.append('// vk_layer_logging.h expects these to be defined')
        body.append('')
        body.append('VKAPI_ATTR VkResult VKAPI_CALL')
        body.append('vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,')
        body.append('                               const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pMsgCallback) {')
        body.append('    return %s::CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);' % self.layer_name)
        body.append('}')
        body.append('')
        body.append('VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(VkInstance instance,')
        body.append('                                                                           VkDebugReportCallbackEXT msgCallback,')
        body.append('                                                                           const VkAllocationCallbacks *pAllocator) {')
        body.append('    %s::DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);' % self.layer_name)
        body.append('}')
        body.append('')
        body.append('VKAPI_ATTR void VKAPI_CALL')
        body.append('vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t object,')
        body.append('                        size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {')
        body.append('    %s::DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);' % self.layer_name)
        body.append('}')

        return "\n".join(body)

    def _gen_layer_interface_v0_functions(self):
        body = []
        body.append('%s' % self.lineinfo.get())
        body.append('// loader-layer interface v0, just wrappers since there is only a layer')
        body.append('')

        body.append('VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,  VkExtensionProperties* pProperties)')
        body.append('{')
        body.append('    return %s::EnumerateInstanceExtensionProperties(pLayerName, pCount, pProperties);' % self.layer_name)
        body.append('}')
        body.append('')
        body.append('VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t *pCount,  VkLayerProperties* pProperties)')
        body.append('{')
        body.append('    return %s::EnumerateInstanceLayerProperties(pCount, pProperties);' % self.layer_name)
        body.append('}')
        body.append('')
        body.append('VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount, VkLayerProperties* pProperties)')
        body.append('{')
        body.append('    // the layer command handles VK_NULL_HANDLE just fine internally')
        body.append('    assert(physicalDevice == VK_NULL_HANDLE);')
        body.append('    return %s::EnumerateDeviceLayerProperties(VK_NULL_HANDLE, pCount, pProperties);' % self.layer_name)
        body.append('}')
        body.append('')
        body.append('VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev, const char *funcName)')
        body.append('{')
        body.append('    return %s::GetDeviceProcAddr(dev, funcName);' % self.layer_name)
        body.append('}')
        body.append('')
        body.append('VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *funcName)')
        body.append('{')
        body.append('    return %s::GetInstanceProcAddr(instance, funcName);' % self.layer_name)
        body.append('}')
        body.append('')
        body.append('VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,')
        body.append('                                                                                    const char *pLayerName, uint32_t *pCount,')
        body.append('                                                                                    VkExtensionProperties *pProperties)')
        body.append('{')
        body.append('    // the layer command handles VK_NULL_HANDLE just fine internally')
        body.append('    assert(physicalDevice == VK_NULL_HANDLE);')
        body.append('    return %s::EnumerateDeviceExtensionProperties(VK_NULL_HANDLE, pLayerName, pCount, pProperties);' % self.layer_name)
        body.append('}')

        return "\n".join(body)

    def _generate_dispatch_entrypoints(self, qual=""):
        if qual:
            qual += " "

        funcs = []
        intercepted = []
        for proto in self.protos:
            if proto.name in ["EnumerateInstanceExtensionProperties",
                              "EnumerateInstanceLayerProperties",
                              "EnumerateDeviceLayerProperties",
                              "GetDeviceProcAddr",
                              "GetInstanceProcAddr"]:
                funcs.append(proto.c_func(attr="VKAPI") + ';')
                intercepted.append(proto)
            else:
                intercept = self.generate_intercept(proto, qual)
                if intercept is None:
                    # fill in default intercept for certain entrypoints
                    if 'CreateDebugReportCallbackEXT' == proto.name:
                        intercept = self._gen_layer_dbg_create_msg_callback()
                    elif 'DestroyDebugReportCallbackEXT' == proto.name:
                        intercept = self._gen_layer_dbg_destroy_msg_callback()
                    elif 'DebugReportMessageEXT' == proto.name:
                        intercept = self._gen_debug_report_msg()
                    elif 'CreateDevice' == proto.name:
                        funcs.append('/* CreateDevice HERE */')

                if intercept is not None:
                    funcs.append(intercept)
                    if not "KHR" in proto.name:
                        intercepted.append(proto)

        instance_lookups = []
        device_lookups = []
        for proto in intercepted:
            if proto_is_global(proto):
                instance_lookups.append("if (!strcmp(name, \"%s\"))" % proto.name)
                instance_lookups.append("    return (PFN_vkVoidFunction) %s;" % (proto.name))
            else:
                device_lookups.append("if (!strcmp(name, \"%s\"))" % proto.name)
                device_lookups.append("    return (PFN_vkVoidFunction) %s;" % (proto.name))

        # add customized intercept_core_device_command
        body = []
        body.append('%s' % self.lineinfo.get())
        body.append("static inline PFN_vkVoidFunction intercept_core_device_command(const char *name)")
        body.append("{")
        body.append(generate_get_proc_addr_check("name"))
        body.append("")
        body.append("    name += 2;")
        body.append("    %s" % "\n    ".join(device_lookups))
        body.append("")
        body.append("    return NULL;")
        body.append("}")
        # add intercept_core_instance_command
        body.append("static inline PFN_vkVoidFunction intercept_core_instance_command(const char *name)")
        body.append("{")
        body.append(generate_get_proc_addr_check("name"))
        body.append("")
        body.append("    name += 2;")
        body.append("    %s" % "\n    ".join(instance_lookups))
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
        exts.append(self._gen_debug_report_msg())
        return "\n".join(exts)

    def _generate_layer_introspection_function(self):
        body = []
        body.append('%s' % self.lineinfo.get())
        if self.layer_name == 'object_tracker':
            body.append('static const VkExtensionProperties instance_extensions[] = {')
            body.append('    {')
            body.append('        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,')
            body.append('        VK_EXT_DEBUG_REPORT_SPEC_VERSION')
            body.append('    }')
            body.append('};')
            body.append('')

            body.append('static const VkLayerProperties globalLayerProps = {')
            body.append('    "VK_LAYER_LUNARG_%s",' % self.layer_name)
            body.append('    VK_LAYER_API_VERSION, // specVersion')
            body.append('    1, // implementationVersion')
            body.append('    "LunarG Validation Layer"')
            body.append('};')
            body.append('')
        else:
            body.append('static const VkLayerProperties globalLayerProps = {')
            body.append('    "VK_LAYER_GOOGLE_%s",' % self.layer_name)
            body.append('    VK_LAYER_API_VERSION, // specVersion')
            body.append('    1, // implementationVersion')
            body.append('    "Google Validation Layer"')
            body.append('};')
            body.append('')

        body.append('VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceLayerProperties(uint32_t *pCount,  VkLayerProperties* pProperties)')
        body.append('{')
        body.append('    return util_GetLayerProperties(1, &globalLayerProps, pCount, pProperties);')
        body.append('}')
        body.append('')
        body.append('VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount, VkLayerProperties* pProperties)')
        body.append('{')
        body.append('    return util_GetLayerProperties(1, &globalLayerProps, pCount, pProperties);')
        body.append('}')
        body.append('')
        body.append('VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,  VkExtensionProperties* pProperties)')
        body.append('{')
        body.append('    if (pLayerName && !strcmp(pLayerName, globalLayerProps.layerName))')
        if self.layer_name == 'object_tracker':
          body.append('        return util_GetExtensionProperties(1, instance_extensions, pCount, pProperties);')
        else:
          body.append('        return util_GetExtensionProperties(0, NULL, pCount, pProperties);')
        body.append('')
        body.append('    return VK_ERROR_LAYER_NOT_PRESENT;')
        body.append('}')
        body.append('')
        body.append('VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,')
        body.append('                                                                  const char *pLayerName, uint32_t *pCount,')
        body.append('                                                                  VkExtensionProperties *pProperties)')
        body.append('{')
        body.append('    if (pLayerName && !strcmp(pLayerName, globalLayerProps.layerName))')
        body.append('        return util_GetExtensionProperties(0, nullptr, pCount, pProperties);')
        body.append('')
        body.append('    assert(physicalDevice);')
        body.append('    VkLayerInstanceDispatchTable* pTable = get_dispatch_table(%s_instance_table_map, physicalDevice);' % self.layer_name)
        body.append('    return pTable->EnumerateDeviceExtensionProperties(physicalDevice, NULL, pCount, pProperties);')
        body.append('}')

        return "\n".join(body)

    def _generate_layer_gpa_function(self, extensions=[], instance_extensions=[]):
        func_body = []
#
# New style of GPA Functions for the new layer_data/layer_logging changes
#
        if self.layer_name in ['object_tracker', 'unique_objects']:
            for ext_enable, ext_list in extensions:
                func_body.append('%s' % self.lineinfo.get())
                func_body.append('static inline PFN_vkVoidFunction intercept_%s_command(const char *name, VkDevice dev)' % ext_enable)
                func_body.append('{')
                func_body.append('    if (dev) {')
                func_body.append('        layer_data *my_data = get_my_data_ptr(get_dispatch_key(dev), layer_data_map);')
                func_body.append('        if (!my_data->%s)' % ext_enable)
                func_body.append('            return nullptr;')
                func_body.append('    }\n')

                for ext_name in ext_list:
                    func_body.append('    if (!strcmp("%s", name))\n'
                                     '        return reinterpret_cast<PFN_vkVoidFunction>(%s);' % (ext_name, ext_name[2:]))
                func_body.append('\n    return nullptr;')
                func_body.append('}\n')

            func_body.append("VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(VkDevice device, const char* funcName)\n"
                             "{\n"
                             "    PFN_vkVoidFunction addr;\n"
                             "    addr = intercept_core_device_command(funcName);\n"
                             "    if (addr)\n"
                             "        return addr;\n"
                             "    assert(device);\n")
            for ext_enable, _ in extensions:
                func_body.append('    addr = intercept_%s_command(funcName, device);' % ext_enable)
                func_body.append('    if (addr)\n'
                                 '        return addr;')
            func_body.append("\n    if (get_dispatch_table(%s_device_table_map, device)->GetDeviceProcAddr == NULL)\n"
                             "        return NULL;\n"
                             "    return get_dispatch_table(%s_device_table_map, device)->GetDeviceProcAddr(device, funcName);\n"
                             "}\n" % (self.layer_name, self.layer_name))

            for ext_enable, ext_list in instance_extensions:
                func_body.append('%s' % self.lineinfo.get())
                func_body.append('static inline PFN_vkVoidFunction intercept_%s_command(const char *name, VkInstance instance)' % ext_enable)
                func_body.append('{')
                if ext_enable == 'msg_callback_get_proc_addr':
                    func_body.append("    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);\n"
                                     "    return debug_report_get_instance_proc_addr(my_data->report_data, name);")
                else:
                    func_body.append("    VkLayerInstanceDispatchTable* pTable = get_dispatch_table(%s_instance_table_map, instance);" % self.layer_name)
                    func_body.append('    if (instanceExtMap.size() == 0 || !instanceExtMap[pTable].%s)' % ext_enable)
                    func_body.append('        return nullptr;\n')

                    for ext_name in ext_list:
                        if wsi_name(ext_name):
                            func_body.append('%s' % wsi_ifdef(ext_name))
                        func_body.append('    if (!strcmp("%s", name))\n'
                                         '        return reinterpret_cast<PFN_vkVoidFunction>(%s);' % (ext_name, ext_name[2:]))
                        if wsi_name(ext_name):
                            func_body.append('%s' % wsi_endif(ext_name))

                    func_body.append('\n    return nullptr;')
                func_body.append('}\n')

            func_body.append("VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(VkInstance instance, const char* funcName)\n"
                             "{\n"
                             "    PFN_vkVoidFunction addr;\n"
                             "    addr = intercept_core_instance_command(funcName);\n"
                             "    if (!addr) {\n"
                             "        addr = intercept_core_device_command(funcName);\n"
                             "    }")

            for ext_enable, _ in extensions:
                func_body.append("    if (!addr) {\n"
                                 "        addr = intercept_%s_command(funcName, VkDevice(VK_NULL_HANDLE));\n"
                                 "    }" % ext_enable)

            func_body.append("    if (addr) {\n"
                             "        return addr;\n"
                             "    }\n"
                             "    assert(instance);\n"
                             )

            for ext_enable, _ in instance_extensions:
                func_body.append('    addr = intercept_%s_command(funcName, instance);' % ext_enable)
                func_body.append('    if (addr)\n'
                                 '        return addr;\n')

            func_body.append("    if (get_dispatch_table(%s_instance_table_map, instance)->GetInstanceProcAddr == NULL) {\n"
                             "        return NULL;\n"
                             "    }\n"
                             "    return get_dispatch_table(%s_instance_table_map, instance)->GetInstanceProcAddr(instance, funcName);\n"
                             "}\n" % (self.layer_name, self.layer_name))
            return "\n".join(func_body)
        else:
            func_body.append('%s' % self.lineinfo.get())
            func_body.append("VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(VkDevice device, const char* funcName)\n"
                             "{\n"
                             "    PFN_vkVoidFunction addr;\n")
            func_body.append("\n"
                             "    loader_platform_thread_once(&initOnce, init%s);\n\n"
                             "    addr = intercept_core_device_command(funcName);\n"
                             "    if (addr)\n"
                             "        return addr;" % self.layer_name)
            func_body.append("    assert(device);\n")
            func_body.append('')
            func_body.append('    VkLayerDispatchTable *pDisp =  device_dispatch_table(device);')
            if 0 != len(extensions):
                extra_space = ""
                for (ext_enable, ext_list) in extensions:
                    if 0 != len(ext_enable):
                        func_body.append('    if (deviceExtMap.size() != 0 && deviceExtMap[pDisp].%s)' % ext_enable)
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
            func_body.append('%s' % self.lineinfo.get())
            func_body.append("VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(VkInstance instance, const char* funcName)\n"
                             "{\n"
                             "    PFN_vkVoidFunction addr;\n"
                             )
            func_body.append(
                             "    loader_platform_thread_once(&initOnce, init%s);\n\n"
                             "    addr = intercept_core_instance_command(funcName);\n"
                             "    if (addr)\n"
                             "        return addr;" % self.layer_name)
            func_body.append("    assert(instance);\n")
            func_body.append("")
            func_body.append("    VkLayerInstanceDispatchTable* pTable = instance_dispatch_table(instance);\n")
            if 0 != len(instance_extensions):
                extra_space = ""
                for (ext_enable, ext_list) in instance_extensions:
                    if 0 != len(ext_enable):
                        if ext_enable == 'msg_callback_get_proc_addr':
                            func_body.append("    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);\n"
                                     "    addr = debug_report_get_instance_proc_addr(my_data->report_data, funcName);\n"
                                     "    if (addr) {\n"
                                     "        return addr;\n"
                                     "    }\n")
                        else:
                            func_body.append('    if (instanceExtMap.size() != 0 && instanceExtMap[pTable].%s)' % ext_enable)
                            func_body.append('    {')
                            extra_space = "    "
                            for ext_name in ext_list:
                                if wsi_name(ext_name):
                                    func_body.append('%s' % wsi_ifdef(ext_name))
                                func_body.append('    %sif (!strcmp("%s", funcName))\n'
                                         '            return reinterpret_cast<PFN_vkVoidFunction>(%s);' % (extra_space, ext_name, ext_name))
                                if wsi_name(ext_name):
                                    func_body.append('%s' % wsi_endif(ext_name))
                            if 0 != len(ext_enable):
                                func_body.append('    }\n')

            func_body.append("    if (pTable->GetInstanceProcAddr == NULL)\n"
                             "        return NULL;\n"
                             "    return pTable->GetInstanceProcAddr(instance, funcName);\n"
                             "}\n")
            return "\n".join(func_body)


    def _generate_layer_initialization(self, init_opts=False, prefix='vk', lockname=None, condname=None):
        func_body = ["#include \"vk_dispatch_table_helper.h\""]
        func_body.append('%s' % self.lineinfo.get())
        func_body.append('static void init_%s(layer_data *my_data, const VkAllocationCallbacks *pAllocator)\n'
                         '{\n' % self.layer_name)
        if init_opts:
            func_body.append('%s' % self.lineinfo.get())
            func_body.append('')
            func_body.append('    layer_debug_actions(my_data->report_data, my_data->logging_callback, pAllocator, "lunarg_%s");' % self.layer_name)
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

class ObjectTrackerSubcommand(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('%s' % self.lineinfo.get())
        header_txt.append('#include "vk_loader_platform.h"')
        header_txt.append('#include "vulkan/vulkan.h"')
        header_txt.append('')
        header_txt.append('#include <stdio.h>')
        header_txt.append('#include <stdlib.h>')
        header_txt.append('#include <string.h>')
        header_txt.append('#include <cinttypes>')
        header_txt.append('')
        header_txt.append('#include <unordered_map>')
        header_txt.append('')
        header_txt.append('#include "vulkan/vk_layer.h"')
        header_txt.append('#include "vk_layer_config.h"')
        header_txt.append('#include "vk_layer_table.h"')
        header_txt.append('#include "vk_layer_data.h"')
        header_txt.append('#include "vk_layer_logging.h"')
        header_txt.append('')
#       NOTE:  The non-autoGenerated code is in the object_tracker.h header file
        header_txt.append('#include "object_tracker.h"')
        header_txt.append('')
        return "\n".join(header_txt)

    def generate_maps(self):
        maps_txt = []
        for o in vulkan.object_type_list:
            maps_txt.append('std::unordered_map<uint64_t, OBJTRACK_NODE*> %sMap;' % (o))
        return "\n".join(maps_txt)

    def _gather_object_uses(self, obj_list, struct_type, obj_set):
    # for each member of struct_type
    #     add objs in obj_list to obj_set
    #     call self for structs
        for m in sorted(vk_helper.struct_dict[struct_type]):
            if vk_helper.struct_dict[struct_type][m]['type'] in obj_list:
                obj_set.add(vk_helper.struct_dict[struct_type][m]['type'])
            elif vk_helper.is_type(vk_helper.struct_dict[struct_type][m]['type'], 'struct'):
                obj_set = obj_set.union(self._gather_object_uses(obj_list, vk_helper.struct_dict[struct_type][m]['type'], obj_set))
        return obj_set

    def generate_procs(self):
        procs_txt = []
        # First parse through funcs and gather dict of all objects seen by each call
        obj_use_dict = {}
        proto_list = vulkan.core.protos + vulkan.ext_khr_surface.protos + vulkan.ext_khr_surface.protos + vulkan.ext_khr_win32_surface.protos + vulkan.ext_khr_device_swapchain.protos
        for proto in proto_list:
            disp_obj = proto.params[0].ty.strip('*').replace('const ', '')
            if disp_obj in vulkan.object_dispatch_list:
                if disp_obj not in obj_use_dict:
                    obj_use_dict[disp_obj] = set()
                for p in proto.params[1:]:
                    base_type = p.ty.strip('*').replace('const ', '')
                    if base_type in vulkan.object_type_list:
                        obj_use_dict[disp_obj].add(base_type)
                    if vk_helper.is_type(base_type, 'struct'):
                        obj_use_dict[disp_obj] = self._gather_object_uses(vulkan.object_type_list, base_type, obj_use_dict[disp_obj])
        #for do in obj_use_dict:
        #    print "Disp obj %s has uses for objs: %s" % (do, ', '.join(obj_use_dict[do]))

        for o in vulkan.object_type_list:# vulkan.core.objects:
            procs_txt.append('%s' % self.lineinfo.get())
            name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', o)
            name = re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()[3:]
            if o in vulkan.object_dispatch_list:
                procs_txt.append('static void create_%s(%s dispatchable_object, %s vkObj, VkDebugReportObjectTypeEXT objType)' % (name, o, o))
            else:
                procs_txt.append('static void create_%s(VkDevice dispatchable_object, %s vkObj, VkDebugReportObjectTypeEXT objType)' % (name, o))
            procs_txt.append('{')
            procs_txt.append('    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType,(uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",')
            procs_txt.append('        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDebugReportObjectTypeEXT(objType),')
            procs_txt.append('        (uint64_t)(vkObj));')
            procs_txt.append('')
            procs_txt.append('    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;')
            procs_txt.append('    pNewObjNode->belongsTo = (uint64_t)dispatchable_object;')
            procs_txt.append('    pNewObjNode->objType = objType;')
            procs_txt.append('    pNewObjNode->status  = OBJSTATUS_NONE;')
            procs_txt.append('    pNewObjNode->vkObj  = (uint64_t)(vkObj);')
            procs_txt.append('    %sMap[(uint64_t)vkObj] = pNewObjNode;' % (o))
            procs_txt.append('    uint32_t objIndex = objTypeToIndex(objType);')
            procs_txt.append('    numObjs[objIndex]++;')
            procs_txt.append('    numTotalObjs++;')
            procs_txt.append('}')
            procs_txt.append('')
            procs_txt.append('%s' % self.lineinfo.get())
            if o in vulkan.object_dispatch_list:
                procs_txt.append('static void destroy_%s(%s dispatchable_object, %s object)' % (name, o, o))
            else:
                procs_txt.append('static void destroy_%s(VkDevice dispatchable_object, %s object)' % (name, o))
            procs_txt.append('{')
            procs_txt.append('    uint64_t object_handle = (uint64_t)(object);')
            procs_txt.append('    auto it = %sMap.find(object_handle);' % o)
            procs_txt.append('    if (it != %sMap.end()) {' % o)
            procs_txt.append('        OBJTRACK_NODE* pNode = it->second;')
            procs_txt.append('        uint32_t objIndex = objTypeToIndex(pNode->objType);')
            procs_txt.append('        assert(numTotalObjs > 0);')
            procs_txt.append('        numTotalObjs--;')
            procs_txt.append('        assert(numObjs[objIndex] > 0);')
            procs_txt.append('        numObjs[objIndex]--;')
            procs_txt.append('        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_NONE, "OBJTRACK",')
            procs_txt.append('           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",')
            procs_txt.append('            string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],')
            procs_txt.append('            string_VkDebugReportObjectTypeEXT(pNode->objType));')
            procs_txt.append('        delete pNode;')
            procs_txt.append('        %sMap.erase(it);' % (o))
            procs_txt.append('    } else {')
            procs_txt.append('        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT ) 0,')
            procs_txt.append('            object_handle, __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",')
            procs_txt.append('            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",')
            procs_txt.append('            object_handle);')
            procs_txt.append('    }')
            procs_txt.append('}')
            procs_txt.append('')
        # Generate the permutations of validate_* functions where for each
        #  dispatchable object type, we have a corresponding validate_* function
        #  for that object and all non-dispatchable objects that are used in API
        #  calls with that dispatchable object.
        procs_txt.append('//%s' % str(sorted(obj_use_dict)))
        for do in sorted(obj_use_dict):
            name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', do)
            name = re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()[3:]
            # First create validate_* func for disp obj
            procs_txt.append('%s' % self.lineinfo.get())
            procs_txt.append('static bool validate_%s(%s dispatchable_object, %s object, VkDebugReportObjectTypeEXT objType, bool null_allowed)' % (name, do, do))
            procs_txt.append('{')
            procs_txt.append('    if (null_allowed && (object == VK_NULL_HANDLE))')
            procs_txt.append('        return false;')
            procs_txt.append('    if (%sMap.find((uint64_t)object) == %sMap.end()) {' % (do, do))
            procs_txt.append('        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",')
            procs_txt.append('            "Invalid %s Object 0x%%" PRIx64 ,(uint64_t)(object));' % do)
            procs_txt.append('    }')
            procs_txt.append('    return false;')
            procs_txt.append('}')
            procs_txt.append('')
            for o in sorted(obj_use_dict[do]):
                if o == do: # We already generated this case above so skip here
                    continue
                name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', o)
                name = re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()[3:]
                procs_txt.append('%s' % self.lineinfo.get())
                procs_txt.append('static bool validate_%s(%s dispatchable_object, %s object, VkDebugReportObjectTypeEXT objType, bool null_allowed)' % (name, do, o))
                procs_txt.append('{')
                procs_txt.append('    if (null_allowed && (object == VK_NULL_HANDLE))')
                procs_txt.append('        return false;')
                if o == "VkImage":
                    procs_txt.append('    // We need to validate normal image objects and those from the swapchain')
                    procs_txt.append('    if ((%sMap.find((uint64_t)object) == %sMap.end()) &&' % (o, o))
                    procs_txt.append('        (swapchainImageMap.find((uint64_t)object) == swapchainImageMap.end())) {')
                else:
                    procs_txt.append('    if (%sMap.find((uint64_t)object) == %sMap.end()) {' % (o, o))
                procs_txt.append('        return log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, objType, (uint64_t)(object), __LINE__, OBJTRACK_INVALID_OBJECT, "OBJTRACK",')
                procs_txt.append('            "Invalid %s Object 0x%%" PRIx64, (uint64_t)(object));' % o)
                procs_txt.append('    }')
                procs_txt.append('    return false;')
                procs_txt.append('}')
            procs_txt.append('')
        procs_txt.append('')
        return "\n".join(procs_txt)

    def generate_destroy_instance(self):
        gedi_txt = []
        gedi_txt.append('%s' % self.lineinfo.get())
        gedi_txt.append('VKAPI_ATTR void VKAPI_CALL DestroyInstance(')
        gedi_txt.append('VkInstance instance,')
        gedi_txt.append('const VkAllocationCallbacks* pAllocator)')
        gedi_txt.append('{')
        gedi_txt.append('    std::unique_lock<std::mutex> lock(global_lock);')
        gedi_txt.append('')
        gedi_txt.append('    dispatch_key key = get_dispatch_key(instance);')
        gedi_txt.append('    layer_data *my_data = get_my_data_ptr(key, layer_data_map);')
        gedi_txt.append('')
        gedi_txt.append('    // Enable the temporary callback(s) here to catch cleanup issues:')
        gedi_txt.append('    bool callback_setup = false;')
        gedi_txt.append('    if (my_data->num_tmp_callbacks > 0) {')
        gedi_txt.append('        if (!layer_enable_tmp_callbacks(my_data->report_data,')
        gedi_txt.append('                                        my_data->num_tmp_callbacks,')
        gedi_txt.append('                                        my_data->tmp_dbg_create_infos,')
        gedi_txt.append('                                        my_data->tmp_callbacks)) {')
        gedi_txt.append('            callback_setup = true;')
        gedi_txt.append('        }')
        gedi_txt.append('    }')
        gedi_txt.append('')
        gedi_txt.append('    validate_instance(instance, instance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, false);')
        gedi_txt.append('')
        gedi_txt.append('    destroy_instance(instance, instance);')
        gedi_txt.append('    // Report any remaining objects in LL')
        gedi_txt.append('')
        gedi_txt.append('    for (auto iit = VkDeviceMap.begin(); iit != VkDeviceMap.end();) {')
        gedi_txt.append('        OBJTRACK_NODE* pNode = iit->second;')
        gedi_txt.append('        if (pNode->belongsTo == (uint64_t)instance) {')
        gedi_txt.append('            log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",')
        gedi_txt.append('                    "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),')
        gedi_txt.append('                    pNode->vkObj);')
        for o in vulkan.core.objects:
            if o in ['VkInstance', 'VkPhysicalDevice', 'VkQueue', 'VkDevice']:
                continue
            gedi_txt.append('            for (auto idt = %sMap.begin(); idt != %sMap.end();) {' % (o, o))
            gedi_txt.append('                OBJTRACK_NODE* pNode = idt->second;')
            gedi_txt.append('                if (pNode->belongsTo == iit->first) {')
            gedi_txt.append('                    log_msg(mid(instance), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",')
            gedi_txt.append('                            "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),')
            gedi_txt.append('                            pNode->vkObj);')
            gedi_txt.append('                    %sMap.erase(idt++);' % o )
            gedi_txt.append('                } else {')
            gedi_txt.append('                    ++idt;')
            gedi_txt.append('                }')
            gedi_txt.append('            }')
        gedi_txt.append('            VkDeviceMap.erase(iit++);')
        gedi_txt.append('        } else {')
        gedi_txt.append('            ++iit;')
        gedi_txt.append('        }')
        gedi_txt.append('    }')
        gedi_txt.append('')
        gedi_txt.append('    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(object_tracker_instance_table_map, instance);')
        gedi_txt.append('    pInstanceTable->DestroyInstance(instance, pAllocator);')
        gedi_txt.append('')
        gedi_txt.append('    // Disable and cleanup the temporary callback(s):')
        gedi_txt.append('    if (callback_setup) {')
        gedi_txt.append('        layer_disable_tmp_callbacks(my_data->report_data,')
        gedi_txt.append('                                    my_data->num_tmp_callbacks,')
        gedi_txt.append('                                    my_data->tmp_callbacks);')
        gedi_txt.append('    }')
        gedi_txt.append('    if (my_data->num_tmp_callbacks > 0) {')
        gedi_txt.append('        layer_free_tmp_callbacks(my_data->tmp_dbg_create_infos,')
        gedi_txt.append('                                 my_data->tmp_callbacks);')
        gedi_txt.append('        my_data->num_tmp_callbacks = 0;')
        gedi_txt.append('    }')
        gedi_txt.append('')
        gedi_txt.append('    // Clean up logging callback, if any')
        gedi_txt.append('    while (my_data->logging_callback.size() > 0) {')
        gedi_txt.append('        VkDebugReportCallbackEXT callback = my_data->logging_callback.back();')
        gedi_txt.append('        layer_destroy_msg_callback(my_data->report_data, callback, pAllocator);')
        gedi_txt.append('        my_data->logging_callback.pop_back();')
        gedi_txt.append('    }')
        gedi_txt.append('')
        gedi_txt.append('    layer_debug_report_destroy_instance(mid(instance));')
        gedi_txt.append('    layer_data_map.erase(key);')
        gedi_txt.append('')
        gedi_txt.append('    instanceExtMap.erase(pInstanceTable);')
        gedi_txt.append('    lock.unlock();')
        # The loader holds a mutex that protects this from other threads
        gedi_txt.append('    object_tracker_instance_table_map.erase(key);')
        gedi_txt.append('}')
        gedi_txt.append('')
        return "\n".join(gedi_txt)

    def generate_destroy_device(self):
        gedd_txt = []
        gedd_txt.append('%s' % self.lineinfo.get())
        gedd_txt.append('VKAPI_ATTR void VKAPI_CALL DestroyDevice(')
        gedd_txt.append('VkDevice device,')
        gedd_txt.append('const VkAllocationCallbacks* pAllocator)')
        gedd_txt.append('{')
        gedd_txt.append('    std::unique_lock<std::mutex> lock(global_lock);')
        gedd_txt.append('    validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);')
        gedd_txt.append('')
        gedd_txt.append('    destroy_device(device, device);')
        gedd_txt.append('    // Report any remaining objects associated with this VkDevice object in LL')
        for o in vulkan.core.objects:
            # DescriptorSets and Command Buffers are destroyed through their pools, not explicitly
            if o in ['VkInstance', 'VkPhysicalDevice', 'VkQueue', 'VkDevice', 'VkDescriptorSet', 'VkCommandBuffer']:
                continue
            gedd_txt.append('    for (auto it = %sMap.begin(); it != %sMap.end();) {' % (o, o))
            gedd_txt.append('        OBJTRACK_NODE* pNode = it->second;')
            gedd_txt.append('        if (pNode->belongsTo == (uint64_t)device) {')
            gedd_txt.append('            log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, pNode->vkObj, __LINE__, OBJTRACK_OBJECT_LEAK, "OBJTRACK",')
            gedd_txt.append('                    "OBJ ERROR : %s object 0x%" PRIxLEAST64 " has not been destroyed.", string_VkDebugReportObjectTypeEXT(pNode->objType),')
            gedd_txt.append('                    pNode->vkObj);')
            gedd_txt.append('            %sMap.erase(it++);' % o )
            gedd_txt.append('        } else {')
            gedd_txt.append('            ++it;')
            gedd_txt.append('        }')
            gedd_txt.append('    }')
            gedd_txt.append('')
        gedd_txt.append("    // Clean up Queue's MemRef Linked Lists")
        gedd_txt.append('    destroyQueueMemRefLists();')
        gedd_txt.append('')
        gedd_txt.append('    lock.unlock();')
        gedd_txt.append('')
        gedd_txt.append('    dispatch_key key = get_dispatch_key(device);')
        gedd_txt.append('    VkLayerDispatchTable *pDisp = get_dispatch_table(object_tracker_device_table_map, device);')
        gedd_txt.append('    pDisp->DestroyDevice(device, pAllocator);')
        gedd_txt.append('    object_tracker_device_table_map.erase(key);')
        gedd_txt.append('')
        gedd_txt.append('}')
        gedd_txt.append('')
        return "\n".join(gedd_txt)

    # Special-case validating some objects -- they may be non-NULL but should
    # only be validated upon meeting some condition specified below.
    def _dereference_conditionally(self, indent, prefix, type_name, name):
        s_code = ''
        if type_name == 'pBufferInfo':
            s_code += '%sif ((%sdescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)         ||\n'    % (indent, prefix)
            s_code += '%s    (%sdescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)         ||\n'    % (indent, prefix)
            s_code += '%s    (%sdescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ||\n'    % (indent, prefix)
            s_code += '%s    (%sdescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)   ) {\n' % (indent, prefix)
        elif type_name == 'pImageInfo':
            s_code += '%sif ((%sdescriptorType == VK_DESCRIPTOR_TYPE_SAMPLER)                ||\n'    % (indent, prefix)
            s_code += '%s    (%sdescriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) ||\n'    % (indent, prefix)
            s_code += '%s    (%sdescriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)       ||\n'    % (indent, prefix)
            s_code += '%s    (%sdescriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)          ||\n'    % (indent, prefix)
            s_code += '%s    (%sdescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)            ) {\n' % (indent, prefix)
        elif type_name == 'pTexelBufferView':
            s_code += '%sif ((%sdescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) ||\n'      % (indent, prefix)
            s_code += '%s    (%sdescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)   ) {\n'   % (indent, prefix)
        elif name == 'pBeginInfo->pInheritanceInfo':
            s_code += '%sOBJTRACK_NODE* pNode = VkCommandBufferMap[(uint64_t)commandBuffer];\n'       % (indent)
            s_code += '%sif ((%s) && (pNode->status & OBJSTATUS_COMMAND_BUFFER_SECONDARY)) {\n'       % (indent, name)
        else:
            s_code += '%sif (%s) {\n' % (indent, name)
        return s_code

    def _gen_obj_validate_code(self, struct_uses, obj_type_mapping, func_name, valid_null_dict, param0_name, indent, prefix, array_index):
        pre_code = ''
        for obj in sorted(struct_uses):
            name = obj
            array = ''
            type_name = ''
            if '[' in obj:
                (name, array) = obj.split('[')
                type_name = name
                array = array.strip(']')
            if isinstance(struct_uses[obj], dict):
                local_prefix = ''
                name = '%s%s' % (prefix, name)
                ptr_type = False
                if 'p' == obj[0]:
                    ptr_type = True
                    tmp_pre = self._dereference_conditionally(indent, prefix, type_name, name)
                    pre_code += tmp_pre
                    indent += '    '
                if array != '':
                    idx = 'idx%s' % str(array_index)
                    array_index += 1
                    pre_code += '%s\n' % self.lineinfo.get()
                    pre_code += '%sfor (uint32_t %s=0; %s<%s%s; ++%s) {\n' % (indent, idx, idx, prefix, array, idx)
                    indent += '    '
                    local_prefix = '%s[%s].' % (name, idx)
                elif ptr_type:
                    local_prefix = '%s->' % (name)
                else:
                    local_prefix = '%s.' % (name)
                tmp_pre = self._gen_obj_validate_code(struct_uses[obj], obj_type_mapping, func_name, valid_null_dict, param0_name, indent, local_prefix, array_index)
                pre_code += tmp_pre
                if array != '':
                    indent = indent[4:]
                    pre_code += '%s}\n' % (indent)
                if ptr_type:
                    indent = indent[4:]
                    pre_code += '%s}\n' % (indent)
            else:
                ptype = struct_uses[obj]
                dbg_obj_type = obj_type_mapping[ptype]
                fname = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', ptype)
                fname = re.sub('([a-z0-9])([A-Z])', r'\1_\2', fname).lower()[3:]
                full_name = '%s%s' % (prefix, name)
                null_obj_ok = 'false'
                # If a valid null param is defined for this func and we have a match, allow NULL
                if func_name in valid_null_dict and True in [name in pn for pn in sorted(valid_null_dict[func_name])]:
                    null_obj_ok = 'true'
                if (array_index > 0) or '' != array:
                    tmp_pre = self._dereference_conditionally(indent, prefix, type_name, full_name)
                    pre_code += tmp_pre
                    indent += '    '
                    if array != '':
                        idx = 'idx%s' % str(array_index)
                        array_index += 1
                        pre_code += '%sfor (uint32_t %s=0; %s<%s%s; ++%s) {\n' % (indent, idx, idx, prefix, array, idx)
                        indent += '    '
                        full_name = '%s[%s]' % (full_name, idx)
                    pre_code += '%s\n' % self.lineinfo.get()
                    pre_code += '%sskipCall |= validate_%s(%s, %s, %s, %s);\n' %(indent, fname, param0_name, full_name, dbg_obj_type, null_obj_ok)
                    if array != '':
                        indent = indent[4:]
                        pre_code += '%s}\n' % (indent)
                    indent = indent[4:]
                    pre_code += '%s}\n' % (indent)
                else:
                    pre_code += '%s\n' % self.lineinfo.get()
                    pre_code += '%sskipCall |= validate_%s(%s, %s, %s, %s);\n' %(indent, fname, param0_name, full_name, dbg_obj_type, null_obj_ok)
        return pre_code

    def generate_intercept(self, proto, qual):
        if proto.name in [ 'CreateDebugReportCallbackEXT', 'EnumerateInstanceLayerProperties', 'EnumerateInstanceExtensionProperties','EnumerateDeviceLayerProperties', 'EnumerateDeviceExtensionProperties' ]:
            # use default version
            return None

        # Create map of object names to object type enums of the form VkName : VkObjectTypeName
        obj_type_mapping = {base_t : base_t.replace("Vk", "VkDebugReportObjectType") for base_t in vulkan.object_type_list}
        # Convert object type enum names from UpperCamelCase to UPPER_CASE_WITH_UNDERSCORES
        for objectName, objectTypeEnum in obj_type_mapping.items():
            obj_type_mapping[objectName] = ucc_to_U_C_C(objectTypeEnum) + '_EXT';
        # Command Buffer Object doesn't follow the rule.
        obj_type_mapping['VkCommandBuffer'] = "VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT"
        obj_type_mapping['VkShaderModule'] = "VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT"

        explicit_object_tracker_functions = [
            "CreateInstance",
            "EnumeratePhysicalDevices",
            "GetPhysicalDeviceQueueFamilyProperties",
            "CreateDevice",
            "GetDeviceQueue",
            "QueueBindSparse",
            "AllocateDescriptorSets",
            "FreeDescriptorSets",
            "CreateGraphicsPipelines",
            "CreateComputePipelines",
            "AllocateCommandBuffers",
            "FreeCommandBuffers",
            "DestroyDescriptorPool",
            "DestroyCommandPool",
            "MapMemory",
            "UnmapMemory",
            "FreeMemory",
            "DestroySwapchainKHR",
            "GetSwapchainImagesKHR"
        ]
        decl = proto.c_func(attr="VKAPI")
        param0_name = proto.params[0].name
        using_line = ''
        create_line = ''
        destroy_line = ''
        # Dict below tracks params that are vk objects. Dict is "loop count"->["params w/ that loop count"] where '0' is params that aren't in an array
        # TODO : Should integrate slightly better code for this purpose from unique_objects layer
        loop_params = defaultdict(list) # Dict uses loop count as key to make final code generation cleaner so params shared in single loop where needed
        loop_types = defaultdict(list)
        # TODO : For now skipping objs that can be NULL. Really should check these and have special case that allows them to be NULL
        #  or better yet, these should be encoded into an API json definition and we generate checks from there
        #  Until then, this is a dict where each func name is a list of object params that can be null (so don't need to be validated)
        #   param names may be directly passed to the function, or may be a field in a struct param
        valid_null_object_names = {'CreateGraphicsPipelines' : ['basePipelineHandle'],
                                   'CreateComputePipelines' : ['basePipelineHandle'],
                                   'BeginCommandBuffer' : ['renderPass', 'framebuffer'],
                                   'QueueSubmit' : ['fence'],
                                   'AcquireNextImageKHR' : ['fence', 'semaphore' ],
                                   'UpdateDescriptorSets' : ['pTexelBufferView'],
                                   'CreateSwapchainKHR' : ['oldSwapchain'],
                                  }
        param_count = 'NONE' # keep track of arrays passed directly into API functions
        for p in proto.params:
            base_type = p.ty.replace('const ', '').strip('*')
            if 'count' in p.name.lower():
                param_count = p.name
            if base_type in vulkan.core.objects:
                # This is an object to potentially check for validity. First see if it's an array
                if '*' in p.ty and 'const' in p.ty and param_count != 'NONE':
                    loop_params[param_count].append(p.name)
                    loop_types[param_count].append(str(p.ty[6:-1]))
                # Not an array, check for just a base Object that's not in exceptions
                elif '*' not in p.ty and (proto.name not in valid_null_object_names or p.name not in valid_null_object_names[proto.name]):
                    loop_params[0].append(p.name)
                    loop_types[0].append(str(p.ty))
            elif vk_helper.is_type(base_type, 'struct'):
                struct_type = base_type
                if vk_helper.typedef_rev_dict[struct_type] in vk_helper.struct_dict:
                    struct_type = vk_helper.typedef_rev_dict[struct_type]
                # Parse elements of this struct param to identify objects and/or arrays of objects
                for m in sorted(vk_helper.struct_dict[struct_type]):
                    if vk_helper.struct_dict[struct_type][m]['type'] in vulkan.core.objects and vk_helper.struct_dict[struct_type][m]['type'] not in ['VkPhysicalDevice', 'VkQueue', 'VkFence', 'VkImage', 'VkDeviceMemory']:
                        if proto.name not in valid_null_object_names or vk_helper.struct_dict[struct_type][m]['name'] not in valid_null_object_names[proto.name]:
                            # This is not great, but gets the job done for now, but If we have a count and this param is a ptr w/
                            #  last letter 's' OR non-'count' string of count is in the param name, then this is a dynamically sized array param
                            param_array = False
                            if param_count != 'NONE':
                                if '*' in p.ty:
                                    if 's' == p.name[-1] or param_count.lower().replace('count', '') in p.name.lower():
                                        param_array = True
                            if param_array:
                                param_name = '%s[i].%s' % (p.name, vk_helper.struct_dict[struct_type][m]['name'])
                            else:
                                param_name = '%s->%s' % (p.name, vk_helper.struct_dict[struct_type][m]['name'])
                            if vk_helper.struct_dict[struct_type][m]['dyn_array']:
                                if param_count != 'NONE': # this will be a double-embedded loop, use comma delineated 'count,name' for param_name
                                    loop_count = '%s[i].%s' % (p.name, vk_helper.struct_dict[struct_type][m]['array_size'])
                                    loop_params[param_count].append('%s,%s' % (loop_count, param_name))
                                    loop_types[param_count].append('%s' % (vk_helper.struct_dict[struct_type][m]['type']))
                                else:
                                    loop_count = '%s->%s' % (p.name, vk_helper.struct_dict[struct_type][m]['array_size'])
                                    loop_params[loop_count].append(param_name)
                                    loop_types[loop_count].append('%s' % (vk_helper.struct_dict[struct_type][m]['type']))
                            else:
                                if '[' in param_name: # dynamic array param, set size
                                    loop_params[param_count].append(param_name)
                                    loop_types[param_count].append('%s' % (vk_helper.struct_dict[struct_type][m]['type']))
                                else:
                                    loop_params[0].append(param_name)
                                    loop_types[0].append('%s' % (vk_helper.struct_dict[struct_type][m]['type']))
        last_param_index = None
        create_func = False
        if True in [create_txt in proto.name for create_txt in ['Create', 'Allocate']]:
            create_func = True
            last_param_index = -1 # For create funcs don't validate last object
        (struct_uses, local_decls) = get_object_uses(vulkan.object_type_list, proto.params[:last_param_index])
        funcs = []
        mutex_unlock = False
        funcs.append('%s\n' % self.lineinfo.get())
        if proto.name in explicit_object_tracker_functions:
            funcs.append('%s%s\n'
                     '{\n'
                     '    return explicit_%s;\n'
                     '}' % (qual, decl, proto.c_call()))
            return "".join(funcs)
        # Temporarily prevent  DestroySurface call from being generated until WSI layer support is fleshed out
        elif 'DestroyInstance' in proto.name or 'DestroyDevice' in proto.name:
            return ""
        else:
            if create_func:
                typ = proto.params[-1].ty.strip('*').replace('const ', '');
                name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', typ)
                name = re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()[3:]
                create_line =  '    {\n'
                create_line += '        std::lock_guard<std::mutex> lock(global_lock);\n'
                create_line += '        if (result == VK_SUCCESS) {\n'
                create_line += '            create_%s(%s, *%s, %s);\n' % (name, param0_name, proto.params[-1].name, obj_type_mapping[typ])
                create_line += '        }\n'
                create_line += '    }\n'
            if 'FreeCommandBuffers' in proto.name:
                typ = proto.params[-1].ty.strip('*').replace('const ', '');
                name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', typ)
                name = re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()[3:]
                funcs.append('%s\n' % self.lineinfo.get())
                destroy_line =  '    loader_platform_thread_lock_mutex(&objLock);\n'
                destroy_line += '    for (uint32_t i = 0; i < commandBufferCount; i++) {\n'
                destroy_line += '        destroy_%s(%s[i], %s[i]);\n' % (name, proto.params[-1].name, proto.params[-1].name)
                destroy_line += '    }\n'
                destroy_line += '    loader_platform_thread_unlock_mutex(&objLock);\n'
            if 'Destroy' in proto.name:
                typ = proto.params[-2].ty.strip('*').replace('const ', '');
                name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', typ)
                name = re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()[3:]
                funcs.append('%s\n' % self.lineinfo.get())
                destroy_line =  '    {\n'
                destroy_line += '        std::lock_guard<std::mutex> lock(global_lock);\n'
                destroy_line += '        destroy_%s(%s, %s);\n' % (name, param0_name, proto.params[-2].name)
                destroy_line += '    }\n'
            indent = '    '
            if len(struct_uses) > 0:
                using_line += '%sbool skipCall = false;\n' % (indent)
                if not mutex_unlock:
                    using_line += '%s{\n' % (indent)
                    indent += '    '
                    using_line += '%sstd::lock_guard<std::mutex> lock(global_lock);\n' % (indent)
                    mutex_unlock = True
                using_line += '// objects to validate: %s\n' % str(sorted(struct_uses))
                using_line += self._gen_obj_validate_code(struct_uses, obj_type_mapping, proto.name, valid_null_object_names, param0_name, indent, '', 0)
            if mutex_unlock:
                indent = indent[4:]
                using_line += '%s}\n' % (indent)
            if len(struct_uses) > 0:
                using_line += '    if (skipCall)\n'
                if proto.ret == "bool":
                    using_line += '        return false;\n'
                elif proto.ret == "VkBool32":
                    using_line += '        return VK_FALSE;\n'
                elif proto.ret != "void":
                    using_line += '        return VK_ERROR_VALIDATION_FAILED_EXT;\n'
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
            if wsi_name(proto.name):
                funcs.append('%s' % wsi_ifdef(proto.name))
            funcs.append('%s%s\n'
                     '{\n'
                     '%s'
                     '%s'
                     '    %sget_dispatch_table(object_tracker_%s_table_map, %s)->%s;\n'
                     '%s'
                     '%s'
                     '}' % (qual, decl, using_line, destroy_line, ret_val, table_type, dispatch_param, proto.c_call(), create_line, stmt))
            if wsi_name(proto.name):
                funcs.append('%s' % wsi_endif(proto.name))
        return "\n\n".join(funcs)

    def generate_body(self):
        self.layer_name = "object_tracker"
        extensions=[('wsi_enabled',
                     ['vkCreateSwapchainKHR',
                      'vkDestroySwapchainKHR', 'vkGetSwapchainImagesKHR',
                      'vkAcquireNextImageKHR', 'vkQueuePresentKHR'])]
        if self.wsi == 'Win32':
            instance_extensions=[('msg_callback_get_proc_addr', []),
                                  ('wsi_enabled',
                                  ['vkDestroySurfaceKHR',
                                   'vkGetPhysicalDeviceSurfaceSupportKHR',
                                   'vkGetPhysicalDeviceSurfaceCapabilitiesKHR',
                                   'vkGetPhysicalDeviceSurfaceFormatsKHR',
                                   'vkGetPhysicalDeviceSurfacePresentModesKHR',
                                   'vkCreateWin32SurfaceKHR',
                                   'vkGetPhysicalDeviceWin32PresentationSupportKHR'])]
        elif self.wsi == 'Android':
            instance_extensions=[('msg_callback_get_proc_addr', []),
                                  ('wsi_enabled',
                                  ['vkDestroySurfaceKHR',
                                   'vkGetPhysicalDeviceSurfaceSupportKHR',
                                   'vkGetPhysicalDeviceSurfaceCapabilitiesKHR',
                                   'vkGetPhysicalDeviceSurfaceFormatsKHR',
                                   'vkGetPhysicalDeviceSurfacePresentModesKHR',
                                   'vkCreateAndroidSurfaceKHR'])]
        elif self.wsi == 'Xcb' or self.wsi == 'Xlib' or self.wsi == 'Wayland' or self.wsi == 'Mir':
            instance_extensions=[('msg_callback_get_proc_addr', []),
                                  ('wsi_enabled',
                                  ['vkDestroySurfaceKHR',
                                   'vkGetPhysicalDeviceSurfaceSupportKHR',
                                   'vkGetPhysicalDeviceSurfaceCapabilitiesKHR',
                                   'vkGetPhysicalDeviceSurfaceFormatsKHR',
                                   'vkGetPhysicalDeviceSurfacePresentModesKHR',
                                   'vkCreateXcbSurfaceKHR',
                                   'vkGetPhysicalDeviceXcbPresentationSupportKHR',
                                   'vkCreateXlibSurfaceKHR',
                                   'vkGetPhysicalDeviceXlibPresentationSupportKHR',
                                   'vkCreateWaylandSurfaceKHR',
                                   'vkGetPhysicalDeviceWaylandPresentationSupportKHR',
                                   'vkCreateMirSurfaceKHR',
                                   'vkGetPhysicalDeviceMirPresentationSupportKHR'])]
        else:
            print('Error: Undefined DisplayServer')
            instance_extensions=[]

        body = ["namespace %s {" % self.layer_name,
                self.generate_maps(),
                self.generate_procs(),
                self.generate_destroy_instance(),
                self.generate_destroy_device(),
                self._generate_dispatch_entrypoints(),
                self._generate_extensions(),
                self._generate_layer_introspection_function(),
                self._generate_layer_gpa_function(extensions,
                                                  instance_extensions),
                "} // namespace %s" % self.layer_name,
                self._gen_layer_logging_workaround(),
                self._gen_layer_interface_v0_functions()]
        return "\n\n".join(body)

class UniqueObjectsSubcommand(Subcommand):
    def generate_header(self):
        header_txt = []
        header_txt.append('%s' % self.lineinfo.get())
        header_txt.append('#include "unique_objects.h"')
        return "\n".join(header_txt)

    # Generate UniqueObjects code for given struct_uses dict of objects that need to be unwrapped
    # vector_name_set is used to make sure we don't replicate vector names
    # first_level_param indicates if elements are passed directly into the function else they're below a ptr/struct
    # TODO : Comment this code
    def _gen_obj_code(self, struct_uses, param_type, indent, prefix, array_index, vector_name_set, first_level_param):
        decls = ''
        pre_code = ''
        post_code = ''
        for obj in sorted(struct_uses):
            name = obj
            array = ''
            if '[' in obj:
                (name, array) = obj.split('[')
                array = array.strip(']')
            ptr_type = False
            if 'p' == obj[0] and obj[1] != obj[1].lower(): # TODO : Not ideal way to determine ptr
                ptr_type = True
            if isinstance(struct_uses[obj], dict):
                local_prefix = ''
                name = '%s%s' % (prefix, name)
                if ptr_type:
                    if first_level_param and name in param_type:
                        pre_code += '%sif (%s) {\n' % (indent, name)
                    else: # shadow ptr will have been initialized at this point so check it vs. source ptr
                        pre_code += '%sif (local_%s) {\n' % (indent, name)
                    indent += '    '
                if array != '':
                    idx = 'idx%s' % str(array_index)
                    array_index += 1
                    if first_level_param and name in param_type:
                        pre_code += '%slocal_%s = new safe_%s[%s];\n' % (indent, name, param_type[name].strip('*'), array)
                        post_code += '    if (local_%s)\n' % (name)
                        post_code += '        delete[] local_%s;\n' % (name)
                    pre_code += '%sfor (uint32_t %s=0; %s<%s%s; ++%s) {\n' % (indent, idx, idx, prefix, array, idx)
                    indent += '    '
                    if first_level_param:
                        pre_code += '%slocal_%s[%s].initialize(&%s[%s]);\n' % (indent, name, idx, name, idx)
                    local_prefix = '%s[%s].' % (name, idx)
                elif ptr_type:
                    if first_level_param and name in param_type:
                        pre_code += '%slocal_%s = new safe_%s(%s);\n' % (indent, name, param_type[name].strip('*'), name)
                        post_code += '    if (local_%s)\n' % (name)
                        post_code += '        delete local_%s;\n' % (name)
                    local_prefix = '%s->' % (name)
                else:
                    local_prefix = '%s.' % (name)
                assert isinstance(decls, object)
                (tmp_decl, tmp_pre, tmp_post) = self._gen_obj_code(struct_uses[obj], param_type, indent, local_prefix, array_index, vector_name_set, False)
                decls += tmp_decl
                pre_code += tmp_pre
                post_code += tmp_post
                if array != '':
                    indent = indent[4:]
                    pre_code += '%s}\n' % (indent)
                if ptr_type:
                    indent = indent[4:]
                    pre_code += '%s}\n' % (indent)
            else:
                if (array_index > 0) or array != '': # TODO : This is not ideal, really want to know if we're anywhere under an array
                    if first_level_param:
                        decls += '%s%s* local_%s = NULL;\n' % (indent, struct_uses[obj], name)
                    if array != '' and not first_level_param: # ptrs under structs will have been initialized so use local_*
                        pre_code += '%sif (local_%s%s) {\n' %(indent, prefix, name)
                    else:
                        pre_code += '%sif (%s%s) {\n' %(indent, prefix, name)
                    indent += '    '
                    if array != '':
                        idx = 'idx%s' % str(array_index)
                        array_index += 1
                        if first_level_param:
                            pre_code += '%slocal_%s = new %s[%s];\n' % (indent, name, struct_uses[obj], array)
                            post_code += '    if (local_%s)\n' % (name)
                            post_code += '        delete[] local_%s;\n' % (name)
                        pre_code += '%sfor (uint32_t %s=0; %s<%s%s; ++%s) {\n' % (indent, idx, idx, prefix, array, idx)
                        indent += '    '
                        name = '%s[%s]' % (name, idx)
                    pName = 'p%s' % (struct_uses[obj][2:])
                    if name not in vector_name_set:
                        vector_name_set.add(name)
                    pre_code += '%slocal_%s%s = (%s)my_map_data->unique_id_mapping[reinterpret_cast<const uint64_t &>(%s%s)];\n' % (indent, prefix, name, struct_uses[obj], prefix, name)
                    if array != '':
                        indent = indent[4:]
                        pre_code += '%s}\n' % (indent)
                    indent = indent[4:]
                    pre_code += '%s}\n' % (indent)
                else:
                    pre_code += '%s\n' % (self.lineinfo.get())
                    deref_txt = '&'
                    if ptr_type:
                        deref_txt = ''
                    if '->' in prefix: # need to update local struct
                        pre_code += '%slocal_%s%s = (%s)my_map_data->unique_id_mapping[reinterpret_cast<const uint64_t &>(%s%s)];\n' % (indent, prefix, name, struct_uses[obj], prefix, name)
                    else:
                        pre_code += '%s%s = (%s)my_map_data->unique_id_mapping[reinterpret_cast<uint64_t &>(%s)];\n' % (indent, name, struct_uses[obj], name)
        return decls, pre_code, post_code

    def generate_intercept(self, proto, qual):
        create_func = False
        destroy_func = False
        last_param_index = None #typcially we look at all params for ndos
        pre_call_txt = '' # code prior to calling down chain such as unwrap uses of ndos
        post_call_txt = '' # code following call down chain such to wrap newly created ndos, or destroy local wrap struct
        funcs = []
        indent = '    ' # indent level for generated code
        decl = proto.c_func(attr="VKAPI")
        # A few API cases that are manual code
        # TODO : Special case Create*Pipelines funcs to handle creating multiple unique objects
        explicit_object_tracker_functions = ['GetSwapchainImagesKHR',
                                             'CreateSwapchainKHR',
                                             'CreateInstance',
                                             'DestroyInstance',
                                             'CreateDevice',
                                             'DestroyDevice',
                                             'CreateComputePipelines',
                                             'CreateGraphicsPipelines'
                                             ]
        # TODO : This is hacky, need to make this a more general-purpose solution for all layers
        ifdef_dict = {'CreateXcbSurfaceKHR': 'VK_USE_PLATFORM_XCB_KHR',
                      'CreateAndroidSurfaceKHR': 'VK_USE_PLATFORM_ANDROID_KHR',
                      'CreateWin32SurfaceKHR': 'VK_USE_PLATFORM_WIN32_KHR',
                      'CreateXlibSurfaceKHR': 'VK_USE_PLATFORM_XLIB_KHR',
                      'CreateWaylandSurfaceKHR': 'VK_USE_PLATFORM_WAYLAND_KHR',
                      'CreateMirSurfaceKHR': 'VK_USE_PLATFORM_MIR_KHR'}
        # Give special treatment to create functions that return multiple new objects
        # This dict stores array name and size of array
        custom_create_dict = {'pDescriptorSets' : 'pAllocateInfo->descriptorSetCount'}
        pre_call_txt += '%s\n' % (self.lineinfo.get())
        if proto.name in explicit_object_tracker_functions:
            funcs.append('%s%s\n'
                     '{\n'
                     '    return explicit_%s;\n'
                     '}' % (qual, decl, proto.c_call()))
            return "".join(funcs)
        if True in [create_txt in proto.name for create_txt in ['Create', 'Allocate']]:
            create_func = True
            last_param_index = -1 # For create funcs don't care if last param is ndo
        if True in [destroy_txt in proto.name for destroy_txt in ['Destroy', 'Free']]:
            destroy_obj_type = proto.params[-2].ty
            if destroy_obj_type in vulkan.object_non_dispatch_list:
                destroy_func = True

        # First thing we need to do is gather uses of non-dispatchable-objects (ndos)
        (struct_uses, local_decls) = get_object_uses(vulkan.object_non_dispatch_list, proto.params[1:last_param_index])

        dispatch_param = proto.params[0].name
        if 'CreateInstance' in proto.name:
           dispatch_param = '*' + proto.params[1].name
        pre_call_txt += '%slayer_data *my_map_data = get_my_data_ptr(get_dispatch_key(%s), layer_data_map);\n' % (indent, dispatch_param)
        if len(struct_uses) > 0:
            pre_call_txt += '// STRUCT USES:%s\n' % sorted(struct_uses)
            if len(local_decls) > 0:
                pre_call_txt += '//LOCAL DECLS:%s\n' % sorted(local_decls)
            if destroy_func: # only one object
                pre_call_txt += '%sstd::unique_lock<std::mutex> lock(global_lock);\n' % (indent)
                for del_obj in sorted(struct_uses):
                    pre_call_txt += '%suint64_t local_%s = reinterpret_cast<uint64_t &>(%s);\n' % (indent, del_obj, del_obj)
                    pre_call_txt += '%s%s = (%s)my_map_data->unique_id_mapping[local_%s];\n' % (indent, del_obj, struct_uses[del_obj], del_obj)
                pre_call_txt += '%slock.unlock();\n' % (indent)
                (pre_decl, pre_code, post_code) = ('', '', '')
            else:
                (pre_decl, pre_code, post_code) = self._gen_obj_code(struct_uses, local_decls, '    ', '', 0, set(), True)
            # This is a bit hacky but works for now. Need to decl local versions of top-level structs
            for ld in sorted(local_decls):
                init_null_txt = 'NULL';
                if '*' not in local_decls[ld]:
                    init_null_txt = '{}';
                if local_decls[ld].strip('*') not in vulkan.object_non_dispatch_list:
                    pre_decl += '    safe_%s local_%s = %s;\n' % (local_decls[ld], ld, init_null_txt)
            if pre_code != '': # lock around map uses
                pre_code = '%s{\n%sstd::lock_guard<std::mutex> lock(global_lock);\n%s%s}\n' % (indent, indent, pre_code, indent)
            pre_call_txt += '%s%s' % (pre_decl, pre_code)
            post_call_txt += '%s' % (post_code)
        elif create_func:
            base_type = proto.params[-1].ty.replace('const ', '').strip('*')
            if base_type not in vulkan.object_non_dispatch_list:
                return None
        else:
            return None

        ret_val = ''
        ret_stmt = ''
        if proto.ret != "void":
            ret_val = "%s result = " % proto.ret
            ret_stmt = "    return result;\n"
        if create_func:
            obj_type = proto.params[-1].ty.strip('*')
            obj_name = proto.params[-1].name
            if obj_type in vulkan.object_non_dispatch_list:
                local_name = "unique%s" % obj_type[2:]
                post_call_txt += '%sif (VK_SUCCESS == result) {\n' % (indent)
                indent += '    '
                post_call_txt += '%sstd::lock_guard<std::mutex> lock(global_lock);\n' % (indent)
                if obj_name in custom_create_dict:
                    post_call_txt += '%s\n' % (self.lineinfo.get())
                    local_name = '%ss' % (local_name) # add 's' to end for vector of many
                    post_call_txt += '%sfor (uint32_t i=0; i<%s; ++i) {\n' % (indent, custom_create_dict[obj_name])
                    indent += '    '
                    post_call_txt += '%suint64_t unique_id = global_unique_id++;\n' % (indent)
                    post_call_txt += '%smy_map_data->unique_id_mapping[unique_id] = reinterpret_cast<uint64_t &>(%s[i]);\n' % (indent, obj_name)
                    post_call_txt += '%s%s[i] = reinterpret_cast<%s&>(unique_id);\n' % (indent, obj_name, obj_type)
                    indent = indent[4:]
                    post_call_txt += '%s}\n' % (indent)
                else:
                    post_call_txt += '%s\n' % (self.lineinfo.get())
                    post_call_txt += '%suint64_t unique_id = global_unique_id++;\n' % (indent)
                    post_call_txt += '%smy_map_data->unique_id_mapping[unique_id] = reinterpret_cast<uint64_t &>(*%s);\n' % (indent, obj_name)
                    post_call_txt += '%s*%s = reinterpret_cast<%s&>(unique_id);\n' % (indent, obj_name, obj_type)
                indent = indent[4:]
                post_call_txt += '%s}\n' % (indent)
        elif destroy_func:
            del_obj = proto.params[-2].name
            if 'count' in del_obj.lower():
                post_call_txt += '%s\n' % (self.lineinfo.get())
                post_call_txt += '%sfor (uint32_t i=0; i<%s; ++i) {\n' % (indent, del_obj)
                del_obj = proto.params[-1].name
                indent += '    '
                post_call_txt += '%sdelete (VkUniqueObject*)%s[i];\n' % (indent, del_obj)
                indent = indent[4:]
                post_call_txt += '%s}\n' % (indent)
            else:
                post_call_txt += '%s\n' % (self.lineinfo.get())
                post_call_txt += '%slock.lock();\n' % (indent)
                post_call_txt += '%smy_map_data->unique_id_mapping.erase(local_%s);\n' % (indent, proto.params[-2].name)

        call_sig = proto.c_call()
        # Replace default params with any custom local params
        for ld in local_decls:
            call_sig = call_sig.replace(ld, '(const %s)local_%s' % (local_decls[ld], ld))
        if proto_is_global(proto):
            table_type = "instance"
        else:
            table_type = "device"
        pre_call_txt += '%s\n' % (self.lineinfo.get())
        open_ifdef = ''
        close_ifdef = ''
        if proto.name in ifdef_dict:
            open_ifdef = '#ifdef %s\n' % (ifdef_dict[proto.name])
            close_ifdef = '#endif\n'
        funcs.append('%s'
                     '%s%s\n'
                     '{\n'
                     '%s'
                     '    %sget_dispatch_table(unique_objects_%s_table_map, %s)->%s;\n'
                     '%s'
                     '%s'
                     '}\n'
                     '%s' % (open_ifdef, qual, decl, pre_call_txt, ret_val, table_type, dispatch_param, call_sig, post_call_txt, ret_stmt, close_ifdef))
        return "\n\n".join(funcs)

    def generate_body(self):
        self.layer_name = "unique_objects"
        extensions=[('wsi_enabled',
                     ['vkCreateSwapchainKHR',
                      'vkDestroySwapchainKHR', 'vkGetSwapchainImagesKHR',
                      'vkAcquireNextImageKHR', 'vkQueuePresentKHR'])]
        if self.wsi == 'Win32':
            instance_extensions=[('wsi_enabled',
                                  ['vkDestroySurfaceKHR',
                                   'vkGetPhysicalDeviceSurfaceSupportKHR',
                                   'vkGetPhysicalDeviceSurfaceCapabilitiesKHR',
                                   'vkGetPhysicalDeviceSurfaceFormatsKHR',
                                   'vkGetPhysicalDeviceSurfacePresentModesKHR',
                                   'vkCreateWin32SurfaceKHR'
                                   ])]
        elif self.wsi == 'Android':
            instance_extensions=[('wsi_enabled',
                                  ['vkDestroySurfaceKHR',
                                   'vkGetPhysicalDeviceSurfaceSupportKHR',
                                   'vkGetPhysicalDeviceSurfaceCapabilitiesKHR',
                                   'vkGetPhysicalDeviceSurfaceFormatsKHR',
                                   'vkGetPhysicalDeviceSurfacePresentModesKHR',
                                   'vkCreateAndroidSurfaceKHR'])]
        elif self.wsi == 'Xcb' or self.wsi == 'Xlib' or self.wsi == 'Wayland' or self.wsi == 'Mir':
            instance_extensions=[('wsi_enabled',
                                  ['vkDestroySurfaceKHR',
                                   'vkGetPhysicalDeviceSurfaceSupportKHR',
                                   'vkGetPhysicalDeviceSurfaceCapabilitiesKHR',
                                   'vkGetPhysicalDeviceSurfaceFormatsKHR',
                                   'vkGetPhysicalDeviceSurfacePresentModesKHR',
                                   'vkCreateXcbSurfaceKHR',
                                   'vkCreateXlibSurfaceKHR',
                                   'vkCreateWaylandSurfaceKHR',
                                   'vkCreateMirSurfaceKHR'
                                   ])]
        else:
            print('Error: Undefined DisplayServer')
            instance_extensions=[]

        body = ["namespace %s {" % self.layer_name,
                self._generate_dispatch_entrypoints(),
                self._generate_layer_introspection_function(),
                self._generate_layer_gpa_function(extensions,
                                                  instance_extensions),
                "} // namespace %s" % self.layer_name,
                self._gen_layer_interface_v0_functions()]
        return "\n\n".join(body)

def main():
    wsi = {
            "Win32",
            "Android",
            "Xcb",
            "Xlib",
            "Wayland",
            "Mir",
    }

    subcommands = {
            "object_tracker" : ObjectTrackerSubcommand,
            "unique_objects" : UniqueObjectsSubcommand,
    }

    if len(sys.argv) < 4 or sys.argv[1] not in wsi or sys.argv[2] not in subcommands or not os.path.exists(sys.argv[3]):
        print("Usage: %s <wsi> <subcommand> <input_header> [outdir]" % sys.argv[0])
        print
        print("Available subcommands are: %s" % " ".join(subcommands))
        exit(1)

    hfp = vk_helper.HeaderFileParser(sys.argv[3])
    hfp.parse()
    vk_helper.enum_val_dict = hfp.get_enum_val_dict()
    vk_helper.enum_type_dict = hfp.get_enum_type_dict()
    vk_helper.struct_dict = hfp.get_struct_dict()
    vk_helper.typedef_fwd_dict = hfp.get_typedef_fwd_dict()
    vk_helper.typedef_rev_dict = hfp.get_typedef_rev_dict()
    vk_helper.types_dict = hfp.get_types_dict()

    outfile = None
    if len(sys.argv) >= 5:
        outfile = sys.argv[4]

    subcmd = subcommands[sys.argv[2]](outfile)
    subcmd.run()

if __name__ == "__main__":
    main()
