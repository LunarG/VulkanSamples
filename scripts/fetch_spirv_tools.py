#!/usr/bin/env python3
#
# Copyright (c) 2018 The Khronos Group Inc.
# Copyright (c) 2020 Valve Corporation
# Copyright (c) 2020 LunarG, Inc.
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


# This script will download the latest spirv-tools release binary and extract the
# spirv-tools binaries needed by the samples
#
# It takes as its lone argument the filname (no path) describing the release
# binary name from the spirv-tools github releases page.

import sys
import os
import shutil
import ssl
import subprocess
import urllib.request
import zipfile

SCRIPTS_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_DIR = os.path.join(SCRIPTS_DIR, '..')
SPIRV_TOOLS_URL = "https://github.com/KhronosGroup/SPIRV-Tools/releases/download/master-tot"

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("ERROR -- must include a single spirv-tools release zipfile name argument")
        sys.exit();

    SPIRV_TOOLS_FILENAME = sys.argv[1]
    SPIRV_TOOLS_COMPLETE_URL = SPIRV_TOOLS_URL + "/" + SPIRV_TOOLS_FILENAME
    SPIRV_TOOLS_OUTFILENAME = os.path.join(REPO_DIR, "spirv-tools", SPIRV_TOOLS_FILENAME)
    SPIRV_TOOLS_PATH = os.path.join(REPO_DIR, "spirv-tools", "bin")
    SPIRV_TOOLS_DIR = os.path.join(REPO_DIR, "spirv-tools")

    if os.path.isdir(SPIRV_TOOLS_DIR):
        if os.path.isdir(SPIRV_TOOLS_PATH):
            dir_contents = os.listdir(SPIRV_TOOLS_PATH)
            for afile in dir_contents:
                if "spirv-as" in afile:
                    print("   Using spirv-tools at %s" % SPIRV_TOOLS_PATH)
                    sys.exit();
    else:
        os.mkdir(SPIRV_TOOLS_DIR)
    print("   Downloading spirv-as binary from spirv-tools releases dir")
    sys.stdout.flush()

    # Download release zip file from glslang github releases site
    with urllib.request.urlopen(SPIRV_TOOLS_COMPLETE_URL, context=ssl._create_unverified_context()) as response, open(SPIRV_TOOLS_OUTFILENAME, 'wb') as out_file:
        shutil.copyfileobj(response, out_file)
    # Unzip the glslang binary archive
    zipped_file = zipfile.ZipFile(SPIRV_TOOLS_OUTFILENAME, 'r')
    namelist = zipped_file.namelist()
    for afile in namelist:
        if "spirv-as" in afile:
            EXE_FILE_PATH = os.path.join(SPIRV_TOOLS_DIR, afile)
            zipped_file.extract(afile, SPIRV_TOOLS_DIR)
            os.chmod(EXE_FILE_PATH, 0o775)
            break
    zipped_file.close()
    sys.exit();
