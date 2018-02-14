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

BUILDDIR=${CURRENT_DIR}
BASEDIR="$BUILDDIR/submodules"

git submodule update --init --recursive

echo "Running LVL update external sources"
cd ${BASEDIR}/Vulkan-LoaderAndValidationLayers
./update_external_sources.sh "$@"