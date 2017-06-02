#!/usr/bin/python3 -i
#
# Copyright (c) 2015-2017 The Khronos Group Inc.
# Copyright (c) 2015-2017 Valve Corporation
# Copyright (c) 2015-2017 LunarG, Inc.
# Copyright (c) 2015-2017 Google Inc.
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
# Author: Mark Young <marky@lunarg.com>

import os,re,sys
import xml.etree.ElementTree as etree
from generator import *
from collections import namedtuple

WSI_EXT_NAMES = ['VK_KHR_surface',
                 'VK_KHR_display',
                 'VK_KHR_xlib_surface',
                 'VK_KHR_xcb_surface',
                 'VK_KHR_wayland_surface',
                 'VK_KHR_mir_surface',
                 'VK_KHR_win32_surface',
                 'VK_KHR_android_surface',
                 'VK_KHR_swapchain',
                 'VK_KHR_display_swapchain']

AVOID_EXT_NAMES = ['VK_EXT_debug_report']

DEVICE_CMDS_NEED_TERM = ['vkGetDeviceProcAddr',
                         'vkCreateSwapchainKHR',
                         'vkCreateSharedSwapchainsKHR',
                         'vkGetDeviceGroupSurfacePresentModesKHX',
                         'vkDebugMarkerSetObjectTagEXT',
                         'vkDebugMarkerSetObjectNameEXT']

#
# LoaderExtensionGeneratorOptions - subclass of GeneratorOptions.
class LoaderExtensionGeneratorOptions(GeneratorOptions):
    def __init__(self,
                 filename = None,
                 directory = '.',
                 apiname = None,
                 profile = None,
                 versions = '.*',
                 emitversions = '.*',
                 defaultExtensions = None,
                 addExtensions = None,
                 removeExtensions = None,
                 sortProcedure = regSortFeatures,
                 prefixText = "",
                 genFuncPointers = True,
                 protectFile = True,
                 protectFeature = True,
                 protectProto = None,
                 protectProtoStr = None,
                 apicall = '',
                 apientry = '',
                 apientryp = '',
                 alignFuncParam = 0,
                 currentExtension = '',
                 extensionOfInterest = 0):
        GeneratorOptions.__init__(self, filename, directory, apiname, profile,
                                  versions, emitversions, defaultExtensions,
                                  addExtensions, removeExtensions, sortProcedure)
        self.prefixText      = prefixText
        self.genFuncPointers = genFuncPointers
        self.prefixText      = None
        self.protectFile     = protectFile
        self.protectFeature  = protectFeature
        self.protectProto    = protectProto
        self.protectProtoStr = protectProtoStr
        self.apicall         = apicall
        self.apientry        = apientry
        self.apientryp       = apientryp
        self.alignFuncParam  = alignFuncParam
