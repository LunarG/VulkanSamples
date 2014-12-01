#!/usr/bin/env python3
#
# XGL
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
import argparse
import os
import sys

# code_gen.py overview
# This script generates code based on input headers
# Initially it's intended to support Mantle and XGL headers and
#  generate wrappers functions that can be used to display
#  structs in a human-readable txt format, as well as utility functions
#  to print enum values as strings


def handle_args():
    parser = argparse.ArgumentParser(description='Perform analysis of vogl trace.')
    parser.add_argument('input_file', help='The input header file from which code will be generated.')
    parser.add_argument('--rel_out_dir', required=False, default='glave_gen', help='Path relative to exec path to write output files. Will be created if needed.')
    parser.add_argument('--abs_out_dir', required=False, default=None, help='Absolute path to write output files. Will be created if needed.')
    parser.add_argument('--gen_enum_string_helper', required=False, action='store_true', default=False, help='Enable generation of helper header file to print string versions of enums.')
    parser.add_argument('--gen_struct_wrappers', required=False, action='store_true', default=False, help='Enable generation of struct wrapper classes.')
    parser.add_argument('--gen_cmake', required=False, action='store_true', default=False, help='Enable generation of cmake file for generated code.')
    parser.add_argument('--gen_graphviz', required=False, action='store_true', default=False, help='Enable generation of graphviz dot file.')
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
#       '->['array_size'] = int indicating size of array members (0 by default)
struct_dict = {}
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
        with open(self.header_file) as f:
            for line in f:
                if block_comment:
                    if '*/' in line:
                        block_comment = False
                    continue
                if '/*' in line:
                    block_comment = True
                elif 0 == len(line.split()):
                    #print("Skipping empty line")
                    continue
                elif line.split()[0].strip().startswith("//"):
                    #print("Skipping commentted line %s" % line)
                    continue
                elif 'typedef enum' in line:
                    (ty_txt, en_txt, base_type) = line.strip().split(None, 2)
                    #print("Found ENUM type %s" % base_type)
                    parse_enum = True
                    self.types_dict[base_type] = 'enum'
                elif 'typedef struct' in line:
                    (ty_txt, st_txt, base_type) = line.strip().split(None, 2)
                    #print("Found STRUCT type: %s" % base_type)
                    parse_struct = True
                    self.types_dict[base_type] = 'struct'
                elif '}' in line and (parse_enum or parse_struct):
                    if len(line.split()) > 1: # deals with embedded union in one struct
                        parse_enum = False
                        parse_struct = False
                        member_num = 0
                        # TODO : Can pull target of typedef here for remapping
                        (cur_char, targ_type) = line.strip().split(None, 1)
                        self.typedef_fwd_dict[base_type] = targ_type.strip(';')
                        self.typedef_rev_dict[targ_type.strip(';')] = base_type
                elif parse_enum:
                    if '=' in line:
                        self._add_enum(line, base_type)
                elif parse_struct:
                    if ';' in line:
                        self._add_struct(line, base_type, member_num)
                        member_num = member_num + 1
                #elif '(' in line:
                    #print("Function: %s" % line)
    
    # populate enum dicts based on enum lines
    def _add_enum(self, line_txt, enum_type):    
        #print("Parsing enum line %s" % line_txt)
        (enum_name, eq_char, enum_val) = line_txt.split(None, 2)
        if '=' != eq_char:
            print("ERROR: Couldn't parse enum line: %s" % line_txt)
        self.enum_val_dict[enum_name] = {}
        self.enum_val_dict[enum_name]['type'] = enum_type
        # strip comma and comment, then extra split in case of no comma w/ comments
        enum_val = enum_val.strip().split(',', 1)[0]
        self.enum_val_dict[enum_name]['val'] = enum_val.split()[0]
        # TODO : Make this more robust, to verify if enum value is unique
        #  Currently just try to cast to int which works ok but missed -(HEX) values
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
    
    # populate struct dicts based on struct lines
    # TODO : Handle ":" bitfield, "**" ptr->ptr and "const type*const*"
    def _add_struct(self, line_txt, struct_type, num):
        #print("Parsing struct line %s" % line_txt)
        if not struct_type in self.struct_dict:
            self.struct_dict[struct_type] = {}
        members = line_txt.strip().split(';', 1)[0] # first strip semicolon & comments
        # TODO : Handle bitfields more correctly
        members = members.strip().split(':', 1)[0] # strip bitfield element
        (member_type, member_name) = members.rsplit(None, 1)
        self.struct_dict[struct_type][num] = {}
        self.struct_dict[struct_type][num]['full_type'] = member_type
        if '*' in member_type:
            self.struct_dict[struct_type][num]['ptr'] = True
            member_type = member_type.strip('*')
        else:
            self.struct_dict[struct_type][num]['ptr'] = False
        if 'const' in member_type:
            self.struct_dict[struct_type][num]['const'] = True
            member_type = member_type.strip('const').strip()
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
            self.struct_dict[struct_type][num]['array'] = True
            self.struct_dict[struct_type][num]['array_size'] = array_size.strip(']')
        else:
            self.struct_dict[struct_type][num]['array'] = False
            self.struct_dict[struct_type][num]['array_size'] = 0
        self.struct_dict[struct_type][num]['name'] = member_name

