#!/usr/bin/env python3
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
# Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
# Author: Tobin Ehlis <tobin@lunarg.com>

import argparse
import os
import sys
import re
import vulkan
from source_line_info import sourcelineinfo

# vk_helper.py overview
# This script generates code based on vulkan input header
#  It generate wrappers functions that can be used to display
#  structs in a human-readable txt format, as well as utility functions
#  to print enum values as strings

def handle_args():
    parser = argparse.ArgumentParser(description='Perform analysis of vogl trace.')
    parser.add_argument('input_file', help='The input header file from which code will be generated.')
    parser.add_argument('--rel_out_dir', required=False, default='vktrace_gen', help='Path relative to exec path to write output files. Will be created if needed.')
    parser.add_argument('--abs_out_dir', required=False, default=None, help='Absolute path to write output files. Will be created if needed.')
    parser.add_argument('--gen_struct_wrappers', required=False, action='store_true', default=False, help='Enable generation of struct wrapper classes.')
    parser.add_argument('--quiet', required=False, action='store_true', default=False, help='Suppress output from running the script.')
    #parser.add_argument('--test', action='store_true', default=False, help='Run simple test.')
    return parser.parse_args()

# TODO : Ideally these data structs would be opaque to user and could be wrapped
#   in their own class(es) to make them friendly for data look-up
# Dicts for Data storage
# enum_val_dict[value_name] = dict keys are the string enum value names for all enums
#    |-------->['type'] = the type of enum class into which the value falls
#    |-------->['val'] = the value assigned to this particular value_name
#    '-------->['unique'] = bool designating if this enum 'val' is unique within this enum 'type'
enum_val_dict = {}
# enum_type_dict['type'] = the type or class of of enum
#  '----->['val_name1', 'val_name2'...] = each type references a list of val_names within this type
enum_type_dict = {}
# struct_dict['struct_basename'] = the base (non-typedef'd) name of the struct
#  |->[<member_num>] = members are stored via their integer placement in the struct
#  |    |->['name'] = string name of this struct member
# ...   |->['full_type'] = complete type qualifier for this member
#       |->['type'] = base type for this member
#       |->['ptr'] = bool indicating if this member is ptr
#       |->['const'] = bool indicating if this member is const
#       |->['struct'] = bool indicating if this member is a struct type
#       |->['array'] = bool indicating if this member is an array
#       |->['dyn_array'] = bool indicating if member is a dynamically sized array
#       '->['array_size'] = For dyn_array, member name used to size array, else int size for static array
struct_dict = {}
struct_order_list = [] # struct names in order they're declared
# Store struct names that require #ifdef guards
#  dict stores struct and enum definitions that are guarded by ifdef as the key
#  and the txt of the ifdef is the value
ifdef_dict = {}
# typedef_fwd_dict stores mapping from orig_type_name -> new_type_name
typedef_fwd_dict = {}
# typedef_rev_dict stores mapping from new_type_name -> orig_type_name
typedef_rev_dict = {} # store new_name -> orig_name mapping
# types_dict['id_name'] = identifier name will map to it's type
#              '---->'type' = currently either 'struct' or 'enum'
types_dict = {}   # store orig_name -> type mapping


# Class that parses header file and generates data structures that can
#  Then be used for other tasks
class HeaderFileParser:
    def __init__(self, header_file=None):
        self.header_file = header_file
        # store header data in various formats, see above for more info
        self.enum_val_dict = {}
        self.enum_type_dict = {}
        self.struct_dict = {}
        self.typedef_fwd_dict = {}
        self.typedef_rev_dict = {}
        self.types_dict = {}
        self.last_struct_count_name = ''

    def setHeaderFile(self, header_file):
        self.header_file = header_file

    def get_enum_val_dict(self):
        return self.enum_val_dict

    def get_enum_type_dict(self):
        return self.enum_type_dict

    def get_struct_dict(self):
        return self.struct_dict

    def get_typedef_fwd_dict(self):
        return self.typedef_fwd_dict

    def get_typedef_rev_dict(self):
        return self.typedef_rev_dict

    def get_types_dict(self):
        return self.types_dict

    # Parse header file into data structures
    def parse(self):
        # parse through the file, identifying different sections
        parse_enum = False
        parse_struct = False
        member_num = 0
        # TODO : Comment parsing is very fragile but handles 2 known files
        block_comment = False
        prev_count_name = ''
        ifdef_txt = ''
        ifdef_active = 0
        exclude_struct_list = ['VkPlatformHandleXcbKHR', 'VkPlatformHandleX11KHR']
        with open(self.header_file) as f:
            for line in f:
                if True in [ifd_txt in line for ifd_txt in ['#ifdef ', '#ifndef ']]:
                    ifdef_txt = line.split()[1]
                    ifdef_active = ifdef_active + 1
                    continue
                if ifdef_active != 0 and '#endif' in line:
                    ifdef_active = ifdef_active - 1
                if block_comment:
                    if '*/' in line:
                        block_comment = False
                    continue
                if '/*' in line:
                    if '*/' in line: # single line block comment
                        continue
                    block_comment = True
                elif 0 == len(line.split()):
                    #print("Skipping empty line")
                    continue
                elif line.split()[0].strip().startswith("//"):
                    #print("Skipping commented line %s" % line)
                    continue
                elif 'typedef enum' in line:
                    (ty_txt, en_txt, base_type) = line.strip().split(None, 2)
                    #print("Found ENUM type %s" % base_type)
                    if '{' == base_type:
                        base_type = 'tmp_enum'
                    parse_enum = True
                    default_enum_val = 0
                    self.types_dict[base_type] = 'enum'
                elif 'typedef struct' in line or 'typedef union' in line:
                    if True in [ex_type in line for ex_type in exclude_struct_list]:
                        continue

                    (ty_txt, st_txt, base_type) = line.strip().split(None, 2)
                    if ' ' in base_type:
                        (ignored, base_type) = base_type.strip().split(None, 1)

                    #print("Found STRUCT type: %s" % base_type)
                    # Note:  This really needs to be updated to handle one line struct definition, like
                    #        typedef struct obj##_T { uint64_t handle; } obj;
                    if ('{' == base_type or not (' ' in base_type)):
                        base_type = 'tmp_struct'
                        parse_struct = True
                        self.types_dict[base_type] = 'struct'
