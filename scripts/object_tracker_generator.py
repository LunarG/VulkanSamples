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

import os,re,sys,string
import xml.etree.ElementTree as etree
from generator import *
from collections import namedtuple
from vuid_mapping import *

# This is a workaround to use a Python 2.7 and 3.x compatible syntax.
from io import open

# ObjectTrackerGeneratorOptions - subclass of GeneratorOptions.
#
# Adds options used by ObjectTrackerOutputGenerator objects during
# object_tracker layer generation.
#
# Additional members
#   prefixText - list of strings to prefix generated header with
#     (usually a copyright statement + calling convention macros).
#   protectFile - True if multiple inclusion protection should be
#     generated (based on the filename) around the entire header.
#   protectFeature - True if #ifndef..#endif protection should be
#     generated around a feature interface in the header file.
#   genFuncPointers - True if function pointer typedefs should be
#     generated
#   protectProto - If conditional protection should be generated
#     around prototype declarations, set to either '#ifdef'
#     to require opt-in (#ifdef protectProtoStr) or '#ifndef'
#     to require opt-out (#ifndef protectProtoStr). Otherwise
#     set to None.
#   protectProtoStr - #ifdef/#ifndef symbol to use around prototype
#     declarations, if protectProto is set
#   apicall - string to use for the function declaration prefix,
#     such as APICALL on Windows.
#   apientry - string to use for the calling convention macro,
#     in typedefs, such as APIENTRY.
#   apientryp - string to use for the calling convention macro
#     in function pointer typedefs, such as APIENTRYP.
#   indentFuncProto - True if prototype declarations should put each
#     parameter on a separate line
#   indentFuncPointer - True if typedefed function pointers should put each
#     parameter on a separate line
#   alignFuncParam - if nonzero and parameters are being put on a
#     separate line, align parameter names at the specified column
class ObjectTrackerGeneratorOptions(GeneratorOptions):
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
                 indentFuncProto = True,
                 indentFuncPointer = False,
                 alignFuncParam = 0):
        GeneratorOptions.__init__(self, filename, directory, apiname, profile,
                                  versions, emitversions, defaultExtensions,
                                  addExtensions, removeExtensions, sortProcedure)
        self.prefixText      = prefixText
        self.genFuncPointers = genFuncPointers
        self.protectFile     = protectFile
        self.protectFeature  = protectFeature
        self.protectProto    = protectProto
        self.protectProtoStr = protectProtoStr
        self.apicall         = apicall
        self.apientry        = apientry
        self.apientryp       = apientryp
        self.indentFuncProto = indentFuncProto
        self.indentFuncPointer = indentFuncPointer
        self.alignFuncParam  = alignFuncParam