#
# LoaderExtensionOutputGenerator - subclass of OutputGenerator.
# Generates dispatch table helper header files for LVL
class LoaderExtensionOutputGenerator(OutputGenerator):
    """Generate dispatch table helper header based on XML element attributes"""
    def __init__(self,
                 errFile = sys.stderr,
                 warnFile = sys.stderr,
                 diagFile = sys.stdout):
        OutputGenerator.__init__(self, errFile, warnFile, diagFile)

        # Internal state - accumulators for different inner block text
        self.ext_instance_dispatch_list = []  # List of extension entries for instance dispatch list
        self.ext_device_dispatch_list = []    # List of extension entries for device dispatch list
        self.core_commands = []               # List of CommandData records for core Vulkan commands
        self.ext_commands = []                # List of CommandData records for extension Vulkan commands
        self.CommandParam = namedtuple('CommandParam', ['type', 'name', 'cdecl'])
        self.CommandData = namedtuple('CommandData', ['name', 'ext_name', 'ext_type', 'protect', 'return_type', 'handle_type', 'params', 'cdecl'])
        self.instanceExtensions = []
        self.ExtensionData = namedtuple('ExtensionData', ['name', 'type', 'protect', 'define', 'num_commands'])

    #
    # Called once at the beginning of each run
    def beginFile(self, genOpts):
        OutputGenerator.beginFile(self, genOpts)

        # User-supplied prefix text, if any (list of strings)
        if (genOpts.prefixText):
            for s in genOpts.prefixText:
                write(s, file=self.outFile)

        # File Comment
        file_comment = '// *** THIS FILE IS GENERATED - DO NOT EDIT ***\n'
        file_comment += '// See loader_extension_generator.py for modifications\n'
        write(file_comment, file=self.outFile)

        # Copyright Notice
        copyright =  '/*\n'
        copyright += ' * Copyright (c) 2015-2017 The Khronos Group Inc.\n'
        copyright += ' * Copyright (c) 2015-2017 Valve Corporation\n'
        copyright += ' * Copyright (c) 2015-2017 LunarG, Inc.\n'
        copyright += ' *\n'
        copyright += ' * Licensed under the Apache License, Version 2.0 (the "License");\n'
        copyright += ' * you may not use this file except in compliance with the License.\n'
        copyright += ' * You may obtain a copy of the License at\n'
        copyright += ' *\n'
        copyright += ' *     http://www.apache.org/licenses/LICENSE-2.0\n'
        copyright += ' *\n'
        copyright += ' * Unless required by applicable law or agreed to in writing, software\n'
        copyright += ' * distributed under the License is distributed on an "AS IS" BASIS,\n'
        copyright += ' * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n'
        copyright += ' * See the License for the specific language governing permissions and\n'
        copyright += ' * limitations under the License.\n'
        copyright += ' *\n'
        copyright += ' * Author: Mark Lobodzinski <mark@lunarg.com>\n'
        copyright += ' * Author: Mark Young <marky@lunarg.com>\n'
        copyright += ' */\n'

        preamble = ''

        if self.genOpts.filename == 'vk_loader_extensions.h':
            preamble += '#pragma once\n'

        elif self.genOpts.filename == 'vk_loader_extensions.c':
            preamble += '#define _GNU_SOURCE\n'
            preamble += '#include <stdio.h>\n'
            preamble += '#include <stdlib.h>\n'
            preamble += '#include <string.h>\n'
            preamble += '#include "vk_loader_platform.h"\n'
            preamble += '#include "loader.h"\n'
            preamble += '#include "vk_loader_extensions.h"\n'
            preamble += '#include <vulkan/vk_icd.h>\n'
            preamble += '#include "wsi.h"\n'
            preamble += '#include "debug_report.h"\n'
            preamble += '#include "extension_manual.h"\n'

        elif self.genOpts.filename == 'vk_layer_dispatch_table.h':
            preamble += '#pragma once\n'
            preamble += '\n'
            preamble += 'typedef PFN_vkVoidFunction (VKAPI_PTR *PFN_GetPhysicalDeviceProcAddr)(VkInstance instance, const char* pName);\n'

        write(copyright, file=self.outFile)
        write(preamble, file=self.outFile)

    #
    # Write generate and write dispatch tables to output file
    def endFile(self):
        file_data = ''

        if self.genOpts.filename == 'vk_loader_extensions.h':
            file_data += self.OutputPrototypesInHeader()
            file_data += self.OutputLoaderTerminators()
            file_data += self.OutputIcdDispatchTable()
            file_data += self.OutputIcdExtensionEnableUnion()

        elif self.genOpts.filename == 'vk_loader_extensions.c':
            file_data += self.OutputUtilitiesInSource()
            file_data += self.OutputIcdDispatchTableInit()
            file_data += self.OutputLoaderDispatchTables()
            file_data += self.OutputLoaderLookupFunc()
            file_data += self.CreateTrampTermFuncs()
            file_data += self.InstExtensionGPA()
            file_data += self.InstantExtensionCreate()
            file_data += self.DeviceExtensionGetTerminator()
            file_data += self.InitInstLoaderExtensionDispatchTable()
            file_data += self.OutputInstantExtensionWhitelistArray()

        elif self.genOpts.filename == 'vk_layer_dispatch_table.h':
            file_data += self.OutputLayerInstanceDispatchTable()
            file_data += self.OutputLayerDeviceDispatchTable()

        write(file_data, file=self.outFile);

        # Finish processing in superclass
        OutputGenerator.endFile(self)

    def beginFeature(self, interface, emit):
        # Start processing in superclass
        OutputGenerator.beginFeature(self, interface, emit)

        enums = interface[0].findall('enum')
        self.currentExtension = ''
        self.name_definition = ''

        for item in enums:
            name_definition = item.get('name')
            if 'EXTENSION_NAME' in name_definition:
                self.name_definition = name_definition

        self.type = interface.get('type')
        self.num_commands = 0
        name = interface.get('name')
        self.currentExtension = name 

    #
    # Process commands, adding to appropriate dispatch tables
    def genCmd(self, cmdinfo, name):
        OutputGenerator.genCmd(self, cmdinfo, name)

        # Get first param type
        params = cmdinfo.elem.findall('param')
        info = self.getTypeNameTuple(params[0])

        self.num_commands += 1

        if 'android' not in name:
            self.AddCommandToDispatchList(self.currentExtension, self.type, name, cmdinfo, info[0])

    def endFeature(self):

        if 'android' not in self.currentExtension:
            self.instanceExtensions.append(self.ExtensionData(name=self.currentExtension,
                                                              type=self.type,
                                                              protect=self.featureExtraProtect,
                                                              define=self.name_definition,
                                                              num_commands=self.num_commands))

        # Finish processing in superclass
        OutputGenerator.endFeature(self)

    #
    # Retrieve the value of the len tag
    def getLen(self, param):
        result = None
        len = param.attrib.get('len')
        if len and len != 'null-terminated':
            # For string arrays, 'len' can look like 'count,null-terminated',
            # indicating that we have a null terminated array of strings.  We
            # strip the null-terminated from the 'len' field and only return
            # the parameter specifying the string count
            if 'null-terminated' in len:
                result = len.split(',')[0]
            else:
                result = len
            result = str(result).replace('::', '->')
        return result

    #
    # Determine if this API should be ignored or added to the instance or device dispatch table
    def AddCommandToDispatchList(self, extension_name, extension_type, name, cmdinfo, handle_type):
        handle = self.registry.tree.find("types/type/[name='" + handle_type + "'][@category='handle']")

        return_type =  cmdinfo.elem.find('proto/type')
        if (return_type != None and return_type.text == 'void'):
           return_type = None

        cmd_params = []

        # Generate a list of commands for use in printing the necessary
        # core instance terminator prototypes
        params = cmdinfo.elem.findall('param')
        lens = set()
        for param in params:
            len = self.getLen(param)
            if len:
                lens.add(len)
        paramsInfo = []
        for param in params:
            paramInfo = self.getTypeNameTuple(param)
            param_type = paramInfo[0]
            param_name = paramInfo[1]
            param_cdecl = self.makeCParamDecl(param, 0)
            cmd_params.append(self.CommandParam(type=param_type, name=param_name,
                                                cdecl=param_cdecl))

        if handle != None and handle_type != 'VkInstance' and handle_type != 'VkPhysicalDevice':
            # The Core Vulkan code will be wrapped in a feature called VK_VERSION_#_#
            # For example: VK_VERSION_1_0 wraps the core 1.0 Vulkan functionality
            if 'VK_VERSION_' in extension_name:
                self.core_commands.append(
                    self.CommandData(name=name, ext_name=extension_name,
                                     ext_type='device',
                                     protect=self.featureExtraProtect,
                                     return_type = return_type,
                                     handle_type = handle_type,
                                     params = cmd_params,
                                     cdecl=self.makeCDecls(cmdinfo.elem)[0]))
            else:
                self.ext_device_dispatch_list.append((name, self.featureExtraProtect))
                self.ext_commands.append(
                    self.CommandData(name=name, ext_name=extension_name,
                                     ext_type=extension_type,
                                     protect=self.featureExtraProtect,
                                     return_type = return_type,
                                     handle_type = handle_type,
                                     params = cmd_params,
                                     cdecl=self.makeCDecls(cmdinfo.elem)[0]))
        else:
            # The Core Vulkan code will be wrapped in a feature called VK_VERSION_#_#
            # For example: VK_VERSION_1_0 wraps the core 1.0 Vulkan functionality
            if 'VK_VERSION_' in extension_name:
                self.core_commands.append(
                    self.CommandData(name=name, ext_name=extension_name,
                                     ext_type='instance',
                                     protect=self.featureExtraProtect,
                                     return_type = return_type,
                                     handle_type = handle_type,
                                     params = cmd_params,
                                     cdecl=self.makeCDecls(cmdinfo.elem)[0]))

            else:
                self.ext_instance_dispatch_list.append((name, self.featureExtraProtect))
                self.ext_commands.append(
                    self.CommandData(name=name, ext_name=extension_name,
                                     ext_type=extension_type,
                                     protect=self.featureExtraProtect,
                                     return_type = return_type,
                                     handle_type = handle_type,
                                     params = cmd_params,
                                     cdecl=self.makeCDecls(cmdinfo.elem)[0]))

    #
    # Retrieve the type and name for a parameter
    def getTypeNameTuple(self, param):
        type = ''
        name = ''
        for elem in param:
            if elem.tag == 'type':
                type = noneStr(elem.text)
            elif elem.tag == 'name':
                name = noneStr(elem.text)
        return (type, name)

    def OutputPrototypesInHeader(self):
        protos = ''
        protos += '// Structures defined externally, but used here\n'
        protos += 'struct loader_instance;\n'
        protos += 'struct loader_icd_term;\n'
        protos += 'struct loader_dev_dispatch_table;\n'
        protos += '\n'
        protos += '// Device extension error function\n'
        protos += 'VKAPI_ATTR VkResult VKAPI_CALL vkDevExtError(VkDevice dev);\n'
        protos += '\n'
        protos += '// Extension interception for vkGetInstanceProcAddr function, so we can return\n'
        protos += '// the appropriate information for any instance extensions we know about.\n'
        protos += 'bool extension_instance_gpa(struct loader_instance *ptr_instance, const char *name, void **addr);\n'
        protos += '\n'
        protos += '// Extension interception for vkCreateInstance function, so we can properly\n'
        protos += '// detect and enable any instance extension information for extensions we know\n'
        protos += '// about.\n'
        protos += 'void extensions_create_instance(struct loader_instance *ptr_instance, const VkInstanceCreateInfo *pCreateInfo);\n'
        protos += '\n'
        protos += '// Extension interception for vkGetDeviceProcAddr function, so we can return\n'
        protos += '// an appropriate terminator if this is one of those few device commands requiring\n'
        protos += '// a terminator.\n'
        protos += 'PFN_vkVoidFunction get_extension_device_proc_terminator(const char *pName);\n'
        protos += '\n'
        protos += '// Dispatch table properly filled in with appropriate terminators for the\n'
        protos += '// supported extensions.\n'
        protos += 'extern const VkLayerInstanceDispatchTable instance_disp;\n'
        protos += '\n'
        protos += '// Array of extension strings for instance extensions we support.\n'
        protos += 'extern const char *const LOADER_INSTANCE_EXTENSIONS[];\n'
        protos += '\n'
        protos += 'VKAPI_ATTR bool VKAPI_CALL loader_icd_init_entries(struct loader_icd_term *icd_term, VkInstance inst,\n'
        protos += '                                                   const PFN_vkGetInstanceProcAddr fp_gipa);\n'
        protos += '\n'
        protos += '// Init Device function pointer dispatch table with core commands\n'
        protos += 'VKAPI_ATTR void VKAPI_CALL loader_init_device_dispatch_table(struct loader_dev_dispatch_table *dev_table, PFN_vkGetDeviceProcAddr gpa,\n'
        protos += '                                                             VkDevice dev);\n'
        protos += '\n'
        protos += '// Init Device function pointer dispatch table with extension commands\n'
        protos += 'VKAPI_ATTR void VKAPI_CALL loader_init_device_extension_dispatch_table(struct loader_dev_dispatch_table *dev_table,\n'
        protos += '                                                                       PFN_vkGetDeviceProcAddr gpa, VkDevice dev);\n'
        protos += '\n'
        protos += '// Init Instance function pointer dispatch table with core commands\n'
        protos += 'VKAPI_ATTR void VKAPI_CALL loader_init_instance_core_dispatch_table(VkLayerInstanceDispatchTable *table, PFN_vkGetInstanceProcAddr gpa,\n'
        protos += '                                                                    VkInstance inst);\n'
        protos += '\n'
        protos += '// Init Instance function pointer dispatch table with core commands\n'
        protos += 'VKAPI_ATTR void VKAPI_CALL loader_init_instance_extension_dispatch_table(VkLayerInstanceDispatchTable *table, PFN_vkGetInstanceProcAddr gpa,\n'
        protos += '                                                                         VkInstance inst);\n'
        protos += '\n'
        protos += '// Device command lookup function\n'
        protos += 'VKAPI_ATTR void* VKAPI_CALL loader_lookup_device_dispatch_table(const VkLayerDispatchTable *table, const char *name);\n'
        protos += '\n'
        protos += '// Instance command lookup function\n'
        protos += 'VKAPI_ATTR void* VKAPI_CALL loader_lookup_instance_dispatch_table(const VkLayerInstanceDispatchTable *table, const char *name,\n'
        protos += '                                                                  bool *found_name);\n'
        protos += '\n'
        protos += 'VKAPI_ATTR bool VKAPI_CALL loader_icd_init_entries(struct loader_icd_term *icd_term, VkInstance inst,\n'
        protos += '                                                   const PFN_vkGetInstanceProcAddr fp_gipa);\n'
        protos += '\n'
        return protos

    def OutputUtilitiesInSource(self):
        protos = ''
        protos += '// Device extension error function\n'
        protos += 'VKAPI_ATTR VkResult VKAPI_CALL vkDevExtError(VkDevice dev) {\n'
        protos += '    struct loader_device *found_dev;\n'
        protos += '    // The device going in is a trampoline device\n'
        protos += '    struct loader_icd_term *icd_term = loader_get_icd_and_device(dev, &found_dev, NULL);\n'
        protos += '\n'
        protos += '    if (icd_term)\n'
        protos += '        loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,\n'
        protos += '                   "Bad destination in loader trampoline dispatch,"\n'
        protos += '                   "Are layers and extensions that you are calling enabled?");\n'
        protos += '    return VK_ERROR_EXTENSION_NOT_PRESENT;\n'
        protos += '}\n\n'
        return protos

    #
    # Create a layer instance dispatch table from the appropriate list and return it as a string
    def OutputLayerInstanceDispatchTable(self):
        commands = []
        table = ''
        cur_extension_name = ''

        table += '// Instance function pointer dispatch table\n'
        table += 'typedef struct VkLayerInstanceDispatchTable_ {\n'

        # First add in an entry for GetPhysicalDeviceProcAddr.  This will not
        # ever show up in the XML or header, so we have to manually add it.
        table += '    // Manually add in GetPhysicalDeviceProcAddr entry\n'
        table += '    PFN_GetPhysicalDeviceProcAddr GetPhysicalDeviceProcAddr;\n'

        for x in range(0, 2):
            if x == 0:
                commands = self.core_commands
            else:
                commands = self.ext_commands

            for cur_cmd in commands:
                is_inst_handle_type = cur_cmd.ext_type == 'instance' or cur_cmd.handle_type == 'VkInstance' or cur_cmd.handle_type == 'VkPhysicalDevice'
                if is_inst_handle_type:

                    if cur_cmd.ext_name != cur_extension_name:
                        if 'VK_VERSION_' in cur_cmd.ext_name:
                            table += '\n    // ---- Core %s commands\n' % cur_cmd.ext_name[11:]
                        else:
                            table += '\n    // ---- %s extension commands\n' % cur_cmd.ext_name
                        cur_extension_name = cur_cmd.ext_name

                    # Remove 'vk' from proto name
                    base_name = cur_cmd.name[2:]

                    if cur_cmd.protect is not None:
                        table += '#ifdef %s\n' % cur_cmd.protect

                    table += '    PFN_%s %s;\n' % (cur_cmd.name, base_name)

                    if cur_cmd.protect is not None:
                        table += '#endif // %s\n' % cur_cmd.protect

        table += '} VkLayerInstanceDispatchTable;\n\n'
        return table

    #
    # Create a layer device dispatch table from the appropriate list and return it as a string
    def OutputLayerDeviceDispatchTable(self):
        commands = []
        table = ''
        cur_extension_name = ''

        table += '// Device function pointer dispatch table\n'
        table += 'typedef struct VkLayerDispatchTable_ {\n'

        for x in range(0, 2):
            if x == 0:
                commands = self.core_commands
            else:
                commands = self.ext_commands

            for cur_cmd in commands:
                is_inst_handle_type = cur_cmd.ext_type == 'instance' or cur_cmd.handle_type == 'VkInstance' or cur_cmd.handle_type == 'VkPhysicalDevice'
                if not is_inst_handle_type:

                    if cur_cmd.ext_name != cur_extension_name:
                        if 'VK_VERSION_' in cur_cmd.ext_name:
                            table += '\n    // ---- Core %s commands\n' % cur_cmd.ext_name[11:]
                        else:
                            table += '\n    // ---- %s extension commands\n' % cur_cmd.ext_name
                        cur_extension_name = cur_cmd.ext_name

                    # Remove 'vk' from proto name
                    base_name = cur_cmd.name[2:]

                    if cur_cmd.protect is not None:
                        table += '#ifdef %s\n' % cur_cmd.protect

                    table += '    PFN_%s %s;\n' % (cur_cmd.name, base_name)

                    if cur_cmd.protect is not None:
                        table += '#endif // %s\n' % cur_cmd.protect

        table += '} VkLayerDispatchTable;\n\n'
        return table

    #
    # Create a dispatch table from the appropriate list and return it as a string
    def OutputIcdDispatchTable(self):
        commands = []
        table = ''
        cur_extension_name = ''

        table += '// ICD function pointer dispatch table\n'
        table += 'struct loader_icd_term_dispatch {\n'

        for x in range(0, 2):
            if x == 0:
                commands = self.core_commands
            else:
                commands = self.ext_commands

            for cur_cmd in commands:
                is_inst_handle_type = cur_cmd.ext_type == 'instance' or cur_cmd.handle_type == 'VkInstance' or cur_cmd.handle_type == 'VkPhysicalDevice'
                if ((is_inst_handle_type or cur_cmd.name in DEVICE_CMDS_NEED_TERM) and
                    (cur_cmd.name != 'vkGetInstanceProcAddr' and cur_cmd.name != 'vkEnumerateDeviceLayerProperties')):

                    if cur_cmd.ext_name != cur_extension_name:
                        if 'VK_VERSION_' in cur_cmd.ext_name:
                            table += '\n    // ---- Core %s commands\n' % cur_cmd.ext_name[11:]
                        else:
                            table += '\n    // ---- %s extension commands\n' % cur_cmd.ext_name
                        cur_extension_name = cur_cmd.ext_name

                    # Remove 'vk' from proto name
                    base_name = cur_cmd.name[2:]

                    if cur_cmd.protect is not None:
                        table += '#ifdef %s\n' % cur_cmd.protect

                    table += '    PFN_%s %s;\n' % (cur_cmd.name, base_name)

                    if cur_cmd.protect is not None:
                        table += '#endif // %s\n' % cur_cmd.protect

        table += '};\n\n'
        return table

    #
    # Init a dispatch table from the appropriate list and return it as a string
    def OutputIcdDispatchTableInit(self):
        commands = []
        cur_extension_name = ''

        table = ''
        table += 'VKAPI_ATTR bool VKAPI_CALL loader_icd_init_entries(struct loader_icd_term *icd_term, VkInstance inst,\n'
        table += '                                                   const PFN_vkGetInstanceProcAddr fp_gipa) {\n'
        table += '\n'
        table += '#define LOOKUP_GIPA(func, required)                                                        \\\n'
        table += '    do {                                                                                   \\\n'
        table += '        icd_term->dispatch.func = (PFN_vk##func)fp_gipa(inst, "vk" #func);                 \\\n'
        table += '        if (!icd_term->dispatch.func && required) {                                        \\\n'
        table += '            loader_log((struct loader_instance *)inst, VK_DEBUG_REPORT_WARNING_BIT_EXT, 0, \\\n'
        table += '                       loader_platform_get_proc_address_error("vk" #func));                \\\n'
        table += '            return false;                                                                  \\\n'
        table += '        }                                                                                  \\\n'
        table += '    } while (0)\n'
        table += '\n'

        skip_gipa_commands = ['vkGetInstanceProcAddr',
                              'vkEnumerateDeviceLayerProperties',
                              'vkCreateInstance',
                              'vkEnumerateInstanceExtensionProperties',
                              'vkEnumerateInstanceLayerProperties',
                             ]

        for x in range(0, 2):
            if x == 0:
                commands = self.core_commands
            else:
                commands = self.ext_commands

            for cur_cmd in commands:
                is_inst_handle_type = cur_cmd.ext_type == 'instance' or cur_cmd.handle_type == 'VkInstance' or cur_cmd.handle_type == 'VkPhysicalDevice'
                if ((is_inst_handle_type or cur_cmd.name in DEVICE_CMDS_NEED_TERM) and (cur_cmd.name not in skip_gipa_commands)):

                    if cur_cmd.ext_name != cur_extension_name:
                        if 'VK_VERSION_' in cur_cmd.ext_name:
                            table += '\n    // ---- Core %s\n' % cur_cmd.ext_name[11:]
                        else:
                            table += '\n    // ---- %s extension commands\n' % cur_cmd.ext_name
                        cur_extension_name = cur_cmd.ext_name

                    # Remove 'vk' from proto name
                    base_name = cur_cmd.name[2:]

                    if cur_cmd.protect is not None:
                        table += '#ifdef %s\n' % cur_cmd.protect

                    # The Core Vulkan code will be wrapped in a feature called VK_VERSION_#_#
                    # For example: VK_VERSION_1_0 wraps the core 1.0 Vulkan functionality
                    if x == 0:
                        table += '    LOOKUP_GIPA(%s, true);\n' % (base_name)
                    else:
                        table += '    LOOKUP_GIPA(%s, false);\n' % (base_name)

                    if cur_cmd.protect is not None:
                        table += '#endif // %s\n' % cur_cmd.protect

        table += '\n'
        table += '#undef LOOKUP_GIPA\n'
        table += '\n'
        table += '    return true;\n'
        table += '};\n\n'
        return table

    #
    # Create the extension enable union
    def OutputIcdExtensionEnableUnion(self):
        extensions = self.instanceExtensions

        union = ''
        union += 'union loader_instance_extension_enables {\n'
        union += '    struct {\n'
        for ext in extensions:
            if ('VK_VERSION_' in ext.name or ext.name in WSI_EXT_NAMES or
                ext.type == 'device' or ext.num_commands == 0):
                continue

            union += '        uint8_t %s : 1;\n' % ext.name[3:].lower()

        union += '    };\n'
        union += '    uint64_t padding[4];\n'
        union += '};\n\n'
        return union

    #
    # Creates the prototypes for the loader's core instance command terminators
    def OutputLoaderTerminators(self):
        terminators = ''
        terminators += '// Loader core instance terminators\n'

        for cur_cmd in self.core_commands:
            is_inst_handle_type = cur_cmd.ext_type == 'instance' or cur_cmd.handle_type == 'VkInstance' or cur_cmd.handle_type == 'VkPhysicalDevice'
            if is_inst_handle_type:
                mod_string = ''
                new_terminator = cur_cmd.cdecl
                mod_string = new_terminator.replace("VKAPI_CALL vk", "VKAPI_CALL terminator_")

                if (cur_cmd.protect != None):
                    terminators += '#ifdef %s\n' % cur_cmd.protect

                terminators += mod_string
                terminators += '\n'

                if (cur_cmd.protect != None):
                    terminators += '#endif // %s\n' % cur_cmd.protect

        terminators += '\n'
        return terminators

    #
    # Creates code to initialize the various dispatch tables
    def OutputLoaderDispatchTables(self):
        commands = []
        tables = ''
        gpa_param = ''
        cur_type = ''
        cur_extension_name = ''

        for x in range(0, 4):
            if x == 0:
                cur_type = 'device'
                gpa_param = 'dev'
                commands = self.core_commands

                tables += '// Init Device function pointer dispatch table with core commands\n'
                tables += 'VKAPI_ATTR void VKAPI_CALL loader_init_device_dispatch_table(struct loader_dev_dispatch_table *dev_table, PFN_vkGetDeviceProcAddr gpa,\n'
                tables += '                                                             VkDevice dev) {\n'
                tables += '    VkLayerDispatchTable *table = &dev_table->core_dispatch;\n'
                tables += '    for (uint32_t i = 0; i < MAX_NUM_UNKNOWN_EXTS; i++) dev_table->ext_dispatch.dev_ext[i] = (PFN_vkDevExt)vkDevExtError;\n'

            elif x == 1:
                cur_type = 'device'
                gpa_param = 'dev'
                commands = self.ext_commands

                tables += '// Init Device function pointer dispatch table with extension commands\n'
                tables += 'VKAPI_ATTR void VKAPI_CALL loader_init_device_extension_dispatch_table(struct loader_dev_dispatch_table *dev_table,\n'
                tables += '                                                                       PFN_vkGetDeviceProcAddr gpa, VkDevice dev) {\n'
                tables += '    VkLayerDispatchTable *table = &dev_table->core_dispatch;\n'

            elif x == 2:
                cur_type = 'instance'
                gpa_param = 'inst'
                commands = self.core_commands

                tables += '// Init Instance function pointer dispatch table with core commands\n'
                tables += 'VKAPI_ATTR void VKAPI_CALL loader_init_instance_core_dispatch_table(VkLayerInstanceDispatchTable *table, PFN_vkGetInstanceProcAddr gpa,\n'
                tables += '                                                                    VkInstance inst) {\n'

            else:
                cur_type = 'instance'
                gpa_param = 'inst'
                commands = self.ext_commands

                tables += '// Init Instance function pointer dispatch table with core commands\n'
                tables += 'VKAPI_ATTR void VKAPI_CALL loader_init_instance_extension_dispatch_table(VkLayerInstanceDispatchTable *table, PFN_vkGetInstanceProcAddr gpa,\n'
                tables += '                                                                        VkInstance inst) {\n'

            for cur_cmd in commands:
                is_inst_handle_type = cur_cmd.ext_type == 'instance' or cur_cmd.handle_type == 'VkInstance' or cur_cmd.handle_type == 'VkPhysicalDevice'
                if ((cur_type == 'instance' and is_inst_handle_type) or (cur_type == 'device' and not is_inst_handle_type)):
                    if cur_cmd.ext_name != cur_extension_name:
                        if 'VK_VERSION_' in cur_cmd.ext_name:
                            tables += '\n    // ---- Core %s commands\n' % cur_cmd.ext_name[11:]
                        else:
                            tables += '\n    // ---- %s extension commands\n' % cur_cmd.ext_name
                        cur_extension_name = cur_cmd.ext_name

                    # Remove 'vk' from proto name
                    base_name = cur_cmd.name[2:]

                    # Names to skip
                    if (base_name == 'CreateInstance' or base_name == 'CreateDevice' or
                        base_name == 'EnumerateInstanceExtensionProperties' or
                        base_name == 'EnumerateInstanceLayerProperties'):
                        continue

                    if cur_cmd.protect is not None:
                        tables += '#ifdef %s\n' % cur_cmd.protect

                    # If we're looking for the proc we are passing in, just point the table to it.  This fixes the issue where
                    # a layer overrides the function name for the loader.
                    if (x <= 1 and base_name == 'GetDeviceProcAddr'):
                        tables += '    table->GetDeviceProcAddr = gpa;\n'
                    elif (x > 1 and base_name == 'GetInstanceProcAddr'):
                        tables += '    table->GetInstanceProcAddr = gpa;\n'
                    else:
                        tables += '    table->%s = (PFN_%s)gpa(%s, "%s");\n' % (base_name, cur_cmd.name, gpa_param, cur_cmd.name)

                    if cur_cmd.protect is not None:
                        tables += '#endif // %s\n' % cur_cmd.protect

            tables += '}\n\n'
        return tables

    #
    # Create a lookup table function from the appropriate list of entrypoints and
    # return it as a string
    def OutputLoaderLookupFunc(self):
        commands = []
        tables = ''
        cur_type = ''
        cur_extension_name = ''

        for x in range(0, 2):
            if x == 0:
                cur_type = 'device'

                tables += '// Device command lookup function\n'
                tables += 'VKAPI_ATTR void* VKAPI_CALL loader_lookup_device_dispatch_table(const VkLayerDispatchTable *table, const char *name) {\n'
                tables += '    if (!name || name[0] != \'v\' || name[1] != \'k\') return NULL;\n'
                tables += '\n'
                tables += '    name += 2;\n'
            else:
                cur_type = 'instance'

                tables += '// Instance command lookup function\n'
                tables += 'VKAPI_ATTR void* VKAPI_CALL loader_lookup_instance_dispatch_table(const VkLayerInstanceDispatchTable *table, const char *name,\n'
                tables += '                                                                 bool *found_name) {\n'
                tables += '    if (!name || name[0] != \'v\' || name[1] != \'k\') {\n'
                tables += '        *found_name = false;\n'
                tables += '        return NULL;\n'
                tables += '    }\n'
                tables += '\n'
                tables += '    *found_name = true;\n'
                tables += '    name += 2;\n'

            for y in range(0, 2):
                if y == 0:
                    commands = self.core_commands
                else:
                    commands = self.ext_commands

                for cur_cmd in commands:
                    is_inst_handle_type = cur_cmd.ext_type == 'instance' or cur_cmd.handle_type == 'VkInstance' or cur_cmd.handle_type == 'VkPhysicalDevice'
                    if ((cur_type == 'instance' and is_inst_handle_type) or (cur_type == 'device' and not is_inst_handle_type)):

                        if cur_cmd.ext_name != cur_extension_name:
                            if 'VK_VERSION_' in cur_cmd.ext_name:
                                tables += '\n    // ---- Core %s commands\n' % cur_cmd.ext_name[11:]
                            else:
                                tables += '\n    // ---- %s extension commands\n' % cur_cmd.ext_name
                            cur_extension_name = cur_cmd.ext_name

                        # Remove 'vk' from proto name
                        base_name = cur_cmd.name[2:]

                        if (base_name == 'CreateInstance' or base_name == 'CreateDevice' or
                            base_name == 'EnumerateInstanceExtensionProperties' or
                            base_name == 'EnumerateInstanceLayerProperties'):
                            continue

                        if cur_cmd.protect is not None:
                            tables += '#ifdef %s\n' % cur_cmd.protect

                        tables += '    if (!strcmp(name, "%s")) return (void *)table->%s;\n' % (base_name, base_name)

                        if cur_cmd.protect is not None:
                            tables += '#endif // %s\n' % cur_cmd.protect

            tables += '\n'
            if x == 1:
                tables += '    *found_name = false;\n'
            tables += '    return NULL;\n'
            tables += '}\n\n'
        return tables

    #
    # Create the appropriate trampoline (and possibly terminator) functinos
    def CreateTrampTermFuncs(self):
        entries = []
        funcs = ''
        cur_extension_name = ''

        # Some extensions have to be manually added.  Skip those in the automatic
        # generation.  They will be manually added later.
        manual_ext_commands = ['vkEnumeratePhysicalDeviceGroupsKHX',
                               'vkGetPhysicalDeviceExternalImageFormatPropertiesNV',
                               'vkGetPhysicalDeviceFeatures2KHR',
                               'vkGetPhysicalDeviceProperties2KHR',
                               'vkGetPhysicalDeviceFormatProperties2KHR',
                               'vkGetPhysicalDeviceImageFormatProperties2KHR',
                               'vkGetPhysicalDeviceQueueFamilyProperties2KHR',
                               'vkGetPhysicalDeviceMemoryProperties2KHR',
                               'vkGetPhysicalDeviceSparseImageFormatProperties2KHR']

        for ext_cmd in self.ext_commands:
            if (ext_cmd.ext_name in WSI_EXT_NAMES or
                ext_cmd.ext_name in AVOID_EXT_NAMES or
                ext_cmd.name in manual_ext_commands):
                continue

            if ext_cmd.ext_name != cur_extension_name:
                if 'VK_VERSION_' in ext_cmd.ext_name:
                    funcs += '\n// ---- Core %s trampoline/terminators\n\n' % ext_cmd.ext_name[11:]
                else:
                    funcs += '\n// ---- %s extension trampoline/terminators\n\n' % ext_cmd.ext_name
                cur_extension_name = ext_cmd.ext_name

            if ext_cmd.protect is not None:
                funcs += '#ifdef %s\n' % ext_cmd.protect

            func_header = ext_cmd.cdecl.replace(";", " {\n")
            tramp_header = func_header.replace("VKAPI_CALL vk", "VKAPI_CALL ")
            return_prefix = '    '
            base_name = ext_cmd.name[2:]
            has_surface = 0
            update_structure_surface = 0
            update_structure_string = ''
            requires_terminator = 0
            surface_var_name = ''
            phys_dev_var_name = ''
            has_return_type = False
            always_use_param_name = True
            surface_type_to_replace = ''
            surface_name_replacement = ''
            physdev_type_to_replace = ''
            physdev_name_replacement = ''

            for param in ext_cmd.params:
                if param.type == 'VkSurfaceKHR':
                    has_surface = 1
                    surface_var_name = param.name
                    requires_terminator = 1
                    always_use_param_name = False
                    surface_type_to_replace = 'VkSurfaceKHR'
                    surface_name_replacement = 'icd_surface->real_icd_surfaces[icd_index]'
                if param.type == 'VkPhysicalDeviceSurfaceInfo2KHR':
                    has_surface = 1
                    surface_var_name = param.name + '->surface'
                    requires_terminator = 1
                    update_structure_surface = 1
                    update_structure_string = '        VkPhysicalDeviceSurfaceInfo2KHR info_copy = *pSurfaceInfo;\n'
                    update_structure_string += '        info_copy.surface = icd_surface->real_icd_surfaces[icd_index];\n'
                    always_use_param_name = False
                    surface_type_to_replace = 'VkPhysicalDeviceSurfaceInfo2KHR'
                    surface_name_replacement = '&info_copy'
                if param.type == 'VkPhysicalDevice':
                    requires_terminator = 1
                    phys_dev_var_name = param.name
                    always_use_param_name = False
                    physdev_type_to_replace = 'VkPhysicalDevice'
                    physdev_name_replacement = 'phys_dev_term->phys_dev'

            if (ext_cmd.return_type != None):
                return_prefix += 'return '
                has_return_type = True

            if (ext_cmd.ext_type == 'instance' or ext_cmd.handle_type == 'VkPhysicalDevice' or
                'DebugMarkerSetObject' in ext_cmd.name or ext_cmd.name in DEVICE_CMDS_NEED_TERM):
                requires_terminator = 1

            if requires_terminator == 1:
                term_header = tramp_header.replace("VKAPI_CALL ", "VKAPI_CALL terminator_")

                funcs += tramp_header

                if ext_cmd.handle_type == 'VkPhysicalDevice':
                    funcs += '    const VkLayerInstanceDispatchTable *disp;\n'
                    funcs += '    VkPhysicalDevice unwrapped_phys_dev = loader_unwrap_physical_device(%s);\n' % (phys_dev_var_name)
                    funcs += '    disp = loader_get_instance_layer_dispatch(%s);\n' % (phys_dev_var_name)
                elif ext_cmd.handle_type == 'VkInstance':
                    funcs += '#error("Not implemented. Likely needs to be manually generated!");\n'
                else:
                    funcs += '    const VkLayerDispatchTable *disp = loader_get_dispatch('
                    funcs += ext_cmd.params[0].name
                    funcs += ');\n'

                if 'DebugMarkerSetObject' in ext_cmd.name:
                    funcs += '    // If this is a physical device, we have to replace it with the proper one for the next call.\n'
                    funcs += '    if (%s->objectType == VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT) {\n' % (ext_cmd.params[1].name)
                    funcs += '        struct loader_physical_device_tramp *phys_dev_tramp = (struct loader_physical_device_tramp *)(uintptr_t)%s->object;\n' % (ext_cmd.params[1].name)
                    funcs += '        %s->object = (uint64_t)(uintptr_t)phys_dev_tramp->phys_dev;\n' % (ext_cmd.params[1].name)
                    funcs += '    }\n'

                funcs += return_prefix
                funcs += 'disp->'
                funcs += base_name
                funcs += '('
                count = 0
                for param in ext_cmd.params:
                    if count != 0:
                        funcs += ', '

                    if param.type == 'VkPhysicalDevice':
                        funcs += 'unwrapped_phys_dev'
                    else:
                        funcs += param.name

                    count += 1
                funcs += ');\n'
                funcs += '}\n\n'

                funcs += term_header
                if ext_cmd.handle_type == 'VkPhysicalDevice':
                    funcs += '    struct loader_physical_device_term *phys_dev_term = (struct loader_physical_device_term *)%s;\n' % (phys_dev_var_name)
                    funcs += '    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;\n'
                    funcs += '    if (NULL == icd_term->dispatch.'
                    funcs += base_name
                    funcs += ') {\n'
                    funcs += '        loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,\n'
                    funcs += '                   "ICD associated with VkPhysicalDevice does not support '
                    funcs += base_name
                    funcs += '");\n'
                    funcs += '    }\n'

                    if has_surface == 1:
                        funcs += '    VkIcdSurface *icd_surface = (VkIcdSurface *)(%s);\n' % (surface_var_name)
                        funcs += '    uint8_t icd_index = phys_dev_term->icd_index;\n'
                        funcs += '    if (NULL != icd_surface->real_icd_surfaces && NULL != (void *)icd_surface->real_icd_surfaces[icd_index]) {\n'

                        # If there's a structure with a surface, we need to update its internals with the correct surface for the ICD
                        if update_structure_surface == 1:
                            funcs += update_structure_string

                        funcs += '    ' + return_prefix + 'icd_term->dispatch.'
                        funcs += base_name
                        funcs += '('
                        count = 0
                        for param in ext_cmd.params:
                            if count != 0:
                                funcs += ', '

                            if not always_use_param_name:
                                if surface_type_to_replace and surface_type_to_replace == param.type:
                                    funcs += surface_name_replacement
                                elif physdev_type_to_replace and physdev_type_to_replace == param.type:
                                    funcs += physdev_name_replacement
                                else:
                                    funcs += param.name
                            else:
                                funcs += param.name

                            count += 1
                        funcs += ');\n'
                        if not has_return_type:
                            funcs += '        return;\n'
                        funcs += '    }\n'

                    funcs += return_prefix
                    funcs += 'icd_term->dispatch.'
                    funcs += base_name
                    funcs += '('
                    count = 0
                    for param in ext_cmd.params:
                        if count != 0:
                            funcs += ', '

                        if param.type == 'VkPhysicalDevice':
                            funcs += 'phys_dev_term->phys_dev'
                        else:
                            funcs += param.name

                        count += 1
                    funcs += ');\n'

                elif has_surface == 1 and ext_cmd.ext_type == 'device':
                    funcs += '    uint32_t icd_index = 0;\n'
                    funcs += '    struct loader_device *dev;\n'
                    funcs += '    struct loader_icd_term *icd_term = loader_get_icd_and_device(device, &dev, &icd_index);\n'
                    funcs += '    if (NULL != icd_term && NULL != icd_term->dispatch.%s) {\n' % base_name
                    funcs += '        VkIcdSurface *icd_surface = (VkIcdSurface *)(uintptr_t)%s;\n' % (surface_var_name)
                    funcs += '        if (NULL != icd_surface->real_icd_surfaces && (VkSurfaceKHR)NULL != icd_surface->real_icd_surfaces[icd_index]) {\n'
                    funcs += '        %sicd_term->dispatch.%s(' % (return_prefix, base_name)
                    count = 0
                    for param in ext_cmd.params:
                        if count != 0:
                            funcs += ', '

                        if param.type == 'VkSurfaceKHR':
                            funcs += 'icd_surface->real_icd_surfaces[icd_index]'
                        else:
                            funcs += param.name

                        count += 1
                    funcs += ');\n'
                    if not has_return_type:
                        funcs += '                return;\n'
                    funcs += '        }\n'
                    funcs += '    %sicd_term->dispatch.%s(' % (return_prefix, base_name)
                    count = 0
                    for param in ext_cmd.params:
                        if count != 0:
                            funcs += ', '
                        funcs += param.name
                        count += 1
                    funcs += ');\n'
                    funcs += '    }\n'
                    if has_return_type:
                        funcs += '    return VK_SUCCESS;\n'

                elif ext_cmd.handle_type == 'VkInstance':
                    funcs += '#error("Not implemented. Likely needs to be manually generated!");\n'

                elif 'DebugMarkerSetObject' in ext_cmd.name:
                    funcs += '    uint32_t icd_index = 0;\n'
                    funcs += '    struct loader_device *dev;\n'
                    funcs += '    struct loader_icd_term *icd_term = loader_get_icd_and_device(%s, &dev, &icd_index);\n' % (ext_cmd.params[0].name)
                    funcs += '    if (NULL != icd_term && NULL != icd_term->dispatch.'
                    funcs += base_name
                    funcs += ') {\n'
                    funcs += '        // If this is a physical device, we have to replace it with the proper one for the next call.\n'
                    funcs += '        if (%s->objectType == VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT) {\n' % (ext_cmd.params[1].name)
                    funcs += '            struct loader_physical_device_term *phys_dev_term = (struct loader_physical_device_term *)(uintptr_t)%s->object;\n' % (ext_cmd.params[1].name)
                    funcs += '            %s->object = (uint64_t)(uintptr_t)phys_dev_term->phys_dev;\n' % (ext_cmd.params[1].name)
                    funcs += '        // If this is a KHR_surface, and the ICD has created its own, we have to replace it with the proper one for the next call.\n'
                    funcs += '        } else if (%s->objectType == VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT) {\n' % (ext_cmd.params[1].name)
                    funcs += '            if (NULL != icd_term && NULL != icd_term->dispatch.CreateSwapchainKHR) {\n'
                    funcs += '                VkIcdSurface *icd_surface = (VkIcdSurface *)(uintptr_t)%s->object;\n' % (ext_cmd.params[1].name)
                    funcs += '                if (NULL != icd_surface->real_icd_surfaces) {\n'
                    funcs += '                    %s->object = (uint64_t)icd_surface->real_icd_surfaces[icd_index];\n' % (ext_cmd.params[1].name)
                    funcs += '                }\n'
                    funcs += '            }\n'
                    funcs += '        }\n'
                    funcs += '        return icd_term->dispatch.'
                    funcs += base_name
                    funcs += '('
                    count = 0
                    for param in ext_cmd.params:
                        if count != 0:
                            funcs += ', '

                        if param.type == 'VkPhysicalDevice':
                            funcs += 'phys_dev_term->phys_dev'
                        elif param.type == 'VkSurfaceKHR':
                            funcs += 'icd_surface->real_icd_surfaces[icd_index]'
                        else:
                            funcs += param.name
                        count += 1

                    funcs += ');\n'
                    funcs += '    } else {\n'
                    funcs += '        return VK_SUCCESS;\n'
                    funcs += '    }\n'

                else:
                    funcs += '#error("Unknown error path!");\n'

                funcs += '}\n\n'
            else:
                funcs += tramp_header

                funcs += '    const VkLayerDispatchTable *disp = loader_get_dispatch('
                funcs += ext_cmd.params[0].name
                funcs += ');\n'

                funcs += return_prefix
                funcs += 'disp->'
                funcs += base_name
                funcs += '('
                count = 0
                for param in ext_cmd.params:
                    if count != 0:
                        funcs += ', '
                    funcs += param.name
                    count += 1
                funcs += ');\n'
                funcs += '}\n\n'

            if ext_cmd.protect is not None:
                funcs += '#endif // %s\n' % ext_cmd.protect

        return funcs


    #
    # Create a function for the extension GPA call
    def InstExtensionGPA(self):
        entries = []
        gpa_func = ''
        cur_extension_name = ''

        gpa_func += '// GPA helpers for extensions\n'
        gpa_func += 'bool extension_instance_gpa(struct loader_instance *ptr_instance, const char *name, void **addr) {\n'
        gpa_func += '    *addr = NULL;\n\n'

        for cur_cmd in self.ext_commands:
            if ('VK_VERSION_' in cur_cmd.ext_name or
                cur_cmd.ext_name in WSI_EXT_NAMES or
                cur_cmd.ext_name in AVOID_EXT_NAMES):
                continue

            if cur_cmd.ext_name != cur_extension_name:
                gpa_func += '\n    // ---- %s extension commands\n' % cur_cmd.ext_name
                cur_extension_name = cur_cmd.ext_name

            if cur_cmd.protect is not None:
                gpa_func += '#ifdef %s\n' % cur_cmd.protect

            base_name = cur_cmd.name[2:]

            if (cur_cmd.ext_type == 'instance'):
                gpa_func += '    if (!strcmp("%s", name)) {\n' % (cur_cmd.name)
                gpa_func += '        *addr = (ptr_instance->enabled_known_extensions.'
                gpa_func += cur_cmd.ext_name[3:].lower()
                gpa_func += ' == 1)\n'
                gpa_func += '                     ? (void *)%s\n' % (base_name)
                gpa_func += '                     : NULL;\n'
                gpa_func += '        return true;\n'
                gpa_func += '    }\n'
            else:
                gpa_func += '    if (!strcmp("%s", name)) {\n' % (cur_cmd.name)
                gpa_func += '        *addr = (void *)%s;\n' % (base_name)
                gpa_func += '        return true;\n'
                gpa_func += '    }\n'

            if cur_cmd.protect is not None:
                gpa_func += '#endif // %s\n' % cur_cmd.protect

        gpa_func += '    return false;\n'
        gpa_func += '}\n\n'

        return gpa_func

    #
    # Create the extension name init function
    def InstantExtensionCreate(self):
        entries = []
        entries = self.instanceExtensions
        count = 0
        cur_extension_name = ''

        create_func = ''
        create_func += '// A function that can be used to query enabled extensions during a vkCreateInstance call\n'
        create_func += 'void extensions_create_instance(struct loader_instance *ptr_instance, const VkInstanceCreateInfo *pCreateInfo) {\n'
        create_func += '    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {\n'
        for ext in entries:
            if ('VK_VERSION_' in ext.name or ext.name in WSI_EXT_NAMES or
                ext.name in AVOID_EXT_NAMES or ext.type == 'device' or
                ext.num_commands == 0):
                continue

            if ext.name != cur_extension_name:
                create_func += '\n    // ---- %s extension commands\n' % ext.name
                cur_extension_name = ext.name

            if ext.protect is not None:
                create_func += '#ifdef %s\n' % ext.protect
            if count == 0:
                create_func += '        if (0 == strcmp(pCreateInfo->ppEnabledExtensionNames[i], '
            else:
                create_func += '        } else if (0 == strcmp(pCreateInfo->ppEnabledExtensionNames[i], '

            create_func += ext.define + ')) {\n'
            create_func += '            ptr_instance->enabled_known_extensions.'
            create_func += ext.name[3:].lower()
            create_func += ' = 1;\n'

            if ext.protect is not None:
                create_func += '#endif // %s\n' % ext.protect
            count += 1

        create_func += '        }\n'
        create_func += '    }\n'
        create_func += '}\n\n'
        return create_func

    #
    # Create code to initialize a dispatch table from the appropriate list of
    # extension entrypoints and return it as a string
    def DeviceExtensionGetTerminator(self):
        term_func = ''
        cur_extension_name = ''

        term_func += '// Some device commands still need a terminator because the loader needs to unwrap something about them.\n'
        term_func += '// In many cases, the item needing unwrapping is a VkPhysicalDevice or VkSurfaceKHR object.  But there may be other items\n'
        term_func += '// in the future.\n'
        term_func += 'PFN_vkVoidFunction get_extension_device_proc_terminator(const char *pName) {\n'
        term_func += '    PFN_vkVoidFunction addr = NULL;\n'

        count = 0
        for ext_cmd in self.ext_commands:
            if ext_cmd.name in DEVICE_CMDS_NEED_TERM:
                if ext_cmd.ext_name != cur_extension_name:
                    if 'VK_VERSION_' in ext_cmd.ext_name:
                        term_func += '\n    // ---- Core %s commands\n' % ext_cmd.ext_name[11:]
                    else:
                        term_func += '\n    // ---- %s extension commands\n' % ext_cmd.ext_name
                    cur_extension_name = ext_cmd.ext_name

                if ext_cmd.protect is not None:
                    term_func += '#ifdef %s\n' % ext_cmd.protect

                if count == 0:
                    term_func += '    if'
                else:
                    term_func += '    } else if'
                term_func += '(!strcmp(pName, "%s")) {\n' % (ext_cmd.name)
                term_func += '        addr = (PFN_vkVoidFunction)terminator_%s;\n' % (ext_cmd.name[2:])

                if ext_cmd.protect is not None:
                    term_func += '#endif // %s\n' % ext_cmd.protect

                count += 1

        if count > 0:
            term_func += '    }\n'

        term_func += '    return addr;\n'
        term_func += '}\n\n'

        return term_func

    #
    # Create code to initialize a dispatch table from the appropriate list of
    # core and extension entrypoints and return it as a string
    def InitInstLoaderExtensionDispatchTable(self):
        commands = []
        table = ''
        cur_extension_name = ''

        table += '// This table contains the loader\'s instance dispatch table, which contains\n'
        table += '// default functions if no instance layers are activated.  This contains\n'
        table += '// pointers to "terminator functions".\n'
        table += 'const VkLayerInstanceDispatchTable instance_disp = {\n'

        for x in range(0, 2):
            if x == 0:
                commands = self.core_commands
            else:
                commands = self.ext_commands

            for cur_cmd in commands:
                if cur_cmd.ext_type == 'instance' or (cur_cmd.ext_type == 'device' and cur_cmd.handle_type == 'VkPhysicalDevice'):
                    if cur_cmd.ext_name != cur_extension_name:
                        if 'VK_VERSION_' in cur_cmd.ext_name:
                            table += '\n    // ---- Core %s commands\n' % cur_cmd.ext_name[11:]
                        else:
                            table += '\n    // ---- %s extension commands\n' % cur_cmd.ext_name
                        cur_extension_name = cur_cmd.ext_name

                    # Remove 'vk' from proto name
                    base_name = cur_cmd.name[2:]

                    if (base_name == 'CreateInstance' or base_name == 'CreateDevice' or
                        base_name == 'EnumerateInstanceExtensionProperties' or
                        base_name == 'EnumerateInstanceLayerProperties'):
                        continue

                    if cur_cmd.protect is not None:
                        table += '#ifdef %s\n' % cur_cmd.protect

                    if base_name == 'GetInstanceProcAddr':
                        table += '    .%s = %s,\n' % (base_name, cur_cmd.name)
                    else:
                        table += '    .%s = terminator_%s,\n' % (base_name, base_name)

                    if cur_cmd.protect is not None:
                        table += '#endif // %s\n' % cur_cmd.protect
        table += '};\n\n'

        return table

    #
    # Create the extension name whitelist array
    def OutputInstantExtensionWhitelistArray(self):
        extensions = self.instanceExtensions

        table = ''
        table += '// A null-terminated list of all of the instance extensions supported by the loader.\n'
        table += '// If an instance extension name is not in this list, but it is exported by one or more of the\n'
        table += '// ICDs detected by the loader, then the extension name not in the list will be filtered out\n'
        table += '// before passing the list of extensions to the application.\n'
        table += 'const char *const LOADER_INSTANCE_EXTENSIONS[] = {\n'
        for ext in extensions:
            if ext.type == 'device' or 'VK_VERSION_' in ext.name:
                continue

            if ext.protect is not None:
                table += '#ifdef %s\n' % ext.protect
            table += '                                                  '
            table += ext.define + ',\n'

            if ext.protect is not None:
                table += '#endif // %s\n' % ext.protect
        table += '                                                  NULL };\n'
        return table

