@echo off
REM Update source for glslang, LunarGLASS, spirv-tools

REM Determine the appropriate CMake strings for the current version of Visual Studio
echo Determining VS version
python .\determine_vs_version.py > vsversion.tmp
set /p VS_VERSION=< vsversion.tmp
echo Detected Visual Studio Version as %VS_VERSION%

REM from that information set the appropriate MSBUILD machine target as weell.
echo Determining MSBUILD target
set MSBUILD_MACHINE_TARGET=x64
@setlocal
@echo off
echo.%VS_VERSION% | findstr /C:"Win32" 1>nul
if errorlevel 1 (
  echo.
) ELSE (
  set MSBUILD_MACHINE_TARGET=x86
)
endlocal
echo Detected MSBuild target as %MSBUILD_MACHINE_TARGET%

REM Cleanup the file we used to collect the VS version output since it's no longer needed.
del /Q /F vsversion.tmp

REM Determine if SVN exists, this is a requirement for LunarGLASS
set SVN_EXE_FOUND=0
for %%X in (svn.exe) do (set FOUND=%%~$PATH:X)
if defined FOUND (
 set SVN_EXE_FOUND=1
)

setlocal EnableDelayedExpansion
set errorCode=0
set BUILD_DIR=%~dp0
set BASE_DIR=%BUILD_DIR%..
set GLSLANG_DIR=%BASE_DIR%\glslang
set LUNARGLASS_DIR=%BASE_DIR%\LunarGLASS
set SPIRV_TOOLS_DIR=%BASE_DIR%\spirv-tools