#                elif 'typedef union' in line:
#                    (ty_txt, st_txt, base_type) = line.strip().split(None, 2)
#                    print("Found UNION type: %s" % base_type)
#                    parse_struct = True
#                    self.types_dict[base_type] = 'struct'
                elif '}' in line and (parse_enum or parse_struct):
                    if len(line.split()) > 1: # deals with embedded union in one struct
                        parse_enum = False
                        parse_struct = False
                        self.last_struct_count_name = ''
                        member_num = 0
                        (cur_char, targ_type) = line.strip().split(None, 1)
                        if 'tmp_struct' == base_type:
                            base_type = targ_type.strip(';')
                            if True in [ex_type in base_type for ex_type in exclude_struct_list]:
                                del self.struct_dict['tmp_struct']
                                continue
                            #print("Found Actual Struct type %s" % base_type)
                            self.struct_dict[base_type] = self.struct_dict['tmp_struct']
                            self.struct_dict.pop('tmp_struct', 0)
                            struct_order_list.append(base_type)
                            self.types_dict[base_type] = 'struct'
                            self.types_dict.pop('tmp_struct', 0)
                        elif 'tmp_enum' == base_type:
                            base_type = targ_type.strip(';')
                            #print("Found Actual ENUM type %s" % base_type)
                            for n in self.enum_val_dict:
                                if 'tmp_enum' == self.enum_val_dict[n]['type']:
                                    self.enum_val_dict[n]['type'] = base_type
