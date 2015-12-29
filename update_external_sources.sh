#!/bin/bash
# Update source for glslang, LunarGLASS, spirv-tools

set -e

LUNARGLASS_REVISION=$(cat $PWD/LunarGLASS_revision)
GLSLANG_REVISION=$(cat $PWD/glslang_revision)
SPIRV_TOOLS_REVISION=$(cat $PWD/spirv-tools_revision)
echo "LUNARGLASS_REVISION=$LUNARGLASS_REVISION"
echo "GLSLANG_REVISION=$GLSLANG_REVISION"
echo "SPIRV_TOOLS_REVISION=$SPIRV_TOOLS_REVISION"

LUNARGLASS_REVISION_R32=$(cat $PWD/LunarGLASS_revision_R32)
echo "LUNARGLASS_REVISION_R32=$LUNARGLASS_REVISION_R32"

BUILDDIR=$PWD
BASEDIR=$BUILDDIR/..

function create_glslang () {
   rm -rf $BASEDIR/glslang
   echo "Creating local glslang repository ($BASEDIR/glslang)."
   mkdir -p $BASEDIR/glslang
   cd $BASEDIR/glslang
   git clone git@gitlab.khronos.org:GLSL/glslang.git .
   git checkout $GLSLANG_REVISION
}

function update_glslang () {
   echo "Updating $BASEDIR/glslang"
   cd $BASEDIR/glslang
   git fetch --all
   git checkout $GLSLANG_REVISION
}

function create_LunarGLASS () {
   rm -rf $BASEDIR/LunarGLASS
   echo "Creating local LunarGLASS repository ($BASEDIR/LunarGLASS)."
   mkdir -p $BASEDIR/LunarGLASS
   cd $BASEDIR/LunarGLASS
   git clone https://github.com/LunarG/LunarGLASS.git .
   mkdir -p Core/LLVM
   cd Core/LLVM 
   wget http://llvm.org/releases/3.4/llvm-3.4.src.tar.gz
   tar --gzip -xf llvm-3.4.src.tar.gz
   git checkout -f .  # put back the LunarGLASS versions of some LLVM files
   git checkout $LUNARGLASS_REVISION
   svn checkout -r $LUNARGLASS_REVISION_R32 --force https://cvs.khronos.org/svn/repos/SPIRV/trunk/LunarGLASS/ .
   svn revert -R .
}

function update_LunarGLASS () {
   echo "Updating $BASEDIR/LunarGLASS"
   cd $BASEDIR/LunarGLASS
   git fetch
   git checkout -f .
   git checkout $LUNARGLASS_REVISION 
   # Figure out how to do this with git
   #git checkout $LUNARGLASS_REVISION |& tee gitout
   #if grep --quiet LLVM gitout ; then
   #   rm -rf $BASEDIR/LunarGLASS/Core/LLVM/llvm-3.4/build
   #fi
   #rm -rf gitout
   if [ ! -d "$BASEDIR/LunarGLASS/.svn" ]; then
      svn checkout -r $LUNARGLASS_REVISION_R32 --force https://cvs.khronos.org/svn/repos/SPIRV/trunk/LunarGLASS/ .
   fi
   svn update -r $LUNARGLASS_REVISION_R32
   svn revert -R .
}

function create_spirv-tools () {
   rm -rf $BASEDIR/spirv-tools
   echo "Creating local spirv-tools repository ($BASEDIR/spirv-tools)."
   mkdir -p $BASEDIR/spirv-tools
   cd $BASEDIR/spirv-tools
   git clone git@gitlab.khronos.org:spirv/spirv-tools.git .
   git checkout $SPIRV_TOOLS_REVISION
}

function update_spirv-tools () {
   echo "Updating $BASEDIR/spirv-tools"
   cd $BASEDIR/spirv-tools
   git fetch --all
   git checkout $SPIRV_TOOLS_REVISION
}

function build_glslang () {
   echo "Building $BASEDIR/glslang"
   cd $BASEDIR/glslang
   mkdir -p build
   cd build
   cmake -D CMAKE_BUILD_TYPE=Release ..
   cmake -D CMAKE_BUILD_TYPE=Release ..
   make
   make install
}

function build_LunarGLASS () {
   echo "Building $BASEDIR/LunarGLASS"
   cd $BASEDIR/LunarGLASS/Core/LLVM/llvm-3.4
   if [ ! -d "$BASEDIR/LunarGLASS/Core/LLVM/llvm-3.4/build" ]; then
      mkdir -p build
      cd build
      ../configure --enable-terminfo=no --enable-curses=no
      REQUIRES_RTTI=1 make -j $(nproc) && make install DESTDIR=`pwd`/install
   fi
   cd $BASEDIR/LunarGLASS
   mkdir -p build
   cd build
   cmake -D CMAKE_BUILD_TYPE=Release ..
   cmake -D CMAKE_BUILD_TYPE=Release ..
   make
   make install
}

function build_spirv-tools () {
   echo "Building $BASEDIR/spirv-tools"
   cd $BASEDIR/spirv-tools
   mkdir -p build
   cd build
   cmake -D CMAKE_BUILD_TYPE=Release ..
   make -j $(nproc)
}

# Parse options
while [[ $# > 0 ]]
do
option="$1"

case $option in
    # option to allow just building glslang components
    -g|--glslang-only)
    GLSLANG_ONLY=true
    echo "Building glslang only: GLSLANG_ONLY=true"
    ;;
    *)
    echo "Unrecognized option: $option"
    exit 1
    ;;
esac
shift
done

if [ ! -d "$BASEDIR/glslang" -o ! -d "$BASEDIR/glslang/.git" -o -d "$BASEDIR/glslang/.svn" ]; then
   create_glslang
fi

if [ ! $GLSLANG_ONLY ]; then
    if [ ! -d "$BASEDIR/LunarGLASS" -o ! -d "$BASEDIR/LunarGLASS/.git" ]; then
       create_LunarGLASS
    fi
    if [ ! -d "$BASEDIR/spirv-tools" -o ! -d "$BASEDIR/spirv-tools/.git" ]; then
       create_spirv-tools
    fi
fi

update_glslang
if [ ! $GLSLANG_ONLY ]; then
    update_LunarGLASS
    update_spirv-tools
fi

build_glslang
if [ ! $GLSLANG_ONLY ]; then
    build_LunarGLASS
    build_spirv-tools
fi
