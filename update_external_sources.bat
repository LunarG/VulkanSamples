@echo off
REM Update source for glslang

REM Determine the appropriate CMake strings for the current version of Visual Studio
echo Determining VS version
python .\scripts\determine_vs_version.py > vsversion.tmp
set /p VS_VERSION=< vsversion.tmp
echo Detected Visual Studio Version as %VS_VERSION%

REM Cleanup the file we used to collect the VS version output since it's no longer needed.
del /Q /F vsversion.tmp

setlocal EnableDelayedExpansion
set errorCode=0
set BUILD_DIR=%~dp0
set BASE_DIR="%BUILD_DIR%external"
set REVISION_DIR="%BUILD_DIR%external_revisions"
set GLSLANG_DIR=%BASE_DIR%\glslang
set do_32=0
set do_64=0
set do_debug=0
set do_release=0

REM // ======== Parameter parsing ======== //

   set arg-use-implicit-component-list=1
   set arg-do-glslang=0
   set arg-no-sync=0
   set arg-no-build=0
   set arg-32=0
   set arg-64=0
   set arg-debug=0
   set arg-release=0

   :parameterLoop

      if "%1"=="" goto:parameterContinue

      if "%1" == "--glslang" (
         set arg-do-glslang=1
         set arg-use-implicit-component-list=0
         echo Building glslang ^(%1^)
         shift
         goto:parameterLoop
      )

      if "%1" == "-g" (
         set arg-do-glslang=1
         set arg-use-implicit-component-list=0
         echo Building glslang ^(%1^)
         shift
         goto:parameterLoop
      )

      if "%1" == "--no-sync" (
         set arg-no-sync=1
         echo Skipping sync ^(%1^)
         shift
         goto:parameterLoop
      )

      if "%1" == "--no-build" (
         set arg-no-build=1
         echo Skipping build ^(%1^)
         shift
         goto:parameterLoop
      )

      if "%1" == "--32" (
         set arg-32=1
         echo 32-bit build requested
         shift
         goto:parameterLoop
      )

      if "%1" == "--64" (
         set arg-64=1
         echo 64-bit build requested
         shift
         goto:parameterLoop
      )

      if "%1" == "--debug" (
         set arg-debug=1
         echo debug build requested
         shift
         goto:parameterLoop
      )

      if "%1" == "--release" (
         set arg-release=1
         echo release build requested
         shift
         goto:parameterLoop
      )

      if "%1" == "--spirv-tools" (
         echo --spirv-tools argument has been deprecated and is no longer necessary
         shift
         goto:parameterLoop
      )

      if "%1" == "-s" (
         echo --s argument has been deprecated and is no longer necessary
         shift
         goto:parameterLoop
      )

      if "%1" == "--all" (
         echo --all argument has been deprecated and is no longer necessary
         set arg-do-glslang=1
         set arg-use-implicit-component-list=0
         echo Building glslang ^(%1^)
         shift
         goto:parameterLoop
      )

      echo.
      echo Unrecognized option "%1"
      echo.
      echo usage: update_external_sources.bat [options]
      echo.
      echo   Available options:
      echo     -g ^| --glslang      enable glslang component
      echo     --all               enable all components
      echo     --no-sync           skip sync from git
      echo     --no-build          skip build
      echo     --32                build for 32-bit
      echo     --64                build for 64-bit
      echo     --debug             build for debug
      echo     --release           build for release
      echo.
      echo   If any component enables are provided, only those components are enabled.
      echo   If no component enables are provided, all components are enabled.
      echo.
      echo   Sync uses git to pull a specific revision.
      echo   Build configures CMake, builds Release and Debug.
      echo.
      echo   --32 without --64 builds only 32-bit, and vice-versa.
      echo   --debug without --release builds only debug, and vice-versa.
      echo   Specifying neither or both --32 and --64 builds both.
      echo   Specifying neither or both --debug and --release builds both.
      echo   So specifying none of these 4 options (default) builds all 4.


      goto:error

   :parameterContinue

   if %arg-use-implicit-component-list% equ 1 (
      echo Building glslang
      set arg-do-glslang=1
   )

   set sync-glslang=0
   set build-glslang=0
   set check-glslang-build-dependencies=0

   if %arg-do-glslang% equ 1 (
      if %arg-no-sync% equ 0 (
         set sync-glslang=1
      )
      if %arg-no-build% equ 0 (
         set check-glslang-build-dependencies=1
         set build-glslang=1
      )
   )

   if %arg-32% equ 1 (
       set do_32=1
   )
   if %arg-64% equ 1 (
       set do_64=1
   )
   if %arg-32% equ 0 (
      if %arg-64% equ 0 (
          set do_32=1
          set do_64=1
      )
   )

   if %arg-debug% equ 1 (
       set do_debug=1
   )
   if %arg-release% equ 1 (
       set do_release=1
   )
   if %arg-debug% equ 0 (
      if %arg-release% equ 0 (
          set do_debug=1
          set do_release=1
      )
   )

   REM this is a debugging aid that can be enabled while debugging command-line parsing
   if 0 equ 1 (
      set arg
      set sync-glslang
      set build-glslang
      set check-glslang-build-dependencies
      goto:error
   )

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
         echo   Get CMake for Windows here:  http://www.cmake.org/cmake/resources/software.html
         echo   Install and ensure each makes it into your PATH, default is "C:\Program Files (x86)\CMake\bin"
         set errorCode=1
      )
   )


   REM goto:main

