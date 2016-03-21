#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Khronos Group Inc.
# Copyright (c) 2015-2016 Valve Corporation
# Copyright (c) 2015-2016 LunarG, Inc.
# Copyright (c) 2015-2016 Google Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and/or associated documentation files (the "Materials"), to
# deal in the Materials without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Materials, and to permit persons to whom the Materials
# are furnished to do so, subject to the following conditions:
#
# The above copyright notice(s) and this permission notice shall be included
# in all copies or substantial portions of the Materials.
#
# THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE MATERIALS OR THE
# USE OR OTHER DEALINGS IN THE MATERIALS
#
# Author: Tobin Ehlis <tobin@lunarg.com>

import argparse
import os
import sys
import vulkan
import platform

# vk_layer_documentation_generate.py overview
# This script is intended to generate documentation based on vulkan layers
#  It parses known validation layer headers for details of the validation checks
#  It parses validation layer source files for specific code where checks are implemented
#  structs in a human-readable txt format, as well as utility functions
#  to print enum values as strings

# NOTE : Initially the script is performing validation of a hand-written document
#  Right now it does 3 checks:
#  1. Verify ENUM codes declared in source are documented
#  2. Verify ENUM codes in document are declared in source
#  3. Verify API function names in document are in the actual API header (vulkan.py)
# Currently script will flag errors in all of these cases

# TODO : Need a formal specification of the syntax for doc generation
#  Initially, these are the basics:
#  1. Validation checks have unique ENUM values defined in validation layer header
#  2. ENUM includes comments for 1-line overview of check and more detailed description
#  3. Actual code implementing checks includes ENUM value in callback
#  4. Code to test checks should include reference to ENUM


# TODO : Need list of known validation layers to use as default input
#  Just a couple of flat lists right now, but may need to make this input file
#  or at least a more dynamic data structure
layer_inputs = { 'draw_state' : {'header' : 'layers/core_validation.h',
                                 'source' : 'layers/core_validation.cpp',
                                 'generated' : False,
                                 'error_enum' : 'DRAW_STATE_ERROR'},
                 'shader_checker' : {'header' : 'layers/core_validation.h',
                                 'source' : 'layers/core_validation.cpp',
                                 'generated' : False,
                                 'error_enum' : 'SHADER_CHECKER_ERROR'},
                 'mem_tracker' : {'header' : 'layers/core_validation.h',
                                  'source' : 'layers/core_validation.cpp',
                                  'generated' : False,
                                  'error_enum' : 'MEM_TRACK_ERROR'},
                 'threading' : {'header' : 'layers/threading.h',
                                'source' : 'dbuild/layers/threading.cpp',
                                'generated' : True,
                                'error_enum' : 'THREADING_CHECKER_ERROR'},
                 'object_tracker' : {'header' : 'layers/object_tracker.h',
                                'source' : 'dbuild/layers/object_tracker.cpp',
                                'generated' : True,
                                'error_enum' : 'OBJECT_TRACK_ERROR',},
                 'device_limits' : {'header' : 'layers/device_limits.h',
                                    'source' : 'layers/device_limits.cpp',
                                    'generated' : False,
                                    'error_enum' : 'DEV_LIMITS_ERROR',},
                 'image' : {'header' : 'layers/image.h',
                            'source' : 'layers/image.cpp',
                            'generated' : False,
                            'error_enum' : 'IMAGE_ERROR',},
                 'swapchain' : {'header' : 'layers/swapchain.h',
                            'source' : 'layers/swapchain.cpp',
                            'generated' : False,
                            'error_enum' : 'SWAPCHAIN_ERROR',},
    }

builtin_headers = [layer_inputs[ln]['header'] for ln in layer_inputs]
builtin_source = [layer_inputs[ln]['source'] for ln in layer_inputs]

# List of extensions in layers that are included in documentation, but not in vulkan.py API set
layer_extension_functions = ['objTrackGetObjects', 'objTrackGetObjectsOfType']

def handle_args():
    parser = argparse.ArgumentParser(description='Generate layer documenation from source.')
    parser.add_argument('--in_headers', required=False, default=builtin_headers, help='The input layer header files from which code will be generated.')
    parser.add_argument('--in_source', required=False, default=builtin_source, help='The input layer source files from which code will be generated.')
    parser.add_argument('--layer_doc', required=False, default='layers/vk_validation_layer_details.md', help='Existing layer document to be validated against actual layers.')
    parser.add_argument('--validate', action='store_true', default=False, help='Validate that there are no mismatches between layer documentation and source. This includes cross-checking the validation checks, and making sure documented Vulkan API calls exist.')
    parser.add_argument('--print_structs', action='store_true', default=False, help='Primarily a debug option that prints out internal data structs used to generate layer docs.')
    parser.add_argument('--print_doc_checks', action='store_true', default=False, help='Primarily a debug option that prints out all of the checks that are documented.')
    return parser.parse_args()

