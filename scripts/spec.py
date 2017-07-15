#!/usr/bin/python -i

import sys
try:
    import urllib.request as urllib2
except ImportError:
    import urllib2
from bs4 import BeautifulSoup
import json
import vuid_mapping

#############################
# spec.py script
#
# Overview - this script is intended to generate validation error codes and message strings from the json spec file
#  that contains all of the valid usage statements. In addition to generating the header file, it provides a number of
#  corrollary services to aid in generating/updating the header.
#
# Ideal flow - Pull the valid usage text and IDs from the spec json, pull the IDs from the validation error database,
#  then update the database with any new IDs from the json file and generate new database and header file.
#
# TODO:
#  1. When VUs go away (in error DB, but not in json) need to report them and remove from DB as deleted
#
#############################


out_filename = "../layers/vk_validation_error_messages.h" # can override w/ '-out <filename>' option
db_filename = "../layers/vk_validation_error_database.txt" # can override w/ '-gendb <filename>' option
json_filename = None # con pass in w/ '-json <filename> option
gen_db = False # set to True when '-gendb <filename>' option provided
json_compare = False # compare existing DB to json file input
json_url = "https://www.khronos.org/registry/vulkan/specs/1.0-extensions/validation/validusage.json"
read_json = False
# This is the root spec link that is used in error messages to point users to spec sections
#old_spec_url = "https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html"
spec_url = "https://www.khronos.org/registry/vulkan/specs/1.0-extensions/html/vkspec.html"
core_url = "https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html"
ext_url = "https://www.khronos.org/registry/vulkan/specs/1.0-extensions/html/vkspec.html"
# After the custom validation error message, this is the prefix for the standard message that includes the
#  spec valid usage language as well as the link to nearest section of spec to that language
error_msg_prefix = "The spec valid usage text states "
validation_error_enum_name = "VALIDATION_ERROR_"

def printHelp():
    print ("Usage: python spec.py [-out <headerfile.h>] [-gendb <databasefile.txt>] [-update] [-json <json_file>] [-help]")
    print ("\n Default script behavior is to parse the specfile and generate a header of unique error enums and corresponding error messages based on the specfile.\n")
    print ("  Default specfile is from online at %s" % (spec_url))
    print ("  Default headerfile is %s" % (out_filename))
    print ("  Default databasefile is %s" % (db_filename))
    print ("\nIf '-gendb' option is specified then a database file is generated to default file or <databasefile.txt> if supplied. The database file stores")
    print ("  the list of enums and their error messages.")
    print ("\nIf '-update' option is specified this triggers the master flow to automate updating header and database files using default db file as baseline")
    print ("  and online spec file as the latest. The default header and database files will be updated in-place for review and commit to the git repo.")
    print ("\nIf '-json' option is used trigger the script to load in data from a json file.")
    print ("\nIf '-json-file' option is it will point to a local json file, else '%s' is used from the web." % (json_url))

def get8digithex(dec_num):
    """Convert a decimal # into an 8-digit hex"""
    if dec_num > 4294967295:
        print ("ERROR: Decimal # %d can't be represented in 8 hex digits" % (dec_num))
        sys.exit()
    hex_num = hex(dec_num)
    return hex_num[2:].zfill(8)