REM // ======== end Dependency checking ======== //

:main

if %errorCode% neq 0 (goto:error)

REM Read the target versions from external file, which is shared with Linux script

if not exist %REVISION_DIR%\glslang_giturl (
   echo.
   echo Missing glslang_giturl file!  Place it in %REVISION_DIR% with git repo URL in it.
   set errorCode=1
   goto:error
)

if not exist %REVISION_DIR%\glslang_revision (
   echo.
   echo Missing glslang_revision file!  Place it in %REVISION_DIR% with target version in it.
   set errorCode=1
   goto:error
)

set /p GLSLANG_GITURL= < %REVISION_DIR%\glslang_giturl
set /p GLSLANG_REVISION= < %REVISION_DIR%\glslang_revision

echo GLSLANG_GITURL=%GLSLANG_GITURL%
echo GLSLANG_REVISION=%GLSLANG_REVISION%


echo Creating and/or updating glslang in %BASE_DIR%

if %sync-glslang% equ 1 (
   if not exist %GLSLANG_DIR% (
      call:create_glslang
   )
   if %errorCode% neq 0 (goto:error)
   call:update_glslang
   if %errorCode% neq 0 (goto:error)
)

if %build-glslang% equ 1 (
   call:build_glslang
   if %errorCode% neq 0 (goto:error)
)

echo.
echo Exiting
goto:finish

:error
echo.
echo Halting due to error
set errorCode=1
goto:finish

:finish
if not "%cd%\" == "%BUILD_DIR%" ( cd %BUILD_DIR% )
exit /b %errorCode%


REM // ======== Functions ======== //

:create_glslang
   echo.
   echo Creating local glslang repository %GLSLANG_DIR%)
   mkdir %GLSLANG_DIR%
   cd %GLSLANG_DIR%
   git clone %GLSLANG_GITURL% .
   git checkout %GLSLANG_REVISION%
   python.exe .\update_glslang_sources.py
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
   python.exe .\update_glslang_sources.py
goto:eof

:build_glslang
   echo.
   echo Building %GLSLANG_DIR%
   cd  %GLSLANG_DIR%

   if not exist build32 (
       mkdir build32
   )
   if not exist build (
       mkdir build
   )

   set GLSLANG_BUILD_DIR=%GLSLANG_DIR%\build32
   if %do_32% equ 1 (
      echo Making 32-bit glslang
      echo *************************
      cd %GLSLANG_BUILD_DIR%

      echo Generating 32-bit Glslang CMake files for Visual Studio %VS_VERSION% -DCMAKE_INSTALL_PREFIX=install ..
      cmake -G "Visual Studio %VS_VERSION%" -DCMAKE_INSTALL_PREFIX=install ..

      if %do_debug% equ 1 (
         echo Building 32-bit Glslang: MSBuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Debug
         msbuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Debug /verbosity:quiet

         REM Check for existence of one lib, even though we should check for all results
         if not exist %GLSLANG_BUILD_DIR%\glslang\Debug\glslangd.lib (
            echo.
            echo glslang 32-bit Debug build failed!
            set errorCode=1
         )
      )
      if %do_release% equ 1 (
         echo Building Glslang: MSBuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Release
         msbuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Release /verbosity:quiet

         REM Check for existence of one lib, even though we should check for all results
         if not exist %GLSLANG_BUILD_DIR%\glslang\Release\glslang.lib (
            echo.
            echo glslang 32-bit Release build failed!
            set errorCode=1
         )
      )
      cd ..
   )

   set GLSLANG_BUILD_DIR=%GLSLANG_DIR%\build
   if %do_64% equ 1 (
      echo Making 64-bit glslang
      echo *************************
      cd %GLSLANG_BUILD_DIR%

      echo Generating 64-bit Glslang CMake files for Visual Studio %VS_VERSION% -DCMAKE_INSTALL_PREFIX=install ..
      cmake -G "Visual Studio %VS_VERSION% Win64" -DCMAKE_INSTALL_PREFIX=install ..

      if %do_debug% equ 1 (
         echo Building 64-bit Glslang: MSBuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Debug
         msbuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Debug /verbosity:quiet

         REM Check for existence of one lib, even though we should check for all results
         if not exist %GLSLANG_BUILD_DIR%\glslang\Debug\glslangd.lib (
            echo.
            echo glslang 64-bit Debug build failed!
            set errorCode=1
         )
      )
      if %do_release% equ 1 (
         echo Building Glslang: MSBuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Release
         msbuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Release /verbosity:quiet

         REM Check for existence of one lib, even though we should check for all results
         if not exist %GLSLANG_BUILD_DIR%\glslang\Release\glslang.lib (
            echo.
            echo glslang 64-bit Release build failed!
            set errorCode=1
         )
      )
      cd ..
   )

goto:eof
