echo off
REM
REM This Windows batch file builds this repository for the following targets:
REM 64/32-bit Release/Debug
REM It uses CMake to genererate the project files and then invokes msbuild
REM to build them.
REM The update_external_sources.bat batch file must be executed before running
REM this batch file
REM
REM Arguments:
REM None: Runs CMake and builds all 4 combinations
REM Argument contains:
REM     cmake (case insensitive): Deletes build and build32 and runs just CMake on both
REM     32: Deletes build32, runs CMake and builds 32-bit versions
REM     64: Deletes build, runs CMake and builds 64-bit versions
REM Example:
REM build_windows_targets.bat 64
REM deletes build, creates build, runs CMake and compiles 64-bit Debug and Release.

set do_cmake=0
set do_32=1
set do_64=1
if "%1"=="" goto no_args
set do_cmake=0
set do_32=0
set do_64=0
for %%a in (%*) do (
    echo.%%a | %WINDIR%\system32\find.exe /I "cmake">Nul && (set do_cmake=1)
    echo.%%a | %WINDIR%\system32\find.exe "32">Nul && (set do_32=1)
    echo.%%a | %WINDIR%\system32\find.exe "64">Nul && (set do_64=1)
)
:no_args
if %do_cmake%==0 (
    if %do_32%==0 (
        if %do_64%==0 (
            echo No valid parameters specified.
            exit /B 1
        )
    )
)

REM Determine the appropriate CMake strings for the current version of Visual Studio
echo Determining VS version
python .\determine_vs_version.py > vsversion.tmp
set /p VS_VERSION=< vsversion.tmp
echo Detected Visual Studio Version as %VS_VERSION%
del /Q /F vsversion.tmp

if %do_cmake%==1 (
    rmdir /Q /S build
    rmdir /Q /S build32
    mkdir build
    pushd build
    echo Generating 64-bit CMake files for Visual Studio %VS_VERSION%
    cmake -G "Visual Studio %VS_VERSION% Win64" ..
    popd
    mkdir build32
    pushd build32
    echo Generating 32-bit CMake files for Visual Studio %VS_VERSION%
    cmake -G "Visual Studio %VS_VERSION%" ..
    popd
)

REM *******************************************
REM 64-bit build
REM *******************************************
if %do_64%==1 (
    rmdir /Q /S build
    mkdir build
    pushd build
    echo Generating 64-bit CMake files for Visual Studio %VS_VERSION%
    cmake -G "Visual Studio %VS_VERSION% Win64" ..
    echo Building 64-bit Debug 
    msbuild ALL_BUILD.vcxproj /p:Platform=x64 /p:Configuration=Debug /verbosity:quiet
    if errorlevel 1 (
       echo.
       echo 64-bit Debug build failed!
       popd
       exit /B 1
    )   
   
    echo Building 64-bit Release 
    msbuild ALL_BUILD.vcxproj /p:Platform=x64 /p:Configuration=Release /verbosity:quiet
    if errorlevel 1 (
       echo.
       echo 64-bit Release build failed!
       popd
       exit /B 1
    )   
    popd
)
 
REM *******************************************
REM 32-bit build
REM *******************************************
  
if %do_32%==1 (
    rmdir /Q /S build32
    mkdir build32
    pushd build32
    echo Generating 32-bit CMake files for Visual Studio %VS_VERSION%
    cmake -G "Visual Studio %VS_VERSION%" ..
    echo Building 32-bit Debug 
    msbuild ALL_BUILD.vcxproj /p:Platform=x86 /p:Configuration=Debug /verbosity:quiet
    if errorlevel 1 (
       echo.
       echo 32-bit Debug build failed!
       popd
       exit /B 1
    )   
       
    echo Building 32-bit Release 
    msbuild ALL_BUILD.vcxproj /p:Platform=x86 /p:Configuration=Release /verbosity:quiet
    if errorlevel 1 (
       echo.
       echo 32-bit Release build failed!
       popd
       exit /B 1
    )   
    popd
)
exit /B 0
