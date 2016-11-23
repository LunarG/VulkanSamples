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
# Author: Tobin Ehlis <tobine@google.com>

import argparse
import os
import sys
import platform

# vk_validation_stats.py overview
# This script is intended to generate statistics on the state of validation code
#  based on information parsed from the source files and the database file
# Here's what it currently does:
#  1. Parse vk_validation_error_database.txt to store claimed state of validation checks
#  2. Parse vk_validation_error_messages.h to verify the actual checks in header vs. the
#     claimed state of the checks
#  3. Parse source files to identify which checks are implemented and verify that this
#     exactly matches the list of checks claimed to be implemented in the database
#  4. Parse test file(s) and verify that reported tests exist
#  5. Report out stats on number of checks, implemented checks, and duplicated checks
#
# TODO:
#  1. Would also like to report out number of existing checks that don't yet use new, unique enum
#  2. Could use notes to store custom fields (like TODO) and print those out here
#  3. Update test code to check if tests use new, unique enums to check for errors instead of strings

db_file = 'vk_validation_error_database.txt'
layer_source_files = [
'core_validation.cpp',
'descriptor_sets.cpp',
'parameter_validation.cpp',
'object_tracker.cpp',
'image.cpp'
]
header_file = 'vk_validation_error_messages.h'
# TODO : Don't hardcode linux path format if we want this to run on windows
test_file = '../tests/layer_validation_tests.cpp'


class ValidationDatabase:
    def __init__(self, filename=db_file):
        self.db_file = filename
        self.delimiter = '~^~'
        self.db_dict = {} # complete dict of all db values per error enum
        # specialized data structs with slices of complete dict
        self.db_implemented_enums = [] # list of all error enums claiming to be implemented in database file
        self.db_enum_to_tests = {} # dict where enum is key to lookup list of tests implementing the enum
        #self.src_implemented_enums
    def read(self):
        """Read a database file into internal data structures, format of each line is <enum><implemented Y|N?><testname><api><errormsg><notes>"""
        #db_dict = {} # This is a simple db of just enum->errormsg, the same as is created from spec
        #max_id = 0
        with open(self.db_file, "r") as infile:
            for line in infile:
                line = line.strip()
                if line.startswith('#') or '' == line:
                    continue
                db_line = line.split(self.delimiter)
                if len(db_line) != 6:
                    print "ERROR: Bad database line doesn't have 6 elements: %s" % (line)
                error_enum = db_line[0]
                implemented = db_line[1]
                testname = db_line[2]
                api = db_line[3]
                error_str = db_line[4]
                note = db_line[5]
                # Read complete database contents into our class var for later use
                self.db_dict[error_enum] = {}
                self.db_dict[error_enum]['check_implemented'] = implemented
                self.db_dict[error_enum]['testname'] = testname
                self.db_dict[error_enum]['api'] = api
                self.db_dict[error_enum]['error_string'] = error_str
                self.db_dict[error_enum]['note'] = note
                # Now build custom data structs
                if 'Y' == implemented:
                    self.db_implemented_enums.append(error_enum)
                if testname.lower() not in ['unknown', 'none']:
                    self.db_enum_to_tests[error_enum] = testname.split(',')
                    #if len(self.db_enum_to_tests[error_enum]) > 1:
                    #    print "Found check %s that has multiple tests: %s" % (error_enum, self.db_enum_to_tests[error_enum])
                    #else:
                    #    print "Check %s has single test: %s" % (error_enum, self.db_enum_to_tests[error_enum])
                #unique_id = int(db_line[0].split('_')[-1])
                #if unique_id > max_id:
                #    max_id = unique_id
        #print "Found %d total enums in database" % (len(self.db_dict.keys()))
        #print "Found %d enums claiming to be implemented in source" % (len(self.db_implemented_enums))
        #print "Found %d enums claiming to have tests implemented" % (len(self.db_enum_to_tests.keys()))

