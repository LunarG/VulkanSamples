#!/usr/bin/env python3
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
layer_inputs = { 'draw_state' : {'header' : 'layers/core_validation_error_enums.h',
                                 'source' : 'layers/core_validation.cpp',
                                 'generated' : False,
                                 'error_enum' : 'DRAW_STATE_ERROR'},
                 'shader_checker' : {'header' : 'layers/core_validation_error_enums.h',
                                 'source' : 'layers/core_validation.cpp',
                                 'generated' : False,
                                 'error_enum' : 'SHADER_CHECKER_ERROR'},
                 'mem_tracker' : {'header' : 'layers/core_validation_error_enums.h',
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
                 'parameter_validation' : {'header' : 'layers/parameter_validation_utils.h',
                                           'source' : 'layers/parameter_validation.cpp',
                                           'generated' : False,
                                           'error_enum' : 'ErrorCode',},
    }

builtin_headers = [layer_inputs[ln]['header'] for ln in layer_inputs]
builtin_source = [layer_inputs[ln]['source'] for ln in layer_inputs]
builtin_tests = ['tests/layer_validation_tests.cpp', ]

# List of extensions in layers that are included in documentation, but not in vulkan.py API set
layer_extension_functions = ['objTrackGetObjects', 'objTrackGetObjectsOfType']

def handle_args():
    parser = argparse.ArgumentParser(description='Generate layer documenation from source.')
    parser.add_argument('--in_headers', required=False, default=builtin_headers, help='The input layer header files from which code will be generated.')
    parser.add_argument('--in_source', required=False, default=builtin_source, help='The input layer source files from which code will be generated.')
    parser.add_argument('--test_source', required=False, default=builtin_tests, help='The input test source files from which code will be generated.')
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
        self.YELLOW = '\033[1;33m'
        self.ENDC = '\033[0m'
        if 'Linux' != platform.system():
            self.GREEN = ''
            self.RED = ''
            self.YELLOW = ''
            self.ENDC = ''

    def green(self):
        return self.GREEN

    def red(self):
        return self.RED

    def yellow(self):
        return self.YELLOW

    def endc(self):
        return self.ENDC

# Class to parse the validation layer test source and store testnames
class TestParser:
    def __init__(self, test_file_list, test_group_name=['VkLayerTest', 'VkWsiEnabledLayerTest']):
        self.test_files = test_file_list
        self.tests_set = set()
        self.test_trigger_txt_list = []
        for tg in test_group_name:
            self.test_trigger_txt_list.append('TEST_F(%s' % tg)
            #print('Test trigger test list: %s' % (self.test_trigger_txt_list))

    # Parse test files into internal data struct
    def parse(self):
        # For each test file, parse test names into set
        grab_next_line = False # handle testname on separate line than wildcard
        for test_file in self.test_files:
            with open(test_file) as tf:
                for line in tf:
                    if True in [line.strip().startswith(comment) for comment in ['//', '/*']]:
                        continue

                    if True in [ttt in line for ttt in self.test_trigger_txt_list]:
                        #print('Test wildcard in line: %s' % (line))
                        testname = line.split(',')[-1]
                        testname = testname.strip().strip(' {)')
                        #print('Inserting test: "%s"' % (testname))
                        if ('' == testname):
                            grab_next_line = True
                            continue
                        self.tests_set.add(testname)
                    if grab_next_line: # test name on its own line
                        grab_next_line = False
                        testname = testname.strip().strip(' {)')
                        self.tests_set.add(testname)

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
                    # If the line includes 'enum' and the expected enum name, start capturing enums
                    if False not in [ex in line for ex in ['enum', layer_inputs[layer_name]['error_enum']]]:
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
                        test_list = [a.strip(',') for a in self.layer_doc_dict[layer_name][check_name]['tests']]
                        self.layer_doc_dict[layer_name][check_name]['tests'] = [a.split('.')[-1] for a in test_list]
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

    # Verify that checks, tests and api references in layer doc match reality
    #  Report API calls from doc that are not found in API
    #  Report checks from doc that are not in actual layers
    #  Report checks from layers that are not captured in doc
    #  Report checks from doc that do not have a valid test
    def validate(self, layer_dict, tests_set):
        #print("tests_set: %s" % (tests_set))
        # Count number of errors found and return it
        errors_found = 0
        warnings_found = 0
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
                # For now warn on missing or invalid tests
                for test in self.layer_doc_dict[ln][chk]['tests']:
                    if '*' in test:
                        # naive way to handle wildcards, just make sure we have matches on parts
                        test_parts = test.split('*')
                        for part in test_parts:
                            part_found = False
                            for t in tests_set:
                                if part in t:
                                    part_found = True
                                    break
                            if not part_found:
                                print(self.txt_color.red() + 'Validation check %s has missing or invalid test : %s' % (chk, test))
                                errors_found += 1
                                break
                    elif test not in tests_set and not chk.endswith('_NONE'):
                        if test == 'TODO':
                            warnings_found += 1
                        else:
                            print(self.txt_color.red() + 'Validation check %s has missing or invalid test : %s' % (chk, test))
                            errors_found += 1
        # Now go through all of the actual checks in the layers and make sure they're covered in the doc
        for ln in layer_dict:
            for chk in layer_dict[ln]['CHECKS']:
                if chk not in self.enum_list:
                    print(self.txt_color.red() + 'Doc is missing check: %s' % (chk) + self.txt_color.endc())
                    errors_found += 1

        return (errors_found, warnings_found)

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
    # Parse test files
    test_parser = TestParser(opts.test_source)
    test_parser.parse()

    # Generate requested types of output
    if opts.print_structs: # Print details of internal data structs
        layer_parser.print_structs()

    layer_doc = LayerDoc(opts.layer_doc)
    layer_doc.parse()
    if opts.print_doc_checks:
        layer_doc.print_checks()

    if opts.validate:
        (num_errors, num_warnings) = layer_doc.validate(layer_parser.layer_dict, test_parser.tests_set)
        txt_color = bcolors()
        if (0 == num_warnings):
            print(txt_color.green() + 'No warning cases found between %s and implementation' % (os.path.basename(opts.layer_doc)) + txt_color.endc())
        else:
            print(txt_color.yellow() + 'Found %s warnings due to missing tests. Missing tests are labeled as "TODO" in "%s."' % (num_warnings, opts.layer_doc))
        if (0 == num_errors):
            print(txt_color.green() + 'No mismatches found between %s and implementation' % (os.path.basename(opts.layer_doc)) + txt_color.endc())
        else:
            return num_errors
    return 0

if __name__ == "__main__":
    sys.exit(main())

