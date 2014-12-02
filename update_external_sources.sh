#!/bin/bash
# Update source for glslang and LLVM
# Copy necessary BIL pieces into glslang and LLVM

BIL_REVISION=29091
LUNARGLASS_REVISION=1055
GLSLANG_REVISION=29091

BUILDDIR=$PWD
BASEDIR=$BUILDDIR/..

function create_glslang () {
   rm -rf $BASEDIR/glslang
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
   mkdir -p $BASEDIR/LunarGLASS/Core/LLVM
   cd $BASEDIR/LunarGLASS/Core/LLVM 
   wget http://llvm.org/releases/3.4/llvm-3.4.src.tar.gz
   tar --gzip -xf llvm-3.4.src.tar.gz
   cd $BASEDIR/LunarGLASS
   svn checkout --force https://lunarglass.googlecode.com/svn/trunk/ .
   svn revert --depth=infinity .
}

function create_BIL () {
   rm -rf $BASEDIR/BIL
   mkdir -p $BASEDIR/BIL
   cd $BASEDIR/BIL
   if [ -z "$KHRONOS_ACCOUNT_NAME" ]; then
      echo "Must define KHRONOS_ACCOUNT_NAME to access BIL component"
      exit 1
   else
      svn checkout --username "$KHRONOS_ACCOUNT_NAME" https://cvs.khronos.org/svn/repos/oglc/trunk/nextgen/proposals/BIL .
   fi
}

function update_LunarGLASS () {
   echo "Updating $BASEDIR/LunarGLASS"
   cd $BASEDIR/LunarGLASS
   svn update -r "$LUNARGLASS_REVISION"
}

function update_BIL () {
   if [ -d "$BASEDIR/BIL" ]; then
     # Update source
     cd $BASEDIR/BIL
     svn update -r "$BIL_REVISION"
     # copy of necessary BIL pieces into glslang
     cp $BASEDIR/BIL/glslangOverlay_into_BIL/* $BASEDIR/glslang/BIL
     cp $BASEDIR/BIL/Bil.h $BASEDIR/glslang/BIL
     cp $BASEDIR/BIL/GLSL450Lib.h $BASEDIR/glslang/BIL
     # copy of necessary BIL pieces into LLVM
     cp $BASEDIR/BIL/ToLLVM/CMakeLists.txt $BASEDIR/LunarGLASS
     cp -r $BASEDIR/BIL/ToLLVM/Standalone $BASEDIR/LunarGLASS
     cp -r $BASEDIR/BIL/ToLLVM/FrontEnds/* $BASEDIR/LunarGLASS/Frontends/
   fi
}

function build_glslang () {
   cd $BASEDIR/glslang
   mkdir -p build
   cd build
   cmake -D CMAKE_BUILD_TYPE=Release ..
   cmake -D CMAKE_BUILD_TYPE=Release ..
   make
   make install
}

function build_LunarGLASS () {
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
if [ ! -d "$BASEDIR/BIL" ]; then
   create_BIL
fi

update_glslang
update_LunarGLASS
update_BIL

build_glslang
build_LunarGLASS