#                            self.enum_val_dict[base_type] = self.enum_val_dict['tmp_enum']
#                            self.enum_val_dict.pop('tmp_enum', 0)
                            self.enum_type_dict[base_type] = self.enum_type_dict['tmp_enum']
                            self.enum_type_dict.pop('tmp_enum', 0)
                            self.types_dict[base_type] = 'enum'
                            self.types_dict.pop('tmp_enum', 0)
                        if ifdef_active:
                            ifdef_dict[base_type] = ifdef_txt
                        self.typedef_fwd_dict[base_type] = targ_type.strip(';')
                        self.typedef_rev_dict[targ_type.strip(';')] = base_type
                        #print("fwd_dict: %s = %s" % (base_type, targ_type))
                elif parse_enum:
                    #if 'VK_MAX_ENUM' not in line and '{' not in line:
                    if True not in [ens in line for ens in ['{', '_MAX_ENUM', '_BEGIN_RANGE', '_END_RANGE', '_NUM = ', '_ENUM_RANGE']]:
                        self._add_enum(line, base_type, default_enum_val)
                        default_enum_val += 1
                elif parse_struct:
                    if ';' in line:
                        self._add_struct(line, base_type, member_num)
                        member_num = member_num + 1

    # populate enum dicts based on enum lines
    def _add_enum(self, line_txt, enum_type, def_enum_val):
        #print("Parsing enum line %s" % line_txt)
        if '=' in line_txt:
            (enum_name, eq_char, enum_val) = line_txt.split(None, 2)
        else:
            enum_name = line_txt.split(',')[0]
            enum_val = str(def_enum_val)
        self.enum_val_dict[enum_name] = {}
        self.enum_val_dict[enum_name]['type'] = enum_type
        # strip comma and comment, then extra split in case of no comma w/ comments
        enum_val = enum_val.strip().split(',', 1)[0]
        self.enum_val_dict[enum_name]['val'] = enum_val.split()[0]
        # Perform conversion of VK_BIT macro
        if 'VK_BIT' in self.enum_val_dict[enum_name]['val']:
            vk_bit_val = self.enum_val_dict[enum_name]['val']
            bit_shift = int(vk_bit_val[vk_bit_val.find('(')+1:vk_bit_val.find(')')], 0)
            self.enum_val_dict[enum_name]['val'] = str(1 << bit_shift)
        else:
            # account for negative values surrounded by parens
            self.enum_val_dict[enum_name]['val'] = self.enum_val_dict[enum_name]['val'].strip(')').replace('-(', '-')
        # Try to cast to int to determine if enum value is unique
        try:
            #print("ENUM val:", self.enum_val_dict[enum_name]['val'])
            int(self.enum_val_dict[enum_name]['val'], 0)
            self.enum_val_dict[enum_name]['unique'] = True
            #print("ENUM has num value")
        except ValueError:
            self.enum_val_dict[enum_name]['unique'] = False
            #print("ENUM is not a number value")
        # Update enum_type_dict as well
        if not enum_type in self.enum_type_dict:
            self.enum_type_dict[enum_type] = []
        self.enum_type_dict[enum_type].append(enum_name)

    # Return True if struct member is a dynamic array
    # RULES : This is a bit quirky based on the API
    # NOTE : Changes in API spec may cause these rules to change
    #  1. There must be a previous uint var w/ 'count' in the name in the struct
    #  2. Dynam array must have 'const' and '*' qualifiers
    #  3a. Name of dynam array must end in 's' char OR
    #  3b. Name of count var minus 'count' must be contained in name of dynamic array
    def _is_dynamic_array(self, full_type, name):
        negative_exceptions = ['pEnabledFeatures', 'pSampleMask']
        positive_exceptions = ['pWaitDstStageMask']
        if name in negative_exceptions:
            return False
        if name in positive_exceptions:
            return True
        if '' != self.last_struct_count_name:
            if 'const' in full_type and '*' in full_type:
                if name.endswith('s') or self.last_struct_count_name.lower().replace('count', '') in name.lower():
                    return True
                # VkWriteDescriptorSet
                if self.last_struct_count_name == "descriptorCount":
                    return True
        return False

    # populate struct dicts based on struct lines
    # TODO : Handle ":" bitfield, "**" ptr->ptr and "const type*const*"
    def _add_struct(self, line_txt, struct_type, num):
        #print("Parsing struct line %s" % line_txt)
        if '{' == struct_type:
            print("Parsing struct '{' w/ line %s" % line_txt)
        if not struct_type in self.struct_dict:
            self.struct_dict[struct_type] = {}
        members = line_txt.strip().split(';', 1)[0] # first strip semicolon & comments
        # TODO : Handle bitfields more correctly
        members = members.strip().split(':', 1)[0] # strip bitfield element
        (member_type, member_name) = members.rsplit(None, 1)
        # Store counts to help recognize and size dynamic arrays
        # Add special case for pObjectEntryCounts -- though it meets the criteria for a 'count', it should not
        # replace the previously identified (and correct) objectCount.
        # TODO: convert to using vk.xml and avoid parsing the header
        if 'count' in member_name.lower() and 'samplecount' != member_name.lower() and 'uint' in member_type and member_name != "pObjectEntryCounts":
            self.last_struct_count_name = member_name
        self.struct_dict[struct_type][num] = {}
        self.struct_dict[struct_type][num]['full_type'] = member_type
        self.struct_dict[struct_type][num]['dyn_array'] = False
        if '*' in member_type:
            self.struct_dict[struct_type][num]['ptr'] = True
            # TODO : Need more general purpose way here to reduce down to basic type
            member_type = member_type.replace(' const*', '')
            member_type = member_type.strip('*')
        else:
            self.struct_dict[struct_type][num]['ptr'] = False
        if 'const' in member_type:
            self.struct_dict[struct_type][num]['const'] = True
            member_type = member_type.replace('const', '').strip()
        else:
            self.struct_dict[struct_type][num]['const'] = False
        # TODO : There is a bug here where it seems that at the time we do this check,
        #    the data is not in the types or typedef_rev_dict, so we never pass this if check
        if is_type(member_type, 'struct'):
            self.struct_dict[struct_type][num]['struct'] = True
        else:
            self.struct_dict[struct_type][num]['struct'] = False
        self.struct_dict[struct_type][num]['type'] = member_type
        if '[' in member_name:
            (member_name, array_size) = member_name.split('[', 1)
            #if 'char' in member_type:
            #    self.struct_dict[struct_type][num]['array'] = False
            #    self.struct_dict[struct_type][num]['array_size'] = 0
            #    self.struct_dict[struct_type][num]['ptr'] = True
            #else:
            self.struct_dict[struct_type][num]['array'] = True
            self.struct_dict[struct_type][num]['array_size'] = array_size.strip(']')
        elif self._is_dynamic_array(self.struct_dict[struct_type][num]['full_type'], member_name):
            #print("Found dynamic array %s of size %s" % (member_name, self.last_struct_count_name))
            self.struct_dict[struct_type][num]['array'] = True
            self.struct_dict[struct_type][num]['dyn_array'] = True
            self.struct_dict[struct_type][num]['array_size'] = self.last_struct_count_name
        elif not 'array' in self.struct_dict[struct_type][num]:
            self.struct_dict[struct_type][num]['array'] = False
            self.struct_dict[struct_type][num]['array_size'] = 0
        self.struct_dict[struct_type][num]['name'] = member_name

# check if given identifier is of specified type_to_check
def is_type(identifier, type_to_check):
    if identifier in types_dict and type_to_check == types_dict[identifier]:
        return True
    if identifier in typedef_rev_dict:
        new_id = typedef_rev_dict[identifier]
        if new_id in types_dict and type_to_check == types_dict[new_id]:
            return True
    return False

# This is a validation function to verify that we can reproduce the original structs
def recreate_structs():
    for struct_name in struct_dict:
        sys.stdout.write("typedef struct %s\n{\n" % struct_name)
        for mem_num in sorted(struct_dict[struct_name]):
            sys.stdout.write("    ")
            if struct_dict[struct_name][mem_num]['const']:
                sys.stdout.write("const ")
            #if struct_dict[struct_name][mem_num]['struct']:
            #    sys.stdout.write("struct ")
            sys.stdout.write (struct_dict[struct_name][mem_num]['type'])
            if struct_dict[struct_name][mem_num]['ptr']:
                sys.stdout.write("*")
            sys.stdout.write(" ")
            sys.stdout.write(struct_dict[struct_name][mem_num]['name'])
            if struct_dict[struct_name][mem_num]['array']:
                sys.stdout.write("[")
                sys.stdout.write(struct_dict[struct_name][mem_num]['array_size'])
                sys.stdout.write("]")
            sys.stdout.write(";\n")
        sys.stdout.write("} ")
        sys.stdout.write(typedef_fwd_dict[struct_name])
        sys.stdout.write(";\n\n")

