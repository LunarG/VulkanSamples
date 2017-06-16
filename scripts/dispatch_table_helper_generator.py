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
# Author: Mark Lobodzinski <mark@lunarg.com>

import os,re,sys
import xml.etree.ElementTree as etree
from generator import *
from collections import namedtuple

#
# DispatchTableHelperOutputGeneratorOptions - subclass of GeneratorOptions.
class DispatchTableHelperOutputGeneratorOptions(GeneratorOptions):
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
                 alignFuncParam = 0):
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
# DispatchTableHelperOutputGenerator - subclass of OutputGenerator.
# Generates dispatch table helper header files for LVL
class DispatchTableHelperOutputGenerator(OutputGenerator):
    """Generate dispatch table helper header based on XML element attributes"""
    def __init__(self,
                 errFile = sys.stderr,
                 warnFile = sys.stderr,
                 diagFile = sys.stdout):
        OutputGenerator.__init__(self, errFile, warnFile, diagFile)
        # Internal state - accumulators for different inner block text
        self.instance_dispatch_list = []      # List of entries for instance dispatch list
        self.device_dispatch_list = []        # List of entries for device dispatch list
    #
    # Called once at the beginning of each run
    def beginFile(self, genOpts):
        OutputGenerator.beginFile(self, genOpts)
        # Protect against multiple inclusions
        self.protect_header = False
        if (genOpts.protectFile and genOpts.filename):
            self.protect_header = True
            headerSym = '__' + re.sub('\.h', '_h_', os.path.basename(genOpts.filename))
            write('#ifndef', headerSym, file=self.outFile)
            write('#define', headerSym, '1', file=self.outFile)
            self.newline()
        # User-supplied prefix text, if any (list of strings)
        if (genOpts.prefixText):
            for s in genOpts.prefixText:
                write(s, file=self.outFile)
        # File Comment
        file_comment = '// *** THIS FILE IS GENERATED - DO NOT EDIT ***\n'
        file_comment += '// See dispatch_helper_generator.py for modifications\n'
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
        copyright += ' * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>\n'
        copyright += ' * Author: Jon Ashburn <jon@lunarg.com>\n'
        copyright += ' * Author: Mark Lobodzinski <mark@lunarg.com>\n'
        copyright += ' */\n'

        preamble = ''
        preamble += '#include <vulkan/vulkan.h>\n'
        preamble += '#include <vulkan/vk_layer.h>\n'
        preamble += '#include <string.h>\n'

        write(copyright, file=self.outFile)
        write(preamble, file=self.outFile)
    #
    # Write generate and write dispatch tables to output file
    def endFile(self):
        device_table = ''
        instance_table = ''

        device_table += self.OutputDispatchTableHelper('device')
        instance_table += self.OutputDispatchTableHelper('instance')

        write(device_table, file=self.outFile);
        write("\n", file=self.outFile)
        write(instance_table, file=self.outFile);

        if self.protect_header:
            self.newline()
            write('#endif', file=self.outFile)
        # Finish processing in superclass
        OutputGenerator.endFile(self)
    #
    # Process commands, adding to appropriate dispatch tables
    def genCmd(self, cmdinfo, name):
        OutputGenerator.genCmd(self, cmdinfo, name)

        avoid_entries = ['vkCreateInstance',
                         'vkCreateDevice']
        # Get first param type
        params = cmdinfo.elem.findall('param')
        info = self.getTypeNameTuple(params[0])

        if name not in avoid_entries:
            self.AddCommandToDispatchList(name, info[0], self.featureExtraProtect)

    #
    # Determine if this API should be ignored or added to the instance or device dispatch table
    def AddCommandToDispatchList(self, name, handle_type, protect):
        handle = self.registry.tree.find("types/type/[name='" + handle_type + "'][@category='handle']")
        if handle == None:
            return
        if handle_type != 'VkInstance' and handle_type != 'VkPhysicalDevice' and name != 'vkGetInstanceProcAddr':
            self.device_dispatch_list.append((name, self.featureExtraProtect))
        else:
            self.instance_dispatch_list.append((name, self.featureExtraProtect))
        return
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
    #
    # Create a dispatch table from the appropriate list and return it as a string
    def OutputDispatchTableHelper(self, table_type):
        entries = []
        table = ''
        if table_type == 'device':
            entries = self.device_dispatch_list
            table += 'static inline void layer_init_device_dispatch_table(VkDevice device, VkLayerDispatchTable *table, PFN_vkGetDeviceProcAddr gpa) {\n'
            table += '    memset(table, 0, sizeof(*table));\n'
            table += '    // Device function pointers\n'
        else:
            entries = self.instance_dispatch_list
            table += 'static inline void layer_init_instance_dispatch_table(VkInstance instance, VkLayerInstanceDispatchTable *table, PFN_vkGetInstanceProcAddr gpa) {\n'
            table += '    memset(table, 0, sizeof(*table));\n'
            table += '    // Instance function pointers\n'

        for item in entries:
            # Remove 'vk' from proto name
            base_name = item[0][2:]

            if item[1] is not None:
                table += '#ifdef %s\n' % item[1]

            # If we're looking for the proc we are passing in, just point the table to it.  This fixes the issue where
            # a layer overrides the function name for the loader.
            if (table_type == 'device' and base_name == 'GetDeviceProcAddr'):
                table += '    table->GetDeviceProcAddr = gpa;\n'
            elif (table_type != 'device' and base_name == 'GetInstanceProcAddr'):
                table += '    table->GetInstanceProcAddr = gpa;\n'
            else:
                table += '    table->%s = (PFN_%s) gpa(%s, "%s");\n' % (base_name, item[0], table_type, item[0])

            if item[1] is not None:
                table += '#endif // %s\n' % item[1]
        table += '}'
        return table