# Little helper class for coloring cmd line output
class bcolors:

    def __init__(self):
        self.GREEN = '\033[0;32m'
        self.RED = '\033[0;31m'
        self.ENDC = '\033[0m'
        if 'Linux' != platform.system():
            self.GREEN = ''
            self.RED = ''
            self.ENDC = ''

    def green(self):
        return self.GREEN

    def red(self):
        return self.RED

    def endc(self):
        return self.ENDC

# Class to parse the layer source code and store details in internal data structs
class LayerParser:
    def __init__(self, header_file_list, source_file_list):
        self.header_files = header_file_list
        self.source_files = source_file_list
        self.layer_dict = {}
        self.api_dict = {}

    # Parse layer header files into internal dict data structs
    def parse(self):
        # For each header file, parse details into dicts
        # TODO : Should have a global dict element to track overall list of checks
        store_enum = False
        for layer_name in layer_inputs:
            hf = layer_inputs[layer_name]['header']
            self.layer_dict[layer_name] = {} # initialize a new dict for this layer
            self.layer_dict[layer_name]['CHECKS'] = [] # enum of checks is stored in a list
            #print('Parsing header file %s as layer name %s' % (hf, layer_name))
            with open(hf) as f:
                for line in f:
                    if True in [line.strip().startswith(comment) for comment in ['//', '/*']]:
                        #print("Skipping comment line: %s" % line)
                        # For now skipping lines starting w/ comment, may use these to capture
                        #  documentation in the future
                        continue

                    # Find enums
                    if store_enum:
                        if '}' in line: # we're done with enum definition
                            store_enum = False
                            continue
                        # grab the enum name as a unique check
                        if ',' in line:
                            # TODO : When documentation for a check is contained in the source,
                            #  this is where we should also capture that documentation so that
                            #  it can then be transformed into desired doc format
                            enum_name = line.split(',')[0].strip()
                            # Flag an error if we have already seen this enum
                            if enum_name in self.layer_dict[layer_name]['CHECKS']:
                                print('ERROR : % layer has duplicate error enum: %s' % (layer_name, enum_name))
                            self.layer_dict[layer_name]['CHECKS'].append(enum_name)
                    # If the line includes 'typedef', 'enum', and the expected enum name, start capturing enums
                    if False not in [ex in line for ex in ['typedef', 'enum', layer_inputs[layer_name]['error_enum']]]:
                        store_enum = True

        # For each source file, parse into dicts
        for sf in self.source_files:
            #print('Parsing source file %s' % sf)
            pass
            # TODO : In the source file we want to see where checks actually occur
            #  Need to build function tree of checks so that we know all of the
            #  checks that occur under a top-level Vulkan API call
            #  Eventually in the validation we can flag ENUMs that aren't being
            #  used in the source, and we can document source code lines as well
            #  as Vulkan API calls where each specific ENUM check is made

    def print_structs(self):
        print('This is where I print the data structs')
        for layer in self.layer_dict:
            print('Layer %s has %i checks:\n%s' % (layer, len(self.layer_dict[layer]['CHECKS'])-1, "\n\t".join(self.layer_dict[layer]['CHECKS'])))