# check if given identifier if of specified type_to_check
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
    def __init__(self, in_struct_dict, prefix, out_dir):
        self.struct_dict = in_struct_dict
        self.include_headers = []
        self.api = prefix
        self.header_filename = os.path.join(out_dir, self.api+"_struct_wrappers.h")
        self.class_filename = os.path.join(out_dir, self.api+"_struct_wrappers.cpp")
        self.string_helper_filename = os.path.join(out_dir, self.api+"_struct_string_helper.h")
        self.string_helper_no_addr_filename = os.path.join(out_dir, self.api+"_struct_string_helper_no_addr.h")
        self.no_addr = False
        self.hfg = CommonFileGen(self.header_filename)
        self.cfg = CommonFileGen(self.class_filename)
        self.shg = CommonFileGen(self.string_helper_filename)
        #print(self.header_filename)
        self.header_txt = ""
        self.definition_txt = ""
        
    def set_include_headers(self, include_headers):
        self.include_headers = include_headers

    def set_no_addr(self, no_addr):
        self.no_addr = no_addr
        if self.no_addr:
            self.shg = CommonFileGen(self.string_helper_no_addr_filename)
        else:
            self.shg = CommonFileGen(self.string_helper_filename)

    # Return class name for given struct name
    def get_class_name(self, struct_name):
        class_name = struct_name.strip('_').lower() + "_struct_wrapper"
        return class_name
        
    def get_file_list(self):
        return [os.path.basename(self.header_filename), os.path.basename(self.class_filename), os.path.basename(self.string_helper_filename)]

    # Generate class header file        
    def generateHeader(self):
        self.hfg.setCopyright(self._generateCopyright())
        self.hfg.setHeader(self._generateHeader())
        self.hfg.setBody(self._generateClassDeclaration())
        self.hfg.setFooter(self._generateFooter())
        self.hfg.generate()
    
    # Generate class definition
    def generateBody(self):
        self.cfg.setCopyright(self._generateCopyright())
        self.cfg.setHeader(self._generateCppHeader())
        self.cfg.setBody(self._generateClassDefinition())
        self.cfg.setFooter(self._generateFooter())
        self.cfg.generate()

    # Generate c-style .h file that contains functions for printing structs
    def generateStringHelper(self):
        print("Generating struct string helper")
        self.shg.setCopyright(self._generateCopyright())
        self.shg.setHeader(self._generateStringHelperHeader())
        self.shg.setBody(self._generateStringHelperFunctions())
        self.shg.generate()

    def _generateCopyright(self):
        return "//This is the copyright\n"

    def _generateCppHeader(self):
        header = []
        header.append("//#includes, #defines, globals and such...\n")
        header.append("#include <stdio.h>\n#include <%s>\n#include <%s_enum_string_helper.h>\n" % (os.path.basename(self.header_filename), self.api))
        return "".join(header)
        
    def _generateClassDefinition(self):
        class_def = []
        if 'xgl' == self.api: # Mantle doesn't have pNext to worry about
            class_def.append(self._generateDynamicPrintFunctions())
        for s in self.struct_dict:
            class_def.append("\n// %s class definition" % self.get_class_name(s))
            class_def.append(self._generateConstructorDefinitions(s))
            class_def.append(self._generateDestructorDefinitions(s))
            class_def.append(self._generateDisplayDefinitions(s))
        return "\n".join(class_def)
        
    def _generateConstructorDefinitions(self, s):
        con_defs = []
        con_defs.append("%s::%s() : m_struct(), m_indent(0), m_dummy_prefix('\\0'), m_origStructAddr(NULL) {}" % (self.get_class_name(s), self.get_class_name(s)))
        # TODO : This is a shallow copy of ptrs
        con_defs.append("%s::%s(%s* pInStruct) : m_indent(0), m_dummy_prefix('\\0')\n{\n    m_struct = *pInStruct;\n    m_origStructAddr = pInStruct;\n}" % (self.get_class_name(s), self.get_class_name(s), typedef_fwd_dict[s]))
        con_defs.append("%s::%s(const %s* pInStruct) : m_indent(0), m_dummy_prefix('\\0')\n{\n    m_struct = *pInStruct;\n    m_origStructAddr = pInStruct;\n}" % (self.get_class_name(s), self.get_class_name(s), typedef_fwd_dict[s]))
        return "\n".join(con_defs)
        
    def _generateDestructorDefinitions(self, s):
        return "%s::~%s() {}" % (self.get_class_name(s), self.get_class_name(s))
        
    def _generateDynamicPrintFunctions(self):
        dp_funcs = []
        dp_funcs.append("\nvoid dynamic_display_full_txt(const XGL_VOID* pStruct, uint32_t indent)\n{\n    // Cast to APP_INFO ptr initially just to pull sType off struct")
        dp_funcs.append("    XGL_STRUCTURE_TYPE sType = ((XGL_APPLICATION_INFO*)pStruct)->sType;\n")
        dp_funcs.append("    switch (sType)\n    {")
        for e in enum_type_dict:
            class_num = 0
            if "_STRUCTURE_TYPE" in e:
                for v in sorted(enum_type_dict[e]):
                    struct_name = v.replace("_STRUCTURE_TYPE", "")
                    class_name = self.get_class_name(struct_name)
                    # TODO : Hand-coded fixes for some exceptions
                    if 'XGL_PIPELINE_CB_STATE_CREATE_INFO' in struct_name:
                        struct_name = 'XGL_PIPELINE_CB_STATE'
                    elif 'XGL_SEMAPHORE_CREATE_INFO' in struct_name:
                        struct_name = 'XGL_QUEUE_SEMAPHORE_CREATE_INFO'
                        class_name = self.get_class_name(struct_name)
                    elif 'XGL_SEMAPHORE_OPEN_INFO' in struct_name:
                        struct_name = 'XGL_QUEUE_SEMAPHORE_OPEN_INFO'
                        class_name = self.get_class_name(struct_name)
                    instance_name = "swc%i" % class_num
                    dp_funcs.append("        case %s:\n        {" % (v))
                    dp_funcs.append("            %s %s((%s*)pStruct);" % (class_name, instance_name, struct_name))
                    dp_funcs.append("            %s.set_indent(indent);" % (instance_name))
                    dp_funcs.append("            %s.display_full_txt();" % (instance_name))
                    dp_funcs.append("        }")
                    dp_funcs.append("        break;")
                    class_num += 1
                dp_funcs.append("    }")
        dp_funcs.append("}\n")
        return "\n".join(dp_funcs)

    def _get_sh_func_name(self, struct):
        return "%s_print_%s" % (self.api, struct.lower().strip("_"))

    # Return elements to create formatted string for given struct member
    def _get_struct_print_formatted(self, struct_member, pre_var_name="prefix", postfix = "\\n", struct_var_name="pStruct", struct_ptr=True, print_array=False):
        struct_op = "->"
        if not struct_ptr:
            struct_op = "."
        member_name = struct_member['name']
        print_type = "p"
        cast_type = ""
        member_post = ""
        array_index = ""
        member_print_post = ""
        if struct_member['array'] and 'CHAR' in struct_member['type']: # just print char array as string
            print_type = "s"
            print_array = False
        elif struct_member['array'] and not print_array:
            # Just print base address of array when not full print_array
            cast_type = "(void*)"
        elif is_type(struct_member['type'], 'enum'):
            cast_type = "string_%s" % struct_member['type']
            print_type = "s"
        elif is_type(struct_member['type'], 'struct'): # print struct address for now
            cast_type = "(void*)"
            if not struct_member['ptr']:
                cast_type = "(void*)&"
        elif 'BOOL' in struct_member['type']:
            print_type = "s"
            member_post = ' ? "TRUE" : "FALSE"'
        elif 'FLOAT' in struct_member['type']:
            print_type = "f"
        elif 'UINT64' in struct_member['type']:
            print_type = "lu"
        elif 'UINT8' in struct_member['type']:
            print_type = "hu"
        elif True in [ui_str in struct_member['type'] for ui_str in ['UINT', '_SIZE', '_FLAGS', '_SAMPLE_MASK']]:
            print_type = "u"
        elif 'INT' in struct_member['type']:
            print_type = "i"
        elif struct_member['ptr']:
            pass
        else:
            #print("Unhandled struct type: %s" % struct_member['type'])
            cast_type = "(void*)"
        if print_array and struct_member['array']:
            member_print_post = "[%u]"
            array_index = " i,"
            member_post = "[i]"
        print_out = "%%s%s%s = %%%s%s" % (member_name, member_print_post, print_type, postfix) # section of print that goes inside of quotes
        print_arg = ", %s,%s %s(%s%s%s)%s" % (pre_var_name, array_index, cast_type, struct_var_name, struct_op, member_name, member_post) # section of print passed to portion in quotes
        if self.no_addr and "p" == print_type:
            print_out = "%%s%s%s = addr\\n" % (member_name, member_print_post) # section of print that goes inside of quotes
            print_arg = ", %s" % (pre_var_name)
        return (print_out, print_arg)

    def _generateStringHelperFunctions(self):
        sh_funcs = []
        # We do two passes, first pass just generates prototypes for all the functsions
        for s in self.struct_dict:
            sh_funcs.append('char* %s(const %s* pStruct, const char* prefix);\n' % (self._get_sh_func_name(s), typedef_fwd_dict[s]))
        sh_funcs.append('\n')
        for s in self.struct_dict:
            p_out = ""
            p_args = ""
            stp_list = [] # stp == "struct to print" a list of structs for this API call that should be printed as structs
            # This isn't great but this pre-pass counts chars in struct members and flags structs w/ pNext
            struct_char_count = 0 # TODO : Use this to vary size of memory allocations for strings?
            for m in self.struct_dict[s]:
                if 'pNext' == self.struct_dict[s][m]['name'] or is_type(self.struct_dict[s][m]['type'], 'struct'):
                    stp_list.append(self.struct_dict[s][m])
                struct_char_count += len(self.struct_dict[s][m]['name']) + 32
            sh_funcs.append('char* %s(const %s* pStruct, const char* prefix)\n{\n    char* str;\n' % (self._get_sh_func_name(s), typedef_fwd_dict[s]))
            sh_funcs.append("    size_t len;\n")
            num_stps = len(stp_list);
            total_strlen_str = ''
            if 0 != num_stps:
                sh_funcs.append("    char* tmpStr;\n")
                sh_funcs.append('    char* extra_indent = (char*)malloc(strlen(prefix) + 3);\n')
                sh_funcs.append('    strcpy(extra_indent, "  ");\n')
                sh_funcs.append('    strncat(extra_indent, prefix, strlen(prefix));\n')
                sh_funcs.append("    char dummy_char = '\\0';\n")
                sh_funcs.append('    char* stp_strs[%i];\n' % num_stps)
                for index in range(num_stps):
                    if (stp_list[index]['ptr']):
                        sh_funcs.append('    if (pStruct->%s) {\n' % stp_list[index]['name'])
                        if 'pNext' == stp_list[index]['name']:
                            sh_funcs.append('        tmpStr = dynamic_display((XGL_VOID*)pStruct->pNext, prefix);\n')
                            sh_funcs.append('        len = 256+strlen(tmpStr);\n')
                            sh_funcs.append('        stp_strs[%i] = (char*)malloc(len);\n' % index)
                            if self.no_addr:
                                sh_funcs.append('        snprintf(stp_strs[%i], len, "   %%spNext (addr)\\n%%s", prefix, tmpStr);\n' % index)
                            else:
                                sh_funcs.append('        snprintf(stp_strs[%i], len, "   %%spNext (%%p)\\n%%s", prefix, (void*)pStruct->pNext, tmpStr);\n' % index)
                            sh_funcs.append('        free(tmpStr);\n')
                        else:
                            sh_funcs.append('        tmpStr = %s(pStruct->%s, extra_indent);\n' % (self._get_sh_func_name(stp_list[index]['type']), stp_list[index]['name']))
                            sh_funcs.append('        len = 256+strlen(tmpStr)+strlen(prefix);\n')
                            sh_funcs.append('        stp_strs[%i] = (char*)malloc(len);\n' % (index))
                            if self.no_addr:
                                sh_funcs.append('        snprintf(stp_strs[%i], len, " %%s%s (addr)\\n%%s", prefix, tmpStr);\n' % (index, stp_list[index]['name']))
                            else:
                                sh_funcs.append('        snprintf(stp_strs[%i], len, " %%s%s (%%p)\\n%%s", prefix, (void*)pStruct->%s, tmpStr);\n' % (index, stp_list[index]['name'], stp_list[index]['name']))
                        sh_funcs.append('    }\n')
                        sh_funcs.append("    else\n        stp_strs[%i] = &dummy_char;\n" % (index))
                    elif stp_list[index]['array']: # TODO : For now just printing first element of array
                        sh_funcs.append('    tmpStr = %s(&pStruct->%s[0], extra_indent);\n' % (self._get_sh_func_name(stp_list[index]['type']), stp_list[index]['name']))
                        sh_funcs.append('    len = 256+strlen(tmpStr);\n')
                        sh_funcs.append('    stp_strs[%i] = (char*)malloc(len);\n' % (index))
                        if self.no_addr:
                            sh_funcs.append('    snprintf(stp_strs[%i], len, " %%s%s[0] (addr)\\n%%s", prefix, tmpStr);\n' % (index, stp_list[index]['name']))
                        else:
                            sh_funcs.append('    snprintf(stp_strs[%i], len, " %%s%s[0] (%%p)\\n%%s", prefix, (void*)&pStruct->%s[0], tmpStr);\n' % (index, stp_list[index]['name'], stp_list[index]['name']))
                    else:
                        sh_funcs.append('    tmpStr = %s(&pStruct->%s, extra_indent);\n' % (self._get_sh_func_name(stp_list[index]['type']), stp_list[index]['name']))
                        sh_funcs.append('    len = 256+strlen(tmpStr);\n')
                        sh_funcs.append('    stp_strs[%i] = (char*)malloc(len);\n' % (index))
                        if self.no_addr:
                            sh_funcs.append('    snprintf(stp_strs[%i], len, " %%s%s (addr)\\n%%s", prefix, tmpStr);\n' % (index, stp_list[index]['name']))
                        else:
                            sh_funcs.append('    snprintf(stp_strs[%i], len, " %%s%s (%%p)\\n%%s", prefix, (void*)&pStruct->%s, tmpStr);\n' % (index, stp_list[index]['name'], stp_list[index]['name']))
                    total_strlen_str += 'strlen(stp_strs[%i]) + ' % index
            sh_funcs.append('    len = %ssizeof(char)*1024;\n' % (total_strlen_str))
            sh_funcs.append('    str = (char*)malloc(len);\n')
            sh_funcs.append('    snprintf(str, len, "')
            for m in sorted(self.struct_dict[s]):
                (p_out1, p_args1) = self._get_struct_print_formatted(self.struct_dict[s][m])
                p_out += p_out1
                p_args += p_args1
            p_out += '"'
            p_args += ");\n"
            sh_funcs.append(p_out)
            sh_funcs.append(p_args)
            if 0 != num_stps:
                sh_funcs.append('    for (int32_t stp_index = %i; stp_index >= 0; stp_index--) {\n' % (num_stps-1))
                sh_funcs.append('        if (0 < strlen(stp_strs[stp_index])) {\n')
                sh_funcs.append('            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));\n')
                sh_funcs.append('            free(stp_strs[stp_index]);\n')
                sh_funcs.append('        }\n')
                sh_funcs.append('    }\n')
            sh_funcs.append("    return str;\n}\n")
        # Add function to dynamically print out unknown struct
        sh_funcs.append("char* dynamic_display(const XGL_VOID* pStruct, const char* prefix)\n{\n")
        sh_funcs.append("    // Cast to APP_INFO ptr initially just to pull sType off struct\n")
        sh_funcs.append("    XGL_STRUCTURE_TYPE sType = ((XGL_APPLICATION_INFO*)pStruct)->sType;\n")
        sh_funcs.append('    char indent[100];\n    strcpy(indent, "    ");\n    strcat(indent, prefix);\n')
        sh_funcs.append("    switch (sType)\n    {\n")
        for e in enum_type_dict:
            if "_STRUCTURE_TYPE" in e:
                for v in sorted(enum_type_dict[e]):
                    struct_name = v.replace("_STRUCTURE_TYPE", "")
                    print_func_name = self._get_sh_func_name(struct_name)
                    # TODO : Hand-coded fixes for some exceptions
                    if 'XGL_PIPELINE_CB_STATE_CREATE_INFO' in struct_name:
                        struct_name = 'XGL_PIPELINE_CB_STATE'
                    elif 'XGL_SEMAPHORE_CREATE_INFO' in struct_name:
                        struct_name = 'XGL_QUEUE_SEMAPHORE_CREATE_INFO'
                        print_func_name = self._get_sh_func_name(struct_name)
                    elif 'XGL_SEMAPHORE_OPEN_INFO' in struct_name:
                        struct_name = 'XGL_QUEUE_SEMAPHORE_OPEN_INFO'
                        print_func_name = self._get_sh_func_name(struct_name)
                    sh_funcs.append('        case %s:\n        {\n' % (v))
                    sh_funcs.append('            return %s((%s*)pStruct, indent);\n' % (print_func_name, struct_name))
                    sh_funcs.append('        }\n')
                    sh_funcs.append('        break;\n')
                sh_funcs.append("    }\n")
        sh_funcs.append("}")
        return "".join(sh_funcs)
                
        
    def _genStructMemberPrint(self, member, s, array, struct_array):
        (p_out, p_arg) = self._get_struct_print_formatted(self.struct_dict[s][member], pre_var_name="&m_dummy_prefix", struct_var_name="m_struct", struct_ptr=False, print_array=True)
        extra_indent = ""
        if array:
            extra_indent = "    "
        if is_type(self.struct_dict[s][member]['type'], 'struct'): # print struct address for now
            struct_array.insert(0, self.struct_dict[s][member])
        elif self.struct_dict[s][member]['ptr']:
            # Special case for VOID* named "pNext"
            if "VOID" in self.struct_dict[s][member]['type'] and "pNext" == self.struct_dict[s][member]['name']:
                struct_array.insert(0, self.struct_dict[s][member])
        return ('    %sprintf("%%*s    %s", m_indent, ""%s);' % (extra_indent, p_out, p_arg), struct_array)

    def _generateDisplayDefinitions(self, s):
        disp_def = []
        struct_array = []
        # Single-line struct print function
        disp_def.append("// Output 'structname = struct_address' on a single line")
        disp_def.append("void %s::display_single_txt()\n{" % self.get_class_name(s))
        disp_def.append('    printf(" %%*s%s = %%p", m_indent, "", (void*)m_origStructAddr);' % typedef_fwd_dict[s])
        disp_def.append("}\n")
        # Private helper function to print struct members
        disp_def.append("// Private helper function that displays the members of the wrapped struct")
        disp_def.append("void %s::display_struct_members()\n{" % self.get_class_name(s))
        i_declared = False
        for member in sorted(self.struct_dict[s]):
            # TODO : Need to display each member based on its type
            # TODO : Need to handle pNext which are structs, but of XGL_VOID* type
            #   Can grab struct type off of header of struct pointed to
            # TODO : Handle Arrays
            if self.struct_dict[s][member]['array']:
                # Create for loop to print each element of array
                if not i_declared:
                    disp_def.append('    uint32_t i;')
                    i_declared = True
                disp_def.append('    for (i = 0; i<%s; i++) {' % self.struct_dict[s][member]['array_size'])
                (return_str, struct_array) = self._genStructMemberPrint(member, s, True, struct_array)
                disp_def.append(return_str)
                disp_def.append('    }')
            else:
                (return_str, struct_array) = self._genStructMemberPrint(member, s, False, struct_array)
                disp_def.append(return_str)
        disp_def.append("}\n")
        i_declared = False
        # Basic print function to display struct members
        disp_def.append("// Output all struct elements, each on their own line")
        disp_def.append("void %s::display_txt()\n{" % self.get_class_name(s))
        disp_def.append('    printf("%%*s%s struct contents at %%p:\\n", m_indent, "", (void*)m_origStructAddr);' % typedef_fwd_dict[s])
        disp_def.append('    this->display_struct_members();')
        disp_def.append("}\n")
        # Advanced print function to display current struct and contents of any pointed-to structs
        disp_def.append("// Output all struct elements, and for any structs pointed to, print complete contents")
        disp_def.append("void %s::display_full_txt()\n{" % self.get_class_name(s))
        disp_def.append('    printf("%%*s%s struct contents at %%p:\\n", m_indent, "", (void*)m_origStructAddr);' % typedef_fwd_dict[s])
        disp_def.append('    this->display_struct_members();')
        class_num = 0
        # TODO : Need to handle arrays of structs here
        for ms in struct_array:
            swc_name = "class%s" % str(class_num)
            if ms['array']:
                if not i_declared:
                    disp_def.append('    uint32_t i;')
                    i_declared = True
                disp_def.append('    for (i = 0; i<%s; i++) {' % ms['array_size'])
                #disp_def.append("        if (m_struct.%s[i]) {" % (ms['name']))
                disp_def.append("            %s %s(&(m_struct.%s[i]));" % (self.get_class_name(ms['type']), swc_name, ms['name']))
                disp_def.append("            %s.set_indent(m_indent + 4);" % (swc_name))
                disp_def.append("            %s.display_full_txt();" % (swc_name))
                #disp_def.append('        }')
                disp_def.append('    }')
            elif 'pNext' == ms['name']:
                # Need some code trickery here
                #  I'm thinking have a generated function that takes pNext ptr value
                #  then it checks sType and in large switch statement creates appropriate
                #  wrapper class type and then prints contents
                disp_def.append("    if (m_struct.%s) {" % (ms['name']))
                #disp_def.append('        printf("%*s    This is where we would call dynamic print function\\n", m_indent, "");')
                disp_def.append('        dynamic_display_full_txt(m_struct.%s, m_indent);' % (ms['name']))
                disp_def.append("    }")
            else:
                if ms['ptr']:
                    disp_def.append("    if (m_struct.%s) {" % (ms['name']))
                    disp_def.append("        %s %s(m_struct.%s);" % (self.get_class_name(ms['type']), swc_name, ms['name']))
                else:
                    disp_def.append("    if (&m_struct.%s) {" % (ms['name']))
                    disp_def.append("        %s %s(&m_struct.%s);" % (self.get_class_name(ms['type']), swc_name, ms['name']))
                disp_def.append("        %s.set_indent(m_indent + 4);" % (swc_name))
                disp_def.append("        %s.display_full_txt();\n    }" % (swc_name))
            class_num += 1
        disp_def.append("}\n")
        return "\n".join(disp_def)
        
    def _generateStringHelperHeader(self):
        header = []
        header.append("//#includes, #defines, globals and such...\n")
        for f in self.include_headers:
            if 'xgl_enum_string_helper' not in f:
                header.append("#include <%s>\n" % f)
        header.append('#include "xgl_enum_string_helper.h"\n\n// Function Prototypes\n')
        header.append("char* dynamic_display(const XGL_VOID* pStruct, const char* prefix);\n")
        return "".join(header)
        
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
        for s in self.struct_dict:
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