class ValidationHeader:
    def __init__(self, filename=header_file):
        self.filename = header_file
        self.enums = []
    def read(self):
        """Read unique error enum header file into internal data structures"""
        grab_enums = False
        with open(self.filename, "r") as infile:
            for line in infile:
                line = line.strip()
                if 'enum UNIQUE_VALIDATION_ERROR_CODE {' in line:
                    grab_enums = True
                    continue
                if grab_enums:
                    if 'VALIDATION_ERROR_MAX_ENUM' in line:
                        grab_enums = False
                        break # done
                    if 'VALIDATION_ERROR_' in line:
                        enum = line.split(' = ')[0]
                        self.enums.append(enum)
        #print "Found %d error enums. First is %s and last is %s." % (len(self.enums), self.enums[0], self.enums[-1])

class ValidationSource:
    def __init__(self, source_file_list):
        self.source_files = source_file_list
        self.enum_count_dict = {} # dict of enum values to the count of how much they're used
        # 1790 is a special case that provides an exception when an extension is enabled. No specific error is flagged, but the exception is handled so add it here
        self.enum_count_dict['VALIDATION_ERROR_01790'] = 1
    def parse(self):
        duplicate_checks = 0
        for sf in self.source_files:
            with open(sf) as f:
                for line in f:
                    if True in [line.strip().startswith(comment) for comment in ['//', '/*']]:
                        continue
                    # Find enums
                    #if 'VALIDATION_ERROR_' in line and True not in [ignore in line for ignore in ['[VALIDATION_ERROR_', 'UNIQUE_VALIDATION_ERROR_CODE']]:
                    if 'VALIDATION_ERROR_' in line and 'UNIQUE_VALIDATION_ERROR_CODE' not in line:
                        # Need to isolate the validation error enum
                        #print("Line has check:%s" % (line))
                        line_list = line.split()
                        enum = ''
                        for str in line_list:
                            if 'VALIDATION_ERROR_' in str and '[VALIDATION_ERROR_' not in str:
                                enum = str.strip(',);')
                                break
                        if enum != '':
                            if enum not in self.enum_count_dict:
                                self.enum_count_dict[enum] = 1
                                #print "Found enum %s implemented for first time in file %s" % (enum, sf)
                            else:
                                self.enum_count_dict[enum] = self.enum_count_dict[enum] + 1
                                #print "Found enum %s implemented for %d time in file %s" % (enum, self.enum_count_dict[enum], sf)
                                duplicate_checks = duplicate_checks + 1
                        #else:
                            #print("Didn't find actual check in line:%s" % (line))
        #print "Found %d unique implemented checks and %d are duplicated at least once" % (len(self.enum_count_dict.keys()), duplicate_checks)

# Class to parse the validation layer test source and store testnames
# TODO: Enhance class to detect use of unique error enums in the test
class TestParser:
    def __init__(self, test_file_list, test_group_name=['VkLayerTest', 'VkPositiveLayerTest', 'VkWsiEnabledLayerTest']):
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
    def __init__(self, test_file_list, test_group_name=['VkLayerTest', 'VkPositiveLayerTest', 'VkWsiEnabledLayerTest']):
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

