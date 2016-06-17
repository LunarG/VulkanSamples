@echo off
REM Update source for glslang, spirv-tools

REM Determine the appropriate CMake strings for the current version of Visual Studio
echo Determining VS version
python .\determine_vs_version.py > vsversion.tmp
set /p VS_VERSION=< vsversion.tmp
echo Detected Visual Studio Version as %VS_VERSION%

REM Cleanup the file we used to collect the VS version output since it's no longer needed.
del /Q /F vsversion.tmp

setlocal EnableDelayedExpansion
set errorCode=0
set BUILD_DIR=%~dp0
set BASE_DIR=%BUILD_DIR%external
set GLSLANG_DIR=%BASE_DIR%\glslang
set SPIRV_TOOLS_DIR=%BASE_DIR%\spirv-tools

REM // ======== Parameter parsing ======== //

   if "%1" == "" (
      echo usage: update_external_sources.bat [options]
      echo.
      echo Available options:
      echo   --sync-glslang      just pull glslang_revision
      echo   --sync-spirv-tools  just pull spirv-tools_revision
      echo   --build-glslang     pulls glslang_revision, configures CMake, builds Release and Debug
      echo   --build-spirv-tools pulls spirv-tools_revision, configures CMake, builds Release and Debug
      echo   --all               sync and build glslang, LunarGLASS, spirv-tools
      goto:finish
   )

   set sync-glslang=0
   set sync-spirv-tools=0
   set build-glslang=0
   set build-spirv-tools=0
   set check-glslang-build-dependencies=0

   :parameterLoop

      if "%1"=="" goto:parameterContinue

      if "%1" == "--sync-glslang" (
         set sync-glslang=1
         shift
         goto:parameterLoop
      )

      if "%1" == "--sync-spirv-tools" (
         set sync-spirv-tools=1
         shift
         goto:parameterLoop
      )

      if "%1" == "--build-glslang" (
         set sync-glslang=1
         set check-glslang-build-dependencies=1
         set build-glslang=1
         shift
         goto:parameterLoop
      )

      if "%1" == "--build-spirv-tools" (
         set sync-spirv-tools=1
         REM glslang has the same needs as spirv-tools
         set check-glslang-build-dependencies=1
         set build-spirv-tools=1
         shift
         goto:parameterLoop
      )

      if "%1" == "--all" (
         set sync-glslang=1
         set sync-spirv-tools=1
         set build-glslang=1
         set build-spirv-tools=1
         set check-glslang-build-dependencies=1
         shift
         goto:parameterLoop
      )

      echo Unrecognized options "%1"
      goto:error

   :parameterContinue

REM // ======== end Parameter parsing ======== //


REM // ======== Dependency checking ======== //
   REM git is required for all paths
   for %%X in (git.exe) do (set FOUND=%%~$PATH:X)
   if not defined FOUND (
      echo Dependency check failed:
      echo   git.exe not found
      echo   Git for Windows can be downloaded here:  https://git-scm.com/download/win
      echo   Install and ensure git.exe makes it into your PATH
      set errorCode=1
   )

   if %check-glslang-build-dependencies% equ 1 (
      for %%X in (cmake.exe) do (set FOUND=%%~$PATH:X)
      if not defined FOUND (
         echo Dependency check failed:
         echo   cmake.exe not found
         echo   Get CNake 2.8 for Windows here:  http://www.cmake.org/cmake/resources/software.html
         echo   Install and ensure each makes it into your PATH, default is "C:\Program Files (x86)\CMake\bin"
         set errorCode=1
      )
   )


   REM goto:main

REM // ======== end Dependency checking ======== //

:main

if %errorCode% neq 0 (goto:error)

REM Read the target versions from external file, which is shared with Linux script

if not exist glslang_revision (
   echo.
   echo Missing glslang_revision file!  Place it next to this script with target version in it.
   set errorCode=1
   goto:error
)

if not exist spirv-tools_revision (
   echo.
   echo Missing spirv-tools_revision file!  Place it next to this script with target version in it.
   set errorCode=1
   goto:error
)

set /p GLSLANG_REVISION= < glslang_revision
set /p SPIRV_TOOLS_REVISION= < spirv-tools_revision
echo GLSLANG_REVISION=%GLSLANG_REVISION%
echo SPIRV_TOOLS_REVISION=%SPIRV_TOOLS_REVISION%


echo Creating and/or updating glslang, spirv-tools in %BASE_DIR%

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

if %build-glslang% equ 1 (
   call:build_glslang
   if %errorCode% neq 0 (goto:error)
)

