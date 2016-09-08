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
        "GetPhysicalDeviceWin32PresentationSupportKHR",
        "GetPhysicalDeviceDisplayPropertiesKHR",
        "GetPhysicalDeviceDisplayPlanePropertiesKHR",
        "GetDisplayPlaneSupportedDisplaysKHR",
        "GetDisplayModePropertiesKHR",
        "CreateDisplayModeKHR",
        "GetDisplayPlaneCapabilitiesKHR",
        "CreateDisplayPlaneSurfaceKHR"
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
        if self.layer_name in ['unique_objects']:
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
        if self.layer_name in ['unique_objects']:
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
        if self.layer_name in ['unique_objects']:
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

            # The WSI-related extensions have independent extension enables
            wsi_sub_enables = {'WIN32': 'win32_enabled',
                               'XLIB': 'xlib_enabled',
                               'XCB': 'xcb_enabled',
                               'MIR': 'mir_enabled',
                               'WAYLAND': 'wayland_enabled',
                               'ANDROID': 'android_enabled'}

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
                            if wsi_sub_enables[wsi_name(ext_name)]:
                                func_body.append('    if ((instanceExtMap[pTable].%s == true) && !strcmp("%s", name))\n'
                                                 '        return reinterpret_cast<PFN_vkVoidFunction>(%s);' % (wsi_sub_enables[wsi_name(ext_name)], ext_name, ext_name[2:]))
                        else:
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
                    if 'p' == array[0] and array[1] != array[1].lower(): # TODO : Not ideal way to determine ptr
                        count_prefix = '*'
                    else:
                        count_prefix = ''
                    idx = 'idx%s' % str(array_index)
                    array_index += 1
                    if first_level_param and name in param_type:
                        pre_code += '%slocal_%s = new safe_%s[%s%s];\n' % (indent, name, param_type[name].strip('*'), count_prefix, array)
                        post_code += '    if (local_%s)\n' % (name)
                        post_code += '        delete[] local_%s;\n' % (name)
                    pre_code += '%sfor (uint32_t %s=0; %s<%s%s%s; ++%s) {\n' % (indent, idx, idx, count_prefix, prefix, array, idx)
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
        explicit_unique_objects_functions = ['GetSwapchainImagesKHR',
                                             'CreateSwapchainKHR',
                                             'CreateInstance',
                                             'DestroyInstance',
                                             'CreateDevice',
                                             'DestroyDevice',
                                             'AllocateMemory',
                                             'CreateComputePipelines',
                                             'CreateGraphicsPipelines',
                                             'GetPhysicalDeviceDisplayPropertiesKHR',
                                             'GetDisplayPlaneSupportedDisplaysKHR',
                                             'GetDisplayModePropertiesKHR'
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
        if proto.name in explicit_unique_objects_functions:
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
                pre_call_txt += '%smy_map_data->unique_id_mapping.erase(local_%s);\n' % (indent, proto.params[-2].name)
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

        call_sig = proto.c_call()
        # Replace default params with any custom local params
        for ld in local_decls:
            const_qualifier = ''
            for p in proto.params:
                if ld == p.name and 'const' in p.ty:
                    const_qualifier = 'const'
            call_sig = call_sig.replace(ld, '(%s %s)local_%s' % (const_qualifier, local_decls[ld], ld))
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
        surface_wsi_instance_exts = [
                      'vkDestroySurfaceKHR',
                      'vkGetPhysicalDeviceSurfaceSupportKHR',
                      'vkGetPhysicalDeviceSurfaceCapabilitiesKHR',
                      'vkGetPhysicalDeviceSurfaceFormatsKHR',
                      'vkGetPhysicalDeviceSurfacePresentModesKHR']
        display_wsi_instance_exts = [
                      'vkGetPhysicalDeviceDisplayPropertiesKHR',
                      'vkGetPhysicalDeviceDisplayPlanePropertiesKHR',
                      'vkGetDisplayPlaneSupportedDisplaysKHR',
                      'vkGetDisplayModePropertiesKHR',
                      'vkCreateDisplayModeKHR',
                      'vkGetDisplayPlaneCapabilitiesKHR',
                      'vkCreateDisplayPlaneSurfaceKHR']
        if self.wsi == 'Win32':
            instance_extensions=[('wsi_enabled', surface_wsi_instance_exts),
                                 ('display_enabled', display_wsi_instance_exts),
                                 ('win32_enabled', ['vkCreateWin32SurfaceKHR'])]
        elif self.wsi == 'Android':
            instance_extensions=[('wsi_enabled', surface_wsi_instance_exts),
                                 ('android_enabled', ['vkCreateAndroidSurfaceKHR'])]
        elif self.wsi == 'Xcb' or self.wsi == 'Xlib' or self.wsi == 'Wayland' or self.wsi == 'Mir':
            instance_extensions=[('wsi_enabled', surface_wsi_instance_exts),
                                 ('display_enabled', display_wsi_instance_exts),
                                 ('xcb_enabled', ['vkCreateXcbSurfaceKHR']),
                                 ('xlib_enabled', ['vkCreateXlibSurfaceKHR']),
                                 ('wayland_enabled',  ['vkCreateWaylandSurfaceKHR']),
                                 ('mir_enabled', ['vkCreateMirSurfaceKHR'])]
        elif self.wsi == 'Display':
            instance_extensions=[('wsi_enabled', surface_wsi_instance_exts),
                                 ('display_enabled', display_wsi_instance_exts)]
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
            "Display",
    }

    subcommands = {
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