class Specification:
    def __init__(self):
        self.tree   = None
        self.error_db_dict = {} # dict of previous error values read in from database file
        self.delimiter = '~^~' # delimiter for db file
        # Global dicts used for tracking spec updates from old to new VUs
        self.orig_no_link_msg_dict = {} # Pair of API,Original msg w/o spec link to ID list mapping
        self.orig_core_msg_dict = {} # Pair of API,Original core msg (no link or section) to ID list mapping
        self.last_mapped_id = -10 # start as negative so we don't hit an accidental sequence
        self.orig_test_imp_enums = set() # Track old enums w/ tests and/or implementation to flag any that aren't carried fwd
        # Dict of data from json DB
        # Key is API,<short_msg> which leads to dict w/ following values
        #   'ext' -> <core|<ext_name>>
        #   'string_vuid' -> <string_vuid>
        #   'number_vuid' -> <numerical_vuid>
        self.json_db = {}
        self.json_missing = 0
        self.struct_to_func_map = {} # Map structs to the API func that they fall under in the spec
        self.duplicate_json_key_count = 0
        self.copyright = """/* THIS FILE IS GENERATED.  DO NOT EDIT. */

/*
 * Vulkan
 *
 * Copyright (c) 2016 Google Inc.
 * Copyright (c) 2016 LunarG, Inc.
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
 */"""

    def readJSON(self):
        """Read in JSON file"""
        if json_filename is not None:
            with open(json_filename) as jsf:
                self.json_data = json.load(jsf, encoding='utf-8')
        else:
            response = urllib2.urlopen(json_url).read().decode('utf-8')
            self.json_data = json.loads(response)

    def parseJSON(self):
        """Parse JSON VUIDs into data struct"""
        # Format of JSON file is:
        # "API": { "core|EXT": [ {"vuid": "<id>", "text": "<VU txt>"}]},
        # "VK_KHX_external_memory" & "VK_KHX_device_group" - extension case (vs. "core")
        for top_level in sorted(self.json_data):
            if "validation" == top_level:
                for api in sorted(self.json_data[top_level]):
                    for ext in sorted(self.json_data[top_level][api]):
                        for vu_txt_dict in self.json_data[top_level][api][ext]:
                            print ("Looking at dict for api:ext entry %s:%s" % (api, ext))
                            vuid = vu_txt_dict['vuid']
                            vutxt = vu_txt_dict['text']
                            #print ("%s:%s:%s:%s" % (api, ext, vuid, vutxt))
                            #print ("VUTXT orig:%s" % (vutxt))
                            just_txt = BeautifulSoup(vutxt, 'html.parser')
                            #print ("VUTXT only:%s" % (just_txt.get_text()))
                            num_vuid = vuid_mapping.convertVUID(vuid)
                            self.json_db[vuid] = {}
                            self.json_db[vuid]['ext'] = ext
                            self.json_db[vuid]['number_vuid'] = num_vuid
                            self.json_db[vuid]['struct_func'] = api
                            just_txt = just_txt.get_text().strip()
                            unicode_map = {
                            u"\u2019" : "'",
                            u"\u2192" : "->",
                            }
                            for um in unicode_map:
                                just_txt = just_txt.replace(um, unicode_map[um])
                            self.json_db[vuid]['vu_txt'] = just_txt.replace("\\", "")
                            print ("Spec vu txt:%s" % (self.json_db[vuid]['vu_txt']))
        #sys.exit()

    def compareJSON(self):
        """Compare parsed json file with existing data read in from DB file"""
        json_db_set = set()
        for vuid in self.json_db: # pull entries out and see which fields we're missing from error_db
            json_db_set.add(vuid)
        for enum in self.error_db_dict:
            vuid_string = self.error_db_dict[enum]['vuid_string']
            if vuid_string not in self.json_db:
                #print ("Full string for %s is:%s" % (enum, full_error_string))
                print ("WARN: Couldn't find vuid_string in json db:%s" % (vuid_string))
                self.json_missing = self.json_missing + 1
                self.error_db_dict[enum]['ext'] = 'core'
                # TODO: Currently GL843 tracks 2 VUs that are missing from json incorrectly
                #  Fix will land in 1.0.51 spec. After that we should take some alternative
                #  action here to indicate that VUs have gone away.
                #  Can have a removed_enums set that we add to and report to user
                #sys.exit()
            else:
                json_db_set.remove(vuid_string)
                self.error_db_dict[enum]['ext'] = self.json_db[vuid_string]['ext']
                if 'core' == self.json_db[vuid_string]['ext'] or '!' in self.json_db[vuid_string]['ext']:
                    spec_link = "%s#%s" % (core_url, vuid_string)
                else:
                    spec_link = "%s#%s" % (ext_url, vuid_string)
                self.error_db_dict[enum]['error_msg'] = "%s'%s' (%s)" % (error_msg_prefix, self.json_db[vuid_string]['vu_txt'], spec_link)
                print ("Updated error_db error_msg:%s" % (self.error_db_dict[enum]['error_msg']))
        #sys.exit()
        print ("These json DB entries are not in error DB:")
        for extra_vuid in json_db_set:
            print ("\t%s" % (extra_vuid))
            # Add these missing entries into the error_db
            # Create link into core or ext spec as needed
            if 'core' == self.json_db[extra_vuid]['ext'] or '!' in self.json_db[extra_vuid]['ext']:
                spec_link = "%s#%s" % (core_url, extra_vuid)
            else:
                spec_link = "%s#%s" % (ext_url, extra_vuid)
            error_enum = "%s%s" % (validation_error_enum_name, get8digithex(self.json_db[extra_vuid]['number_vuid']))
            self.error_db_dict[error_enum] = {}
            self.error_db_dict[error_enum]['check_implemented'] = 'N'
            self.error_db_dict[error_enum]['testname'] = 'None'
            self.error_db_dict[error_enum]['api'] = self.json_db[extra_vuid]['struct_func']
            self.error_db_dict[error_enum]['vuid_string'] = extra_vuid
            self.error_db_dict[error_enum]['error_msg'] = "%s'%s' (%s)" % (error_msg_prefix, self.json_db[extra_vuid]['vu_txt'], spec_link)
            self.error_db_dict[error_enum]['note'] = ''
            self.error_db_dict[error_enum]['ext'] = self.json_db[extra_vuid]['ext']
            implicit = False
            last_segment = extra_vuid.split("-")[-1]
            if last_segment in vuid_mapping.implicit_type_map:
                implicit = True
            elif not last_segment.isdigit(): # Explicit ids should only have digits in last segment
                print ("ERROR: Found last segment of val error ID that isn't in implicit map and doesn't have numbers in last segment: %s" % (last_segment))
                sys.exit()
            self.error_db_dict[error_enum]['implicit'] = implicit

    def genHeader(self, header_file):
        """Generate a header file based on the contents of a parsed spec"""
        print ("Generating header %s..." % (header_file))
        file_contents = []
        file_contents.append(self.copyright)
        file_contents.append('\n#pragma once')
        file_contents.append('\n// Disable auto-formatting for generated file')
        file_contents.append('// clang-format off')
        file_contents.append('\n#include <unordered_map>')
        file_contents.append('\n// enum values for unique validation error codes')
        file_contents.append('//  Corresponding validation error message for each enum is given in the mapping table below')
        file_contents.append('//  When a given error occurs, these enum values should be passed to the as the messageCode')
        file_contents.append('//  parameter to the PFN_vkDebugReportCallbackEXT function')
        enum_decl = ['enum UNIQUE_VALIDATION_ERROR_CODE {\n    VALIDATION_ERROR_UNDEFINED = -1,']
        error_string_map = ['static std::unordered_map<int, char const *const> validation_error_map{']
        enum_value = 0
        max_enum_val = 0
        for enum in sorted(self.error_db_dict):
            #print ("Header enum is %s" % (enum))
            # TMP: Use updated value
            vuid_str = self.error_db_dict[enum]['vuid_string']
            if vuid_str in self.json_db:
                enum_value = self.json_db[vuid_str]['number_vuid']
            else:
                enum_value = vuid_mapping.convertVUID(vuid_str)
            new_enum = "%s%s" % (validation_error_enum_name, get8digithex(enum_value))
            enum_decl.append('    %s = 0x%s,' % (new_enum, get8digithex(enum_value)))
            error_string_map.append('    {%s, "%s"},' % (new_enum, self.error_db_dict[enum]['error_msg']))
            max_enum_val = max(max_enum_val, enum_value)
        enum_decl.append('    %sMAX_ENUM = %d,' % (validation_error_enum_name, max_enum_val + 1))
        enum_decl.append('};')
        error_string_map.append('};\n')
        file_contents.extend(enum_decl)
        file_contents.append('\n// Mapping from unique validation error enum to the corresponding error message')
        file_contents.append('// The error message should be appended to the end of a custom error message that is passed')
        file_contents.append('// as the pMessage parameter to the PFN_vkDebugReportCallbackEXT function')
        file_contents.extend(error_string_map)
        #print ("File contents: %s" % (file_contents))
        with open(header_file, "w") as outfile:
            outfile.write("\n".join(file_contents))
    def genDB(self, db_file):
        """Generate a database of check_enum, check_coded?, testname, API, VUID_string, core|ext, error_string, notes"""
        db_lines = []
        # Write header for database file
        db_lines.append("# This is a database file with validation error check information")
        db_lines.append("# Comments are denoted with '#' char")
        db_lines.append("# The format of the lines is:")
        db_lines.append("# <error_enum>%s<check_implemented>%s<testname>%s<api>%s<vuid_string>%s<core|ext>%s<errormsg>%s<note>" % (self.delimiter, self.delimiter, self.delimiter, self.delimiter, self.delimiter, self.delimiter, self.delimiter))
        db_lines.append("# error_enum: Unique error enum for this check of format %s<uniqueid>" % validation_error_enum_name)
        db_lines.append("# check_implemented: 'Y' if check has been implemented in layers, or 'N' for not implemented")
        db_lines.append("# testname: Name of validation test for this check, 'Unknown' for unknown, 'None' if not implemented, or 'NotTestable' if cannot be implemented")
        db_lines.append("# api: Vulkan API function that this check is related to")
        db_lines.append("# vuid_string: Unique string to identify this check")
        db_lines.append("# core|ext: Either 'core' for core spec or some extension string that indicates the extension required for this VU to be relevant")
        db_lines.append("# errormsg: The unique error message for this check that includes spec language and link")
        db_lines.append("# note: Free txt field with any custom notes related to the check in question")
        for enum in sorted(self.error_db_dict):
            print ("Gen DB for enum %s" % (enum))
            implicit = self.error_db_dict[enum]['implicit']
            implemented = self.error_db_dict[enum]['check_implemented']
            testname = self.error_db_dict[enum]['testname']
            note = self.error_db_dict[enum]['note']
            core_ext = self.error_db_dict[enum]['ext']
            self.error_db_dict[enum]['vuid_string'] = self.error_db_dict[enum]['vuid_string']
            if implicit and 'implicit' not in note: # add implicit note
                if '' != note:
                    note = "implicit, %s" % (note)
                else:
                    note = "implicit"
            db_lines.append("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s" % (enum, self.delimiter, implemented, self.delimiter, testname, self.delimiter, self.error_db_dict[enum]['api'], self.delimiter, self.error_db_dict[enum]['vuid_string'], self.delimiter, core_ext, self.delimiter, self.error_db_dict[enum]['error_msg'], self.delimiter, note))
        db_lines.append("\n") # newline at end of file
        print ("Generating database file %s" % (db_file))
        with open(db_file, "w") as outfile:
            outfile.write("\n".join(db_lines))
    def readDB(self, db_file):
        """Read a db file into a dict, refer to genDB function above for format of each line"""
        with open(db_file, "r") as infile:
            for line in infile:
                line = line.strip()
                if line.startswith('#') or '' == line:
                    continue
                db_line = line.split(self.delimiter)
                if len(db_line) != 8:
                    print ("ERROR: Bad database line doesn't have 8 elements: %s" % (line))
                error_enum = db_line[0]
                implemented = db_line[1]
                testname = db_line[2]
                api = db_line[3]
                vuid_str = db_line[4]
                core_ext = db_line[5]
                error_str = db_line[6]
                note = db_line[7]
                # Also read complete database contents into our class var for later use
                self.error_db_dict[error_enum] = {}
                self.error_db_dict[error_enum]['check_implemented'] = implemented
                self.error_db_dict[error_enum]['testname'] = testname
                self.error_db_dict[error_enum]['api'] = api
                self.error_db_dict[error_enum]['vuid_string'] = vuid_str
                self.error_db_dict[error_enum]['ext'] = core_ext
                self.error_db_dict[error_enum]['error_msg'] = error_str
                self.error_db_dict[error_enum]['note'] = note
                implicit = False
                last_segment = vuid_str.split("-")[-1]
                if last_segment in vuid_mapping.implicit_type_map:
                    implicit = True
                elif not last_segment.isdigit(): # Explicit ids should only have digits in last segment
                    print ("ERROR: Found last segment of val error ID that isn't in implicit map and doesn't have numbers in last segment: %s" % (last_segment))
                    sys.exit()
                self.error_db_dict[error_enum]['implicit'] = implicit
