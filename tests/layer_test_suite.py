#!/usr/bin/env python3
#
# VK
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
import subprocess
import sys

# layer_test_suite.py overview
# This script runs tests and verifies results
# It's intended to wrap tests run with layers and verify the following:
# 1. Any expected layers are inserted
# 2. Any expected layer errors are output
# 3. No unexpected errors are output

def handle_args():
    parser = argparse.ArgumentParser(description='Run Vulkan test suite and report errors.')
    parser.add_argument('--script_name', required=False, default='./run_all_tests_with_layers.sh', help='The script file to be executed and have its output checked.')
    return parser.parse_args()

expected_layers = ['DrawState', 'MemTracker', 'ParamChecker', 'ObjectTracker']

# Format of this dict is <testname> key points to a list of expected error text
# The start of each line of output for any given test is compared against the expected error txt
expected_errors = {'XglRenderTest.CubeWithVertexFetchAndMVP' : ['{OBJTRACK}ERROR : OBJ ERROR : DEPTH_STENCIL_VIEW',
                                                                '{OBJTRACK}ERROR : OBJ ERROR : GPU_MEMORY',
                                                                '{OBJTRACK}ERROR : OBJ ERROR : IMAGE'],
                   'XglRenderTest.CubeWithVertexFetchAndMVPAndTexture' : ['{OBJTRACK}ERROR : OBJ ERROR : COMMAND_BUFFER',
                                                                          '{OBJTRACK}ERROR : OBJ ERROR : DEPTH_STENCIL_VIEW',
                                                                          '{OBJTRACK}ERROR : OBJ ERROR : GPU_MEMORY',
                                                                          '{OBJTRACK}ERROR : OBJ ERROR : IMAGE'],
                   'XglTest.Fence' : ['{OBJTRACK}ERROR : OBJECT VALIDATION WARNING: FENCE'],
                   #'XglRenderTest.VKTriangle_OutputLocation' : ['{OBJTRACK}ERROR : vkQueueSubmit Memory reference count'],
                   'XglRenderTest.TriangleWithVertexFetch' : ['{OBJTRACK}ERROR : OBJ ERROR : COMMAND_BUFFER'],
                   'XglRenderTest.TriangleMRT' : ['{OBJTRACK}ERROR : OBJ ERROR : COMMAND_BUFFER'],
                   'XglRenderTest.QuadWithIndexedVertexFetch' : ['{OBJTRACK}ERROR : OBJ ERROR : COMMAND_BUFFER', '{OBJTRACK}ERROR : OBJ ERROR : COMMAND_BUFFER'],
                   'XglRenderTest.GreyandRedCirclesonBlue' : ['{OBJTRACK}ERROR : OBJ ERROR : COMMAND_BUFFER'],
                   'XglRenderTest.RedCirclesonBlue' : ['{OBJTRACK}ERROR : OBJ ERROR : COMMAND_BUFFER'],
                   'XglRenderTest.GreyCirclesonBlueFade' : ['{OBJTRACK}ERROR : OBJ ERROR : COMMAND_BUFFER'],
                   'XglRenderTest.GreyCirclesonBlueDiscard' : ['{OBJTRACK}ERROR : OBJ ERROR : COMMAND_BUFFER'],
                   'XglRenderTest.TriVertFetchAndVertID' : ['{OBJTRACK}ERROR : OBJ ERROR : COMMAND_BUFFER'],
                   'XglRenderTest.TriVertFetchDeadAttr' : ['{OBJTRACK}ERROR : OBJ ERROR : COMMAND_BUFFER'],}

# Verify that expected errors are hit
# Return True if all expected errors for any matched tests are found and no unexpected errors are found
# Return False if a test with expected errors doesn't have all expected errors and/or any unexpected errors occur
def verify_errors(out_lines):
    print "Verifying expected errors and making sure no unexpected errors occur."
    result = True
    # If we hit a testname line, then catch test name
    # If testname is in dict, then get expected errors
    # Pop expected errors off of the list
    #  for any expected errors remaining, flag an issue
    testname = ''
    ee_list = []
    errors_found = 0
    check_expected_errors = False
    for line in out_lines:
        if '[ RUN      ]' in line:
            testname = line[line.find(']')+1:].split()[0].strip()
            #print "Found testname %s" % testname
            if testname in expected_errors:
                ee_list = expected_errors[testname];
                #print "testname %s has %i expected errors in ee_list %s" % (testname, len(ee_list), ", ".join(ee_list))
                check_expected_errors = True
        elif '[       OK ]' in line:
            if len(ee_list) > 0:
                print "ERROR : Failed to find expected error(s) for %s: %s" % (testname, ", ".join(ee_list))
                result = False
            check_expected_errors = False
            ee_list = []
            errors_found = 0
        elif check_expected_errors:
            if True in [line.startswith(ex_err) for ex_err in ee_list]:
                for ee in ee_list:
                    if line.startswith(ee):
                        #print "Found expected error %s in line %s" % (ee, line)
                        errors_found += 1
                        ee_list.remove(ee)
                        if len(ee_list) == 0:
                            print "Found all %i expected errors for testname %s" % (errors_found, testname)
        elif 'ERROR' in line:
            print "ERROR : Found unexpected error %s for test %s" % (line, testname)
            result = False
    return result

def verify_layers(out_lines):
    for line in out_lines:
        if line.startswith("Inserting layer "):
            layer_name = line.split()[2]
            #print "Found layer %s" % (layer_name)
            if layer_name in expected_layers:
                expected_layers.remove(layer_name)
                if 0 == len(expected_layers):
                    print "Found all expected layers"
                    return True
    print "Failed to find layers: %s" % ", ".join(expected_layers)
    return False

def main(argv=None):
    # parse args
    opts = handle_args()
    # run test and capture output
    print "Running test script %s" % (opts.script_name)
    test_result = subprocess.check_output(opts.script_name, stderr=subprocess.STDOUT) #, shell=True)
    result_lines = test_result.split("\n")
    if verify_layers(result_lines):
        if verify_errors(result_lines):
            print "Verify errors completed successfully"
        else:
            print "Verify errors FAILED! See ERROR messages above"
    else:
        print "Verify errors FAILED! See ERROR messages above"
    print "Done running test suite"
    # verify output
    

if __name__ == "__main__":
    sys.exit(main())
