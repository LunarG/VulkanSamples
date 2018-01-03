#!/bin/bash

set -e

# If any tracked revision no longer matches the local revision, blast the extenal toolchain directories

function check_revision()
{
  echo Checking current revision for $1 in $2
  if [ -d $2/.git ]; then
    current_rev=$(git --git-dir=$2/.git rev-parse HEAD);
  fi
  echo current_rev for $1 is $current_rev;
  tracked_rev=$(cat $3);
  echo tracked_rev for $1 is $tracked_rev;
  if [ "$current_rev" != "$tracked_rev" ]; then
    echo Revisions for $1 do not match.;
    if [ -d external ]; then
      echo Removing current desktop toolchain;
      rm -rf external/*;
    fi
    if [ -d build-android/external ]; then
      echo Removing current android toolchain;
      rm -rf build-android/external/*;
    fi
    echo Done removing toolchains.
    exit 0;
  fi
}

# Parameters are tool, current git repo location, tracked revision location
tool=glslang
dir=external/glslang
rev=external_revisions/glslang_revision
check_revision $tool $dir $rev

tool=glslang_android
dir=build-android/external/glslang
rev=build-android/glslang_revision_android
check_revision $tool $dir $rev

tool=spirv-tools_android
dir=build-android/external/spirv-tools
rev=build-android/spirv-tools_revision_android
check_revision $tool $dir $rev

tool=spirv-headers_android
dir=build-android/external/spirv-tools/external/spirv-headers
rev=build-android/spirv-headers_revision_android
check_revision $tool $dir $rev

tool=shaderc_android
dir=build-android/external/shaderc
rev=build-android/shaderc_revision_android
check_revision $tool $dir $rev

exit 0