class EnumCodeGen:
    def __init__(self, enum_type_dict=None, enum_val_dict=None, typedef_fwd_dict=None, in_file=None, out_file=None):
        self.et_dict = enum_type_dict
        self.ev_dict = enum_val_dict
        self.tf_dict = typedef_fwd_dict
        self.in_file = in_file
        self.out_file = out_file
        self.efg = CommonFileGen(self.out_file)
        
    def generateStringHelper(self):
        self.efg.setHeader(self._generateSHHeader())
        self.efg.setBody(self._generateSHBody())
        self.efg.generate()
    
    def _generateSHBody(self):
        body = []
#        with open(self.out_file, "a") as hf:
            # bet == base_enum_type, fet == final_enum_type
        for bet in self.et_dict:
            fet = self.tf_dict[bet]
            body.append("static const char* string_%s(%s input_value)\n{\n    switch ((%s)input_value)\n    {\n" % (fet, fet, fet))
            for e in sorted(self.et_dict[bet]):
                if (self.ev_dict[e]['unique']):
                    body.append('    case %s:\n        return "%s";\n' % (e, e))
            body.append('    default:\n        return "Unhandled %s";\n    }\n    return "Unhandled %s";\n}\n\n' % (fet, fet))
        return "\n".join(body)
    
    def _generateSHHeader(self):
        return "#pragma once\n\n#include <%s>\n\n" % self.in_file
        