REM // ======== Parameter parsing ======== //

   if "%1" == "" (
      echo usage: update_external_sources.bat [options]
      echo.
      echo Available options:
      echo   --sync-glslang      just pull glslang_revision
      echo   --sync-LunarGLASS   just pull LunarGLASS_revision
      echo   --sync-spirv-tools  just pull spirv-tools_revision
      echo   --build-glslang     pulls glslang_revision, configures CMake, builds Release and Debug
      echo   --build-LunarGLASS  pulls LunarGLASS_revision, configures CMake, builds Release and Debug
      echo   --build-spirv-tools pulls spirv-tools_revision, configures CMake, builds Release and Debug
      echo   --all               sync and build glslang, LunarGLASS, spirv-tools
      goto:finish
   )

   set sync-glslang=0
   set sync-LunarGLASS=0
   set sync-spirv-tools=0
   set build-glslang=0
   set build-LunarGLASS=0
   set build-spirv-tools=0
   set check-glslang-build-dependencies=0
   set check-LunarGLASS-fetch-dependencies=0
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
         set check-LunarGLASS-fetch-dependencies=1
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

      if "%1" == "--build-LunarGLASS" (
         set sync-LunarGLASS=1
         set check-LunarGLASS-fetch-dependencies=1
         set check-LunarGLASS-build-dependencies=1
         set build-LunarGLASS=1
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
         
         REM Only attempt to build LunarGLASS if we find SVN
         if %SVN_EXE_FOUND% equ 1 (
             set sync-LunarGLASS=1
             set build-LunarGLASS=1
             set check-LunarGLASS-fetch-dependencies=1
             set check-LunarGLASS-build-dependencies=1
         )
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

   if %check-LunarGLASS-fetch-dependencies% equ 1 (
      if %SVN_EXE_FOUND% equ 0 (
         echo Dependency check failed:
         echo   svn.exe not found
         echo   Get Subversion for Windows here:  http://sourceforge.net/projects/win32svn/
         echo   Install and ensure svn.exe makes it into your PATH, default is "C:\Program Files (x86)\Subversion\bin"
         set errorCode=1
      )

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

if not exist spirv-tools_revision (
   echo.
   echo Missing spirv-tools_revision file!  Place it next to this script with target version in it.
   set errorCode=1
   goto:error
)

set /p LUNARGLASS_REVISION= < LunarGLASS_revision
set /p GLSLANG_REVISION= < glslang_revision
set /p SPIRV_TOOLS_REVISION= < spirv-tools_revision
echo LUNARGLASS_REVISION=%LUNARGLASS_REVISION%
echo GLSLANG_REVISION=%GLSLANG_REVISION%
echo SPIRV_TOOLS_REVISION=%SPIRV_TOOLS_REVISION%

set /p LUNARGLASS_REVISION_R32= < LunarGLASS_revision_R32
echo LUNARGLASS_REVISION_R32=%LUNARGLASS_REVISION_R32%

echo Creating and/or updating glslang, LunarGLASS, spirv-tools in %BASE_DIR%

if %sync-glslang% equ 1 (
   rd /S /Q %GLSLANG_DIR%
   if not exist %GLSLANG_DIR% (
      call:create_glslang
   )
   if %errorCode% neq 0 (goto:error)
   call:update_glslang
   if %errorCode% neq 0 (goto:error)
)

if %sync-LunarGLASS% equ 1 (
   rd /S /Q %LUNARGLASS_DIR%
   if not exist %LUNARGLASS_DIR% (
      call:create_LunarGLASS
   )
   if %errorCode% neq 0 (goto:error)
   call:update_LunarGLASS
   if %errorCode% neq 0 (goto:error)
)

if %sync-spirv-tools% equ 1 (
   rd /S /Q %SPIRV_TOOLS_DIR%
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

if %build-LunarGLASS% equ 1 (
   call:build_LunarGLASS
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
   git clone git@gitlab.khronos.org:GLSL/glslang.git .
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

:create_LunarGLASS
   REM Windows complains if it can't find the directory below, no need to call
   REM rd /S /Q %LUNARGLASS_DIR%
   echo.
   echo Creating local LunarGLASS repository %LUNARGLASS_DIR%)
   mkdir %LUNARGLASS_DIR%
   cd %LUNARGLASS_DIR%
   git clone https://github.com/LunarG/LunarGLASS.git .
   git checkout %LUNARGLASS_REVISION%
   cd Core\LLVM
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
   REM put back the LunarGLASS github versions of some LLVM files
   git checkout -f .
   REM overwrite with private gitlab versions of some files
   svn checkout -r %LUNARGLASS_REVISION_R32% --force https://cvs.khronos.org/svn/repos/SPIRV/trunk/LunarGLASS/ .
   svn revert -R .
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
   git fetch --all
   git checkout -f %LUNARGLASS_REVISION% .
   if not exist %LUNARGLASS_DIR%\.svn (
      svn checkout -r %LUNARGLASS_REVISION_R32% --force https://cvs.khronos.org/svn/repos/SPIRV/trunk/LunarGLASS/ .
   )
   svn update -r %LUNARGLASS_REVISION_R325
   svn revert -R .
goto:eof

:create_spirv-tools
   echo.
   echo Creating local spirv-tools repository %SPIRV_TOOLS_DIR%)
   mkdir %SPIRV_TOOLS_DIR%
   cd %SPIRV_TOOLS_DIR%
   git clone git@gitlab.khronos.org:spirv/spirv-tools.git .
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
   rmdir /s /q build32
   rmdir /s /q build
   
   echo Making 32-bit glslang
   echo *************************
   mkdir build32
   set GLSLANG_BUILD_DIR=%GLSLANG_DIR%\build32
   cd %GLSLANG_BUILD_DIR%

   echo Generating 32-bit Glslang CMake files for Visual Studio %VS_VERSION% -DCMAKE_INSTALL_PREFIX=install ..
   cmake -G "Visual Studio %VS_VERSION%" -DCMAKE_INSTALL_PREFIX=install ..
   
   echo Building 32-bit Glslang: MSBuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Debug
   msbuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Debug
   
   REM Check for existence of one lib, even though we should check for all results
   if not exist %GLSLANG_BUILD_DIR%\glslang\Debug\glslang.lib (
      echo.
      echo glslang 32-bit Debug build failed!
      set errorCode=1
   )
   echo Building Glslang: MSBuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Release
   msbuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Release
   
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
   msbuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Debug
   
   REM Check for existence of one lib, even though we should check for all results
   if not exist %GLSLANG_BUILD_DIR%\glslang\Debug\glslang.lib (
      echo.
      echo glslang 64-bit Debug build failed!
      set errorCode=1
   )
   echo Building Glslang: MSBuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Release
   msbuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Release
   
   REM Check for existence of one lib, even though we should check for all results
   if not exist %GLSLANG_BUILD_DIR%\glslang\Release\glslang.lib (
      echo.
      echo glslang 64-bit Release build failed!
      set errorCode=1
   )
goto:eof

:build_LunarGLASS
   echo.
   echo Building %LUNARGLASS_DIR%
   set LLVM_DIR=%LUNARGLASS_DIR%\Core\LLVM\llvm-3.4
   cd %LLVM_DIR%

   REM Cleanup any old directories lying around.
   rmdir /s /q build32
   rmdir /s /q build
   
   echo Making 32-bit LLVM
   echo *************************
   mkdir build32
   set LLVM_BUILD_DIR=%LLVM_DIR%\build32
   cd %LLVM_BUILD_DIR%

   echo Generating 32-bit LLVM CMake files for Visual Studio %VS_VERSION% -DCMAKE_INSTALL_PREFIX=install ..
   cmake -G "Visual Studio %VS_VERSION%" -DCMAKE_INSTALL_PREFIX=install ..
   
   echo Building 32-bit LLVM: MSBuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Release
   msbuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Release
   REM Check for existence of one lib, even though we should check for all results
   if not exist %LLVM_BUILD_DIR%\lib\Release\LLVMCore.lib (
      echo.
      echo LLVM 32-bit Release build failed!
      set errorCode=1
      goto:eof
   )
   REM disable Debug build of LLVM until LunarGLASS cmake files are updated to
   REM handle Debug and Release builds of glslang simultaneously, instead of
   REM whatever last lands in "./build32/install"
   REM   echo Building 32-bit LLVM: MSBuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Debug
   REM   msbuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Debug
   REM Check for existence of one lib, even though we should check for all results
   REM   if not exist %LLVM_BUILD_DIR%\lib\Debug\LLVMCore.lib (
   REM      echo.
   REM      echo LLVM 32-bit Debug build failed!
   REM      set errorCode=1
   REM      goto:eof
   REM   )

   cd ..
 
   echo Making 64-bit LLVM
   echo *************************
   mkdir build
   set LLVM_BUILD_DIR=%LLVM_DIR%\build
   cd %LLVM_BUILD_DIR%

   echo Generating 64-bit LLVM CMake files for Visual Studio %VS_VERSION% -DCMAKE_INSTALL_PREFIX=install ..
   cmake -G "Visual Studio %VS_VERSION% Win64" -DCMAKE_INSTALL_PREFIX=install ..
   
   echo Building 64-bit LLVM: MSBuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Release
   msbuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Release
   REM Check for existence of one lib, even though we should check for all results
   if not exist %LLVM_BUILD_DIR%\lib\Release\LLVMCore.lib (
      echo.
      echo LLVM 64-bit Release build failed!
      set errorCode=1
      goto:eof
   )
   REM disable Debug build of LLVM until LunarGLASS cmake files are updated to
   REM handle Debug and Release builds of glslang simultaneously, instead of
   REM whatever last lands in "./build/install"
   REM   echo Building 64-bit LLVM: MSBuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Debug
   REM   msbuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Debug
   REM Check for existence of one lib, even though we should check for all results
   REM   if not exist %LLVM_BUILD_DIR%\lib\Debug\LLVMCore.lib (
   REM      echo.
   REM      echo LLVM 64-bit Debug build failed!
   REM      set errorCode=1
   REM      goto:eof
   REM   )

   cd %LUNARGLASS_DIR%

   REM Cleanup any old directories lying around.
   rmdir /s /q build32
   rmdir /s /q build
   
   echo Making 32-bit LunarGLASS
   echo *************************
   mkdir build32
   set LUNARGLASS_BUILD_DIR=%LUNARGLASS_DIR%\build32
   cd %LUNARGLASS_BUILD_DIR%
   
   echo Generating 32-bit LunarGlass CMake files for Visual Studio %VS_VERSION% -DCMAKE_INSTALL_PREFIX=install ..
   cmake -G "Visual Studio %VS_VERSION%" -DCMAKE_INSTALL_PREFIX=install ..
   
   echo Building 32-bit LunarGlass: MSBuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Release
   msbuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Release
   
   REM Check for existence of one lib, even though we should check for all results
   if not exist %LUNARGLASS_BUILD_DIR%\Core\Release\core.lib (
      echo.
      echo LunarGLASS 32-bit Release build failed!
      set errorCode=1
      goto:eof
   )
   
   REM disable Debug build of LunarGLASS until its cmake file can be updated to
   REM handle Debug and Release builds of glslang simultaneously, instead of
   REM whatever last lands in "./build/install"
   REM   echo Building 32-bit LunarGlass: MSBuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Debug
   REM   msbuild INSTALL.vcxproj /p:Platform=x86 /p:Configuration=Debug
   REM Check for existence of one lib, even though we should check for all results
   REM  if not exist %LUNARGLASS_BUILD_DIR%\Core\Debug\core.lib (
   REM     echo.
   REM     echo LunarGLASS 32-bit Debug build failed!
   REM     set errorCode=1
   REM     goto:eof
   REM  )
   
   cd ..
   
   echo Making 64-bit LunarGLASS
   echo *************************
   mkdir build
   set LUNARGLASS_BUILD_DIR=%LUNARGLASS_DIR%\build
   cd %LUNARGLASS_BUILD_DIR%
   
   echo Generating 64-bit LunarGlass CMake files for Visual Studio %VS_VERSION% -DCMAKE_INSTALL_PREFIX=install ..
   cmake -G "Visual Studio %VS_VERSION% Win64" -DCMAKE_INSTALL_PREFIX=install ..
   
   echo Building 64-bit LunarGlass: MSBuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Release
   msbuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Release
   
   REM Check for existence of one lib, even though we should check for all results
   if not exist %LUNARGLASS_BUILD_DIR%\Core\Release\core.lib (
      echo.
      echo LunarGLASS 64-bit Release build failed!
      set errorCode=1
      goto:eof
   )
   
   REM disable Debug build of LunarGLASS until its cmake file can be updated to
   REM handle Debug and Release builds of glslang simultaneously, instead of
   REM whatever last lands in "./build/install"
   REM   echo Building 64-bit LunarGlass: MSBuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Debug
   REM   msbuild INSTALL.vcxproj /p:Platform=x64 /p:Configuration=Debug
   REM Check for existence of one lib, even though we should check for all results
   REM  if not exist %LUNARGLASS_BUILD_DIR%\Core\Debug\core.lib (
   REM     echo.
   REM     echo LunarGLASS 64-bit Debug build failed!
   REM     set errorCode=1
   REM     goto:eof
   REM  )
goto:eof

:build_spirv-tools
   echo.
   echo Building %SPIRV_TOOLS_DIR%
   cd  %SPIRV_TOOLS_DIR%

   REM Cleanup any old directories lying around.
   rmdir /s /q build32
   rmdir /s /q build
   
   echo Making 32-bit spirv-tools
   echo *************************
   mkdir build32
   set SPIRV_TOOLS_BUILD_DIR=%SPIRV_TOOLS_DIR%\build32

   cd %SPIRV_TOOLS_BUILD_DIR%
   
   echo Generating 32-bit spirv-tools CMake files for Visual Studio %VS_VERSION% ..
   cmake -G "Visual Studio %VS_VERSION%" ..
   
   echo Building 32-bit spirv-tools: MSBuild ALL_BUILD.vcxproj /p:Platform=x86 /p:Configuration=Debug
   msbuild ALL_BUILD.vcxproj /p:Platform=x86 /p:Configuration=Debug
   
   REM Check for existence of one lib, even though we should check for all results
   if not exist %SPIRV_TOOLS_BUILD_DIR%\Debug\SPIRV-Tools.lib (
      echo.
      echo spirv-tools 32-bit Debug build failed!
      set errorCode=1
   )
   
   echo Building 32-bit spirv-tools: MSBuild ALL_BUILD.vcxproj /p:Platform=x86 /p:Configuration=Release
   msbuild ALL_BUILD.vcxproj /p:Platform=x86 /p:Configuration=Release

   REM Check for existence of one lib, even though we should check for all results
   if not exist %SPIRV_TOOLS_BUILD_DIR%\Release\SPIRV-Tools.lib (
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
   msbuild ALL_BUILD.vcxproj /p:Platform=x64 /p:Configuration=Debug
   
   REM Check for existence of one lib, even though we should check for all results
   if not exist %SPIRV_TOOLS_BUILD_DIR%\Debug\SPIRV-Tools.lib (
      echo.
      echo spirv-tools 64-bit Debug build failed!
      set errorCode=1
   )
   
   echo Building 64-bit spirv-tools: MSBuild ALL_BUILD.vcxproj /p:Platform=x64 /p:Configuration=Release
   msbuild ALL_BUILD.vcxproj /p:Platform=x64 /p:Configuration=Release

   REM Check for existence of one lib, even though we should check for all results
   if not exist %SPIRV_TOOLS_BUILD_DIR%\Release\SPIRV-Tools.lib (
      echo.
      echo spirv-tools 64-bit Release build failed!
      set errorCode=1
   )
goto:eof
