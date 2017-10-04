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
# Author: Tobin Ehlis <tobine@google.com>

import os,re,sys
import xml.etree.ElementTree as etree
from generator import *
from collections import namedtuple

#
# HelperFileOutputGeneratorOptions - subclass of GeneratorOptions.
class HelperFileOutputGeneratorOptions(GeneratorOptions):
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
                 library_name = '',
                 helper_file_type = ''):
        GeneratorOptions.__init__(self, filename, directory, apiname, profile,
                                  versions, emitversions, defaultExtensions,
                                  addExtensions, removeExtensions, sortProcedure)
        self.prefixText       = prefixText
        self.genFuncPointers  = genFuncPointers
        self.protectFile      = protectFile
        self.protectFeature   = protectFeature
        self.protectProto     = protectProto
        self.protectProtoStr  = protectProtoStr
        self.apicall          = apicall
        self.apientry         = apientry
        self.apientryp        = apientryp
        self.alignFuncParam   = alignFuncParam
        self.library_name     = library_name
        self.helper_file_type = helper_file_type
#
# HelperFileOutputGenerator - subclass of OutputGenerator. Outputs Vulkan helper files
class HelperFileOutputGenerator(OutputGenerator):
    """Generate helper file based on XML element attributes"""
    def __init__(self,
                 errFile = sys.stderr,
                 warnFile = sys.stderr,
                 diagFile = sys.stdout):
        OutputGenerator.__init__(self, errFile, warnFile, diagFile)
        # Internal state - accumulators for different inner block text
        self.enum_output = ''                             # string built up of enum string routines
        self.struct_size_h_output = ''                    # string built up of struct size header output
        self.struct_size_c_output = ''                    # string built up of struct size source output
        # Internal state - accumulators for different inner block text
        self.structNames = []                             # List of Vulkan struct typenames
        self.structTypes = dict()                         # Map of Vulkan struct typename to required VkStructureType
        self.structMembers = []                           # List of StructMemberData records for all Vulkan structs
        self.object_types = []                            # List of all handle types
        self.debug_report_object_types = []               # Handy copy of debug_report_object_type enum data
        self.core_object_types = []                       # Handy copy of core_object_type enum data
        self.device_extension_info = dict()               # Dict of device extension name defines and ifdef values
        self.instance_extension_info = dict()             # Dict of instance extension name defines and ifdef values

        # Named tuples to store struct and command data
        self.StructType = namedtuple('StructType', ['name', 'value'])
        self.CommandParam = namedtuple('CommandParam', ['type', 'name', 'ispointer', 'isstaticarray', 'isconst', 'iscount', 'len', 'extstructs', 'cdecl'])
        self.StructMemberData = namedtuple('StructMemberData', ['name', 'members', 'ifdef_protect'])
    #
    # Called once at the beginning of each run
    def beginFile(self, genOpts):
        OutputGenerator.beginFile(self, genOpts)
        # User-supplied prefix text, if any (list of strings)
        self.helper_file_type = genOpts.helper_file_type
        self.library_name = genOpts.library_name
        # File Comment
        file_comment = '// *** THIS FILE IS GENERATED - DO NOT EDIT ***\n'
        file_comment += '// See helper_file_generator.py for modifications\n'
        write(file_comment, file=self.outFile)
        # Copyright Notice
        copyright = ''
        copyright += '\n'
        copyright += '/***************************************************************************\n'
        copyright += ' *\n'
        copyright += ' * Copyright (c) 2015-2017 The Khronos Group Inc.\n'
        copyright += ' * Copyright (c) 2015-2017 Valve Corporation\n'
        copyright += ' * Copyright (c) 2015-2017 LunarG, Inc.\n'
        copyright += ' * Copyright (c) 2015-2017 Google Inc.\n'
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
        copyright += ' * Author: Courtney Goeltzenleuchter <courtneygo@google.com>\n'
        copyright += ' * Author: Tobin Ehlis <tobine@google.com>\n'
        copyright += ' * Author: Chris Forbes <chrisforbes@google.com>\n'
        copyright += ' *\n'
        copyright += ' ****************************************************************************/\n'
        write(copyright, file=self.outFile)
    #
    # Write generated file content to output file
    def endFile(self):
        dest_file = ''
        dest_file += self.OutputDestFile()
        # Remove blank lines at EOF
        if dest_file.endswith('\n'):
            dest_file = dest_file[:-1]
        write(dest_file, file=self.outFile);
        # Finish processing in superclass
        OutputGenerator.endFile(self)
    #
    # Override parent class to be notified of the beginning of an extension
    def beginFeature(self, interface, emit):
        # Start processing in superclass
        OutputGenerator.beginFeature(self, interface, emit)
        if self.featureName == 'VK_VERSION_1_0':
            return
        nameElem = interface[0][1]
        name = nameElem.get('name')
        if 'EXTENSION_NAME' not in name:
            print("Error in vk.xml file -- extension name is not available")
        if interface.get('type') == 'instance':
            self.instance_extension_info[name] = self.featureExtraProtect
        else:
            self.device_extension_info[name] = self.featureExtraProtect
    #
    # Override parent class to be notified of the end of an extension
    def endFeature(self):
        # Finish processing in superclass
        OutputGenerator.endFeature(self)
    #
    # Grab group (e.g. C "enum" type) info to output for enum-string conversion helper
    def genGroup(self, groupinfo, groupName):
        OutputGenerator.genGroup(self, groupinfo, groupName)
        groupElem = groupinfo.elem
        # For enum_string_header
        if self.helper_file_type == 'enum_string_header':
            value_list = []
            for elem in groupElem.findall('enum'):
                if elem.get('supported') != 'disabled':
                    item_name = elem.get('name')
                    value_list.append(item_name)
            if value_list is not None:
                self.enum_output += self.GenerateEnumStringConversion(groupName, value_list)
        elif self.helper_file_type == 'object_types_header':
            if groupName == 'VkDebugReportObjectTypeEXT':
                for elem in groupElem.findall('enum'):
                    if elem.get('supported') != 'disabled':
                        item_name = elem.get('name')
                        self.debug_report_object_types.append(item_name)
            elif groupName == 'VkObjectType':
                for elem in groupElem.findall('enum'):
                    if elem.get('supported') != 'disabled':
                        item_name = elem.get('name')
                        self.core_object_types.append(item_name)

    #
    # Called for each type -- if the type is a struct/union, grab the metadata
    def genType(self, typeinfo, name):
        OutputGenerator.genType(self, typeinfo, name)
        typeElem = typeinfo.elem
        # If the type is a struct type, traverse the imbedded <member> tags generating a structure.
        # Otherwise, emit the tag text.
        category = typeElem.get('category')
        if category == 'handle':
            self.object_types.append(name)
        elif (category == 'struct' or category == 'union'):
            self.structNames.append(name)
            self.genStruct(typeinfo, name)
    #
    # Generate a VkStructureType based on a structure typename
    def genVkStructureType(self, typename):
        # Add underscore between lowercase then uppercase
        value = re.sub('([a-z0-9])([A-Z])', r'\1_\2', typename)
        # Change to uppercase
        value = value.upper()
        # Add STRUCTURE_TYPE_
        return re.sub('VK_', 'VK_STRUCTURE_TYPE_', value)
    #
    # Check if the parameter passed in is a pointer
    def paramIsPointer(self, param):
        ispointer = False
        for elem in param:
            if ((elem.tag is not 'type') and (elem.tail is not None)) and '*' in elem.tail:
                ispointer = True
        return ispointer
    #
    # Check if the parameter passed in is a static array
    def paramIsStaticArray(self, param):
        isstaticarray = 0
        paramname = param.find('name')
        if (paramname.tail is not None) and ('[' in paramname.tail):
            isstaticarray = paramname.tail.count('[')
        return isstaticarray
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
    # Extract length values from latexmath.  Currently an inflexible solution that looks for specific
    # patterns that are found in vk.xml.  Will need to be updated when new patterns are introduced.
    def parseLateXMath(self, source):
        name = 'ERROR'
        decoratedName = 'ERROR'
        if 'mathit' in source:
            # Matches expressions similar to 'latexmath:[\lceil{\mathit{rasterizationSamples} \over 32}\rceil]'
            match = re.match(r'latexmath\s*\:\s*\[\s*\\l(\w+)\s*\{\s*\\mathit\s*\{\s*(\w+)\s*\}\s*\\over\s*(\d+)\s*\}\s*\\r(\w+)\s*\]', source)
            if not match or match.group(1) != match.group(4):
                raise 'Unrecognized latexmath expression'
            name = match.group(2)
            # Need to add 1 for ceiling function; otherwise, the allocated packet
            # size will be less than needed during capture for some title which use
            # this in VkPipelineMultisampleStateCreateInfo. based on ceiling function
            # definition,it is '{0}%{1}?{0}/{1} + 1:{0}/{1}'.format(*match.group(2, 3)),
            # its value <= '{}/{} + 1'.
            if match.group(1) == 'ceil':
                decoratedName = '{}/{} + 1'.format(*match.group(2, 3))
            else:
                decoratedName = '{}/{}'.format(*match.group(2, 3))
        else:
            # Matches expressions similar to 'latexmath : [dataSize \over 4]'
            match = re.match(r'latexmath\s*\:\s*\[\s*(\w+)\s*\\over\s*(\d+)\s*\]', source)
            name = match.group(1)
            decoratedName = '{}/{}'.format(*match.group(1, 2))
        return name, decoratedName
    #
    # Retrieve the value of the len tag
    def getLen(self, param):
        result = None
        len = param.attrib.get('len')
        if len and len != 'null-terminated':
            # For string arrays, 'len' can look like 'count,null-terminated', indicating that we
            # have a null terminated array of strings.  We strip the null-terminated from the
            # 'len' field and only return the parameter specifying the string count
            if 'null-terminated' in len:
                result = len.split(',')[0]
            else:
                result = len
            if 'latexmath' in len:
                param_type, param_name = self.getTypeNameTuple(param)
                len_name, result = self.parseLateXMath(len)
            # Spec has now notation for len attributes, using :: instead of platform specific pointer symbol
            result = str(result).replace('::', '->')
        return result
    #
    # Check if a structure is or contains a dispatchable (dispatchable = True) or 
    # non-dispatchable (dispatchable = False) handle
    def TypeContainsObjectHandle(self, handle_type, dispatchable):
        if dispatchable:
            type_key = 'VK_DEFINE_HANDLE'
        else:
            type_key = 'VK_DEFINE_NON_DISPATCHABLE_HANDLE'
        handle = self.registry.tree.find("types/type/[name='" + handle_type + "'][@category='handle']")
        if handle is not None and handle.find('type').text == type_key:
            return True
        # if handle_type is a struct, search its members
        if handle_type in self.structNames:
            member_index = next((i for i, v in enumerate(self.structMembers) if v[0] == handle_type), None)
            if member_index is not None:
                for item in self.structMembers[member_index].members:
                    handle = self.registry.tree.find("types/type/[name='" + item.type + "'][@category='handle']")
                    if handle is not None and handle.find('type').text == type_key:
                        return True
        return False
    #
    # Generate local ready-access data describing Vulkan structures and unions from the XML metadata
    def genStruct(self, typeinfo, typeName):
        OutputGenerator.genStruct(self, typeinfo, typeName)
        members = typeinfo.elem.findall('.//member')
        # Iterate over members once to get length parameters for arrays
        lens = set()
        for member in members:
            len = self.getLen(member)
            if len:
                lens.add(len)
        # Generate member info
        membersInfo = []
        for member in members:
            # Get the member's type and name
            info = self.getTypeNameTuple(member)
            type = info[0]
            name = info[1]
            cdecl = self.makeCParamDecl(member, 1)
            # Process VkStructureType
            if type == 'VkStructureType':
                # Extract the required struct type value from the comments
                # embedded in the original text defining the 'typeinfo' element
                rawXml = etree.tostring(typeinfo.elem).decode('ascii')
                result = re.search(r'VK_STRUCTURE_TYPE_\w+', rawXml)
                if result:
                    value = result.group(0)
                else:
                    value = self.genVkStructureType(typeName)
                # Store the required type value
                self.structTypes[typeName] = self.StructType(name=name, value=value)
            # Store pointer/array/string info
            isstaticarray = self.paramIsStaticArray(member)
            membersInfo.append(self.CommandParam(type=type,
                                                 name=name,
                                                 ispointer=self.paramIsPointer(member),
                                                 isstaticarray=isstaticarray,
                                                 isconst=True if 'const' in cdecl else False,
                                                 iscount=True if name in lens else False,
                                                 len=self.getLen(member),
                                                 extstructs=self.registry.validextensionstructs[typeName] if name == 'pNext' else None,
                                                 cdecl=cdecl))
        self.structMembers.append(self.StructMemberData(name=typeName, members=membersInfo, ifdef_protect=self.featureExtraProtect))
    #
    # Enum_string_header: Create a routine to convert an enumerated value into a string
    def GenerateEnumStringConversion(self, groupName, value_list):
        outstring = '\n'
        outstring += 'static inline const char* string_%s(%s input_value)\n' % (groupName, groupName)
        outstring += '{\n'
        outstring += '    switch ((%s)input_value)\n' % groupName
        outstring += '    {\n'
        for item in value_list:
            outstring += '        case %s:\n' % item
            outstring += '            return "%s";\n' % item
        outstring += '        default:\n'
        outstring += '            return "Unhandled %s";\n' % groupName
        outstring += '    }\n'
        outstring += '}\n'
        return outstring
    #
    # Tack on a helper which, given an index into a VkPhysicalDeviceFeatures structure, will print the corresponding feature name
    def DeIndexPhysDevFeatures(self):
        pdev_members = None
        for name, members, ifdef in self.structMembers:
            if name == 'VkPhysicalDeviceFeatures':
                pdev_members = members
                break
        deindex = '\n'
        deindex += 'static const char * GetPhysDevFeatureString(uint32_t index) {\n'
        deindex += '    const char * IndexToPhysDevFeatureString[] = {\n'
        for feature in pdev_members:
            deindex += '        "%s",\n' % feature.name
        deindex += '    };\n\n'
        deindex += '    return IndexToPhysDevFeatureString[index];\n'
        deindex += '}\n'
        return deindex
    #
    # Combine enum string helper header file preamble with body text and return
    def GenerateEnumStringHelperHeader(self):
            enum_string_helper_header = '\n'
            enum_string_helper_header += '#pragma once\n'
            enum_string_helper_header += '#ifdef _WIN32\n'
            enum_string_helper_header += '#pragma warning( disable : 4065 )\n'
            enum_string_helper_header += '#endif\n'
            enum_string_helper_header += '\n'
            enum_string_helper_header += '#include <vulkan/vulkan.h>\n'
            enum_string_helper_header += '\n'
            enum_string_helper_header += self.enum_output
            enum_string_helper_header += self.DeIndexPhysDevFeatures()
            return enum_string_helper_header
    #
    # struct_size_header: build function prototypes for header file
    def GenerateStructSizeHeader(self):
        outstring = ''
        outstring += 'size_t get_struct_chain_size(const void* struct_ptr);\n'
        for item in self.structMembers:
            lower_case_name = item.name.lower()
            if item.ifdef_protect != None:
                outstring += '#ifdef %s\n' % item.ifdef_protect
            outstring += 'size_t vk_size_%s(const %s* struct_ptr);\n' % (item.name.lower(), item.name)
            if item.ifdef_protect != None:
                outstring += '#endif // %s\n' % item.ifdef_protect
        outstring += '#ifdef __cplusplus\n'
        outstring += '}\n'
        outstring += '#endif'
        return outstring
    #
    # Combine struct size helper header file preamble with body text and return
    def GenerateStructSizeHelperHeader(self):
        struct_size_helper_header = '\n'
        struct_size_helper_header += '#ifdef __cplusplus\n'
        struct_size_helper_header += 'extern "C" {\n'
        struct_size_helper_header += '#endif\n'
        struct_size_helper_header += '\n'
        struct_size_helper_header += '#include <stdio.h>\n'
        struct_size_helper_header += '#include <stdlib.h>\n'
        struct_size_helper_header += '#include <vulkan/vulkan.h>\n'
        struct_size_helper_header += '\n'
        struct_size_helper_header += '// Function Prototypes\n'
        struct_size_helper_header += self.GenerateStructSizeHeader()
        return struct_size_helper_header
    #
    # Helper function for declaring a counter variable only once
    def DeclareCounter(self, string_var, declare_flag):
        if declare_flag == False:
            string_var += '        uint32_t i = 0;\n'
            declare_flag = True
        return string_var, declare_flag
    #
    # Build the header of the get_struct_chain_size function
    def GenerateChainSizePreamble(self):
        preable = '\nsize_t get_struct_chain_size(const void* struct_ptr) {\n'
        preable += '    // Use VkApplicationInfo as struct until actual type is resolved\n'
        preable += '    VkApplicationInfo* pNext = (VkApplicationInfo*)struct_ptr;\n'
        preable += '    size_t struct_size = 0;\n'
        preable += '    while (pNext) {\n'
        preable += '        switch (pNext->sType) {\n'
        return preable
    #
    # Build the footer of the get_struct_chain_size function
    def GenerateChainSizePostamble(self):
        postamble  = '            default:\n'
        postamble += '                assert(0);\n'
        postamble += '                struct_size += 0;\n'
        postamble += '        }\n'
        postamble += '        pNext = (VkApplicationInfo*)pNext->pNext;\n'
        postamble += '    }\n'
        postamble += '    return struct_size;\n'
        postamble += '}'
        return postamble
    #
    # struct_size_helper source -- create bodies of struct size helper functions
    def GenerateStructSizeSource(self):
        # Construct the body of the routine and get_struct_chain_size() simultaneously
        struct_size_body = ''
        chain_size  = self.GenerateChainSizePreamble()
        for item in self.structMembers:
            struct_size_body += '\n'
            lower_case_name = item.name.lower()
            if item.ifdef_protect != None:
                struct_size_body += '#ifdef %s\n' % item.ifdef_protect
                chain_size += '#ifdef %s\n' % item.ifdef_protect
            if item.name in self.structTypes:
                chain_size += '            case %s: {\n' % self.structTypes[item.name].value
                chain_size += '                struct_size += vk_size_%s((%s*)pNext);\n' % (item.name.lower(), item.name)
                chain_size += '                break;\n'
                chain_size += '            }\n'
            struct_size_body += 'size_t vk_size_%s(const %s* struct_ptr) {\n' % (item.name.lower(), item.name)
            struct_size_body += '    size_t struct_size = 0;\n'
            struct_size_body += '    if (struct_ptr) {\n'
            struct_size_body += '        struct_size = sizeof(%s);\n' % item.name
            counter_declared = False
            for member in item.members:
                vulkan_type = next((i for i, v in enumerate(self.structMembers) if v[0] == member.type), None)
                if member.ispointer == True:
                    if vulkan_type is not None:
                        # If this is another Vulkan structure call generated size function
                        if member.len is not None:
                            struct_size_body, counter_declared = self.DeclareCounter(struct_size_body, counter_declared)
                            struct_size_body += '        for (i = 0; i < struct_ptr->%s; i++) {\n' % member.len
                            struct_size_body += '            struct_size += vk_size_%s(&struct_ptr->%s[i]);\n' % (member.type.lower(), member.name)
                            struct_size_body += '        }\n'
                        else:
                            struct_size_body += '        struct_size += vk_size_%s(struct_ptr->%s);\n' % (member.type.lower(), member.name)
                    else:
                        if member.type == 'char':
                            # Deal with sizes of character strings
                            if member.len is not None:
                                struct_size_body, counter_declared = self.DeclareCounter(struct_size_body, counter_declared)
                                struct_size_body += '        for (i = 0; i < struct_ptr->%s; i++) {\n' % member.len
                                struct_size_body += '            struct_size += (sizeof(char*) + (sizeof(char) * (1 + strlen(struct_ptr->%s[i]))));\n' % (member.name)
                                struct_size_body += '        }\n'
                            else:
                                struct_size_body += '        struct_size += (struct_ptr->%s != NULL) ? sizeof(char)*(1+strlen(struct_ptr->%s)) : 0;\n' % (member.name, member.name)
                        else:
                            if member.len is not None:
                                # Avoid using 'sizeof(void)', which generates compile-time warnings/errors
                                checked_type = member.type
                                if checked_type == 'void':
                                    checked_type = 'void*'
                                struct_size_body += '        struct_size += (struct_ptr->%s ) * sizeof(%s);\n' % (member.len, checked_type)
            struct_size_body += '    }\n'
            struct_size_body += '    return struct_size;\n'
            struct_size_body += '}\n'
            if item.ifdef_protect != None:
                struct_size_body += '#endif // %s\n' % item.ifdef_protect
                chain_size += '#endif // %s\n' % item.ifdef_protect
        chain_size += self.GenerateChainSizePostamble()
        struct_size_body += chain_size;
        return struct_size_body
    #
    # Combine struct size helper source file preamble with body text and return
    def GenerateStructSizeHelperSource(self):
        struct_size_helper_source = '\n'
        struct_size_helper_source += '#include "vk_struct_size_helper.h"\n'
        struct_size_helper_source += '#include <string.h>\n'
        struct_size_helper_source += '#include <assert.h>\n'
        struct_size_helper_source += '\n'
        struct_size_helper_source += '// Function Definitions\n'
        struct_size_helper_source += self.GenerateStructSizeSource()
        return struct_size_helper_source
    #
    # Combine safe struct helper header file preamble with body text and return
    def GenerateSafeStructHelperHeader(self):
        safe_struct_helper_header = '\n'
        safe_struct_helper_header += '#pragma once\n'
        safe_struct_helper_header += '#include <vulkan/vulkan.h>\n'
        safe_struct_helper_header += '\n'
        safe_struct_helper_header += self.GenerateSafeStructHeader()
        return safe_struct_helper_header
    #
    # safe_struct header: build function prototypes for header file
    def GenerateSafeStructHeader(self):
        safe_struct_header = ''
        for item in self.structMembers:
            if self.NeedSafeStruct(item) == True:
                safe_struct_header += '\n'
                if item.ifdef_protect != None:
                    safe_struct_header += '#ifdef %s\n' % item.ifdef_protect
                safe_struct_header += 'struct safe_%s {\n' % (item.name)
                for member in item.members:
                    if member.type in self.structNames:
                        member_index = next((i for i, v in enumerate(self.structMembers) if v[0] == member.type), None)
                        if member_index is not None and self.NeedSafeStruct(self.structMembers[member_index]) == True:
                            if member.ispointer:
                                safe_struct_header += '    safe_%s* %s;\n' % (member.type, member.name)
                            else:
                                safe_struct_header += '    safe_%s %s;\n' % (member.type, member.name)
                            continue
                    if member.len is not None and (self.TypeContainsObjectHandle(member.type, True) or self.TypeContainsObjectHandle(member.type, False)):
                            safe_struct_header += '    %s* %s;\n' % (member.type, member.name)
                    else:
                        safe_struct_header += '%s;\n' % member.cdecl
                safe_struct_header += '    safe_%s(const %s* in_struct);\n' % (item.name, item.name)
                safe_struct_header += '    safe_%s(const safe_%s& src);\n' % (item.name, item.name)
                safe_struct_header += '    safe_%s& operator=(const safe_%s& src);\n' % (item.name, item.name)
                safe_struct_header += '    safe_%s();\n' % item.name
                safe_struct_header += '    ~safe_%s();\n' % item.name
                safe_struct_header += '    void initialize(const %s* in_struct);\n' % item.name
                safe_struct_header += '    void initialize(const safe_%s* src);\n' % item.name
                safe_struct_header += '    %s *ptr() { return reinterpret_cast<%s *>(this); }\n' % (item.name, item.name)
                safe_struct_header += '    %s const *ptr() const { return reinterpret_cast<%s const *>(this); }\n' % (item.name, item.name)
                safe_struct_header += '};\n'
                if item.ifdef_protect != None:
                    safe_struct_header += '#endif // %s\n' % item.ifdef_protect
        return safe_struct_header
    #
    # Generate extension helper header file
    def GenerateExtensionHelperHeader(self):
        extension_helper_header = '\n'
        extension_helper_header += '#ifndef VK_EXTENSION_HELPER_H_\n'
        extension_helper_header += '#define VK_EXTENSION_HELPER_H_\n'
        struct  = '\n'
        extension_helper_header += '#include <vulkan/vulkan.h>\n'
        extension_helper_header += '#include <string.h>\n'
        extension_helper_header += '#include <utility>\n'
        extension_helper_header += '\n'
        extension_dict = dict()
        for type in ['Instance', 'Device']:
            if type == 'Instance':
                extension_dict = self.instance_extension_info
                struct += 'struct InstanceExtensions { \n'
            else:
                extension_dict = self.device_extension_info
                struct += 'struct DeviceExtensions : public InstanceExtensions { \n'
            for ext_name, ifdef in extension_dict.items():
                bool_name = ext_name.lower()
                bool_name = re.sub('_extension_name', '', bool_name)
                struct += '    bool %s{false};\n' % bool_name
            struct += '\n'
            if type == 'Instance':
                struct += '    void InitFromInstanceCreateInfo(const VkInstanceCreateInfo *pCreateInfo) {\n'
            else:
                struct += '    void InitFromDeviceCreateInfo(const InstanceExtensions *instance_extensions, const VkDeviceCreateInfo *pCreateInfo) {\n'
            struct += '\n'
            struct += '        static const std::pair<char const *, bool %sExtensions::*> known_extensions[]{\n' % type
            for ext_name, ifdef in extension_dict.items():
                if ifdef is not None:
                    struct += '#ifdef %s\n' % ifdef
                bool_name = ext_name.lower()
                bool_name = re.sub('_extension_name', '', bool_name)
                struct += '            {%s, &%sExtensions::%s},\n' % (ext_name, type, bool_name)
                if ifdef is not None:
                    struct += '#endif\n'
            struct += '        };\n'
            struct += '\n'
            struct += '        // Initialize struct data\n'
            for ext_name, ifdef in self.instance_extension_info.items():
                bool_name = ext_name.lower()
                bool_name = re.sub('_extension_name', '', bool_name)
                if type == 'Device':
                    struct += '        %s = instance_extensions->%s;\n' % (bool_name, bool_name)
            struct += '\n'
            struct += '        for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {\n'
            struct += '            for (auto ext : known_extensions) {\n'
            struct += '                if (!strcmp(ext.first, pCreateInfo->ppEnabledExtensionNames[i])) {\n'
            struct += '                    this->*(ext.second) = true;\n'
            struct += '                    break;\n'
            struct += '                }\n'
            struct += '            }\n'
            struct += '        }\n'
            struct += '    }\n'
            struct += '};\n'
            struct += '\n'
        extension_helper_header += struct
        extension_helper_header += '\n'
        extension_helper_header += '#endif // VK_EXTENSION_HELPER_H_\n'
        return extension_helper_header
    #
    # Combine object types helper header file preamble with body text and return
    def GenerateObjectTypesHelperHeader(self):
        object_types_helper_header = '\n'
        object_types_helper_header += '#pragma once\n'
        object_types_helper_header += '\n'
        object_types_helper_header += '#include <vulkan/vulkan.h>\n\n'
        object_types_helper_header += self.GenerateObjectTypesHeader()
        return object_types_helper_header
    #
    # Object types header: create object enum type header file
    def GenerateObjectTypesHeader(self):
        object_types_header = '// Object Type enum for validation layer internal object handling\n'
        object_types_header += 'typedef enum VulkanObjectType {\n'
        object_types_header += '    kVulkanObjectTypeUnknown = 0,\n'
        enum_num = 1
        type_list = [];

        # Output enum definition as each handle is processed, saving the names to use for the conversion routine
        for item in self.object_types:
            fixup_name = item[2:]
            enum_entry = 'kVulkanObjectType%s' % fixup_name
            object_types_header += '    ' + enum_entry
            object_types_header += ' = %d,\n' % enum_num
            enum_num += 1
            type_list.append(enum_entry)
        object_types_header += '    kVulkanObjectTypeMax = %d,\n' % enum_num
        object_types_header += '} VulkanObjectType;\n\n'

        # Output name string helper
        object_types_header += '// Array of object name strings for OBJECT_TYPE enum conversion\n'
        object_types_header += 'static const char * const object_string[kVulkanObjectTypeMax] = {\n'
        object_types_header += '    "Unknown",\n'
        for item in self.object_types:
            fixup_name = item[2:]
            object_types_header += '    "%s",\n' % fixup_name
        object_types_header += '};\n'

        # Output a conversion routine from the layer object definitions to the debug report definitions
        object_types_header += '\n'
        object_types_header += '// Helper array to get Vulkan VK_EXT_debug_report object type enum from the internal layers version\n'
        object_types_header += 'const VkDebugReportObjectTypeEXT get_debug_report_enum[] = {\n'
        for object_type in type_list:
            done = False
            search_type = object_type.replace("kVulkanObjectType", "").lower()
            for vk_object_type in self.debug_report_object_types:
                target_type = vk_object_type.replace("VK_DEBUG_REPORT_OBJECT_TYPE_", "").lower()
                target_type = target_type[:-4]
                target_type = target_type.replace("_", "")
                if search_type == target_type:
                    object_types_header += '    %s,   // %s\n' % (vk_object_type, object_type)
                    done = True
                    break
            if done == False:
                object_types_header += '    VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, // No Match\n'
        object_types_header += '};\n'

        # Output a conversion routine from the layer object definitions to the core object type definitions
        object_types_header += '\n'
        object_types_header += '// Helper array to get Official Vulkan VkObjectType enum from the internal layers version\n'
        object_types_header += 'const VkObjectType get_object_type_enum[] = {\n'
        for object_type in type_list:
            done = False
            search_type = object_type.replace("kVulkanObjectType", "").lower()
            for vk_object_type in self.core_object_types:
                target_type = vk_object_type.replace("VK_OBJECT_TYPE_", "").lower()
                target_type = target_type.replace("_", "")
                if search_type == target_type:
                    object_types_header += '    %s,   // %s\n' % (vk_object_type, object_type)
                    done = True
                    break
            if done == False:
                object_types_header += '    VK_OBJECT_TYPE_UNKNOWN, // No Match\n'
        object_types_header += '};\n'

        return object_types_header
    #
    # Determine if a structure needs a safe_struct helper function
    # That is, it has an sType or one of its members is a pointer
    def NeedSafeStruct(self, structure):
        if 'sType' == structure.name:
            return True
        for member in structure.members:
            if member.ispointer == True:
                return True
        return False
    #
    # Combine safe struct helper source file preamble with body text and return
    def GenerateSafeStructHelperSource(self):
        safe_struct_helper_source = '\n'
        safe_struct_helper_source += '#include "vk_safe_struct.h"\n'
        safe_struct_helper_source += '#include <string.h>\n'
        safe_struct_helper_source += '\n'
        safe_struct_helper_source += self.GenerateSafeStructSource()
        return safe_struct_helper_source
    #
    # safe_struct source -- create bodies of safe struct helper functions
    def GenerateSafeStructSource(self):
        safe_struct_body = []
        wsi_structs = ['VkXlibSurfaceCreateInfoKHR',
                       'VkXcbSurfaceCreateInfoKHR',
                       'VkWaylandSurfaceCreateInfoKHR',
                       'VkMirSurfaceCreateInfoKHR',
                       'VkAndroidSurfaceCreateInfoKHR',
                       'VkWin32SurfaceCreateInfoKHR'
                       ]
        for item in self.structMembers:
            if self.NeedSafeStruct(item) == False:
                continue
            if item.name in wsi_structs:
                continue
            if item.ifdef_protect != None:
                safe_struct_body.append("#ifdef %s\n" % item.ifdef_protect)
            ss_name = "safe_%s" % item.name
            init_list = ''          # list of members in struct constructor initializer
            default_init_list = ''  # Default constructor just inits ptrs to nullptr in initializer
            init_func_txt = ''      # Txt for initialize() function that takes struct ptr and inits members
            construct_txt = ''      # Body of constuctor as well as body of initialize() func following init_func_txt
            destruct_txt = ''
            # VkWriteDescriptorSet is special case because pointers may be non-null but ignored
            custom_construct_txt = {'VkWriteDescriptorSet' :
                                    '    switch (descriptorType) {\n'
                                    '        case VK_DESCRIPTOR_TYPE_SAMPLER:\n'
                                    '        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:\n'
                                    '        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:\n'
                                    '        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:\n'
                                    '        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:\n'
                                    '        if (descriptorCount && in_struct->pImageInfo) {\n'
                                    '            pImageInfo = new VkDescriptorImageInfo[descriptorCount];\n'
                                    '            for (uint32_t i=0; i<descriptorCount; ++i) {\n'
                                    '                pImageInfo[i] = in_struct->pImageInfo[i];\n'
                                    '            }\n'
                                    '        }\n'
                                    '        break;\n'
                                    '        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:\n'
                                    '        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:\n'
                                    '        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:\n'
                                    '        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:\n'
                                    '        if (descriptorCount && in_struct->pBufferInfo) {\n'
                                    '            pBufferInfo = new VkDescriptorBufferInfo[descriptorCount];\n'
                                    '            for (uint32_t i=0; i<descriptorCount; ++i) {\n'
                                    '                pBufferInfo[i] = in_struct->pBufferInfo[i];\n'
                                    '            }\n'
                                    '        }\n'
                                    '        break;\n'
                                    '        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:\n'
                                    '        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:\n'
                                    '        if (descriptorCount && in_struct->pTexelBufferView) {\n'
                                    '            pTexelBufferView = new VkBufferView[descriptorCount];\n'
                                    '            for (uint32_t i=0; i<descriptorCount; ++i) {\n'
                                    '                pTexelBufferView[i] = in_struct->pTexelBufferView[i];\n'
                                    '            }\n'
                                    '        }\n'
                                    '        break;\n'
                                    '        default:\n'
                                    '        break;\n'
                                    '    }\n',
                                    'VkShaderModuleCreateInfo' :
                                    '    if (in_struct->pCode) {\n'
                                    '        pCode = reinterpret_cast<uint32_t *>(new uint8_t[codeSize]);\n'
                                    '        memcpy((void *)pCode, (void *)in_struct->pCode, codeSize);\n'
                                    '    }\n'}
            custom_destruct_txt = {'VkShaderModuleCreateInfo' :
                                   '    if (pCode)\n'
                                   '        delete[] reinterpret_cast<const uint8_t *>(pCode);\n' }

            for member in item.members:
                m_type = member.type
                if member.type in self.structNames:
                    member_index = next((i for i, v in enumerate(self.structMembers) if v[0] == member.type), None)
                    if member_index is not None and self.NeedSafeStruct(self.structMembers[member_index]) == True:
                        m_type = 'safe_%s' % member.type
                if member.ispointer and 'safe_' not in m_type and self.TypeContainsObjectHandle(member.type, False) == False:
                    # Ptr types w/o a safe_struct, for non-null case need to allocate new ptr and copy data in
                    if m_type in ['void', 'char']:
                        # For these exceptions just copy initial value over for now
                        init_list += '\n    %s(in_struct->%s),' % (member.name, member.name)
                        init_func_txt += '    %s = in_struct->%s;\n' % (member.name, member.name)
                    else:
                        default_init_list += '\n    %s(nullptr),' % (member.name)
                        init_list += '\n    %s(nullptr),' % (member.name)
                        init_func_txt += '    %s = nullptr;\n' % (member.name)
                        if 'pNext' != member.name and 'void' not in m_type:
                            if not member.isstaticarray and (member.len is None or '/' in member.len):
                                construct_txt += '    if (in_struct->%s) {\n' % member.name
                                construct_txt += '        %s = new %s(*in_struct->%s);\n' % (member.name, m_type, member.name)
                                construct_txt += '    }\n'
                                destruct_txt += '    if (%s)\n' % member.name
                                destruct_txt += '        delete %s;\n' % member.name
                            else:
                                construct_txt += '    if (in_struct->%s) {\n' % member.name
                                construct_txt += '        %s = new %s[in_struct->%s];\n' % (member.name, m_type, member.len)
                                construct_txt += '        memcpy ((void *)%s, (void *)in_struct->%s, sizeof(%s)*in_struct->%s);\n' % (member.name, member.name, m_type, member.len)
                                construct_txt += '    }\n'
                                destruct_txt += '    if (%s)\n' % member.name
                                destruct_txt += '        delete[] %s;\n' % member.name
                elif member.isstaticarray or member.len is not None:
                    if member.len is None:
                        # Extract length of static array by grabbing val between []
                        static_array_size = re.match(r"[^[]*\[([^]]*)\]", member.cdecl)
                        construct_txt += '    for (uint32_t i=0; i<%s; ++i) {\n' % static_array_size.group(1)
                        construct_txt += '        %s[i] = in_struct->%s[i];\n' % (member.name, member.name)
                        construct_txt += '    }\n'
                    else:
                        # Init array ptr to NULL
                        default_init_list += '\n    %s(nullptr),' % member.name
                        init_list += '\n    %s(nullptr),' % member.name
                        init_func_txt += '    %s = nullptr;\n' % member.name
                        array_element = 'in_struct->%s[i]' % member.name
                        if member.type in self.structNames:
                            member_index = next((i for i, v in enumerate(self.structMembers) if v[0] == member.type), None)
                            if member_index is not None and self.NeedSafeStruct(self.structMembers[member_index]) == True:
                                array_element = '%s(&in_struct->safe_%s[i])' % (member.type, member.name)
                        construct_txt += '    if (%s && in_struct->%s) {\n' % (member.len, member.name)
                        construct_txt += '        %s = new %s[%s];\n' % (member.name, m_type, member.len)
                        destruct_txt += '    if (%s)\n' % member.name
                        destruct_txt += '        delete[] %s;\n' % member.name
                        construct_txt += '        for (uint32_t i=0; i<%s; ++i) {\n' % (member.len)
                        if 'safe_' in m_type:
                            construct_txt += '            %s[i].initialize(&in_struct->%s[i]);\n' % (member.name, member.name)
                        else:
                            construct_txt += '            %s[i] = %s;\n' % (member.name, array_element)
                        construct_txt += '        }\n'
                        construct_txt += '    }\n'
                elif member.ispointer == True:
                    construct_txt += '    if (in_struct->%s)\n' % member.name
                    construct_txt += '        %s = new %s(in_struct->%s);\n' % (member.name, m_type, member.name)
                    construct_txt += '    else\n'
                    construct_txt += '        %s = NULL;\n' % member.name
                    destruct_txt += '    if (%s)\n' % member.name
                    destruct_txt += '        delete %s;\n' % member.name
                elif 'safe_' in m_type:
                    init_list += '\n    %s(&in_struct->%s),' % (member.name, member.name)
                    init_func_txt += '    %s.initialize(&in_struct->%s);\n' % (member.name, member.name)
                else:
                    init_list += '\n    %s(in_struct->%s),' % (member.name, member.name)
                    init_func_txt += '    %s = in_struct->%s;\n' % (member.name, member.name)
            if '' != init_list:
                init_list = init_list[:-1] # hack off final comma
            if item.name in custom_construct_txt:
                construct_txt = custom_construct_txt[item.name]
            if item.name in custom_destruct_txt:
                destruct_txt = custom_destruct_txt[item.name]
            safe_struct_body.append("\n%s::%s(const %s* in_struct) :%s\n{\n%s}" % (ss_name, ss_name, item.name, init_list, construct_txt))
            if '' != default_init_list:
                default_init_list = " :%s" % (default_init_list[:-1])
            safe_struct_body.append("\n%s::%s()%s\n{}" % (ss_name, ss_name, default_init_list))
            # Create slight variation of init and construct txt for copy constructor that takes a src object reference vs. struct ptr
            copy_construct_init = init_func_txt.replace('in_struct->', 'src.')
            copy_construct_txt = construct_txt.replace(' (in_struct->', ' (src.')     # Exclude 'if' blocks from next line
            copy_construct_txt = copy_construct_txt.replace('(in_struct->', '(*src.') # Pass object to copy constructors
            copy_construct_txt = copy_construct_txt.replace('in_struct->', 'src.')    # Modify remaining struct refs for src object
            copy_assign_txt = '    if (&src == this) return *this;\n\n' + destruct_txt + '\n' + copy_construct_init + copy_construct_txt + '\n    return *this;'
            safe_struct_body.append("\n%s::%s(const %s& src)\n{\n%s%s}" % (ss_name, ss_name, ss_name, copy_construct_init, copy_construct_txt)) # Copy constructor
            safe_struct_body.append("\n%s& %s::operator=(const %s& src)\n{\n%s\n}" % (ss_name, ss_name, ss_name, copy_assign_txt)) # Copy assignment operator
            safe_struct_body.append("\n%s::~%s()\n{\n%s}" % (ss_name, ss_name, destruct_txt))
            safe_struct_body.append("\nvoid %s::initialize(const %s* in_struct)\n{\n%s%s}" % (ss_name, item.name, init_func_txt, construct_txt))
            # Copy initializer uses same txt as copy constructor but has a ptr and not a reference
            init_copy = copy_construct_init.replace('src.', 'src->')
            init_construct = copy_construct_txt.replace('src.', 'src->')
            safe_struct_body.append("\nvoid %s::initialize(const %s* src)\n{\n%s%s}" % (ss_name, ss_name, init_copy, init_construct))
            if item.ifdef_protect != None:
                safe_struct_body.append("#endif // %s\n" % item.ifdef_protect)
        return "\n".join(safe_struct_body)
    #
    # Create a helper file and return it as a string
    def OutputDestFile(self):
        if self.helper_file_type == 'enum_string_header':
            return self.GenerateEnumStringHelperHeader()
        elif self.helper_file_type == 'struct_size_header':
            return self.GenerateStructSizeHelperHeader()
        elif self.helper_file_type == 'struct_size_source':
            return self.GenerateStructSizeHelperSource()
        elif self.helper_file_type == 'safe_struct_header':
            return self.GenerateSafeStructHelperHeader()
        elif self.helper_file_type == 'safe_struct_source':
            return self.GenerateSafeStructHelperSource()
        elif self.helper_file_type == 'object_types_header':
            return self.GenerateObjectTypesHelperHeader()
        elif self.helper_file_type == 'extension_helper_header':
            return self.GenerateExtensionHelperHeader()
        else:
            return 'Bad Helper File Generator Option %s' % self.helper_file_type