# ObjectTrackerOutputGenerator - subclass of OutputGenerator.
# Generates object_tracker layer object validation code
#
# ---- methods ----
# ObjectTrackerOutputGenerator(errFile, warnFile, diagFile) - args as for OutputGenerator. Defines additional internal state.
# ---- methods overriding base class ----
# beginFile(genOpts)
# endFile()
# beginFeature(interface, emit)
# endFeature()
# genCmd(cmdinfo)
# genStruct()
# genType()
class ObjectTrackerOutputGenerator(OutputGenerator):
    """Generate ObjectTracker code based on XML element attributes"""
    # This is an ordered list of sections in the header file.
    ALL_SECTIONS = ['command']
    def __init__(self,
                 errFile = sys.stderr,
                 warnFile = sys.stderr,
                 diagFile = sys.stdout):
        OutputGenerator.__init__(self, errFile, warnFile, diagFile)
        self.INDENT_SPACES = 4
        self.intercepts = []
        self.instance_extensions = []
        self.device_extensions = []
        # Commands which are not autogenerated but still intercepted
        self.no_autogen_list = [
            'vkDestroyInstance',
            'vkDestroyDevice',
            'vkUpdateDescriptorSets',
            'vkDestroyDebugReportCallbackEXT',
            'vkDebugReportMessageEXT',
            'vkGetPhysicalDeviceQueueFamilyProperties',
            'vkFreeCommandBuffers',
            'vkDestroySwapchainKHR',
            'vkDestroyDescriptorPool',
            'vkDestroyCommandPool',
            'vkGetPhysicalDeviceQueueFamilyProperties2KHR',
            'vkResetDescriptorPool',
            'vkBeginCommandBuffer',
            'vkCreateDebugReportCallbackEXT',
            'vkEnumerateInstanceLayerProperties',
            'vkEnumerateDeviceLayerProperties',
            'vkEnumerateInstanceExtensionProperties',
            'vkEnumerateDeviceExtensionProperties',
            'vkCreateDevice',
            'vkCreateInstance',
            'vkEnumeratePhysicalDevices',
            'vkAllocateCommandBuffers',
            'vkAllocateDescriptorSets',
            'vkFreeDescriptorSets',
            'vkCmdPushDescriptorSetKHR',
            'vkDebugMarkerSetObjectNameEXT',
            'vkGetPhysicalDeviceProcAddr',
            'vkGetDeviceProcAddr',
            'vkGetInstanceProcAddr',
            'vkEnumerateInstanceExtensionProperties',
            'vkEnumerateInstanceLayerProperties',
            'vkEnumerateDeviceLayerProperties',
            'vkGetDeviceProcAddr',
            'vkGetInstanceProcAddr',
            'vkEnumerateDeviceExtensionProperties',
            'vk_layerGetPhysicalDeviceProcAddr',
            'vkNegotiateLoaderLayerInterfaceVersion',
            'vkCreateComputePipelines',
            'vkGetDeviceQueue',
            'vkGetSwapchainImagesKHR',
            'vkCreateDescriptorSetLayout',
            ]
        # These VUIDS are not implicit, but are best handled in this layer. Codegen for vkDestroy calls will generate a key
        # which is translated here into a good VU.  Saves ~40 checks.
        self.manual_vuids = dict()
        self.manual_vuids = {
            "fence-compatalloc": "VALIDATION_ERROR_24e008c2",
            "fence-nullalloc": "VALIDATION_ERROR_24e008c4",
            "event-compatalloc": "VALIDATION_ERROR_24c008f4",
            "event-nullalloc": "VALIDATION_ERROR_24c008f6",
            "buffer-compatalloc": "VALIDATION_ERROR_23c00736",
            "buffer-nullalloc": "VALIDATION_ERROR_23c00738",
            "image-compatalloc": "VALIDATION_ERROR_252007d2",
            "image-nullalloc": "VALIDATION_ERROR_252007d4",
            "shaderModule-compatalloc": "VALIDATION_ERROR_26a00888",
            "shaderModule-nullalloc": "VALIDATION_ERROR_26a0088a",
            "pipeline-compatalloc": "VALIDATION_ERROR_25c005fc",
            "pipeline-nullalloc": "VALIDATION_ERROR_25c005fe",
            "sampler-compatalloc": "VALIDATION_ERROR_26600876",
            "sampler-nullalloc": "VALIDATION_ERROR_26600878",
            "renderPass-compatalloc": "VALIDATION_ERROR_264006d4",
            "renderPass-nullalloc": "VALIDATION_ERROR_264006d6",
            "descriptorUpdateTemplate-compatalloc": "VALIDATION_ERROR_248002c8",
            "descriptorUpdateTemplate-nullalloc": "VALIDATION_ERROR_248002ca",
            "imageView-compatalloc": "VALIDATION_ERROR_25400806",
            "imageView-nullalloc": "VALIDATION_ERROR_25400808",
            "pipelineCache-compatalloc": "VALIDATION_ERROR_25e00606",
            "pipelineCache-nullalloc": "VALIDATION_ERROR_25e00608",
            "pipelineLayout-compatalloc": "VALIDATION_ERROR_26000256",
            "pipelineLayout-nullalloc": "VALIDATION_ERROR_26000258",
            "descriptorSetLayout-compatalloc": "VALIDATION_ERROR_24600238",
            "descriptorSetLayout-nullalloc": "VALIDATION_ERROR_2460023a",
            "semaphore-compatalloc": "VALIDATION_ERROR_268008e4",
            "semaphore-nullalloc": "VALIDATION_ERROR_268008e6",
            "queryPool-compatalloc": "VALIDATION_ERROR_26200634",
            "queryPool-nullalloc": "VALIDATION_ERROR_26200636",
            "bufferView-compatalloc": "VALIDATION_ERROR_23e00752",
            "bufferView-nullalloc": "VALIDATION_ERROR_23e00754",
            "surface-compatalloc": "VALIDATION_ERROR_26c009e6",
            "surface-nullalloc": "VALIDATION_ERROR_26c009e8",
            "framebuffer-compatalloc": "VALIDATION_ERROR_250006fa",
            "framebuffer-nullalloc": "VALIDATION_ERROR_250006fc",
           }

        # Commands shadowed by interface functions and are not implemented
        self.interface_functions = [
            ]
        self.headerVersion = None
        # Internal state - accumulators for different inner block text
        self.sections = dict([(section, []) for section in self.ALL_SECTIONS])
        self.cmdMembers = []
        self.cmd_feature_protect = []  # Save ifdef's for each command
        self.cmd_info_data = []        # Save the cmdinfo data for validating the handles when processing is complete
        self.structMembers = []        # List of StructMemberData records for all Vulkan structs
        self.extension_structs = []    # List of all structs or sister-structs containing handles
                                       # A sister-struct may contain no handles but shares <validextensionstructs> with one that does
        self.structTypes = dict()      # Map of Vulkan struct typename to required VkStructureType
        self.struct_member_dict = dict()
        # Named tuples to store struct and command data
        self.StructType = namedtuple('StructType', ['name', 'value'])
        self.CmdMemberData = namedtuple('CmdMemberData', ['name', 'members'])
        self.CmdInfoData = namedtuple('CmdInfoData', ['name', 'cmdinfo'])
        self.CmdExtraProtect = namedtuple('CmdExtraProtect', ['name', 'extra_protect'])
        self.CommandParam = namedtuple('CommandParam', ['type', 'name', 'ispointer', 'isconst', 'isoptional', 'iscount', 'len', 'extstructs', 'cdecl', 'islocal', 'iscreate', 'isdestroy', 'feature_protect'])
        self.StructMemberData = namedtuple('StructMemberData', ['name', 'members'])
        self.object_types = []         # List of all handle types
        self.valid_vuids = set()       # Set of all valid VUIDs
        self.vuid_file = None
        # Cover cases where file is built from scripts directory, Lin/Win, or Android build structure
        # Set cwd to the script directory to more easily locate the header.
        previous_dir = os.getcwd()
        os.chdir(os.path.dirname(sys.argv[0]))
        vuid_filename_locations = [
            './vk_validation_error_messages.h',
            '../layers/vk_validation_error_messages.h',
            '../../layers/vk_validation_error_messages.h',
            '../../../layers/vk_validation_error_messages.h',
            ]
        for vuid_filename in vuid_filename_locations:
            if os.path.isfile(vuid_filename):
                self.vuid_file = open(vuid_filename, "r", encoding="utf8")
                break
        if self.vuid_file == None:
            print("Error: Could not find vk_validation_error_messages.h")
            sys.exit(1)
        os.chdir(previous_dir)
    #
    # Check if the parameter passed in is optional
    def paramIsOptional(self, param):
        # See if the handle is optional
        isoptional = False
        # Simple, if it's optional, return true
        optString = param.attrib.get('optional')
        if optString:
            if optString == 'true':
                isoptional = True
            elif ',' in optString:
                opts = []
                for opt in optString.split(','):
                    val = opt.strip()
                    if val == 'true':
                        opts.append(True)
                    elif val == 'false':
                        opts.append(False)
                    else:
                        print('Unrecognized len attribute value',val)
                isoptional = opts
        return isoptional
    #
    # Convert decimal number to 8 digit hexadecimal lower-case representation
    def IdToHex(self, dec_num):
        if dec_num > 4294967295:
            print ("ERROR: Decimal # %d can't be represented in 8 hex digits" % (dec_num))
            sys.exit()
        hex_num = hex(dec_num)
        return hex_num[2:].zfill(8)
    #
    # Get VUID identifier from implicit VUID tag
    def GetVuid(self, vuid_string):
        if '->' in vuid_string:
           return "VALIDATION_ERROR_UNDEFINED"
        vuid_num = self.IdToHex(convertVUID(vuid_string))
        if vuid_num in self.valid_vuids:
            vuid = "VALIDATION_ERROR_%s" % vuid_num
        else:
            vuid = "VALIDATION_ERROR_UNDEFINED"
        return vuid
    #
    # Increases indent by 4 spaces and tracks it globally
    def incIndent(self, indent):
        inc = ' ' * self.INDENT_SPACES
        if indent:
            return indent + inc
        return inc
    #
    # Decreases indent by 4 spaces and tracks it globally
    def decIndent(self, indent):
        if indent and (len(indent) > self.INDENT_SPACES):
            return indent[:-self.INDENT_SPACES]
        return ''
    #
    # Override makeProtoName to drop the "vk" prefix
    def makeProtoName(self, name, tail):
        return self.genOpts.apientry + name[2:] + tail
    #
    # Check if the parameter passed in is a pointer to an array
    def paramIsArray(self, param):
        return param.attrib.get('len') is not None
    #
    # Generate the object tracker undestroyed object validation function
    def GenReportFunc(self):
        output_func = ''
        output_func += 'void ReportUndestroyedObjects(VkDevice device, enum UNIQUE_VALIDATION_ERROR_CODE error_code) {\n'
        output_func += '    DeviceReportUndestroyedObjects(device, kVulkanObjectTypeCommandBuffer, error_code);\n'
        for handle in self.object_types:
            if self.isHandleTypeNonDispatchable(handle):
                output_func += '    DeviceReportUndestroyedObjects(device, %s, error_code);\n' % (self.GetVulkanObjType(handle))
        output_func += '}\n'
        return output_func
    #
    # Called at beginning of processing as file is opened
    def beginFile(self, genOpts):
        OutputGenerator.beginFile(self, genOpts)
        # Open vk_validation_error_messages.h file to verify computed VUIDs
        for line in self.vuid_file:
            # Grab hex number from enum definition
            vuid_list = line.split('0x')
            # If this is a valid enumeration line, remove trailing comma and CR
            if len(vuid_list) == 2:
                vuid_num = vuid_list[1][:-2]
                # Make sure this is a good hex number before adding to set
                if len(vuid_num) == 8 and all(c in string.hexdigits for c in vuid_num):
                    self.valid_vuids.add(vuid_num)
        # File Comment
        file_comment = '// *** THIS FILE IS GENERATED - DO NOT EDIT ***\n'
        file_comment += '// See object_tracker_generator.py for modifications\n'
        write(file_comment, file=self.outFile)
        # Copyright Statement
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
        copyright += ' *\n'
        copyright += ' ****************************************************************************/\n'
        write(copyright, file=self.outFile)
        # Namespace
        self.newline()
        write('#include "object_tracker.h"', file = self.outFile)
        self.newline()
        write('namespace object_tracker {', file = self.outFile)
    #
    # Now that the data is all collected and complete, generate and output the object validation routines
    def endFile(self):
        self.struct_member_dict = dict(self.structMembers)
        # Generate the list of APIs that might need to handle wrapped extension structs
        # self.GenerateCommandWrapExtensionList()
        self.WrapCommands()
        # Build undestroyed objects reporting function
        report_func = self.GenReportFunc()
        self.newline()
        write('// ObjectTracker undestroyed objects validation function', file=self.outFile)
        write('%s' % report_func, file=self.outFile)
        # Actually write the interface to the output file.
        if (self.emit):
            self.newline()
            if (self.featureExtraProtect != None):
                write('#ifdef', self.featureExtraProtect, file=self.outFile)
            # Write the object_tracker code to the file
            if (self.sections['command']):
                if (self.genOpts.protectProto):
                    write(self.genOpts.protectProto,
                          self.genOpts.protectProtoStr, file=self.outFile)
                write('\n'.join(self.sections['command']), end=u'', file=self.outFile)
            if (self.featureExtraProtect != None):
                write('\n#endif //', self.featureExtraProtect, file=self.outFile)
            else:
                self.newline()

        # Record intercepted procedures
        write('// Map of all APIs to be intercepted by this layer', file=self.outFile)
        write('const std::unordered_map<std::string, void*> name_to_funcptr_map = {', file=self.outFile)
        write('\n'.join(self.intercepts), file=self.outFile)
        write('};\n', file=self.outFile)
        self.newline()
        write('} // namespace object_tracker', file=self.outFile)
        # Finish processing in superclass
        OutputGenerator.endFile(self)
    #
    # Processing point at beginning of each extension definition
    def beginFeature(self, interface, emit):
        # Start processing in superclass
        OutputGenerator.beginFeature(self, interface, emit)
        self.headerVersion = None

        if self.featureName != 'VK_VERSION_1_0':
            white_list_entry = []
            if (self.featureExtraProtect != None):
                white_list_entry += [ '#ifdef %s' % self.featureExtraProtect ]
            white_list_entry += [ '"%s"' % self.featureName ]
            if (self.featureExtraProtect != None):
                white_list_entry += [ '#endif' ]
            featureType = interface.get('type')
            if featureType == 'instance':
                self.instance_extensions += white_list_entry
            elif featureType == 'device':
                self.device_extensions += white_list_entry
    #
    # Processing point at end of each extension definition
    def endFeature(self):
        # Finish processing in superclass
        OutputGenerator.endFeature(self)
    #
    # Process enums, structs, etc.
    def genType(self, typeinfo, name):
        OutputGenerator.genType(self, typeinfo, name)
        typeElem = typeinfo.elem
        # If the type is a struct type, traverse the imbedded <member> tags generating a structure.
        # Otherwise, emit the tag text.
        category = typeElem.get('category')
        if (category == 'struct' or category == 'union'):
            self.genStruct(typeinfo, name)
        if category == 'handle':
            self.object_types.append(name)
    #
    # Append a definition to the specified section
    def appendSection(self, section, text):
        # self.sections[section].append('SECTION: ' + section + '\n')
        self.sections[section].append(text)
    #
    # Check if the parameter passed in is a pointer
    def paramIsPointer(self, param):
        ispointer = False
        for elem in param:
            if ((elem.tag is not 'type') and (elem.tail is not None)) and '*' in elem.tail:
                ispointer = True
        return ispointer
    #
    # Get the category of a type
    def getTypeCategory(self, typename):
        types = self.registry.tree.findall("types/type")
        for elem in types:
            if (elem.find("name") is not None and elem.find('name').text == typename) or elem.attrib.get('name') == typename:
                return elem.attrib.get('category')
    #
    # Check if a parent object is dispatchable or not
    def isHandleTypeObject(self, handletype):
        handle = self.registry.tree.find("types/type/[name='" + handletype + "'][@category='handle']")
        if handle is not None:
            return True
        else:
            return False
    #
    # Check if a parent object is dispatchable or not
    def isHandleTypeNonDispatchable(self, handletype):
        handle = self.registry.tree.find("types/type/[name='" + handletype + "'][@category='handle']")
        if handle is not None and handle.find('type').text == 'VK_DEFINE_NON_DISPATCHABLE_HANDLE':
            return True
        else:
            return False
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
            # Spec has now notation for len attributes, using :: instead of platform specific pointer symbol
            result = str(result).replace('::', '->')
        return result
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
    # Struct parameter check generation.
    # This is a special case of the <type> tag where the contents are interpreted as a set of
    # <member> tags instead of freeform C type declarations. The <member> tags are just like
    # <param> tags - they are a declaration of a struct or union member. Only simple member
    # declarations are supported (no nested structs etc.)
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
            cdecl = self.makeCParamDecl(member, 0)
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
            extstructs = member.attrib.get('validextensionstructs') if name == 'pNext' else None
            membersInfo.append(self.CommandParam(type=type,
                                                 name=name,
                                                 ispointer=self.paramIsPointer(member),
                                                 isconst=True if 'const' in cdecl else False,
                                                 isoptional=self.paramIsOptional(member),
                                                 iscount=True if name in lens else False,
                                                 len=self.getLen(member),
                                                 extstructs=extstructs,
                                                 cdecl=cdecl,
                                                 islocal=False,
                                                 iscreate=False,
                                                 isdestroy=False,
                                                 feature_protect=self.featureExtraProtect))
        self.structMembers.append(self.StructMemberData(name=typeName, members=membersInfo))
    #
    # Insert a lock_guard line
    def lock_guard(self, indent):
        return '%sstd::lock_guard<std::mutex> lock(global_lock);\n' % indent
    #
    # Determine if a struct has an object as a member or an embedded member
    def struct_contains_object(self, struct_item):
        struct_member_dict = dict(self.structMembers)
        struct_members = struct_member_dict[struct_item]

        for member in struct_members:
            if self.isHandleTypeObject(member.type):
                return True
            elif member.type in struct_member_dict:
                if self.struct_contains_object(member.type) == True:
                    return True
        return False
    #
    # Return list of struct members which contain, or whose sub-structures contain an obj in a given list of parameters or members
    def getParmeterStructsWithObjects(self, item_list):
        struct_list = set()
        for item in item_list:
            paramtype = item.find('type')
            typecategory = self.getTypeCategory(paramtype.text)
            if typecategory == 'struct':
                if self.struct_contains_object(paramtype.text) == True:
                    struct_list.add(item)
        return struct_list
    #
    # Return list of objects from a given list of parameters or members
    def getObjectsInParameterList(self, item_list, create_func):
        object_list = set()
        if create_func == True:
            member_list = item_list[0:-1]
        else:
            member_list = item_list
        for item in member_list:
            if self.isHandleTypeObject(paramtype.text):
                object_list.add(item)
        return object_list
    #
    # Construct list of extension structs containing handles, or extension structs that share a <validextensionstructs>
    # tag WITH an extension struct containing handles. 
    def GenerateCommandWrapExtensionList(self):
        for struct in self.structMembers:
            if (len(struct.members) > 1) and struct.members[1].extstructs is not None:
                found = False;
                for item in struct.members[1].extstructs.split(','):
                    if item != '' and self.struct_contains_object(item) == True:
                        found = True
                if found == True:
                    for item in struct.members[1].extstructs.split(','):
                        if item != '' and item not in self.extension_structs:
                            self.extension_structs.append(item)
    #
    # Returns True if a struct may have a pNext chain containing an object
    def StructWithExtensions(self, struct_type):
        if struct_type in self.struct_member_dict:
            param_info = self.struct_member_dict[struct_type]
            if (len(param_info) > 1) and param_info[1].extstructs is not None:
                for item in param_info[1].extstructs.split(','):
                    if item in self.extension_structs:
                        return True
        return False
    #
    # Generate VulkanObjectType from object type
    def GetVulkanObjType(self, type):
        return 'kVulkanObjectType%s' % type[2:]
    #
    # Return correct dispatch table type -- instance or device
    def GetDispType(self, type):
        return 'instance' if type in ['VkInstance', 'VkPhysicalDevice'] else 'device'
    #
    # Generate source for creating a Vulkan object
    def generate_create_object_code(self, indent, proto, params, cmd_info):
        create_obj_code = ''
        handle_type = params[-1].find('type')
        if self.isHandleTypeObject(handle_type.text):
            # Check for special case where multiple handles are returned
            object_array = False
            if cmd_info[-1].len is not None:
                object_array = True;
            handle_name = params[-1].find('name')
            create_obj_code += '%sif (VK_SUCCESS == result) {\n' % (indent)
            indent = self.incIndent(indent)
            create_obj_code += '%sstd::lock_guard<std::mutex> lock(global_lock);\n' % (indent)
            object_dest = '*%s' % handle_name.text
            if object_array == True:
                create_obj_code += '%sfor (uint32_t index = 0; index < %s; index++) {\n' % (indent, cmd_info[-1].len)
                indent = self.incIndent(indent)
                object_dest = '%s[index]' % cmd_info[-1].name
            create_obj_code += '%sCreateObject(%s, %s, %s, pAllocator);\n' % (indent, params[0].find('name').text, object_dest, self.GetVulkanObjType(cmd_info[-1].type))
            if object_array == True:
                indent = self.decIndent(indent)
                create_obj_code += '%s}\n' % indent
            indent = self.decIndent(indent)
            create_obj_code += '%s}\n' % (indent)
        return create_obj_code
    #
    # Generate source for destroying a non-dispatchable object
    def generate_destroy_object_code(self, indent, proto, cmd_info):
        destroy_obj_code = ''
        object_array = False
        if True in [destroy_txt in proto.text for destroy_txt in ['Destroy', 'Free']]:
            # Check for special case where multiple handles are returned
            if cmd_info[-1].len is not None:
                object_array = True;
                param = -1
            else:
                param = -2
            compatalloc_vuid_string = '%s-compatalloc' % cmd_info[param].name
            nullalloc_vuid_string = '%s-nullalloc' % cmd_info[param].name
            compatalloc_vuid = self.manual_vuids.get(compatalloc_vuid_string, "VALIDATION_ERROR_UNDEFINED")
            nullalloc_vuid = self.manual_vuids.get(nullalloc_vuid_string, "VALIDATION_ERROR_UNDEFINED")
            if self.isHandleTypeObject(cmd_info[param].type) == True:
                if object_array == True:
                    # This API is freeing an array of handles -- add loop control
                    destroy_obj_code += 'HEY, NEED TO DESTROY AN ARRAY\n'
                else:
                    # Call Destroy a single time
                    destroy_obj_code += '%sif (skip) return;\n' % indent
                    destroy_obj_code += '%s{\n' % indent
                    destroy_obj_code += '%s    std::lock_guard<std::mutex> lock(global_lock);\n' % indent
                    destroy_obj_code += '%s    DestroyObject(%s, %s, %s, pAllocator, %s, %s);\n' % (indent, cmd_info[0].name, cmd_info[param].name, self.GetVulkanObjType(cmd_info[param].type), compatalloc_vuid, nullalloc_vuid)
                    destroy_obj_code += '%s}\n' % indent
        return object_array, destroy_obj_code
    #
    # Output validation for a single object (obj_count is NULL) or a counted list of objects
    def outputObjects(self, obj_type, obj_name, obj_count, prefix, index, indent, destroy_func, destroy_array, disp_name, parent_name, null_allowed, top_level):
        decl_code = ''
        pre_call_code = ''
        post_call_code = ''
        param_vuid_string = 'VUID-%s-%s-parameter' % (parent_name, obj_name)
        parent_vuid_string = 'VUID-%s-%s-parent' % (parent_name, obj_name)
        param_vuid = self.GetVuid(param_vuid_string)
        parent_vuid = self.GetVuid(parent_vuid_string)
        # If no parent VUID for this member, look for a commonparent VUID
        if parent_vuid == 'VALIDATION_ERROR_UNDEFINED':
            commonparent_vuid_string = 'VUID-%s-commonparent' % parent_name
            parent_vuid = self.GetVuid(commonparent_vuid_string)
        if obj_count is not None:
            pre_call_code += '%s    if (%s%s) {\n' % (indent, prefix, obj_name)
            indent = self.incIndent(indent)
            pre_call_code += '%s    for (uint32_t %s = 0; %s < %s; ++%s) {\n' % (indent, index, index, obj_count, index)
            indent = self.incIndent(indent)
            pre_call_code += '%s    skip |= ValidateObject(%s, %s%s[%s], %s, %s, %s, %s);\n' % (indent, disp_name, prefix, obj_name, index, self.GetVulkanObjType(obj_type), null_allowed, param_vuid, parent_vuid)
            indent = self.decIndent(indent)
            pre_call_code += '%s    }\n' % indent
            indent = self.decIndent(indent)
            pre_call_code += '%s    }\n' % indent
        else:
            pre_call_code += '%s    skip |= ValidateObject(%s, %s%s, %s, %s, %s, %s);\n' % (indent, disp_name, prefix, obj_name, self.GetVulkanObjType(obj_type), null_allowed, param_vuid, parent_vuid)
        return decl_code, pre_call_code, post_call_code
    #
    # first_level_param indicates if elements are passed directly into the function else they're below a ptr/struct
    # create_func means that this is API creates or allocates objects
    # destroy_func indicates that this API destroys or frees objects
    # destroy_array means that the destroy_func operated on an array of objects
    def validate_objects(self, members, indent, prefix, array_index, create_func, destroy_func, destroy_array, disp_name, parent_name, first_level_param):
        decls = ''
        pre_code = ''
        post_code = ''
        index = 'index%s' % str(array_index)
        array_index += 1
        # Process any objects in this structure and recurse for any sub-structs in this struct
        for member in members:
            # Handle objects
            if member.iscreate and first_level_param and member == members[-1]:
                continue
            if self.isHandleTypeObject(member.type) == True:
                count_name = member.len
                if (count_name is not None):
                    count_name = '%s%s' % (prefix, member.len)
                null_allowed = member.isoptional
                (tmp_decl, tmp_pre, tmp_post) = self.outputObjects(member.type, member.name, count_name, prefix, index, indent, destroy_func, destroy_array, disp_name, parent_name, str(null_allowed).lower(), first_level_param)
                decls += tmp_decl
                pre_code += tmp_pre
                post_code += tmp_post
            # Handle Structs that contain objects at some level
            elif member.type in self.struct_member_dict:
                # Structs at first level will have an object
                if self.struct_contains_object(member.type) == True:
                    struct_info = self.struct_member_dict[member.type]
                    # Struct Array
                    if member.len is not None:
                        # Update struct prefix
                        new_prefix = '%s%s' % (prefix, member.name)
                        pre_code += '%s    if (%s%s) {\n' % (indent, prefix, member.name)
                        indent = self.incIndent(indent)
                        pre_code += '%s    for (uint32_t %s = 0; %s < %s%s; ++%s) {\n' % (indent, index, index, prefix, member.len, index)
                        indent = self.incIndent(indent)
                        local_prefix = '%s[%s].' % (new_prefix, index)
                        # Process sub-structs in this struct
                        (tmp_decl, tmp_pre, tmp_post) = self.validate_objects(struct_info, indent, local_prefix, array_index, create_func, destroy_func, destroy_array, disp_name, member.type, False)
                        decls += tmp_decl
                        pre_code += tmp_pre
                        post_code += tmp_post
                        indent = self.decIndent(indent)
                        pre_code += '%s    }\n' % indent
                        indent = self.decIndent(indent)
                        pre_code += '%s    }\n' % indent
                    # Single Struct
                    else:
                        # Update struct prefix
                        new_prefix = '%s%s->' % (prefix, member.name)
                        # Declare safe_VarType for struct
                        pre_code += '%s    if (%s%s) {\n' % (indent, prefix, member.name)
                        indent = self.incIndent(indent)
                        # Process sub-structs in this struct
                        (tmp_decl, tmp_pre, tmp_post) = self.validate_objects(struct_info, indent, new_prefix, array_index, create_func, destroy_func, destroy_array, disp_name, member.type, False)
                        decls += tmp_decl
                        pre_code += tmp_pre
                        post_code += tmp_post
                        indent = self.decIndent(indent)
                        pre_code += '%s    }\n' % indent
        return decls, pre_code, post_code
    #
    # For a particular API, generate the object handling code
    def generate_wrapping_code(self, cmd):
        indent = '    '
        proto = cmd.find('proto/name')
        params = cmd.findall('param')
        if proto.text is not None:
            cmd_member_dict = dict(self.cmdMembers)
            cmd_info = cmd_member_dict[proto.text]
            disp_name = cmd_info[0].name
            # Handle object create operations
            if cmd_info[0].iscreate:
                create_obj_code = self.generate_create_object_code(indent, proto, params, cmd_info)
            else:
                create_obj_code = ''
            # Handle object destroy operations
            if cmd_info[0].isdestroy:
                (destroy_array, destroy_object_code) = self.generate_destroy_object_code(indent, proto, cmd_info)
            else:
                destroy_array = False
                destroy_object_code = ''
            paramdecl = ''
            param_pre_code = ''
            param_post_code = ''
            create_func = True if create_obj_code else False
            destroy_func = True if destroy_object_code else False
            (paramdecl, param_pre_code, param_post_code) = self.validate_objects(cmd_info, indent, '', 0, create_func, destroy_func, destroy_array, disp_name, proto.text, True)
            param_post_code += create_obj_code
            if destroy_object_code:
                if destroy_array == True:
                    param_post_code += destroy_object_code
                else:
                    param_pre_code += destroy_object_code
            if param_pre_code:
                if (not destroy_func) or (destroy_array):
                    param_pre_code = '%s{\n%s%s%s%s}\n' % ('    ', indent, self.lock_guard(indent), param_pre_code, indent)
        return paramdecl, param_pre_code, param_post_code
    #
    # Capture command parameter info needed to create, destroy, and validate objects
    def genCmd(self, cmdinfo, cmdname):

        # Add struct-member type information to command parameter information
        OutputGenerator.genCmd(self, cmdinfo, cmdname)
        members = cmdinfo.elem.findall('.//param')
        # Iterate over members once to get length parameters for arrays
        lens = set()
        for member in members:
            len = self.getLen(member)
            if len:
                lens.add(len)
        struct_member_dict = dict(self.structMembers)
        # Generate member info
        membersInfo = []
        constains_extension_structs = False
        for member in members:
            # Get type and name of member
            info = self.getTypeNameTuple(member)
            type = info[0]
            name = info[1]
            cdecl = self.makeCParamDecl(member, 0)
            # Check for parameter name in lens set
            iscount = True if name in lens else False
            len = self.getLen(member)
            isconst = True if 'const' in cdecl else False
            ispointer = self.paramIsPointer(member)
            # Mark param as local if it is an array of objects
            islocal = False;
            if self.isHandleTypeObject(type) == True:
                if (len is not None) and (isconst == True):
                    islocal = True
            # Or if it's a struct that contains an object
            elif type in struct_member_dict:
                if self.struct_contains_object(type) == True:
                    islocal = True
            isdestroy = True if True in [destroy_txt in cmdname for destroy_txt in ['Destroy', 'Free']] else False
            iscreate = True if True in [create_txt in cmdname for create_txt in ['Create', 'Allocate', 'Enumerate', 'RegisterDeviceEvent', 'RegisterDisplayEvent']] or ('vkGet' in cmdname and member == members[-1] and ispointer == True)  else False
            extstructs = member.attrib.get('validextensionstructs') if name == 'pNext' else None
            membersInfo.append(self.CommandParam(type=type,
                                                 name=name,
                                                 ispointer=ispointer,
                                                 isconst=isconst,
                                                 isoptional=self.paramIsOptional(member),
                                                 iscount=iscount,
                                                 len=len,
                                                 extstructs=extstructs,
                                                 cdecl=cdecl,
                                                 islocal=islocal,
                                                 iscreate=iscreate,
                                                 isdestroy=isdestroy,
                                                 feature_protect=self.featureExtraProtect))
        self.cmdMembers.append(self.CmdMemberData(name=cmdname, members=membersInfo))
        self.cmd_info_data.append(self.CmdInfoData(name=cmdname, cmdinfo=cmdinfo))
        self.cmd_feature_protect.append(self.CmdExtraProtect(name=cmdname, extra_protect=self.featureExtraProtect))
    #
    # Create code Create, Destroy, and validate Vulkan objects
    def WrapCommands(self):
        cmd_member_dict = dict(self.cmdMembers)
        cmd_info_dict = dict(self.cmd_info_data)
        cmd_protect_dict = dict(self.cmd_feature_protect)
        for api_call in self.cmdMembers:
            cmdname = api_call.name
            cmdinfo = cmd_info_dict[api_call.name]
            if cmdname in self.interface_functions:
                continue
            if cmdname in self.no_autogen_list:
                decls = self.makeCDecls(cmdinfo.elem)
                self.appendSection('command', '')
                self.appendSection('command', '// Declare only')
                self.appendSection('command', decls[0])
                self.intercepts += [ '    {"%s", (void *)%s},' % (cmdname,cmdname[2:]) ]
                continue
            # Generate object handling code
            (api_decls, api_pre, api_post) = self.generate_wrapping_code(cmdinfo.elem)
            # If API doesn't contain any object handles, don't fool with it
            if not api_decls and not api_pre and not api_post:
                continue
            feature_extra_protect = cmd_protect_dict[api_call.name]
            if (feature_extra_protect != None):
                self.appendSection('command', '')
                self.appendSection('command', '#ifdef '+ feature_extra_protect)
                self.intercepts += [ '#ifdef %s' % feature_extra_protect ]
            # Add intercept to procmap
            self.intercepts += [ '    {"%s", (void*)%s},' % (cmdname,cmdname[2:]) ]
            decls = self.makeCDecls(cmdinfo.elem)
            self.appendSection('command', '')
            self.appendSection('command', decls[0][:-1])
            self.appendSection('command', '{')
            self.appendSection('command', '    bool skip = false;')
            # Handle return values, if any
            resulttype = cmdinfo.elem.find('proto/type')
            if (resulttype != None and resulttype.text == 'void'):
              resulttype = None
            if (resulttype != None):
                assignresult = resulttype.text + ' result = '
            else:
                assignresult = ''
            # Pre-pend declarations and pre-api-call codegen
            if api_decls:
                self.appendSection('command', "\n".join(str(api_decls).rstrip().split("\n")))
            if api_pre:
                self.appendSection('command', "\n".join(str(api_pre).rstrip().split("\n")))
            # Generate the API call itself
            # Gather the parameter items
            params = cmdinfo.elem.findall('param/name')
            # Pull out the text for each of the parameters, separate them by commas in a list
            paramstext = ', '.join([str(param.text) for param in params])
            # Use correct dispatch table
            disp_type = cmdinfo.elem.find('param/type').text
            disp_name = cmdinfo.elem.find('param/name').text
            dispatch_table = 'get_dispatch_table(ot_%s_table_map, %s)->' % (self.GetDispType(disp_type), disp_name)
            API = cmdinfo.elem.attrib.get('name').replace('vk', dispatch_table, 1)
            # Put all this together for the final down-chain call
            if assignresult != '':
                if resulttype.text == 'VkResult':
                    self.appendSection('command', '    if (skip) return VK_ERROR_VALIDATION_FAILED_EXT;')
                elif resulttype.text == 'VkBool32':
                    self.appendSection('command', '    if (skip) return VK_FALSE;')
                else:
                    raise Exception('Unknown result type ' + resulttype.text)
            else:
                self.appendSection('command', '    if (skip) return;')
            self.appendSection('command', '    ' + assignresult + API + '(' + paramstext + ');')
            # And add the post-API-call codegen
            self.appendSection('command', "\n".join(str(api_post).rstrip().split("\n")))
            # Handle the return result variable, if any
            if (resulttype != None):
                self.appendSection('command', '    return result;')
            self.appendSection('command', '}')
            if (feature_extra_protect != None):
                self.appendSection('command', '#endif // '+ feature_extra_protect)
                self.intercepts += [ '#endif' ]