if __name__ == "__main__":
    i = 1
    use_online = True # Attempt to grab spec from online by default
    while (i < len(sys.argv)):
        arg = sys.argv[i]
        i = i + 1
        if (arg == '-json-file'):
            json_filename = sys.argv[i]
            i = i + 1
        elif (arg == '-json'):
            read_json = True
        elif (arg == '-json-compare'):
            json_compare = True
        elif (arg == '-out'):
            out_filename = sys.argv[i]
            i = i + 1
        elif (arg == '-gendb'):
            gen_db = True
            # Set filename if supplied, else use default
            if i < len(sys.argv) and not sys.argv[i].startswith('-'):
                db_filename = sys.argv[i]
                i = i + 1
        elif (arg == '-update'):
            read_json = True
            json_compare = True
            gen_db = True
        elif (arg in ['-help', '-h']):
            printHelp()
            sys.exit()
    spec = Specification()
    if read_json:
        spec.readJSON()
        spec.parseJSON()
        #sys.exit()
    if (json_compare):
        # Read in current spec info from db file
        (orig_err_msg_dict) = spec.readDB(db_filename)
        spec.compareJSON()
        print ("Found %d missing db entries in json db" % (spec.json_missing))
        print ("Found %d duplicate json entries" % (spec.duplicate_json_key_count))
        spec.genDB(db_filename)
    if (gen_db):
        spec.genDB(db_filename)
    print ("Writing out file (-out) to '%s'" % (out_filename))
    spec.genHeader(out_filename)