def main(argv=None):
    # parse db
    val_db = ValidationDatabase()
    val_db.read()
    # parse header
    val_header = ValidationHeader()
    val_header.read()
    # Create parser for layer files
    val_source = ValidationSource(layer_source_files)
    val_source.parse()
    # Parse test files
    test_parser = TestParser([test_file, ])
    test_parser.parse()

    # Process stats - Just doing this inline in main, could make a fancy class to handle
    #   all the processing of data and then get results from that
    txt_color = bcolors()
    print("Validation Statistics")
    # First give number of checks in db & header and report any discrepancies
    db_enums = len(val_db.db_dict.keys())
    hdr_enums = len(val_header.enums)
    print(" Database file includes %d unique checks" % (db_enums))
    print(" Header file declares %d unique checks" % (hdr_enums))
    tmp_db_dict = val_db.db_dict
    db_missing = []
    for enum in val_header.enums:
        if not tmp_db_dict.pop(enum, False):
            db_missing.append(enum)
    if db_enums == hdr_enums and len(db_missing) == 0 and len(tmp_db_dict.keys()) == 0:
        print(txt_color.green() + "  Database and Header match, GREAT!" + txt_color.endc())
    else:
        print(txt_color.red() + "  Uh oh, Database doesn't match Header :(" + txt_color.endc())
        if len(db_missing) != 0:
            print(txt_color.red() + "   The following checks are in header but missing from database:" + txt_color.endc())
            for missing_enum in db_missing:
                print(txt_color.red() + "    %s" % (missing_enum) + txt_color.endc())
        if len(tmp_db_dict.keys()) != 0:
            print(txt_color.red() + "   The following checks are in database but haven't been declared in the header:" + txt_color.endc())
            for extra_enum in tmp_db_dict:
                print(txt_color.red() + "    %s" % (extra_enum) + txt_color.endc())
    # Report out claimed implemented checks vs. found actual implemented checks
    imp_not_found = [] # Checks claimed to implemented in DB file but no source found
    imp_not_claimed = [] # Checks found implemented but not claimed to be in DB
    multiple_uses = False # Flag if any enums are used multiple times
    for db_imp in val_db.db_implemented_enums:
        if db_imp not in val_source.enum_count_dict:
            imp_not_found.append(db_imp)
    for src_enum in val_source.enum_count_dict:
        if val_source.enum_count_dict[src_enum] > 1:
            multiple_uses = True
        if src_enum not in val_db.db_implemented_enums:
            imp_not_claimed.append(src_enum)
    print(" Database file claims that %d checks (%s) are implemented in source." % (len(val_db.db_implemented_enums), "{0:.0f}%".format(float(len(val_db.db_implemented_enums))/db_enums * 100)))
    if len(imp_not_found) == 0 and len(imp_not_claimed) == 0:
        print(txt_color.green() + "  All claimed Database implemented checks have been found in source, and no source checks aren't claimed in Database, GREAT!" + txt_color.endc())
    else:
        print(txt_color.red() + "  Uh oh, Database claimed implemented don't match Source :(" + txt_color.endc())
        if len(imp_not_found) != 0:
            print(txt_color.red() + "   The following %d checks are claimed to be implemented in Database, but weren't found in source:" % (len(imp_not_found)) + txt_color.endc())
            for not_imp_enum in imp_not_found:
                print(txt_color.red() + "    %s" % (not_imp_enum) + txt_color.endc())
        if len(imp_not_claimed) != 0:
            print(txt_color.red() + "   The following checks are implemented in source, but not claimed to be in Database:" + txt_color.endc())
            for imp_enum in imp_not_claimed:
                print(txt_color.red() + "    %s" % (imp_enum) + txt_color.endc())
    if multiple_uses:
        print(txt_color.yellow() + "  Note that some checks are used multiple times. These may be good candidates for new valid usage spec language." + txt_color.endc())
        print(txt_color.yellow() + "  Here is a list of each check used multiple times with its number of uses:" + txt_color.endc())
        for enum in val_source.enum_count_dict:
            if val_source.enum_count_dict[enum] > 1:
                print(txt_color.yellow() + "   %s: %d" % (enum, val_source.enum_count_dict[enum]) + txt_color.endc())
    # Now check that tests claimed to be implemented are actual test names
    bad_testnames = []
    for enum in val_db.db_enum_to_tests:
        for testname in val_db.db_enum_to_tests[enum]:
            if testname not in test_parser.tests_set:
                bad_testnames.append(testname)
    print(" Database file claims that %d checks have tests written." % len(val_db.db_enum_to_tests))
    if len(bad_testnames) == 0:
        print(txt_color.green() + "  All claimed tests have valid names. That's good!" + txt_color.endc())
    else:
        print(txt_color.red() + "  The following testnames in Database appear to be invalid:")
        for bt in bad_testnames:
            print(txt_color.red() + "   %s" % (bt) + txt_color.endc())

    return 0

if __name__ == "__main__":
    sys.exit(main())