class CMakeGen:
    def __init__(self, struct_wrapper=None, out_dir=None):
        self.sw = struct_wrapper
        self.include_headers = []
        self.add_lib_file_list = self.sw.get_file_list()
        self.out_dir = out_dir
        self.out_file = os.path.join(self.out_dir, "CMakeLists.txt")
        self.cmg = CommonFileGen(self.out_file)
        
    def generate(self):
        self.cmg.setBody(self._generateBody())
        self.cmg.generate()
        
    def _generateBody(self):
        body = []
        body.append("project(%s)" % os.path.basename(self.out_dir))
        body.append("cmake_minimum_required(VERSION 2.8)\n")
        body.append("add_library(${PROJECT_NAME} %s)\n" % " ".join(self.add_lib_file_list))
        body.append('set(COMPILE_FLAGS "-fpermissive")')
        body.append('set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}")\n')
        body.append("include_directories(${SRC_DIR}/thirdparty/${GEN_API}/inc/)\n")
        body.append("target_include_directories (%s PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})\n" % os.path.basename(self.out_dir))
        return "\n".join(body)

class GraphVizGen:
    def __init__(self, struct_dict, prefix, out_dir):
        self.struct_dict = struct_dict
        self.api = prefix
        self.out_file = os.path.join(out_dir, self.api+"_struct_graphviz_helper.h")
        self.gvg = CommonFileGen(self.out_file)

    def generate(self):
        self.gvg.setCopyright("//This is the copyright\n")
        self.gvg.setHeader(self._generateHeader())
        self.gvg.setBody(self._generateBody())
        #self.gvg.setFooter('}')
        self.gvg.generate()

    def set_include_headers(self, include_headers):
        self.include_headers = include_headers

    def _generateHeader(self):
        header = []
        header.append("//#includes, #defines, globals and such...\n")
        for f in self.include_headers:
            if 'xgl_enum_string_helper' not in f:
                header.append("#include <%s>\n" % f)
        #header.append('#include "xgl_enum_string_helper.h"\n\n// Function Prototypes\n')
        header.append("\nchar* dynamic_gv_display(const XGL_VOID* pStruct, const char* prefix);\n")
        return "".join(header)

    def _get_gv_func_name(self, struct):
        return "%s_gv_print_%s" % (self.api, struct.lower().strip("_"))

    # Return elements to create formatted string for given struct member
    def _get_struct_gv_print_formatted(self, struct_member, pre_var_name="", postfix = "\\n", struct_var_name="pStruct", struct_ptr=True, print_array=False, port_label=""):
        struct_op = "->"
        pre_var_name = '"%s "' % struct_member['full_type']
        if not struct_ptr:
            struct_op = "."
        member_name = struct_member['name']
        print_type = "p"
        cast_type = ""
        member_post = ""
        array_index = ""
        member_print_post = ""
        if struct_member['array'] and 'CHAR' in struct_member['type']: # just print char array as string
            print_type = "s"
            print_array = False
        elif struct_member['array'] and not print_array:
            # Just print base address of array when not full print_array
            cast_type = "(void*)"
        elif is_type(struct_member['type'], 'enum'):
            cast_type = "string_%s" % struct_member['type']
            print_type = "s"
        elif is_type(struct_member['type'], 'struct'): # print struct address for now
            cast_type = "(void*)"
            if not struct_member['ptr']:
                cast_type = "(void*)&"
        elif 'BOOL' in struct_member['type']:
            print_type = "s"
            member_post = ' ? "TRUE" : "FALSE"'
        elif 'FLOAT' in struct_member['type']:
            print_type = "f"
        elif 'UINT64' in struct_member['type']:
            print_type = "lu"
        elif 'UINT8' in struct_member['type']:
            print_type = "hu"
        elif True in [ui_str in struct_member['type'] for ui_str in ['UINT', '_SIZE', '_FLAGS', '_SAMPLE_MASK']]:
            print_type = "u"
        elif 'INT' in struct_member['type']:
            print_type = "i"
        elif struct_member['ptr']:
            pass
        else:
            #print("Unhandled struct type: %s" % struct_member['type'])
            cast_type = "(void*)"
        if print_array and struct_member['array']:
            member_print_post = "[%u]"
            array_index = " i,"
            member_post = "[i]"
        print_out = "<TR><TD>%%s%s%s</TD><TD%s>%%%s%s</TD></TR>" % (member_name, member_print_post, port_label, print_type, postfix) # section of print that goes inside of quotes
        print_arg = ", %s,%s %s(%s%s%s)%s" % (pre_var_name, array_index, cast_type, struct_var_name, struct_op, member_name, member_post) # section of print passed to portion in quotes
        return (print_out, print_arg)

    def _generateBody(self):
        gv_funcs = []
        array_func_list = [] # structs for which we'll generate an array version of their print function
        array_func_list.append('xgl_descriptor_slot_info')
        # For first pass, generate prototype
        for s in self.struct_dict:
            gv_funcs.append('char* %s(const %s* pStruct, const char* myNodeName);\n' % (self._get_gv_func_name(s), typedef_fwd_dict[s]))
            if s.lower().strip("_") in array_func_list:
                gv_funcs.append('char* %s_array(XGL_UINT count, const %s* pStruct, const char* myNodeName);\n' % (self._get_gv_func_name(s), typedef_fwd_dict[s]))
        gv_funcs.append('\n')
        for s in self.struct_dict:
            p_out = ""
            p_args = ""
            stp_list = [] # stp == "struct to print" a list of structs for this API call that should be printed as structs
            # the fields below are a super-hacky way for now to get port labels into GV output, TODO : Clean this up!            
            pl_dict = {}
            struct_num = 0
            # This isn't great but this pre-pass counts chars in struct members and flags structs w/ pNext            
            for m in sorted(self.struct_dict[s]):
                if 'pNext' == self.struct_dict[s][m]['name'] or is_type(self.struct_dict[s][m]['type'], 'struct'):
                    stp_list.append(self.struct_dict[s][m])
                    if 'pNext' == self.struct_dict[s][m]['name']:
                        pl_dict[m] = ' PORT=\\"pNext\\"'
                    else:
                        pl_dict[m] = ' PORT=\\"struct%i\\"' % struct_num
                    struct_num += 1
            gv_funcs.append('char* %s(const %s* pStruct, const char* myNodeName)\n{\n    char* str;\n' % (self._get_gv_func_name(s), typedef_fwd_dict[s]))
            num_stps = len(stp_list);
            total_strlen_str = ''
            if 0 != num_stps:
                gv_funcs.append("    char* tmpStr;\n")
                gv_funcs.append("    char nodeName[100];\n")
                gv_funcs.append("    char dummy_char = '\\0';\n")
                gv_funcs.append('    char* stp_strs[%i];\n' % num_stps)
                for index in range(num_stps):
                    if (stp_list[index]['ptr']):
                        if 'pDescriptorInfo' == stp_list[index]['name']:
                            gv_funcs.append('    if (pStruct->pDescriptorInfo && (0 != pStruct->descriptorCount)) {\n')
                        else:
                            gv_funcs.append('    if (pStruct->%s) {\n' % stp_list[index]['name'])
                        if 'pNext' == stp_list[index]['name']:
                            gv_funcs.append('        sprintf(nodeName, "pNext_%p", (void*)pStruct->pNext);\n')
                            gv_funcs.append('        tmpStr = dynamic_gv_display((XGL_VOID*)pStruct->pNext, nodeName);\n')
                            gv_funcs.append('        stp_strs[%i] = (char*)malloc(256+strlen(tmpStr)+strlen(nodeName)+strlen(myNodeName));\n' % index)
                            gv_funcs.append('        sprintf(stp_strs[%i], "%%s\\n\\"%%s\\":pNext -> \\"%%s\\" [];\\n", tmpStr, myNodeName, nodeName);\n' % index)
                            gv_funcs.append('        free(tmpStr);\n')
                        else:
                            gv_funcs.append('        sprintf(nodeName, "%s_%%p", (void*)pStruct->%s);\n' % (stp_list[index]['name'], stp_list[index]['name']))
                            if 'pDescriptorInfo' == stp_list[index]['name']:
                                gv_funcs.append('        tmpStr = %s_array(pStruct->descriptorCount, pStruct->%s, nodeName);\n' % (self._get_gv_func_name(stp_list[index]['type']), stp_list[index]['name']))
                            else:
                                gv_funcs.append('        tmpStr = %s(pStruct->%s, nodeName);\n' % (self._get_gv_func_name(stp_list[index]['type']), stp_list[index]['name']))
                            gv_funcs.append('        stp_strs[%i] = (char*)malloc(256+strlen(tmpStr)+strlen(nodeName)+strlen(myNodeName));\n' % (index))
                            gv_funcs.append('        sprintf(stp_strs[%i], "%%s\\n\\"%%s\\":struct%i -> \\"%%s\\" [];\\n", tmpStr, myNodeName, nodeName);\n' % (index, index))
                        gv_funcs.append('    }\n')
                        gv_funcs.append("    else\n        stp_strs[%i] = &dummy_char;\n" % (index))
                    elif stp_list[index]['array']: # TODO : For now just printing first element of array
                        gv_funcs.append('    sprintf(nodeName, "%s_%%p", (void*)&pStruct->%s[0]);\n' % (stp_list[index]['name'], stp_list[index]['name']))
                        gv_funcs.append('    tmpStr = %s(&pStruct->%s[0], nodeName);\n' % (self._get_gv_func_name(stp_list[index]['type']), stp_list[index]['name']))
                        gv_funcs.append('    stp_strs[%i] = (char*)malloc(256+strlen(tmpStr)+strlen(nodeName)+strlen(myNodeName));\n' % (index))
                        gv_funcs.append('    sprintf(stp_strs[%i], "%%s\\n\\"%%s\\":struct%i -> \\"%%s\\" [];\\n", tmpStr, myNodeName, nodeName);\n' % (index, index))
                    else:
                        gv_funcs.append('    sprintf(nodeName, "%s_%%p", (void*)&pStruct->%s);\n' % (stp_list[index]['name'], stp_list[index]['name']))
                        gv_funcs.append('    tmpStr = %s(&pStruct->%s, nodeName);\n' % (self._get_gv_func_name(stp_list[index]['type']), stp_list[index]['name']))
                        gv_funcs.append('    stp_strs[%i] = (char*)malloc(256+strlen(tmpStr)+strlen(nodeName)+strlen(myNodeName));\n' % (index))
                        gv_funcs.append('    sprintf(stp_strs[%i], "%%s\\n\\"%%s\\":struct%i -> \\"%%s\\" [];\\n", tmpStr, myNodeName, nodeName);\n' % (index, index))
                    total_strlen_str += 'strlen(stp_strs[%i]) + ' % index
            gv_funcs.append('    str = (char*)malloc(%ssizeof(char)*2048);\n' % (total_strlen_str))
            gv_funcs.append('    sprintf(str, "\\"%s\\" [\\nlabel = <<TABLE BORDER=\\"0\\" CELLBORDER=\\"1\\" CELLSPACING=\\"0\\"><TR><TD COLSPAN=\\"2\\">%s (%p)</TD></TR>')
            p_args = ", myNodeName, myNodeName, pStruct"
            for m in sorted(self.struct_dict[s]):
                plabel = ""
                if m in pl_dict:
                    plabel = pl_dict[m]
                (p_out1, p_args1) = self._get_struct_gv_print_formatted(self.struct_dict[s][m], port_label=plabel)
                p_out += p_out1
                p_args += p_args1
            p_out += '</TABLE>>\\n];\\n\\n"'
            p_args += ");\n"
            gv_funcs.append(p_out)
            gv_funcs.append(p_args)
            if 0 != num_stps:
                gv_funcs.append('    for (int32_t stp_index = %i; stp_index >= 0; stp_index--) {\n' % (num_stps-1))
                gv_funcs.append('        if (0 < strlen(stp_strs[stp_index])) {\n')
                gv_funcs.append('            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));\n')
                gv_funcs.append('            free(stp_strs[stp_index]);\n')
                gv_funcs.append('        }\n')
                gv_funcs.append('    }\n')
            gv_funcs.append("    return str;\n}\n")
            if s.lower().strip("_") in array_func_list:
                gv_funcs.append('char* %s_array(XGL_UINT count, const %s* pStruct, const char* myNodeName)\n{\n    char* str;\n    char tmpStr[1024];\n' % (self._get_gv_func_name(s), typedef_fwd_dict[s]))
                gv_funcs.append('    str = (char*)malloc(sizeof(char)*1024*count);\n')
                gv_funcs.append('    sprintf(str, "\\"%s\\" [\\nlabel = <<TABLE BORDER=\\"0\\" CELLBORDER=\\"1\\" CELLSPACING=\\"0\\"><TR><TD COLSPAN=\\"3\\">%s (%p)</TD></TR>", myNodeName, myNodeName, pStruct);\n')
                gv_funcs.append('    for (uint32_t i=0; i < count; i++) {\n')
                gv_funcs.append('        sprintf(tmpStr, "');
                p_args = ""
                p_out = ""
                for m in sorted(self.struct_dict[s]):
                    if 2 == m: # TODO : Hard-coded hack to skip last element of union for xgl_descriptor_slot_info struct
                        continue
                    plabel = ""
                    (p_out1, p_args1) = self._get_struct_gv_print_formatted(self.struct_dict[s][m], port_label=plabel)
                    if 0 == m: # Add array index notation at end of first row (TODO : ROWSPAN# should be dynamic based on number of elements, but hard-coding for now)
                        p_out1 = '%s<TD ROWSPAN=\\"2\\" PORT=\\"slot%%u\\">%%u</TD></TR>' % (p_out1[:-5])
                        p_args1 += ', i, i'
                    p_out += p_out1
                    p_args += p_args1
                p_out += '"'
                p_args += ");\n"
                p_args = p_args.replace('->', '[i].')
                gv_funcs.append(p_out);
                gv_funcs.append(p_args);
                gv_funcs.append('        strncat(str, tmpStr, strlen(tmpStr));\n')
                gv_funcs.append('    }\n')
                gv_funcs.append('    strncat(str, "</TABLE>>\\n];\\n\\n", 20);\n')
                # TODO : Another hard-coded hack.  Tie these slots to "magical" DS0_MEMORY slots that should appear separately
                gv_funcs.append('    for (uint32_t i=0; i < count; i++) {\n')
                gv_funcs.append('        sprintf(tmpStr, "\\"%s\\":slot%u -> \\"DS0_MEMORY\\":slot%u [];\\n", myNodeName, i, i);\n')
                gv_funcs.append('        strncat(str, tmpStr, strlen(tmpStr));\n')
                gv_funcs.append('    }\n')
                gv_funcs.append('    return str;\n}\n')
        # Add function to dynamically print out unknown struct
        gv_funcs.append("char* dynamic_gv_display(const XGL_VOID* pStruct, const char* nodeName)\n{\n")
        gv_funcs.append("    // Cast to APP_INFO ptr initially just to pull sType off struct\n")
        gv_funcs.append("    XGL_STRUCTURE_TYPE sType = ((XGL_APPLICATION_INFO*)pStruct)->sType;\n")
        gv_funcs.append("    switch (sType)\n    {\n")
        for e in enum_type_dict:
            if "_STRUCTURE_TYPE" in e:
                for v in sorted(enum_type_dict[e]):
                    struct_name = v.replace("_STRUCTURE_TYPE", "")
                    print_func_name = self._get_gv_func_name(struct_name)
                    # TODO : Hand-coded fixes for some exceptions
                    if 'XGL_PIPELINE_CB_STATE_CREATE_INFO' in struct_name:
                        struct_name = 'XGL_PIPELINE_CB_STATE'
                    elif 'XGL_SEMAPHORE_CREATE_INFO' in struct_name:
                        struct_name = 'XGL_QUEUE_SEMAPHORE_CREATE_INFO'
                        print_func_name = self._get_gv_func_name(struct_name)
                    elif 'XGL_SEMAPHORE_OPEN_INFO' in struct_name:
                        struct_name = 'XGL_QUEUE_SEMAPHORE_OPEN_INFO'
                        print_func_name = self._get_gv_func_name(struct_name)
                    gv_funcs.append('        case %s:\n' % (v))
                    gv_funcs.append('            return %s((%s*)pStruct, nodeName);\n' % (print_func_name, struct_name))
                    #gv_funcs.append('        }\n')
                    #gv_funcs.append('        break;\n')
                gv_funcs.append("    }\n")
        gv_funcs.append("}")
        return "".join(gv_funcs)





