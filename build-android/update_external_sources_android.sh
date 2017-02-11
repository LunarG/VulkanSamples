#!/bin/bash
# Update source for glslang, spirv-tools, shaderc

# Copyright 2016 The Android Open Source Project
# Copyright (C) 2015 Valve Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -e

ANDROIDBUILDDIR=$PWD
BUILDDIR=$ANDROIDBUILDDIR
BASEDIR=$BUILDDIR/external

GLSLANG_REVISION=$(cat $ANDROIDBUILDDIR/glslang_revision_android)
SPIRV_TOOLS_REVISION=$(cat $ANDROIDBUILDDIR/spirv-tools_revision_android)
SPIRV_HEADERS_REVISION=$(cat $ANDROIDBUILDDIR/spirv-headers_revision_android)
SHADERC_REVISION=$(cat $ANDROIDBUILDDIR/shaderc_revision_android)

echo "GLSLANG_REVISION=$GLSLANG_REVISION"
echo "SPIRV_TOOLS_REVISION=$SPIRV_TOOLS_REVISION"
echo "SHADERC_REVISION=$SHADERC_REVISION"

if [[ $(uname) == "Linux" ]]; then
    cores=$(ncpus || echo 4)
elif [[ $(uname) == "Darwin" ]]; then
    cores=$(sysctl -n hw.ncpu)
fi

function create_glslang () {
   rm -rf $BASEDIR/glslang
   echo "Creating local glslang repository ($BASEDIR/glslang)."
   mkdir -p $BASEDIR/glslang
   cd $BASEDIR/glslang
   git clone https://android.googlesource.com/platform/external/shaderc/glslang .
   git checkout $GLSLANG_REVISION
}

function update_glslang () {
   echo "Updating $BASEDIR/glslang"
   cd $BASEDIR/glslang
   git fetch --all
   git checkout $GLSLANG_REVISION
}

function create_spirv-tools () {
   rm -rf $BASEDIR/spirv-tools
   echo "Creating local spirv-tools repository ($BASEDIR/spirv-tools)."
   mkdir -p $BASEDIR/spirv-tools
   cd $BASEDIR/spirv-tools
   git clone https://android.googlesource.com/platform/external/shaderc/spirv-tools .
   git checkout $SPIRV_TOOLS_REVISION
}

function update_spirv-tools () {
   echo "Updating $BASEDIR/spirv-tools"
   cd $BASEDIR/spirv-tools
   git fetch --all
   git checkout $SPIRV_TOOLS_REVISION
}

function create_spirv-headers () {
   rm -rf $BASEDIR/spirv-tools/external/spirv-headers
   echo "Creating local spirv-headers repository ($BASEDIR/spirv-tools/external/spirv-headers)."
   mkdir -p $BASEDIR/spirv-tools/external/spirv-headers
   cd $BASEDIR/spirv-tools/external/spirv-headers
   git clone https://android.googlesource.com/platform/external/shaderc/spirv-headers .
   git checkout $SPIRV_HEADERS_REVISION
}

function update_spirv-headers () {
   echo "Updating $BASEDIR/spirv-tools/external/spirv-headers"
   cd $BASEDIR/spirv-tools/external/spirv-headers
   git fetch --all
   git checkout $SPIRV_HEADERS_REVISION
}

function create_shaderc () {
   rm -rf $BASEDIR/shaderc
   echo "Creating local shaderc repository ($BASEDIR/shaderc)."
   cd $BASEDIR
   git clone https://android.googlesource.com/platform/external/shaderc/shaderc
   cd shaderc
   git checkout $SHADERC_REVISION
}

function update_shaderc () {
   echo "Updating $BASEDIR/shaderc"
   cd $BASEDIR/shaderc
   git fetch --all
   git checkout $SHADERC_REVISION
}

function build_shaderc () {
   echo "Building $BASEDIR/shaderc"
   cd $BASEDIR/shaderc/android_test
   ndk-build THIRD_PARTY_PATH=../.. -j $cores
}

if [ ! -d "$BASEDIR/glslang" -o ! -d "$BASEDIR/glslang/.git" -o -d "$BASEDIR/glslang/.svn" ]; then
   create_glslang
fi
 update_glslang


if [ ! -d "$BASEDIR/spirv-tools" -o ! -d "$BASEDIR/spirv-tools/.git" ]; then
   create_spirv-tools
fi
update_spirv-tools

if [ ! -d "$BASEDIR/spirv-tools/external/spirv-headers" -o ! -d "$BASEDIR/spirv-tools/external/spirv-headers/.git" ]; then
   create_spirv-headers
fi
update_spirv-headers

if [ ! -d "$BASEDIR/shaderc" -o ! -d "$BASEDIR/shaderc/.git" ]; then
     create_shaderc
fi
update_shaderc
build_shaderc

echo ""
echo "${0##*/} finished."