if %build-spirv-tools% equ 1 (
   call:build_spirv-tools
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

:build_glslang
   echo.
   echo Building %GLSLANG_DIR%
   cd  %GLSLANG_DIR%

   REM Cleanup any old directories lying around.
   if exist build32 (
      rmdir /s /q build32
   )
   if exist build (
      rmdir /s /q build
   )
   
   echo Making 32-bit glslang
   echo *************************
   mkdir build32
   set GLSLANG_BUILD_DIR=%GLSLANG_DIR%\build32
   cd %GLSLANG_BUILD_DIR%

   echo Generating 32-bit Glslang CMake files for Visual Studio %VS_VERSION% -DCMAKE_INSTALL_PREFIX=install ..
   cmake -G "Visual Studio %VS_VERSION%" -DCMAKE_INSTALL_PREFIX=install ..
   
   echo Building 32-bit Glslang: MSBuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Debug
   msbuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Debug /verbosity:quiet
   
   REM Check for existence of one lib, even though we should check for all results
   if not exist %GLSLANG_BUILD_DIR%\glslang\Debug\glslang.lib (
      echo.
      echo glslang 32-bit Debug build failed!
      set errorCode=1
   )
   echo Building Glslang: MSBuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Release
   msbuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Release /verbosity:quiet
   
   REM Check for existence of one lib, even though we should check for all results
   if not exist %GLSLANG_BUILD_DIR%\glslang\Release\glslang.lib (
      echo.
      echo glslang 32-bit Release build failed!
      set errorCode=1
   )
   
   cd ..
 
   echo Making 64-bit glslang
   echo *************************
   mkdir build
   set GLSLANG_BUILD_DIR=%GLSLANG_DIR%\build
   cd %GLSLANG_BUILD_DIR%

   echo Generating 64-bit Glslang CMake files for Visual Studio %VS_VERSION% -DCMAKE_INSTALL_PREFIX=install ..
   cmake -G "Visual Studio %VS_VERSION% Win64" -DCMAKE_INSTALL_PREFIX=install ..
   
   echo Building 64-bit Glslang: MSBuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Debug
   msbuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Debug /verbosity:quiet
   
   REM Check for existence of one lib, even though we should check for all results
   if not exist %GLSLANG_BUILD_DIR%\glslang\Debug\glslang.lib (
      echo.
      echo glslang 64-bit Debug build failed!
      set errorCode=1
   )
   echo Building Glslang: MSBuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Release
   msbuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Release /verbosity:quiet
   
   REM Check for existence of one lib, even though we should check for all results
   if not exist %GLSLANG_BUILD_DIR%\glslang\Release\glslang.lib (
      echo.
      echo glslang 64-bit Release build failed!
      set errorCode=1
   )
goto:eof

:build_spirv-tools
   echo.
   echo Building %SPIRV_TOOLS_DIR%
   cd  %SPIRV_TOOLS_DIR%

   REM Cleanup any old directories lying around.
   if exist build32 (
      rmdir /s /q build32
   )
   if exist build (
      rmdir /s /q build
   )

   echo Making 32-bit spirv-tools
   echo *************************
   mkdir build32
   set SPIRV_TOOLS_BUILD_DIR=%SPIRV_TOOLS_DIR%\build32

   cd %SPIRV_TOOLS_BUILD_DIR%
   
   echo Generating 32-bit spirv-tools CMake files for Visual Studio %VS_VERSION% ..
   cmake -G "Visual Studio %VS_VERSION%" ..
   
   echo Building 32-bit spirv-tools: MSBuild ALL_BUILD.vcxproj /p:Platform=x86 /p:Configuration=Debug
   msbuild ALL_BUILD.vcxproj /p:Platform=x86 /p:Configuration=Debug /verbosity:quiet
   
   REM Check for existence of one lib, even though we should check for all results
   if not exist %SPIRV_TOOLS_BUILD_DIR%\source\Debug\SPIRV-Tools.lib (
      echo.
      echo spirv-tools 32-bit Debug build failed!
      set errorCode=1
   )
   
   echo Building 32-bit spirv-tools: MSBuild ALL_BUILD.vcxproj /p:Platform=x86 /p:Configuration=Release
   msbuild ALL_BUILD.vcxproj /p:Platform=x86 /p:Configuration=Release /verbosity:quiet

   REM Check for existence of one lib, even though we should check for all results
   if not exist %SPIRV_TOOLS_BUILD_DIR%\source\Release\SPIRV-Tools.lib (
      echo.
      echo spirv-tools 32-bit Release build failed!
      set errorCode=1
   )
   
   cd ..
 
   echo Making 64-bit spirv-tools  
   echo *************************
   mkdir build
   set SPIRV_TOOLS_BUILD_DIR=%SPIRV_TOOLS_DIR%\build
   cd %SPIRV_TOOLS_BUILD_DIR%

   echo Generating 64-bit spirv-tools CMake files for Visual Studio %VS_VERSION% ..
   cmake -G "Visual Studio %VS_VERSION% Win64" ..
   
   echo Building 64-bit spirv-tools: MSBuild ALL_BUILD.vcxproj /p:Platform=x64 /p:Configuration=Debug
   msbuild ALL_BUILD.vcxproj /p:Platform=x64 /p:Configuration=Debug /verbosity:quiet
   
   REM Check for existence of one lib, even though we should check for all results
   if not exist %SPIRV_TOOLS_BUILD_DIR%\source\Debug\SPIRV-Tools.lib (
      echo.
      echo spirv-tools 64-bit Debug build failed!
      set errorCode=1
   )
   
   echo Building 64-bit spirv-tools: MSBuild ALL_BUILD.vcxproj /p:Platform=x64 /p:Configuration=Release
   msbuild ALL_BUILD.vcxproj /p:Platform=x64 /p:Configuration=Release /verbosity:quiet

   REM Check for existence of one lib, even though we should check for all results
   if not exist %SPIRV_TOOLS_BUILD_DIR%\source\Release\SPIRV-Tools.lib (
      echo.
      echo spirv-tools 64-bit Release build failed!
      set errorCode=1
   )
goto:eof
