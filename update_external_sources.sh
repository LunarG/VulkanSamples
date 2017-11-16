#!/bin/bash
# Update source for glslang

set -e

if [[ $(uname) == "Linux" || $(uname) =~ "CYGWIN" ]]; then
    CURRENT_DIR="$(dirname "$(readlink -f ${BASH_SOURCE[0]})")"
    CORE_COUNT=$(nproc || echo 4)
elif [[ $(uname) == "Darwin" ]]; then
    CURRENT_DIR="$(dirname "$(python -c 'import os,sys;print(os.path.realpath(sys.argv[1]))' ${BASH_SOURCE[0]})")"
    CORE_COUNT=$(sysctl -n hw.ncpu || echo 4)
fi
echo CURRENT_DIR=$CURRENT_DIR
echo CORE_COUNT=$CORE_COUNT

REVISION_DIR="$CURRENT_DIR/external_revisions"

GLSLANG_GITURL=$(cat "${REVISION_DIR}/glslang_giturl")
GLSLANG_REVISION=$(cat "${REVISION_DIR}/glslang_revision")

echo "GLSLANG_GITURL=${GLSLANG_GITURL}"
echo "GLSLANG_REVISION=${GLSLANG_REVISION}"

BUILDDIR=${CURRENT_DIR}
BASEDIR="$BUILDDIR/external"

function create_glslang () {
   rm -rf "${BASEDIR}"/glslang
   echo "Creating local glslang repository (${BASEDIR}/glslang)."
   mkdir -p "${BASEDIR}"/glslang
   cd "${BASEDIR}"/glslang
   git clone ${GLSLANG_GITURL} .
   git checkout ${GLSLANG_REVISION}
   ./update_glslang_sources.py
}

function update_glslang () {
   echo "Updating ${BASEDIR}/glslang"
   cd "${BASEDIR}"/glslang
   git fetch --all
   git checkout --force ${GLSLANG_REVISION}
   ./update_glslang_sources.py
}

function build_glslang () {
   echo "Building ${BASEDIR}/glslang"
   cd "${BASEDIR}"/glslang
   mkdir -p build
   cd build
   cmake -D CMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install ..
   make -j $CORE_COUNT
   make install
}

INCLUDE_GLSLANG=false
NO_SYNC=false
NO_BUILD=false
USE_IMPLICIT_COMPONENT_LIST=true

# Parse options
while [[ $# > 0 ]]
do
  option="$1"

  case $option in
      # options to specify build of glslang components
      -g|--glslang)
      INCLUDE_GLSLANG=true
      USE_IMPLICIT_COMPONENT_LIST=false
      echo "Building glslang ($option)"
      ;;
      # options to specify build of spirv-tools components
      -s|--spirv-tools)
      echo "($option) is deprecated and is no longer necessary"
      ;;
      # option to specify skipping sync from git
      --no-sync)
      NO_SYNC=true
      echo "Skipping sync ($option)"
      ;;
      # option to specify skipping build
      --no-build)
      NO_BUILD=true
      echo "Skipping build ($option)"
      ;;
      *)
      echo "Unrecognized option: $option"
      echo "Usage: update_external_sources.sh [options]"
      echo "  Available options:"
      echo "    -g | --glslang      # enable glslang component"
      echo "    --no-sync           # skip sync from git"
      echo "    --no-build          # skip build"
      echo "  If any component enables are provided, only those components are enabled."
      echo "  If no component enables are provided, all components are enabled."
      echo "  Sync uses git to pull a specific revision."
      echo "  Build configures CMake, builds Release."
      exit 1
      ;;
  esac
  shift
done

if [ ${USE_IMPLICIT_COMPONENT_LIST} == "true" ]; then
  echo "Building glslang"
  INCLUDE_GLSLANG=true
fi

if [ ${INCLUDE_GLSLANG} == "true" ]; then
  if [ ${NO_SYNC} == "false" ]; then
    if [ ! -d "${BASEDIR}/glslang" -o ! -d "${BASEDIR}/glslang/.git" -o -d "${BASEDIR}/glslang/.svn" ]; then
       create_glslang
    fi
    update_glslang
  fi
  if [ ${NO_BUILD} == "false" ]; then
    build_glslang
  fi
fi
