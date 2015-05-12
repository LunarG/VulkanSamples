#!/bin/bash
# Update source for glslang and LLVM

set -e

LUNARGLASS_REVISION=$(cat $PWD/LunarGLASS_revision)
GLSLANG_REVISION=$(cat $PWD/glslang_revision)
echo "LUNARGLASS_REVISION=$LUNARGLASS_REVISION"
echo "GLSLANG_REVISION=$GLSLANG_REVISION"

BUILDDIR=$PWD
BASEDIR=$BUILDDIR/..

function create_glslang () {
   rm -rf $BASEDIR/glslang
   echo "Creating local glslang repository ($BASEDIR/glslang)."
   mkdir -p $BASEDIR/glslang
   cd $BASEDIR/glslang
   svn checkout https://cvs.khronos.org/svn/repos/ogl/trunk/ecosystem/public/sdk/tools/glslang .
}

function update_glslang () {
   echo "Updating $BASEDIR/glslang"
   cd $BASEDIR/glslang
   svn update -r "$GLSLANG_REVISION"
}

function create_LunarGLASS () {
   rm -rf $BASEDIR/LunarGLASS
   echo "Creating local LunarGLASS repository ($BASEDIR/LunarGLASS)."
   mkdir -p $BASEDIR/LunarGLASS/Core/LLVM
   cd $BASEDIR/LunarGLASS/Core/LLVM 
   wget http://llvm.org/releases/3.4/llvm-3.4.src.tar.gz
   tar --gzip -xf llvm-3.4.src.tar.gz
   cd $BASEDIR/LunarGLASS
   svn checkout --force https://lunarglass.googlecode.com/svn/trunk/ .
   svn revert --depth=infinity .
}

function update_LunarGLASS () {
   echo "Updating $BASEDIR/LunarGLASS"
   cd $BASEDIR/LunarGLASS
   svn update -r "$LUNARGLASS_REVISION"
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

if [ ! -d "$BASEDIR/glslang" ]; then
   create_glslang
fi
if [ ! -d "$BASEDIR/LunarGLASS" ]; then
   create_LunarGLASS
fi

update_glslang
update_LunarGLASS

build_glslang
build_LunarGLASS