#
# TODO: Fix construction of struct name
def get_struct_name_from_struct_type(struct_type):
    # Note: All struct types are now camel-case
    # Debug Report has an inconsistency - so need special case.
    caps_struct_name = struct_type.replace("_STRUCTURE_TYPE", "")
    char_idx = 0
    struct_name = ''
    for char in caps_struct_name:
        if (0 == char_idx) or (caps_struct_name[char_idx-1] == '_'):
            struct_name += caps_struct_name[char_idx]
        elif (caps_struct_name[char_idx] == '_'):
            pass
        else:
            struct_name += caps_struct_name[char_idx].lower()
        char_idx += 1

    # Vendor extension structs ending in vendor TLA need to be uppercase.
    if (caps_struct_name[-2:] == "NV"):
        struct_name = struct_name[:-2] + caps_struct_name[-2:]
    if ((caps_struct_name[-3:] == "AMD") or (caps_struct_name[-3:] == "IMG") or (caps_struct_name[-3:] == "EXT")):
        struct_name = struct_name[:-3] + caps_struct_name[-3:]

    return struct_name

# Emit an ifdef if incoming func matches a platform identifier
def add_platform_wrapper_entry(list, func):
    if (re.match(r'.*Xlib.*', func)):
        list.append("#ifdef VK_USE_PLATFORM_XLIB_KHR")
    if (re.match(r'.*Xcb.*', func)):
        list.append("#ifdef VK_USE_PLATFORM_XCB_KHR")
    if (re.match(r'.*Wayland.*', func)):
        list.append("#ifdef VK_USE_PLATFORM_WAYLAND_KHR")
    if (re.match(r'.*Mir.*', func)):
        list.append("#ifdef VK_USE_PLATFORM_MIR_KHR")
    if (re.match(r'.*Android.*', func)):
        list.append("#ifdef VK_USE_PLATFORM_ANDROID_KHR")
    if (re.match(r'.*Win32.*', func)):
        list.append("#ifdef VK_USE_PLATFORM_WIN32_KHR")

# Emit an endif if incoming func matches a platform identifier
def add_platform_wrapper_exit(list, func):
    if (re.match(r'.*Xlib.*', func)):
        list.append("#endif //VK_USE_PLATFORM_XLIB_KHR")
    if (re.match(r'.*Xcb.*', func)):
        list.append("#endif //VK_USE_PLATFORM_XCB_KHR")
    if (re.match(r'.*Wayland.*', func)):
        list.append("#endif //VK_USE_PLATFORM_WAYLAND_KHR")
    if (re.match(r'.*Mir.*', func)):
        list.append("#endif //VK_USE_PLATFORM_MIR_KHR")
    if (re.match(r'.*Android.*', func)):
        list.append("#endif //VK_USE_PLATFORM_ANDROID_KHR")
    if (re.match(r'.*Win32.*', func)):
        list.append("#endif //VK_USE_PLATFORM_WIN32_KHR")

# class for writing common file elements
# Here's how this class lays out a file:
#  COPYRIGHT
#  HEADER
#  BODY
#  FOOTER
#
# For each of these sections, there's a "set*" function
# The class as a whole has a generate function which will write each section in order
class CommonFileGen:
    def __init__(self, filename=None, copyright_txt="", header_txt="", body_txt="", footer_txt=""):
        self.filename = filename
        self.contents = {'copyright': copyright_txt, 'header': header_txt, 'body': body_txt, 'footer': footer_txt}
        # TODO : Set a default copyright & footer at least

    def setFilename(self, filename):
        self.filename = filename

    def setCopyright(self, c):
        self.contents['copyright'] = c

    def setHeader(self, h):
        self.contents['header'] = h

    def setBody(self, b):
        self.contents['body'] = b

    def setFooter(self, f):
        self.contents['footer'] = f

    def generate(self):
        #print("Generate to file %s" % self.filename)
        with open(self.filename, "w") as f:
            f.write(self.contents['copyright'])
            f.write(self.contents['header'])
            f.write(self.contents['body'])
            f.write(self.contents['footer'])