# Class to parse hand-written md layer documentation into a dict and then validate its contents
class LayerDoc:
    def __init__(self, source_file):
        self.layer_doc_filename = source_file
        self.txt_color = bcolors()
        # Main data struct to store info from layer doc
        self.layer_doc_dict = {}
        # Comprehensive list of all validation checks recorded in doc
        self.enum_list = []

    # Parse the contents of doc into data struct
    def parse(self):
        layer_name = 'INIT'
        parse_layer_details = False
        detail_trigger = '| Check | '
        parse_pending_work = False
        pending_trigger = ' Pending Work'
        parse_overview = False
        overview_trigger = ' Overview'
        enum_prefix = ''

        with open(self.layer_doc_filename) as f:
            for line in f:
                if parse_pending_work:
                    if '.' in line and line.strip()[0].isdigit():
                        todo_item = line.split('.')[1].strip()
                        self.layer_doc_dict[layer_name]['pending'].append(todo_item)
                if pending_trigger in line and '##' in line:
                    parse_layer_details = False
                    parse_pending_work = True
                    parse_overview = False
                    self.layer_doc_dict[layer_name]['pending'] = []
                if parse_layer_details:
                    # Grab details but skip the fomat line with a bunch of '-' chars
                    if '|' in line and line.count('-') < 20:
                        detail_sections = line.split('|')
                        #print("Details elements from line %s: %s" % (line, detail_sections))
                        check_name = '%s%s' % (enum_prefix, detail_sections[3].strip())
                        if '_NA' in check_name:
                            # TODO : Should clean up these NA checks in the doc, skipping them for now
                            continue
                        self.enum_list.append(check_name)
                        self.layer_doc_dict[layer_name][check_name] = {}
                        self.layer_doc_dict[layer_name][check_name]['summary_txt'] = detail_sections[1].strip()
                        self.layer_doc_dict[layer_name][check_name]['details_txt'] = detail_sections[2].strip()
                        self.layer_doc_dict[layer_name][check_name]['api_list'] = detail_sections[4].split()
                        self.layer_doc_dict[layer_name][check_name]['tests'] = detail_sections[5].split()
                        self.layer_doc_dict[layer_name][check_name]['notes'] = detail_sections[6].strip()
                        # strip any unwanted commas from api and test names
                        self.layer_doc_dict[layer_name][check_name]['api_list'] = [a.strip(',') for a in self.layer_doc_dict[layer_name][check_name]['api_list']]
                        self.layer_doc_dict[layer_name][check_name]['tests'] = [a.strip(',') for a in self.layer_doc_dict[layer_name][check_name]['tests']]
                # Trigger details parsing when we have table header
                if detail_trigger in line:
                    parse_layer_details = True
                    parse_pending_work = False
                    parse_overview = False
                    enum_txt = line.split('|')[3]
                    if '*' in enum_txt:
                        enum_prefix = enum_txt.split()[-1].strip('*').strip()
                        #print('prefix: %s' % enum_prefix)
                if parse_overview:
                    self.layer_doc_dict[layer_name]['overview'] += line
                if overview_trigger in line and '##' in line:
                    parse_layer_details = False
                    parse_pending_work = False
                    parse_overview = True
                    layer_name = line.split()[1]
                    self.layer_doc_dict[layer_name] = {}
                    self.layer_doc_dict[layer_name]['overview'] = ''

    # Verify that checks and api references in layer doc match reality
    #  Report API calls from doc that are not found in API
    #  Report checks from doc that are not in actual layers
    #  Report checks from layers that are not captured in doc
    def validate(self, layer_dict):
        # Count number of errors found and return it
        errors_found = 0
        # First we'll go through the doc datastructures and flag any issues
        for chk in self.enum_list:
            doc_layer_found = False
            for real_layer in layer_dict:
                if chk in layer_dict[real_layer]['CHECKS']:
                    #print('Found actual layer check %s in doc' % (chk))
                    doc_layer_found = True
                    continue
            if not doc_layer_found:
                print(self.txt_color.red() + 'Actual layers do not contain documented check: %s' % (chk) + self.txt_color.endc())
                errors_found += 1
        # Now go through API names in doc and verify they're real
        # First we're going to transform proto names from vulkan.py into single list
        core_api_names = [p.name for p in vulkan.core.protos]
        wsi_s_names = [p.name for p in vulkan.ext_khr_surface.protos]
        wsi_ds_names = [p.name for p in vulkan.ext_khr_device_swapchain.protos]
        dbg_rpt_names = [p.name for p in vulkan.lunarg_debug_report.protos]
        api_names = core_api_names + wsi_s_names + wsi_ds_names + dbg_rpt_names
        for ln in self.layer_doc_dict:
            for chk in self.layer_doc_dict[ln]:
                if chk in ['overview', 'pending']:
                    continue
                for api in self.layer_doc_dict[ln][chk]['api_list']:
                    if api[2:] not in api_names and api not in layer_extension_functions:
                        print(self.txt_color.red() + 'Doc references invalid function: %s' % (api) + self.txt_color.endc())
                        errors_found += 1
        # Now go through all of the actual checks in the layers and make sure they're covered in the doc
        for ln in layer_dict:
            for chk in layer_dict[ln]['CHECKS']:
                if chk not in self.enum_list:
                    print(self.txt_color.red() + 'Doc is missing check: %s' % (chk) + self.txt_color.endc())
                    errors_found += 1

        return errors_found

    # Print all of the checks captured in the doc
    def print_checks(self):
        print('Checks captured in doc:\n%s' % ('\n\t'.join(self.enum_list)))

def main(argv=None):
    # Parse args
    opts = handle_args()
    # Create parser for layer files
    layer_parser = LayerParser(opts.in_headers, opts.in_source)
    # Parse files into internal data structs
    layer_parser.parse()

    # Generate requested types of output
    if opts.print_structs: # Print details of internal data structs
        layer_parser.print_structs()

    layer_doc = LayerDoc(opts.layer_doc)
    layer_doc.parse()
    if opts.print_doc_checks:
        layer_doc.print_checks()

    if opts.validate:
        num_errors = layer_doc.validate(layer_parser.layer_dict)
        if (0 == num_errors):
            txt_color = bcolors()
            print(txt_color.green() + 'No mismatches found between %s and implementation' % (os.path.basename(opts.layer_doc)) + txt_color.endc())
        else:
            return num_errors
    return 0

if __name__ == "__main__":
    sys.exit(main())

