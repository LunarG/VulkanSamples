@echo off
REM Update source for glslang (and eventually LunarGLASS)

setlocal EnableDelayedExpansion
set errorCode=0
set BUILD_DIR=%~dp0
set BASE_DIR=%BUILD_DIR%..
set GLSLANG_DIR=%BASE_DIR%\glslang
set LUNARGLASS_DIR=%BASE_DIR%\LunarGLASS

REM // ======== Parameter parsing ======== //

   if "%1" == "" (
      echo usage: update_external_sources.bat [options]
      echo.
      echo Available options:
      echo   --sync-glslang      just pull glslang_revision
      echo   --sync-LunarGLASS   just pull LunarGLASS_revision
      echo   --build-glslang     pulls glslang_revision, configures CMake, builds Release and Debug
      echo   --build-LunarGLASS  pulls LunarGLASS_revision, configures CMake, builds Release and Debug
      echo   --all               sync and build both glslang and LunarGLASS
      goto:finish
   )

   set sync-glslang=0
   set sync-LunarGLASS=0
   set build-glslang=0
   set build-LunarGLASS=0
   set check-fetch-dependencies=0
   set check-glslang-build-dependencies=0
   set check-LunarGLASS-build-dependencies=0

   :parameterLoop

      if "%1"=="" goto:parameterContinue

      if "%1" == "--sync-glslang" (
         set sync-glslang=1
		   shift
		   goto:parameterLoop
      )

      if "%1" == "--sync-LunarGLASS" (
         set sync-LunarGLASS=1
         set check-fetch-dependencies=1
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

      if "%1" == "--build-LunarGLASS" (
         set sync-LunarGLASS=1
         set check-fetch-dependencies=1
         set check-LunarGLASS-build-dependencies=1
         set build-LunarGLASS=1
         shift
         goto:parameterLoop
      )

      if "%1" == "--all" (
         set sync-glslang=1
         set sync-LunarGLASS=1
         set build-glslang=1
         set build-LunarGLASS=1
         set check-fetch-dependencies=1
         set check-glslang-build-dependencies=1
         set check-LunarGLASS-build-dependencies=1
         shift
         goto:parameterLoop
      )

      echo Unrecognized options "%1"
      goto:error

   :parameterContinue

REM // ======== end Parameter parsing ======== //


REM // ======== Dependency checking ======== //

   for %%X in (svn.exe) do (set FOUND=%%~$PATH:X)
   if not defined FOUND (
      echo Dependency check failed:
      echo   svn.exe not found
      echo   Get Subversion for Windows here:  http://sourceforge.net/projects/win32svn/
      echo   Install and ensure the svn.exe makes it into your PATH, default is "C:\Program Files (x86)\Subversion\bin"
      set errorCode=1
   )

   if %check-fetch-dependencies% equ 1 (
      for %%X in (wget.exe) do (set FOUND=%%~$PATH:X)
      if not defined FOUND (
         echo Dependency check failed:
         echo   wget.exe not found
         echo   Get wget for Windows here:  http://gnuwin32.sourceforge.net/packages/wget.htm
         echo   Easiest to select "Complete package, except sources" link which will install and setup PATH
         echo   Install and ensure each makes it into your PATH, default is "C:\Program Files (x86)\GnuWin32\bin"
         set errorCode=1
      )

      for %%X in (gzip.exe) do (set FOUND=%%~$PATH:X)
      if not defined FOUND (
         echo Dependency check failed:
         echo   gzip.exe not found
         echo   Get gzip for Windows here:  http://gnuwin32.sourceforge.net/packages/gzip.htm
         echo   Easiest to select "Complete package, except sources" link which will install and setup PATH
         echo   Install and ensure each makes it into your PATH, default is "C:\Program Files (x86)\GnuWin32\bin"
         set errorCode=1
      )

      for %%X in (tar.exe) do (set FOUND=%%~$PATH:X)
      if not defined FOUND (
         echo Dependency check failed:
         echo   tar.exe not found
         echo   Get tar for Windows here:  http://gnuwin32.sourceforge.net/packages/gtar.htm
         echo   Easiest to select Binaries/Setup link which will install and setup PATH
         echo   Install and ensure each makes it into your PATH, default is "C:\Program Files (x86)\GnuWin32\bin"
         set errorCode=1
      )
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

   if %check-LunarGLASS-build-dependencies% equ 1 (
      for %%X in (python.exe) do (set FOUND=%%~$PATH:X)
      if not defined FOUND (
         echo Dependency check failed:
         echo   python.exe not found
         echo   Get python 2.7x for Windows here:  http://www.python.org/download/releases/2.7.6/
         echo   Install and ensure each makes it into your PATH, default is "C:\Python27"
         set errorCode=1
      )

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
if not exist LunarGLASS_revision (
   echo.
   echo Missing LunarGLASS_revision file!  Place it next to this script with target version in it.
   set errorCode=1
   goto:error
)

if not exist glslang_revision (
   echo.
   echo Missing glslang_revision file!  Place it next to this script with target version in it.
   set errorCode=1
   goto:error
)

set /p LUNARGLASS_REVISION= < LunarGLASS_revision
set /p GLSLANG_REVISION= < glslang_revision
echo LUNARGLASS_REVISION=%LUNARGLASS_REVISION%
echo GLSLANG_REVISION=%GLSLANG_REVISION%

echo Creating and/or updating glslang and LunarGLASS in %BASE_DIR%

if %sync-glslang% equ 1 (
   if not exist %GLSLANG_DIR% (
      call:create_glslang
   )
   if %errorCode% neq 0 (goto:error)
   call:update_glslang
   if %errorCode% neq 0 (goto:error)
)

if %sync-LunarGLASS% equ 1 (
   if not exist %LUNARGLASS_DIR% (
      call:create_LunarGLASS
   )
   if %errorCode% neq 0 (goto:error)
   call:update_LunarGLASS
   if %errorCode% neq 0 (goto:error)
)

if %build-glslang% equ 1 (
   call:build_glslang
   if %errorCode% neq 0 (goto:error)
)

if %build-LunarGLASS% equ 1 (
   call:build_LunarGLASS
   if %errorCode% neq 0 (goto:error)
)

REM If we made it here, we are golden
echo.
echo Success
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
   REM Windows complains if it can't find the directory below, no need to call
   REM rd /S /Q %GLSLANG_DIR%
   echo.
   echo Creating local glslang repository %GLSLANG_DIR%)
   mkdir %GLSLANG_DIR%
   cd %GLSLANG_DIR%
   svn checkout https://cvs.khronos.org/svn/repos/ogl/trunk/ecosystem/public/sdk/tools/glslang@%GLSLANG_REVISION% .
   if not exist %GLSLANG_DIR%\SPIRV (
      echo glslang source download failed!
      set errorCode=1
   )
goto:eof

:update_glslang
   echo.
   echo Updating %GLSLANG_DIR%
   cd %GLSLANG_DIR%
   svn update -r %GLSLANG_REVISION%
   REM Just in case we are moving backward, do a revert
   svn revert -R .
goto:eof

:create_LunarGLASS
   REM Windows complains if it can't find the directory below, no need to call
   REM rd /S /Q %LUNARGLASS_DIR%
   echo.
   echo Creating local LunarGLASS repository %LUNARGLASS_DIR%)
   mkdir %LUNARGLASS_DIR%\Core\LLVM
   cd %LUNARGLASS_DIR%\Core\LLVM
   echo.
   echo Downloading LLVM archive...
   wget http://llvm.org/releases/3.4/llvm-3.4.src.tar.gz
   REM tar on windows can't filter through gzip, so the below line doesn't work
   REM tar --gzip -xf llvm-3.4.src.tar.gz
   echo.
   echo Unzipping the archive...
   echo gzip --decompress --verbose --keep llvm-3.4.src.tar.gz
   gzip --decompress --verbose --keep llvm-3.4.src.tar.gz
   echo.
   echo Extracting the archive... (this is slow)
   echo tar -xf llvm-3.4.src.tar
   tar -xf llvm-3.4.src.tar
   if not exist %LUNARGLASS_DIR%\Core\LLVM\llvm-3.4\lib (
      echo .
      echo LLVM source download failed!
      echo Delete LunarGLASS directory and try again
      set errorCode=1
      goto:eof
   )
   echo.
   echo Syncing LunarGLASS source...
   cd %LUNARGLASS_DIR%
   svn checkout --force https://lunarglass.googlecode.com/svn/trunk/ .
   svn revert --depth=infinity .
   if not exist %LUNARGLASS_DIR%\Frontends\SPIRV (
      echo.
      echo LunarGLASS source download failed!
      set errorCode=1
   )
goto:eof

:update_LunarGLASS
   echo.
   echo Updating %LUNARGLASS_DIR%
   cd %LUNARGLASS_DIR%
   svn update -r %LUNARGLASS_REVISION%
   REM Just in case we are moving backward, do a revert
   svn revert -R .
goto:eof

:build_glslang
   echo.
   echo Building %GLSLANG_DIR%
   cd  %GLSLANG_DIR%
   mkdir build
   set GLSLANG_BUILD_DIR=%GLSLANG_DIR%\build
   cd %GLSLANG_BUILD_DIR%
   cmake -G"Visual Studio 12 2013 Win64" -DCMAKE_INSTALL_PREFIX=install ..
   msbuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Release
   REM Check for existence of one lib, even though we should check for all results
   if not exist %GLSLANG_BUILD_DIR%\glslang\Release\glslang.lib (
      echo.
      echo glslang Release build failed!
      set errorCode=1
   )
   msbuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Debug
   REM Check for existence of one lib, even though we should check for all results
   if not exist %GLSLANG_BUILD_DIR%\glslang\Debug\glslang.lib (
      echo.
      echo glslang Debug build failed!
      set errorCode=1
   )
goto:eof

:build_LunarGLASS
   echo.
   echo Building %LUNARGLASS_DIR%
   set LLVM_DIR=%LUNARGLASS_DIR%\Core\LLVM\llvm-3.4
   cd %LLVM_DIR%
   mkdir build
   set LLVM_BUILD_DIR=%LLVM_DIR%\build
   cd %LLVM_BUILD_DIR%
   cmake -G"Visual Studio 12 2013 Win64" -DCMAKE_INSTALL_PREFIX=install ..
   msbuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Release
   REM Check for existence of one lib, even though we should check for all results
   if not exist %LLVM_BUILD_DIR%\lib\Release\LLVMCore.lib (
      echo.
      echo LLVM Release build failed!
      set errorCode=1
      goto:eof
   )
   msbuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Debug
   REM Check for existence of one lib, even though we should check for all results
   if not exist %LLVM_BUILD_DIR%\lib\Debug\LLVMCore.lib (
      echo.
      echo LLVM Debug build failed!
      set errorCode=1
      goto:eof
   )
   cd %LUNARGLASS_DIR%
   mkdir build
   set LUNARGLASS_BUILD_DIR=%LUNARGLASS_DIR%\build
   cd %LUNARGLASS_BUILD_DIR%
   cmake -G"Visual Studio 12 2013 Win64" -DCMAKE_INSTALL_PREFIX=install ..
   msbuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Release
   REM Check for existence of one lib, even though we should check for all results
   if not exist %LUNARGLASS_BUILD_DIR%\Core\Release\core.lib (
      echo.
      echo LunarGLASS build failed!
      set errorCode=1
      goto:eof
   )
   msbuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Debug
   REM Check for existence of one lib, even though we should check for all results
   if not exist %LUNARGLASS_BUILD_DIR%\Core\Debug\core.lib (
      echo.
      echo LunarGLASS build failed!
      set errorCode=1
      goto:eof
   )
goto:eof