# class for writing a wrapper class for structures
# The wrapper class wraps the structs and includes utility functions for
#  setting/getting member values and displaying the struct data in various formats
class StructWrapperGen:
    def __init__(self, in_struct_dict, prefix, out_dir, quiet):
        self.struct_dict = in_struct_dict
        self.include_headers = []
        self.lineinfo = sourcelineinfo()
        self.api = prefix
        if prefix.lower() == "vulkan":
            self.api_prefix = "vk"
        else:
            self.api_prefix = prefix
        self.safe_struct_header_filename = os.path.join(out_dir, self.api_prefix+"_safe_struct.h")
        self.safe_struct_source_filename = os.path.join(out_dir, self.api_prefix+"_safe_struct.cpp")
        # Safe Struct (ss) header and source files
        self.ssh = CommonFileGen(self.safe_struct_header_filename)
        self.sss = CommonFileGen(self.safe_struct_source_filename)
        self.header_txt = ""
        self.definition_txt = ""
        self.quiet = quiet

    def set_include_headers(self, include_headers):
        self.include_headers = include_headers

    # Return class name for given struct name
    def get_class_name(self, struct_name):
        class_name = struct_name.strip('_').lower() + "_struct_wrapper"
        return class_name

    def get_file_list(self):
        return [os.path.basename(self.header_filename), os.path.basename(self.class_filename), os.path.basename(self.string_helper_filename)]

    # Safe Structs are versions of vulkan structs with non-const safe ptrs
    #  that make shadowing structures and clean-up of shadowed structures very simple
    def generateSafeStructHeader(self):
        self.ssh.setCopyright(self._generateCopyright())
        self.ssh.setHeader(self._generateSafeStructHeader())
        self.ssh.setBody(self._generateSafeStructDecls())
        self.ssh.generate()

    def generateSafeStructs(self):
        self.sss.setCopyright(self._generateCopyright())
        self.sss.setHeader(self._generateSafeStructSourceHeader())
        self.sss.setBody(self._generateSafeStructSource())
        self.sss.generate()

    def _generateCopyright(self):
        copyright = []
        copyright.append('/* THIS FILE IS GENERATED.  DO NOT EDIT. */');
        copyright.append('');
        copyright.append('/*');
        copyright.append(' * Vulkan');
        copyright.append(' *');
        copyright.append(' * Copyright (c) 2015-2016 The Khronos Group Inc.');
        copyright.append(' * Copyright (c) 2015-2016 Valve Corporation.');
        copyright.append(' * Copyright (c) 2015-2016 LunarG, Inc.');
        copyright.append(' * Copyright (c) 2015-2016 Google Inc.');
        copyright.append(' *');
        copyright.append(' * Licensed under the Apache License, Version 2.0 (the "License");');
        copyright.append(' * you may not use this file except in compliance with the License.');
        copyright.append(' * You may obtain a copy of the License at');
        copyright.append(' *');
        copyright.append(' *     http://www.apache.org/licenses/LICENSE-2.0');
        copyright.append(' *');
        copyright.append(' * Unless required by applicable law or agreed to in writing, software');
        copyright.append(' * distributed under the License is distributed on an "AS IS" BASIS,');
        copyright.append(' * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.');
        copyright.append(' * See the License for the specific language governing permissions and');
        copyright.append(' * limitations under the License.');
        copyright.append(' *');
        copyright.append(' * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>');
        copyright.append(' * Author: Tobin Ehlis <tobin@lunarg.com>');
        copyright.append(' */');
        copyright.append('');
        return "\n".join(copyright)

    def _generateCppHeader(self):
        header = []
        header.append("//#includes, #defines, globals and such...\n")
        header.append("#include <stdio.h>\n#include <%s>\n#include <%s_enum_string_helper.h>\n" % (os.path.basename(self.header_filename), self.api_prefix))
        return "".join(header)

    def _get_func_name(self, struct, mid_str):
        return "%s_%s_%s" % (self.api_prefix, mid_str, struct.lower().strip("_"))

    def _get_size_helper_func_name(self, struct):
        return self._get_func_name(struct, 'size')

    def _generateHeader(self):
        header = []
        header.append("//#includes, #defines, globals and such...\n")
        for f in self.include_headers:
            header.append("#include <%s>\n" % f)
        return "".join(header)

    # Declarations
    def _generateConstructorDeclarations(self, s):
        constructors = []
        constructors.append("    %s();\n" % self.get_class_name(s))
        constructors.append("    %s(%s* pInStruct);\n" % (self.get_class_name(s), typedef_fwd_dict[s]))
        constructors.append("    %s(const %s* pInStruct);\n" % (self.get_class_name(s), typedef_fwd_dict[s]))
        return "".join(constructors)

    def _generateDestructorDeclarations(self, s):
        return "    virtual ~%s();\n" % self.get_class_name(s)

    def _generateDisplayDeclarations(self, s):
        return "    void display_txt();\n    void display_single_txt();\n    void display_full_txt();\n"

    def _generateGetSetDeclarations(self, s):
        get_set = []
        get_set.append("    void set_indent(uint32_t indent) { m_indent = indent; }\n")
        for member in sorted(self.struct_dict[s]):
            # TODO : Skipping array set/get funcs for now
            if self.struct_dict[s][member]['array']:
                continue
            get_set.append("    %s get_%s() { return m_struct.%s; }\n" % (self.struct_dict[s][member]['full_type'], self.struct_dict[s][member]['name'], self.struct_dict[s][member]['name']))
            if not self.struct_dict[s][member]['const']:
                get_set.append("    void set_%s(%s inValue) { m_struct.%s = inValue; }\n" % (self.struct_dict[s][member]['name'], self.struct_dict[s][member]['full_type'], self.struct_dict[s][member]['name']))
        return "".join(get_set)

    def _generatePrivateMembers(self, s):
        priv = []
        priv.append("\nprivate:\n")
        priv.append("    %s m_struct;\n" % typedef_fwd_dict[s])
        priv.append("    const %s* m_origStructAddr;\n" % typedef_fwd_dict[s])
        priv.append("    uint32_t m_indent;\n")
        priv.append("    const char m_dummy_prefix;\n")
        priv.append("    void display_struct_members();\n")
        return "".join(priv)

    def _generateClassDeclaration(self):
        class_decl = []
        for s in sorted(self.struct_dict):
            class_decl.append("\n//class declaration")
            class_decl.append("class %s\n{\npublic:" % self.get_class_name(s))
            class_decl.append(self._generateConstructorDeclarations(s))
            class_decl.append(self._generateDestructorDeclarations(s))
            class_decl.append(self._generateDisplayDeclarations(s))
            class_decl.append(self._generateGetSetDeclarations(s))
            class_decl.append(self._generatePrivateMembers(s))
            class_decl.append("};\n")
        return "\n".join(class_decl)

    def _generateFooter(self):
        return "\n//any footer info for class\n"

    def _getSafeStructName(self, struct):
        return "safe_%s" % (struct)

    # If struct has sType or ptr members, generate safe type
    def _hasSafeStruct(self, s):
        exceptions = ['VkPhysicalDeviceFeatures']
        if s in exceptions:
            return False
        if 'sType' == self.struct_dict[s][0]['name']:
            return True
        for m in self.struct_dict[s]:
            if self.struct_dict[s][m]['ptr']:
                return True
        return False

    def _generateSafeStructHeader(self):
        header = []
        header.append("//#includes, #defines, globals and such...\n")
        header.append('#pragma once\n')
        header.append('#include "vulkan/vulkan.h"')
        return "".join(header)

    # If given ty is in obj list, or is a struct that contains anything in obj list, return True
    def _typeHasObject(self, ty, obj):
        if ty in obj:
            return True
        if is_type(ty, 'struct'):
            for m in self.struct_dict[ty]:
                if self.struct_dict[ty][m]['type'] in obj:
                    return True
        return False

    def _generateSafeStructDecls(self):
        ss_decls = []
        for s in struct_order_list:
            if not self._hasSafeStruct(s):
                continue
            if s in ifdef_dict:
                ss_decls.append('#ifdef %s' % ifdef_dict[s])
            ss_name = self._getSafeStructName(s)
            ss_decls.append("\nstruct %s {" % (ss_name))
            for m in sorted(self.struct_dict[s]):
                m_type = self.struct_dict[s][m]['type']
                if is_type(m_type, 'struct') and self._hasSafeStruct(m_type):
                    m_type = self._getSafeStructName(m_type)
                if self.struct_dict[s][m]['array_size'] != 0 and not self.struct_dict[s][m]['dyn_array']:
                    ss_decls.append("    %s %s[%s];" % (m_type, self.struct_dict[s][m]['name'], self.struct_dict[s][m]['array_size']))
                elif self.struct_dict[s][m]['ptr'] and 'safe_' not in m_type and not self._typeHasObject(m_type, vulkan.object_non_dispatch_list):#m_type in ['char', 'float', 'uint32_t', 'void', 'VkPhysicalDeviceFeatures']: # We'll never overwrite char* so it can remain const
                    ss_decls.append("    %s %s;" % (self.struct_dict[s][m]['full_type'], self.struct_dict[s][m]['name']))
                elif self.struct_dict[s][m]['array']:
                    ss_decls.append("    %s* %s;" % (m_type, self.struct_dict[s][m]['name']))
                elif self.struct_dict[s][m]['ptr']:
                    ss_decls.append("    %s* %s;" % (m_type, self.struct_dict[s][m]['name']))
                else:
                    ss_decls.append("    %s %s;" % (m_type, self.struct_dict[s][m]['name']))
            ss_decls.append("    %s(const %s* pInStruct);" % (ss_name, s))
            ss_decls.append("    %s(const %s& src);" % (ss_name, ss_name)) # Copy constructor
            ss_decls.append("    %s();" % (ss_name)) # Default constructor
            ss_decls.append("    ~%s();" % (ss_name))
            ss_decls.append("    void initialize(const %s* pInStruct);" % (s))
            ss_decls.append("    void initialize(const %s* src);" % (ss_name))
            ss_decls.append("    %s *ptr() { return reinterpret_cast<%s *>(this); }" % (s, s))
            ss_decls.append("    %s const *ptr() const { return reinterpret_cast<%s const *>(this); }" % (s, s))
            ss_decls.append("};")
            if s in ifdef_dict:
                ss_decls.append('#endif')
        return "\n".join(ss_decls)

    def _generateSafeStructSourceHeader(self):
        header = []
        header.append("//#includes, #defines, globals and such...\n")
        header.append('#include "vk_safe_struct.h"\n#include <string.h>\n\n')
        return "".join(header)

    def _generateSafeStructSource(self):
        ss_src = []
        for s in struct_order_list:
            if not self._hasSafeStruct(s):
                continue
            if s in ifdef_dict:
                ss_src.append('#ifdef %s' % ifdef_dict[s])
            ss_name = self._getSafeStructName(s)
            init_list = '' # list of members in struct constructor initializer
            default_init_list = '' # Default constructor just inits ptrs to nullptr in initializer
            init_func_txt = '' # Txt for initialize() function that takes struct ptr and inits members
            construct_txt = '' # Body of constuctor as well as body of initialize() func following init_func_txt
            destruct_txt = ''
            # VkWriteDescriptorSet is special case because pointers may be non-null but ignored
            # TODO : This is ugly, figure out better way to do this
            custom_construct_txt = {'VkWriteDescriptorSet' :
                                    '    switch (descriptorType) {\n'
                                    '        case VK_DESCRIPTOR_TYPE_SAMPLER:\n'
                                    '        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:\n'
                                    '        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:\n'
                                    '        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:\n'
                                    '        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:\n'
                                    '        if (descriptorCount && pInStruct->pImageInfo) {\n'
                                    '            pImageInfo = new VkDescriptorImageInfo[descriptorCount];\n'
                                    '            for (uint32_t i=0; i<descriptorCount; ++i) {\n'
                                    '                pImageInfo[i] = pInStruct->pImageInfo[i];\n'
                                    '            }\n'
                                    '        }\n'
                                    '        break;\n'
                                    '        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:\n'
                                    '        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:\n'
                                    '        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:\n'
                                    '        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:\n'
                                    '        if (descriptorCount && pInStruct->pBufferInfo) {\n'
                                    '            pBufferInfo = new VkDescriptorBufferInfo[descriptorCount];\n'
                                    '            for (uint32_t i=0; i<descriptorCount; ++i) {\n'
                                    '                pBufferInfo[i] = pInStruct->pBufferInfo[i];\n'
                                    '            }\n'
                                    '        }\n'
                                    '        break;\n'
                                    '        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:\n'
                                    '        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:\n'
                                    '        if (descriptorCount && pInStruct->pTexelBufferView) {\n'
                                    '            pTexelBufferView = new VkBufferView[descriptorCount];\n'
                                    '            for (uint32_t i=0; i<descriptorCount; ++i) {\n'
                                    '                pTexelBufferView[i] = pInStruct->pTexelBufferView[i];\n'
                                    '            }\n'
                                    '        }\n'
                                    '        break;\n'
                                    '        default:\n'
                                    '        break;\n'
                                    '    }\n'}
            for m in self.struct_dict[s]:
                m_name = self.struct_dict[s][m]['name']
                m_type = self.struct_dict[s][m]['type']
                if is_type(m_type, 'struct') and self._hasSafeStruct(m_type):
                    m_type = self._getSafeStructName(m_type)
                if self.struct_dict[s][m]['ptr'] and 'safe_' not in m_type and not self._typeHasObject(m_type, vulkan.object_non_dispatch_list):# in ['char', 'float', 'uint32_t', 'void', 'VkPhysicalDeviceFeatures']) or 'pp' == self.struct_dict[s][m]['name'][0:1]:
                    # Ptr types w/o a safe_struct, for non-null case need to allocate new ptr and copy data in
                    if 'KHR' in ss_name or m_type in ['void', 'char']:
                        # For these exceptions just copy initial value over for now
                        init_list += '\n\t%s(pInStruct->%s),' % (m_name, m_name)
                        init_func_txt += '    %s = pInStruct->%s;\n' % (m_name, m_name)
                    else:
                        default_init_list += '\n\t%s(nullptr),' % (m_name)
                        init_list += '\n\t%s(nullptr),' % (m_name)
                        init_func_txt += '    %s = nullptr;\n' % (m_name)
                        if 'pNext' != m_name and 'void' not in m_type:
                            if not self.struct_dict[s][m]['array']:
                                construct_txt += '    if (pInStruct->%s) {\n' % (m_name)
                                construct_txt += '        %s = new %s(*pInStruct->%s);\n' % (m_name, m_type, m_name)
                                construct_txt += '    }\n'
                                destruct_txt += '    if (%s)\n' % (m_name)
                                destruct_txt += '        delete %s;\n' % (m_name)
                            else: # new array and then init each element
                                construct_txt += '    if (pInStruct->%s) {\n' % (m_name)
                                construct_txt += '        %s = new %s[pInStruct->%s];\n' % (m_name, m_type, self.struct_dict[s][m]['array_size'])
                                #construct_txt += '        std::copy (pInStruct->%s, pInStruct->%s+pInStruct->%s, %s);\n' % (m_name, m_name, self.struct_dict[s][m]['array_size'], m_name)
                                construct_txt += '        memcpy ((void *)%s, (void *)pInStruct->%s, sizeof(%s)*pInStruct->%s);\n' % (m_name, m_name, m_type, self.struct_dict[s][m]['array_size'])
                                construct_txt += '    }\n'
                                destruct_txt += '    if (%s)\n' % (m_name)
                                destruct_txt += '        delete[] %s;\n' % (m_name)
                elif self.struct_dict[s][m]['array']:
                    if not self.struct_dict[s][m]['dyn_array']:
                        # Handle static array case
                        construct_txt += '    for (uint32_t i=0; i<%s; ++i) {\n' % (self.struct_dict[s][m]['array_size'])
                        construct_txt += '        %s[i] = pInStruct->%s[i];\n' % (m_name, m_name)
                        construct_txt += '    }\n'
                    else:
                        # Init array ptr to NULL
                        default_init_list += '\n\t%s(nullptr),' % (m_name)
                        init_list += '\n\t%s(nullptr),' % (m_name)
                        init_func_txt += '    %s = nullptr;\n' % (m_name)
                        array_element = 'pInStruct->%s[i]' % (m_name)
                        if is_type(self.struct_dict[s][m]['type'], 'struct') and self._hasSafeStruct(self.struct_dict[s][m]['type']):
                            array_element = '%s(&pInStruct->%s[i])' % (self._getSafeStructName(self.struct_dict[s][m]['type']), m_name)
                        construct_txt += '    if (%s && pInStruct->%s) {\n' % (self.struct_dict[s][m]['array_size'], m_name)
                        construct_txt += '        %s = new %s[%s];\n' % (m_name, m_type, self.struct_dict[s][m]['array_size'])
                        destruct_txt += '    if (%s)\n' % (m_name)
                        destruct_txt += '        delete[] %s;\n' % (m_name)
                        construct_txt += '        for (uint32_t i=0; i<%s; ++i) {\n' % (self.struct_dict[s][m]['array_size'])
                        if 'safe_' in m_type:
                            construct_txt += '            %s[i].initialize(&pInStruct->%s[i]);\n' % (m_name, m_name)
                        else:
                            construct_txt += '            %s[i] = %s;\n' % (m_name, array_element)
                        construct_txt += '        }\n'
                        construct_txt += '    }\n'
                elif self.struct_dict[s][m]['ptr']:
                    construct_txt += '    if (pInStruct->%s)\n' % (m_name)
                    construct_txt += '        %s = new %s(pInStruct->%s);\n' % (m_name, m_type, m_name)
                    construct_txt += '    else\n'
                    construct_txt += '        %s = NULL;\n' % (m_name)
                    destruct_txt += '    if (%s)\n' % (m_name)
                    destruct_txt += '        delete %s;\n' % (m_name)
                elif 'safe_' in m_type: # inline struct, need to pass in reference for constructor
                    init_list += '\n\t%s(&pInStruct->%s),' % (m_name, m_name)
                    init_func_txt += '        %s.initialize(&pInStruct->%s);\n' % (m_name, m_name)
                else:
                    init_list += '\n\t%s(pInStruct->%s),' % (m_name, m_name)
                    init_func_txt += '    %s = pInStruct->%s;\n' % (m_name, m_name)
            if '' != init_list:
                init_list = init_list[:-1] # hack off final comma
            if s in custom_construct_txt:
                construct_txt = custom_construct_txt[s]
            ss_src.append("\n%s::%s(const %s* pInStruct) :%s\n{\n%s}" % (ss_name, ss_name, s, init_list, construct_txt))
            if '' != default_init_list:
                default_init_list = " :%s" % (default_init_list[:-1])
            ss_src.append("\n%s::%s()%s\n{}" % (ss_name, ss_name, default_init_list))
            # Create slight variation of init and construct txt for copy constructor that takes a src object reference vs. struct ptr
            copy_construct_init = init_func_txt.replace('pInStruct->', 'src.')
            copy_construct_txt = construct_txt.replace(' (pInStruct->', ' (src.') # Exclude 'if' blocks from next line
            copy_construct_txt = copy_construct_txt.replace('(pInStruct->', '(*src.') # Pass object to copy constructors
            copy_construct_txt = copy_construct_txt.replace('pInStruct->', 'src.') # Modify remaining struct refs for src object
            ss_src.append("\n%s::%s(const %s& src)\n{\n%s%s}" % (ss_name, ss_name, ss_name, copy_construct_init, copy_construct_txt)) # Copy constructor
            ss_src.append("\n%s::~%s()\n{\n%s}" % (ss_name, ss_name, destruct_txt))
            ss_src.append("\nvoid %s::initialize(const %s* pInStruct)\n{\n%s%s}" % (ss_name, s, init_func_txt, construct_txt))
            # Copy initializer uses same txt as copy constructor but has a ptr and not a reference
            init_copy = copy_construct_init.replace('src.', 'src->')
            init_construct = copy_construct_txt.replace('src.', 'src->')
            ss_src.append("\nvoid %s::initialize(const %s* src)\n{\n%s%s}" % (ss_name, ss_name, init_copy, init_construct))
            if s in ifdef_dict:
                ss_src.append('#endif')
        return "\n".join(ss_src)