#    def _generateHeader(self):
#        hdr = []
#        hdr.append('digraph g {\ngraph [\nrankdir = "LR"\n];')
#        hdr.append('node [\nfontsize = "16"\nshape = "plaintext"\n];')
#        hdr.append('edge [\n];\n')
#        return "\n".join(hdr)
#        
#    def _generateBody(self):
#        body = []
#        for s in self.struc_dict:
#            field_num = 1
#            body.append('"%s" [\nlabel = <<TABLE BORDER="0" CELLBORDER="1" CELLSPACING="0"> <TR><TD COLSPAN="2" PORT="f0">%s</TD></TR>' % (s, typedef_fwd_dict[s]))
#            for m in sorted(self.struc_dict[s]):
#                body.append('<TR><TD PORT="f%i">%s</TD><TD PORT="f%i">%s</TD></TR>' % (field_num, self.struc_dict[s][m]['full_type'], field_num+1, self.struc_dict[s][m]['name']))
#                field_num += 2
#            body.append('</TABLE>>\n];\n')
#        return "".join(body)

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
    if (opts.abs_out_dir is not None):
        enum_filename = os.path.join(opts.abs_out_dir, os.path.basename(opts.input_file).strip(".h")+"_enum_string_helper.h")
    else:
        enum_filename = os.path.join(os.getcwd(), opts.rel_out_dir, os.path.basename(opts.input_file).strip(".h")+"_enum_string_helper.h")
    enum_filename = os.path.abspath(enum_filename)
    if not os.path.exists(os.path.dirname(enum_filename)):
        print("Creating output dir %s" % os.path.dirname(enum_filename))
        os.mkdir(os.path.dirname(enum_filename))
    if opts.gen_enum_string_helper:
        print("Generating enum string helper to %s" % enum_filename)
        eg = EnumCodeGen(enum_type_dict, enum_val_dict, typedef_fwd_dict, os.path.basename(opts.input_file), enum_filename)
        eg.generateStringHelper()
    #for struct in struct_dict:
    #print(struct)
    if opts.gen_struct_wrappers:
        sw = StructWrapperGen(struct_dict, os.path.basename(opts.input_file).strip(".h"), os.path.dirname(enum_filename))
        #print(sw.get_class_name(struct))
        sw.set_include_headers([os.path.basename(opts.input_file),os.path.basename(enum_filename),"stdint.h","stdio.h","stdlib.h"])
        print("Generating struct wrapper header to %s" % sw.header_filename)
        sw.generateHeader()
        print("Generating struct wrapper class to %s" % sw.class_filename)
        sw.generateBody()
        sw.generateStringHelper()
        # Generate a 2nd helper file that excludes addrs
        sw.set_no_addr(True)
        sw.generateStringHelper()
    if opts.gen_cmake:
        cmg = CMakeGen(sw, os.path.dirname(enum_filename))
        cmg.generate()
    if opts.gen_graphviz:
        gv = GraphVizGen(struct_dict, os.path.basename(opts.input_file).strip(".h"), os.path.dirname(enum_filename))
        gv.set_include_headers([os.path.basename(opts.input_file),os.path.basename(enum_filename),"stdint.h","stdio.h","stdlib.h"])
        gv.generate()
    print("DONE!")
    #print(typedef_rev_dict)
    #print(types_dict)
    #recreate_structs()

if __name__ == "__main__":
    sys.exit(main())
