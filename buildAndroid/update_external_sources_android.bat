@echo off
REM Update source for glslang, spirv-tools, and shaderc

REM
REM Copyright 2016 The Android Open Source Project
REM Copyright (C) 2015 Valve Corporation
REM
REM Licensed under the Apache License, Version 2.0 (the "License");
REM you may not use this file except in compliance with the License.
REM You may obtain a copy of the License at
REM
REM      http://www.apache.org/licenses/LICENSE-2.0
REM
REM Unless required by applicable law or agreed to in writing, software
REM distributed under the License is distributed on an "AS IS" BASIS,
REM WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM See the License for the specific language governing permissions and
REM limitations under the License.
REM

setlocal EnableDelayedExpansion
set errorCode=0
set ANDROID_BUILD_DIR=%~dp0
set BUILD_DIR=%ANDROID_BUILD_DIR%..
set BASE_DIR=%BUILD_DIR%\..
set GLSLANG_DIR=%BASE_DIR%\glslang
set SPIRV_TOOLS_DIR=%BASE_DIR%\spirv-tools
set SHADERC_DIR=%BASE_DIR%\shaderc

for %%X in (where.exe) do (set FOUND=%%~$PATH:X)
if not defined FOUND (
   echo Dependency check failed:
   echo   where.exe not found
   echo   This script requires Windows Vista or later, which includes where.exe.
   set errorCode=1
)

where /q git.exe
if %ERRORLEVEL% equ 1 (
   echo Dependency check failed:
   echo   git.exe not found
   echo   Git for Windows can be downloaded here:  https://git-scm.com/download/win
   echo   Install and ensure git.exe makes it into your PATH
   set errorCode=1
)

where /q ndk-build.cmd
if %ERRORLEVEL% equ 1 (
   echo Dependency check failed:
   echo   ndk-build.cmd not found
   echo   Android NDK can be downloaded here:  http://developer.android.com/ndk/guides/setup.html
   echo   Install and ensure ndk-build.cmd makes it into your PATH
   set errorCode=1
)

REM ensure where is working with below false test
REM where /q foo
REM if %ERRORLEVEL% equ 1 (
REM echo foo
REM )

:main

if %errorCode% neq 0 (goto:error)

REM Read the target versions from external file, which is shared with Linux script

if not exist %BUILD_DIR%\glslang_revision (
   echo.
   echo Missing glslang_revision file. Place it in %BUILD_DIR%
   goto:error
)

if not exist %BUILD_DIR%\spirv-tools_revision (
   echo.
   echo Missing spirv-tools_revision file. Place it in %BUILD_DIR%
   set errorCode=1
   goto:error
)

if not exist %ANDROID_BUILD_DIR%\shaderc_revision (
   echo.
   echo Missing shaderc_revision file. Place it in %BUILD_DIR%
   set errorCode=1
   goto:error
)

set /p GLSLANG_REVISION= < glslang_revision
set /p SPIRV_TOOLS_REVISION= < spirv-tools_revision
set /p SHADERC_REVISION= < shaderc_revision
echo GLSLANG_REVISION=%GLSLANG_REVISION%
echo SPIRV_TOOLS_REVISION=%SPIRV_TOOLS_REVISION%
echo SHADERC_REVISION=%SHADERC_REVISION%


echo Creating and/or updating glslang, spirv-tools, shaderc in %BASE_DIR%

set sync-glslang=1
set sync-spirv-tools=1
set sync-shaderc=1
set build-shaderc=1

if %sync-glslang% equ 1 (
   if exist %GLSLANG_DIR% (
      rd /S /Q %GLSLANG_DIR%
   )
   if not exist %GLSLANG_DIR% (
      call:create_glslang
   )
   if %errorCode% neq 0 (goto:error)
   call:update_glslang
   if %errorCode% neq 0 (goto:error)
)

if %sync-spirv-tools% equ 1 (
   if exist %SPIRV_TOOLS_DIR% (
      rd /S /Q %SPIRV_TOOLS_DIR%
   )
   if %errorlevel% neq 0 (goto:error)
   if not exist %SPIRV_TOOLS_DIR% (
      call:create_spirv-tools
   )
   if %errorCode% neq 0 (goto:error)
   call:update_spirv-tools
   if %errorCode% neq 0 (goto:error)
)

if %sync-shaderc% equ 1 (
   if exist %SHADERC_DIR% (
      rd /S /Q %SHADERC_DIR%
   )
   if not exist %SHADERC_DIR% (
      call:create_shaderc
   )
   if %errorCode% neq 0 (goto:error)
   call:update_shaderc
   if %errorCode% neq 0 (goto:error)
)

if %build-shaderc% equ 1 (
   call:build_shaderc
   if %errorCode% neq 0 (goto:error)
)

echo.
echo Exiting
goto:finish

:error
echo.
echo Halting due to error
goto:finish

:finish
if not "%cd%\" == "%BUILD_DIR%" ( cd %BUILD_DIR% )
endlocal
goto:eof



REM // ======== Functions ======== //

:create_glslang
   echo.
   echo Creating local glslang repository %GLSLANG_DIR%)
   mkdir %GLSLANG_DIR%
   cd %GLSLANG_DIR%
   git clone https://github.com/KhronosGroup/glslang.git .
   git checkout %GLSLANG_REVISION%
   if not exist %GLSLANG_DIR%\SPIRV (
      echo glslang source download failed!
      set errorCode=1
   )
goto:eof

:update_glslang
   echo.
   echo Updating %GLSLANG_DIR%
   cd %GLSLANG_DIR%
   git fetch --all
   git checkout %GLSLANG_REVISION%
goto:eof

:create_spirv-tools
   echo.
   echo Creating local spirv-tools repository %SPIRV_TOOLS_DIR%)
   mkdir %SPIRV_TOOLS_DIR%
   cd %SPIRV_TOOLS_DIR%
   git clone https://github.com/KhronosGroup/SPIRV-Tools.git .
   git checkout %SPIRV_TOOLS_REVISION%
   if not exist %SPIRV_TOOLS_DIR%\source (
      echo spirv-tools source download failed!
      set errorCode=1
   )
goto:eof

:update_spirv-tools
   echo.
   echo Updating %SPIRV_TOOLS_DIR%
   cd %SPIRV_TOOLS_DIR%
   git fetch --all
   git checkout %SPIRV_TOOLS_REVISION%
goto:eof

:create_shaderc
   echo.
   echo Creating local shaderc repository %SHADERC_DIR%)
   mkdir %SHADERC_DIR%
   cd %SHADERC_DIR%
   git clone git@github.com:google/shaderc.git .
   git checkout %SHADERC_REVISION%
   if not exist %SHADERC_DIR%\libshaderc (
      echo shaderc source download failed!
      set errorCode=1
   )
goto:eof

:update_shaderc
   echo.
   echo Updating %SHADERC_DIR%
   cd %SHADERC_DIR%
   git fetch --all
   git checkout %SHADERC_REVISION%
goto:eof

:build_shaderc
   echo.
   echo Building %SHADERC_DIR%
   cd %SHADERC_DIR%\android_test
   echo Building shaderc with Android NDK
   call ndk-build THIRD_PARTY_PATH=../.. -j 4
   REM Check for existence of one lib, even though we should check for all results
   if not exist %SHADERC_DIR%\android_test\obj\local\x86\libshaderc.a (
      echo.
      echo shaderc build failed!
      set errorCode=1
   )
goto:eof