def main(argv=None):
    opts = handle_args()
    # Parse input file and fill out global dicts
    hfp = HeaderFileParser(opts.input_file)
    hfp.parse()
    # TODO : Don't want these to be global, see note at top about wrapper classes
    global enum_val_dict
    global enum_type_dict
    global struct_dict
    global typedef_fwd_dict
    global typedef_rev_dict
    global types_dict
    enum_val_dict = hfp.get_enum_val_dict()
    enum_type_dict = hfp.get_enum_type_dict()
    struct_dict = hfp.get_struct_dict()
    # TODO : Would like to validate struct data here to verify that all of the bools for struct members are correct at this point
    typedef_fwd_dict = hfp.get_typedef_fwd_dict()
    typedef_rev_dict = hfp.get_typedef_rev_dict()
    types_dict = hfp.get_types_dict()
    #print(enum_val_dict)
    #print(typedef_dict)
    #print(struct_dict)
    input_header = os.path.basename(opts.input_file)
    if 'vulkan.h' == input_header:
        input_header = "vulkan/vulkan.h"

    prefix = os.path.basename(opts.input_file).strip(".h")
    if prefix == "vulkan":
        prefix = "vk"
    if (opts.abs_out_dir is not None):
        enum_sh_filename = os.path.join(opts.abs_out_dir, prefix+"_enum_string_helper.h")
    else:
        enum_sh_filename = os.path.join(os.getcwd(), opts.rel_out_dir, prefix+"_enum_string_helper.h")
    enum_sh_filename = os.path.abspath(enum_sh_filename)
    if not os.path.exists(os.path.dirname(enum_sh_filename)):
        if not opts.quiet:
            print("Creating output dir %s" % os.path.dirname(enum_sh_filename))
        os.mkdir(os.path.dirname(enum_sh_filename))
    if opts.gen_struct_wrappers:
        sw = StructWrapperGen(struct_dict, os.path.basename(opts.input_file).strip(".h"), os.path.dirname(enum_sh_filename), opts.quiet)
        sw.set_include_headers([input_header,os.path.basename(enum_sh_filename),"stdint.h","cinttypes", "stdio.h","stdlib.h"])
        sw.set_include_headers(["stdio.h", "stdlib.h", input_header])
        sw.generateSafeStructHeader()
        sw.generateSafeStructs()
    if not opts.quiet:
        print("DONE!")
    #print(typedef_rev_dict)
    #print(types_dict)
    #recreate_structs()

if __name__ == "__main__":
    sys.exit(main())
